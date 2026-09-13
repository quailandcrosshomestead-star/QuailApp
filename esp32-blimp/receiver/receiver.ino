// =====================================================================
//  ESP-NOW Blimp - RECEIVER (goes on the gondola / airship)
// ---------------------------------------------------------------------
//  Receives control packets from the handheld transmitter over ESP-NOW
//  and drives three brushed DC motors through two DRV8833 H-bridges:
//
//     * Left thruster   (horizontal)
//     * Right thruster  (horizontal)
//     * Vertical motor  (up / down)
//
//  Differential thrust:  left = forward + yaw,  right = forward - yaw
//  so both thrusters together = go forward/back, opposite = turn.
//
//  Target: ESP32 dev module, Arduino-ESP32 core 3.x (see note on the
//  ESP-NOW receive callback if you are still on core 2.x).
// =====================================================================

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "protocol.h"

// ----------------------- MOTOR WIRING --------------------------------
// Each brushed motor needs TWO pins (an H-bridge half each): drive one
// pin with PWM and hold the other LOW to spin one way; swap for reverse.
//
// DRV8833 #1  ->  Left + Right thrusters
// DRV8833 #2  ->  Vertical motor
//
// Pick output-capable pins. Avoid 6-11 (flash), 34-39 (input only),
// and the strapping pins 0/2/12/15 for cleaner boot behavior.
static const int LEFT_A  = 25;   // DRV8833 #1  AIN1
static const int LEFT_B  = 26;   // DRV8833 #1  AIN2
static const int RIGHT_A = 27;   // DRV8833 #1  BIN1
static const int RIGHT_B = 14;   // DRV8833 #1  BIN2
static const int VERT_A  = 32;   // DRV8833 #2  AIN1
static const int VERT_B  = 33;   // DRV8833 #2  AIN2

// Optional: tie both DRV8833 nSLEEP pins to this GPIO for a hardware
// failsafe (drive LOW = motors coast, HIGH = enabled). Set to -1 and
// wire nSLEEP straight to 3V3 if you don't want software sleep control.
static const int SLEEP_PIN = 13;

static const int STATUS_LED = 2;  // onboard LED on most dev modules

// If a motor spins the wrong way, flip its flag here instead of
// re-soldering. (true = reverse that motor.)
static const bool LEFT_REVERSE  = false;
static const bool RIGHT_REVERSE = false;
static const bool VERT_REVERSE  = false;

// ----------------------- PWM CONFIG ----------------------------------
static const int      PWM_FREQ = 20000;          // 20 kHz = silent
static const int      PWM_RES  = 10;             // 10-bit -> 0..1023
static const int      PWM_MAX  = (1 << PWM_RES) - 1;

// ----------------------- TUNING --------------------------------------
// Smallest duty that actually makes a small motor turn. Below this the
// motor just buzzes, so we snap small commands up to it (or to 0).
static const int MOTOR_DEADSTART = 60;           // out of PWM_MAX

// Slew limiter: max change in duty per control update. Softer = gentler
// on the envelope and battery, less "lurch". Raise for snappier response.
static const int SLEW_PER_UPDATE = 40;

// Failsafe: if no valid packet arrives within this window, cut motors.
static const uint32_t FAILSAFE_MS = 400;

// ----------------------- STATE ---------------------------------------
volatile ControlPacket rxPkt;            // last packet received (ISR ctx)
volatile uint32_t      lastRxMs = 0;
volatile bool          haveLink = false;

int curLeft = 0, curRight = 0, curVert = 0;   // current (slew-limited) duty

// =====================================================================
//  Low-level motor drive
// =====================================================================
void driveMotor(int pinA, int pinB, int value, bool reverse) {
  value = constrain(value, -PWM_MAX, PWM_MAX);
  if (reverse) value = -value;
  if (value >= 0) {
    ledcWrite(pinA, value);
    ledcWrite(pinB, 0);
  } else {
    ledcWrite(pinA, 0);
    ledcWrite(pinB, -value);
  }
}

void allStop() {
  ledcWrite(LEFT_A, 0);  ledcWrite(LEFT_B, 0);
  ledcWrite(RIGHT_A, 0); ledcWrite(RIGHT_B, 0);
  ledcWrite(VERT_A, 0);  ledcWrite(VERT_B, 0);
  curLeft = curRight = curVert = 0;
}

// Map an axis value (-AXIS_FS..+AXIS_FS) to PWM duty, applying a
// dead-start floor so tiny commands don't just make the motor whine.
int axisToDuty(int axis) {
  int duty = (int)((long)axis * PWM_MAX / AXIS_FS);
  duty = constrain(duty, -PWM_MAX, PWM_MAX);
  if (abs(duty) < MOTOR_DEADSTART) {
    if (abs(duty) < MOTOR_DEADSTART / 3) return 0;         // truly idle
    duty = (duty > 0) ? MOTOR_DEADSTART : -MOTOR_DEADSTART; // kick to move
  }
  return duty;
}

int slew(int current, int target) {
  if (target > current) return min(current + SLEW_PER_UPDATE, target);
  if (target < current) return max(current - SLEW_PER_UPDATE, target);
  return current;
}

// =====================================================================
//  ESP-NOW receive callback
//  core 3.x signature shown below. For core 2.x use:
//      void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// =====================================================================
void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(ControlPacket)) return;
  ControlPacket p;
  memcpy(&p, data, sizeof(p));
  if (p.version != BLIMP_PROTO_VERSION) return;   // ignore foreign packets
  rxPkt = p;
  lastRxMs = millis();
  haveLink = true;
}

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[blimp-rx] booting");

  pinMode(STATUS_LED, OUTPUT);
  if (SLEEP_PIN >= 0) { pinMode(SLEEP_PIN, OUTPUT); digitalWrite(SLEEP_PIN, HIGH); }

  // Attach PWM to every motor pin (core 3.x LEDC API).
  const int pins[] = {LEFT_A, LEFT_B, RIGHT_A, RIGHT_B, VERT_A, VERT_B};
  for (int pin : pins) ledcAttach(pin, PWM_FREQ, PWM_RES);
  allStop();

  // Radio: station mode, fixed channel, no sleep (keeps latency low).
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(BLIMP_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_ps(WIFI_PS_NONE);

  Serial.print("[blimp-rx] MAC: ");
  Serial.println(WiFi.macAddress());   // note this for locked/paired mode

  if (esp_now_init() != ESP_OK) {
    Serial.println("[blimp-rx] ESP-NOW init FAILED - halting");
    while (true) { digitalWrite(STATUS_LED, millis() / 100 % 2); }
  }
  esp_now_register_recv_cb(onRecv);
  Serial.println("[blimp-rx] ready, listening for transmitter");
}

// =====================================================================
void loop() {
  static uint32_t lastUpdate = 0;
  uint32_t now = millis();

  // Run the control mixer at a steady ~100 Hz.
  if (now - lastUpdate < 10) return;
  lastUpdate = now;

  bool linkFresh = haveLink && (now - lastRxMs < FAILSAFE_MS);

  // Snapshot the volatile packet.
  ControlPacket p = rxPkt;
  bool armed = linkFresh && (p.flags & FLAG_ARMED);

  int tLeft = 0, tRight = 0, tVert = 0;
  if (armed) {
    // Differential-thrust mixing.
    int fwd = p.forward;
    int yaw = p.yaw;
    int left  = fwd + yaw;
    int right = fwd - yaw;

    // Keep the turn ratio when the sum saturates past full scale.
    int m = max(abs(left), abs(right));
    if (m > AXIS_FS) { left = (int)((long)left * AXIS_FS / m);
                       right = (int)((long)right * AXIS_FS / m); }

    tLeft  = axisToDuty(left);
    tRight = axisToDuty(right);
    tVert  = axisToDuty(p.vertical);
  }

  // Slew-limit toward the targets (0 when disarmed / link lost).
  curLeft  = slew(curLeft,  tLeft);
  curRight = slew(curRight, tRight);
  curVert  = slew(curVert,  tVert);

  driveMotor(LEFT_A,  LEFT_B,  curLeft,  LEFT_REVERSE);
  driveMotor(RIGHT_A, RIGHT_B, curRight, RIGHT_REVERSE);
  driveMotor(VERT_A,  VERT_B,  curVert,  VERT_REVERSE);

  // Status LED: solid = armed, slow blink = linked-but-safe,
  // fast blink = no link (failsafe).
  if (armed)          digitalWrite(STATUS_LED, HIGH);
  else if (linkFresh) digitalWrite(STATUS_LED, (now / 500) % 2);
  else                digitalWrite(STATUS_LED, (now / 100) % 2);
}
