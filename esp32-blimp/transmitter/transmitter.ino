// =====================================================================
//  ESP-NOW Blimp - TRANSMITTER (handheld controller)
// ---------------------------------------------------------------------
//  Reads two analog joysticks and sends control packets to the blimp
//  ~50x/second over ESP-NOW.
//
//     Left stick  Y  -> forward / back   (both thrusters)
//     Left stick  X  -> yaw (turn)       (differential thrusters)
//     Right stick Y  -> vertical up/down
//
//  An ARM button toggles whether the motors are allowed to spin. The
//  transmitter LED is solid when armed, blinking when safe/disarmed.
//
//  Target: ESP32 dev module, Arduino-ESP32 core 3.x.
// =====================================================================

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "protocol.h"

// ----------------------- JOYSTICK WIRING -----------------------------
// ESP-NOW uses the WiFi radio, which makes ADC2 pins unreadable. You
// MUST use ADC1-only pins for the joysticks: 32, 33, 34, 35, 36, 39.
static const int PIN_FORWARD = 34;   // left stick  Y  (VRy)
static const int PIN_YAW     = 35;   // left stick  X  (VRx)
static const int PIN_VERT    = 32;   // right stick Y  (VRy)

static const int PIN_ARM_BTN = 4;    // momentary button to GND (toggles arm)
static const int STATUS_LED  = 2;    // onboard LED

// If a stick pushes the wrong direction, flip it here.
static const bool INV_FORWARD = false;
static const bool INV_YAW     = false;
static const bool INV_VERT    = false;

// ----------------------- FEEL / TUNING -------------------------------
static const int   DEADBAND   = 60;    // raw ADC counts ignored near center
static const float EXPO       = 0.40;  // 0 = linear, ->1 = softer center
static const int   ADC_MAX    = 4095;  // 12-bit ADC
static const uint32_t SEND_MS = 20;    // 50 Hz

// Receiver MAC. Default is broadcast (works with one blimp, no pairing).
// To lock to ONE blimp, replace with the MAC the receiver prints on boot.
uint8_t peerMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ----------------------- STATE ---------------------------------------
int centerFwd = 2048, centerYaw = 2048, centerVert = 2048;
bool armed = false;
uint16_t seq = 0;

// =====================================================================
//  Joystick reading: center + deadband + expo -> -AXIS_FS..+AXIS_FS
// =====================================================================
int readAxis(int pin, int center, bool invert) {
  int raw = analogRead(pin);
  int v = raw - center;
  if (abs(v) < DEADBAND) return 0;
  // Remove the deadband so motion starts smoothly from zero.
  v = (v > 0) ? (v - DEADBAND) : (v + DEADBAND);

  // Normalize to -1..+1 using the smaller half-span (safe for off-center).
  float span = (float)min(center, ADC_MAX - center) - DEADBAND;
  if (span < 1) span = 1;
  float n = constrain(v / span, -1.0f, 1.0f);

  // Cubic expo: gentle around center, full authority at the ends.
  float shaped = (1.0f - EXPO) * n + EXPO * n * n * n;

  int out = (int)(shaped * AXIS_FS);
  if (invert) out = -out;
  return constrain(out, -AXIS_FS, AXIS_FS);
}

void calibrateCenters() {
  long f = 0, y = 0, v = 0;
  const int N = 64;
  for (int i = 0; i < N; i++) {
    f += analogRead(PIN_FORWARD);
    y += analogRead(PIN_YAW);
    v += analogRead(PIN_VERT);
    delay(2);
  }
  centerFwd = f / N; centerYaw = y / N; centerVert = v / N;
  Serial.printf("[blimp-tx] centers: fwd=%d yaw=%d vert=%d\n",
                centerFwd, centerYaw, centerVert);
}

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[blimp-tx] booting");

  pinMode(STATUS_LED, OUTPUT);
  pinMode(PIN_ARM_BTN, INPUT_PULLUP);
  analogReadResolution(12);

  // IMPORTANT: leave the sticks centered (and throttle/vertical released
  // to center) during power-up so calibration reads true center.
  calibrateCenters();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(BLIMP_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_ps(WIFI_PS_NONE);
  Serial.print("[blimp-tx] MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[blimp-tx] ESP-NOW init FAILED - halting");
    while (true) { digitalWrite(STATUS_LED, millis() / 100 % 2); }
  }

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, peerMac, 6);
  peer.channel = BLIMP_WIFI_CHANNEL;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("[blimp-tx] add_peer FAILED");
  }
  Serial.println("[blimp-tx] ready (DISARMED). Press ARM to enable motors.");
}

// =====================================================================
//  Arm button: toggle on press, with debounce.
// =====================================================================
void serviceArmButton() {
  static bool lastStable = HIGH;
  static bool lastRead = HIGH;
  static uint32_t lastChange = 0;
  bool r = digitalRead(PIN_ARM_BTN);
  if (r != lastRead) { lastRead = r; lastChange = millis(); }
  if (millis() - lastChange > 30 && r != lastStable) {
    lastStable = r;
    if (r == LOW) {          // pressed
      armed = !armed;
      Serial.printf("[blimp-tx] %s\n", armed ? "ARMED" : "DISARMED");
    }
  }
}

// =====================================================================
void loop() {
  serviceArmButton();

  static uint32_t lastSend = 0;
  uint32_t now = millis();
  if (now - lastSend < SEND_MS) return;
  lastSend = now;

  ControlPacket p;
  p.version  = BLIMP_PROTO_VERSION;
  p.flags    = armed ? FLAG_ARMED : 0;
  p.seq      = seq++;
  p.forward  = readAxis(PIN_FORWARD, centerFwd, INV_FORWARD);
  p.yaw      = readAxis(PIN_YAW,     centerYaw, INV_YAW);
  p.vertical = readAxis(PIN_VERT,    centerVert, INV_VERT);

  esp_now_send(peerMac, (uint8_t *)&p, sizeof(p));

  // LED: solid when armed, slow blink when disarmed.
  digitalWrite(STATUS_LED, armed ? HIGH : ((now / 500) % 2));
}
