/*
 * ============================================================================
 *  Laser Tripwire Security Alarm  —  ESP32-C3
 * ============================================================================
 *  A laser diode points across a doorway / coop entrance at a photoresistor
 *  (LDR). While the beam lands on the LDR the reading stays high. When
 *  something crosses the beam the reading drops, and the alarm sounds.
 *
 *  Features
 *    - Auto-calibration at boot (no manual threshold tuning needed)
 *    - Debounced trip detection (ignores momentary flicker / bugs / dust)
 *    - Latching alarm that keeps sounding until you reset it
 *    - Auto-reset after a timeout (so it doesn't scream forever)
 *    - Reset with the onboard BOOT button
 *    - Serial output for tuning and debugging
 *    - Works with both ACTIVE buzzers and PASSIVE piezo buzzers (siren)
 *
 *  Author: QuailApp homestead project
 *  Board:  ESP32-C3 (e.g. ESP32-C3 SuperMini / DevKitM-1)
 *  See README.md in this folder for wiring, parts, and flashing steps.
 * ============================================================================
 */

// ---------------------------------------------------------------------------
//  PIN ASSIGNMENTS  (safe defaults for ESP32-C3 SuperMini)
// ---------------------------------------------------------------------------
const int PIN_LDR       = 2;   // Analog input from the LDR voltage divider (ADC1)
const int PIN_LASER     = 4;   // Drives the laser diode (KY-008 or bare diode via resistor)
const int PIN_BUZZER    = 5;   // Buzzer signal pin
const int PIN_LED       = 8;   // Onboard status LED (active LOW on the SuperMini)
const int PIN_RESET_BTN = 9;   // Onboard BOOT button (active LOW). Press to silence/reset.

// ---------------------------------------------------------------------------
//  BUZZER TYPE
//    ACTIVE buzzer  -> makes sound on its own, just needs HIGH/LOW  (default)
//    PASSIVE piezo  -> needs a tone() frequency; we play a rising/falling siren
//  Set to true if you have a passive piezo element (2 legs, no built-in tone).
// ---------------------------------------------------------------------------
const bool PASSIVE_BUZZER = false;
const int  SIREN_LOW_HZ   = 600;   // Passive-buzzer siren sweep range
const int  SIREN_HIGH_HZ  = 1600;

// ---------------------------------------------------------------------------
//  BEHAVIOUR TUNING
// ---------------------------------------------------------------------------
const float   TRIP_FRACTION      = 0.60;  // Trip when light falls below 60% of the
                                          // calibrated baseline. Lower = less
                                          // sensitive, higher = more sensitive.
const uint16_t TRIP_CONFIRM_MS   = 40;    // Beam must stay broken this long to trip
                                          // (rejects bugs, dust, brief flickers).
const uint32_t ALARM_DURATION_MS = 30000; // Auto-silence after 30 s (0 = never).
const uint16_t CALIB_SAMPLES     = 200;   // Readings averaged during calibration.
const uint16_t LOOP_DELAY_MS     = 5;     // Sensor poll interval.

// ---------------------------------------------------------------------------
//  RUNTIME STATE
// ---------------------------------------------------------------------------
int      baseline   = 0;      // Calibrated "beam present" light level.
int      threshold  = 0;      // baseline * TRIP_FRACTION.
bool     alarmOn    = false;  // Is the alarm currently sounding?
uint32_t alarmStart = 0;      // millis() when the alarm started.
uint32_t brokenSince = 0;     // millis() when the beam first went dark (0 = intact).

// ---------------------------------------------------------------------------

void calibrate() {
  Serial.println(F("[CALIB] Aligning... keep the beam ON the sensor, path clear."));
  digitalWrite(PIN_LASER, HIGH);   // Make sure the laser is on.
  delay(500);                      // Let it settle.

  long sum = 0;
  for (uint16_t i = 0; i < CALIB_SAMPLES; i++) {
    sum += analogRead(PIN_LDR);
    delay(2);
  }
  baseline  = sum / CALIB_SAMPLES;
  threshold = (int)(baseline * TRIP_FRACTION);

  Serial.print(F("[CALIB] Baseline (beam present) = "));
  Serial.println(baseline);
  Serial.print(F("[CALIB] Trip threshold          = "));
  Serial.println(threshold);

  if (baseline < 200) {
    Serial.println(F("[WARN] Baseline is very low. Check laser alignment/power"));
    Serial.println(F("       and the LDR wiring before trusting the alarm."));
  }

  // Two short beeps = calibration done / armed.
  beep(80); delay(100); beep(80);
  Serial.println(F("[ARMED] Watching the beam."));
}

// Short confirmation beep (works for both buzzer types).
void beep(uint16_t ms) {
  if (PASSIVE_BUZZER) {
    tone(PIN_BUZZER, 2000, ms);
    delay(ms);
    noTone(PIN_BUZZER);
  } else {
    digitalWrite(PIN_BUZZER, HIGH);
    delay(ms);
    digitalWrite(PIN_BUZZER, LOW);
  }
}

void startAlarm() {
  alarmOn    = true;
  alarmStart = millis();
  digitalWrite(PIN_LED, LOW);   // LED on (active LOW).
  Serial.println(F("[ALARM] *** BEAM BROKEN — INTRUSION DETECTED ***"));
}

void stopAlarm() {
  alarmOn = false;
  if (PASSIVE_BUZZER) noTone(PIN_BUZZER);
  else                digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, HIGH);  // LED off.
  brokenSince = 0;
  Serial.println(F("[RESET] Alarm cleared — re-armed."));
}

// Keeps the siren wailing while the alarm is latched on.
void serviceAlarm() {
  if (PASSIVE_BUZZER) {
    // Sweep the frequency to create a rising/falling siren.
    uint32_t phase = (millis() - alarmStart) % 1000;      // 0..999 ms cycle
    int freq = (phase < 500)
                 ? map(phase, 0, 499, SIREN_LOW_HZ, SIREN_HIGH_HZ)
                 : map(phase, 500, 999, SIREN_HIGH_HZ, SIREN_LOW_HZ);
    tone(PIN_BUZZER, freq);
  } else {
    // Active buzzer: pulse it on/off so it sounds like an alarm, not a drone.
    bool on = ((millis() - alarmStart) / 250) % 2 == 0;   // 250 ms on/off
    digitalWrite(PIN_BUZZER, on ? HIGH : LOW);
  }
  // Blink the status LED in time with the alarm.
  digitalWrite(PIN_LED, (((millis() - alarmStart) / 250) % 2) ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== ESP32-C3 Laser Tripwire Alarm ==="));

  pinMode(PIN_LASER, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RESET_BTN, INPUT_PULLUP);

  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_LED, HIGH);   // LED off (active LOW).

  // ADC range: readings are 0..4095 (12-bit). Default attenuation reads ~0-3.1V.
  analogReadResolution(12);

  calibrate();
}

void loop() {
  int light = analogRead(PIN_LDR);

  // ---- Reset button: press to silence a sounding alarm and re-arm ----
  if (digitalRead(PIN_RESET_BTN) == LOW) {
    if (alarmOn) stopAlarm();
    delay(200);  // simple debounce
  }

  if (alarmOn) {
    serviceAlarm();

    // Auto-silence after the configured duration.
    if (ALARM_DURATION_MS > 0 && (millis() - alarmStart) > ALARM_DURATION_MS) {
      stopAlarm();
    }
  } else {
    // ---- Watch the beam ----
    if (light < threshold) {
      // Beam is currently broken. Has it stayed broken long enough?
      if (brokenSince == 0) brokenSince = millis();
      else if (millis() - brokenSince >= TRIP_CONFIRM_MS) {
        startAlarm();
      }
    } else {
      brokenSince = 0;  // Beam restored before confirmation — ignore.
    }

    // Heartbeat log ~ once per second while armed (handy for tuning).
    static uint32_t lastLog = 0;
    if (millis() - lastLog > 1000) {
      lastLog = millis();
      Serial.print(F("[ARMED] light="));
      Serial.print(light);
      Serial.print(F("  threshold="));
      Serial.println(threshold);
    }
  }

  delay(LOOP_DELAY_MS);
}
