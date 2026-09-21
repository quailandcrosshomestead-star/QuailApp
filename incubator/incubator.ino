/*
  Quail Incubator -- Arduino IoT Cloud controller
  Thing: "Quail Incubator"

  This sketch turns an Arduino IoT Cloud board (Nano 33 IoT, MKR WiFi 1010,
  ESP32, etc.) into a networked incubator controller for quail eggs. It reads a
  temperature/humidity sensor and drives four relays (heater, mister, fan and
  egg-turner) to hold the eggs at the target conditions, while reporting live
  state to the Arduino IoT Cloud dashboard.

  Default timeline matches the companion Quail Egg Manager app:
    - 18-day incubation
    - Day 15 = "lockdown": egg turning stops and humidity is raised

  Cloud variables (see thingProperties.h):
    String status;            (READ)  human-readable state summary
    float  temperature;       (READ)  measured deg C
    float  humidity;          (READ)  measured %RH
    float  tempSetpoint;      (RW)    target deg C
    float  humiditySetpoint;  (RW)    target %RH
    int    incubationDay;     (READ)  current day, 1-based
    bool   autoMode;          (RW)    true = board regulates automatically
    bool   heater/mister/fan/turner; (RW) relay states
    bool   turnNow;           (RW)    momentary: turn eggs now
    bool   resetCycle;        (RW)    momentary: restart at day 1
    bool   outOfRange;        (READ)  true when outside the safe band

  Sensor: DHT22 (AM2302) by default, via the Adafruit DHT sensor library.
  Swap DHT_TYPE / the read code below if you use a different sensor.
*/

#include "thingProperties.h"
#include <DHT.h>

// ----------------------------------------------------------------------------
// Hardware configuration -- adjust to match your wiring.
// ----------------------------------------------------------------------------
#define DHT_PIN         2      // Data pin of the DHT22 sensor
#define DHT_TYPE        DHT22  // DHT22 (AM2302) / DHT11 / DHT21

#define PIN_HEATER      3      // Relay: heat source (bulb / ceramic / mat)
#define PIN_MISTER      4      // Relay: humidifier / mister
#define PIN_FAN         5      // Relay: circulation fan
#define PIN_TURNER      6      // Relay: egg-turner motor

// Most low-cost relay boards are ACTIVE-LOW (LOW = energised). Set to false if
// your relays energise on a HIGH signal.
#define RELAY_ACTIVE_LOW  true

// ----------------------------------------------------------------------------
// Control tuning.
// ----------------------------------------------------------------------------
static const float DEFAULT_TEMP_SP      = 37.5;  // deg C (~99.5 F, forced air)
static const float DEFAULT_HUM_SP       = 50.0;  // %RH, days 1..lockdown
static const float LOCKDOWN_HUM_SP      = 65.0;  // %RH during lockdown

static const float TEMP_HYST            = 0.25;  // deg C deadband around SP
static const float HUM_HYST             = 2.0;   // %RH deadband around SP

static const float TEMP_ALARM_BAND      = 1.0;   // deg C from SP -> out of range
static const float HUM_ALARM_BAND       = 10.0;  // %RH from SP -> out of range

// Safe clamps for user-entered setpoints.
static const float TEMP_SP_MIN = 30.0, TEMP_SP_MAX = 40.0;
static const float HUM_SP_MIN  = 20.0, HUM_SP_MAX  = 90.0;

static const int   TOTAL_DAYS           = 18;    // quail incubation length
static const int   LOCKDOWN_DAY         = 15;    // stop turning from this day

static const unsigned long SENSOR_INTERVAL_MS = 5000UL;          // 5 s
static const unsigned long TURN_INTERVAL_S    = 3UL * 3600UL;    // start a turn every 3 h
// Egg-turner motor is a continuous-rotation AC gear motor (e.g. TY-50AF,
// ~2.5-3 rpm, auto-reverses at the tray end-stops). Each "turn" powers it for a
// fixed run-time; adjust to match how far your tray travels per turn.
static const unsigned long TURN_RUN_MS        = 12UL * 1000UL;   // run 12 s per turn

// Epoch values below this (2020-01-01) mean the RTC has not synced yet.
static const unsigned long MIN_VALID_EPOCH = 1577836800UL;

// ----------------------------------------------------------------------------
// Runtime state.
// ----------------------------------------------------------------------------
DHT dht(DHT_PIN, DHT_TYPE);

bool          sensorValid       = false;   // last read succeeded?
unsigned long lastSensorReadMs  = 0;

bool          turning           = false;   // a timed turn run is in progress
unsigned long turnStartMs       = 0;
unsigned long lastTurnEpoch     = 0;       // epoch of the last completed turn
unsigned long cycleStartEpoch   = 0;       // epoch when the cycle began

// ----------------------------------------------------------------------------
// Relay helpers.
// ----------------------------------------------------------------------------
static inline void writeRelay(int pin, bool on) {
  digitalWrite(pin, (on ^ RELAY_ACTIVE_LOW) ? HIGH : LOW);
}

// Set an output and keep its Cloud variable in sync.
static void setHeater(bool on) { heater = on; writeRelay(PIN_HEATER, on); }
static void setMister(bool on) { mister = on; writeRelay(PIN_MISTER, on); }
static void setFan(bool on)    { fan    = on; writeRelay(PIN_FAN,    on); }

static bool isLockdown() { return incubationDay >= LOCKDOWN_DAY; }

// ----------------------------------------------------------------------------
// Setup.
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  delay(1500);

  // Relays off before anything else, so a reset never leaves the heater on.
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_MISTER, OUTPUT);
  pinMode(PIN_FAN,    OUTPUT);
  pinMode(PIN_TURNER, OUTPUT);
  writeRelay(PIN_HEATER, false);
  writeRelay(PIN_MISTER, false);
  writeRelay(PIN_FAN,    false);
  writeRelay(PIN_TURNER, false);

  dht.begin();

  // Sensible defaults. The Cloud restores the last dashboard values on connect,
  // so these only apply on a first boot / offline run.
  tempSetpoint     = DEFAULT_TEMP_SP;
  humiditySetpoint = DEFAULT_HUM_SP;
  autoMode         = true;
  incubationDay    = 1;
  status           = "Starting up";

  initProperties();
  ArduinoCloud.begin(ArduinoIoTPreferredConnection);
  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();
}

// ----------------------------------------------------------------------------
// Main loop.
// ----------------------------------------------------------------------------
void loop() {
  ArduinoCloud.update();
  unsigned long nowMs = millis();

  // Periodic sensor read + regulation.
  if (nowMs - lastSensorReadMs >= SENSOR_INTERVAL_MS) {
    lastSensorReadMs = nowMs;

    readSensors();
    updateIncubationDay();

    if (autoMode) {
      runAutoControl();
      checkScheduledTurn();
    }

    evaluateAlarms();
    updateStatus();
  }

  // A timed turn run must end on time, so service it every loop.
  serviceTurn(nowMs);
}

// ----------------------------------------------------------------------------
// Sensing.
// ----------------------------------------------------------------------------
void readSensors() {
  float t = dht.readTemperature(); // Celsius
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    sensorValid = false;           // keep the last good values on the dashboard
  } else {
    temperature = t;
    humidity    = h;
    sensorValid = true;
  }
}

// ----------------------------------------------------------------------------
// Incubation day tracking (driven by Cloud/RTC time when available).
// ----------------------------------------------------------------------------
void updateIncubationDay() {
  unsigned long now = ArduinoCloud.getLocalTime();
  if (now < MIN_VALID_EPOCH) return; // no valid time yet -> leave day as-is

  if (cycleStartEpoch == 0) cycleStartEpoch = now; // anchor on first real time

  long days = (long)((now - cycleStartEpoch) / 86400UL) + 1;
  if (days < 1) days = 1;
  incubationDay = (int)days;
}

void resetIncubationCycle() {
  unsigned long now = ArduinoCloud.getLocalTime();
  cycleStartEpoch = (now >= MIN_VALID_EPOCH) ? now : 0;
  lastTurnEpoch   = cycleStartEpoch;
  incubationDay   = 1;
}

// ----------------------------------------------------------------------------
// Automatic regulation.
// ----------------------------------------------------------------------------
void runAutoControl() {
  // During lockdown, hold the higher humidity target automatically.
  if (isLockdown() && humiditySetpoint < LOCKDOWN_HUM_SP) {
    humiditySetpoint = LOCKDOWN_HUM_SP;
  }

  if (!sensorValid) {
    // Fail safe: without a trusted reading, never keep heating or misting.
    setHeater(false);
    setMister(false);
    setFan(true); // keep air moving so heat can't pool at the element
    return;
  }

  // Heater: on below (SP - hyst), off above (SP + hyst), hold in the deadband.
  if (temperature <= tempSetpoint - TEMP_HYST)      setHeater(true);
  else if (temperature >= tempSetpoint + TEMP_HYST) setHeater(false);

  // Mister: same hysteresis around the humidity setpoint.
  if (humidity <= humiditySetpoint - HUM_HYST)      setMister(true);
  else if (humidity >= humiditySetpoint + HUM_HYST) setMister(false);

  // Forced-air incubator: circulation fan runs continuously in auto mode.
  setFan(true);
}

// ----------------------------------------------------------------------------
// Egg turning.
// ----------------------------------------------------------------------------
void startTurn() {
  if (turning) return;
  turning     = true;
  turnStartMs = millis();
  turner      = true;
  writeRelay(PIN_TURNER, true);

  unsigned long now = ArduinoCloud.getLocalTime();
  if (now >= MIN_VALID_EPOCH) lastTurnEpoch = now;
}

// Ends the timed turn run after TURN_RUN_MS and clears the momentary trigger.
void serviceTurn(unsigned long nowMs) {
  if (turning && (nowMs - turnStartMs >= TURN_RUN_MS)) {
    turning = false;
    turner  = false;
    writeRelay(PIN_TURNER, false);
    if (turnNow) turnNow = false;
  }
}

// Scheduled turns in auto mode, suspended during lockdown.
void checkScheduledTurn() {
  if (isLockdown()) return;

  unsigned long now = ArduinoCloud.getLocalTime();
  if (now < MIN_VALID_EPOCH) return;        // need real time to schedule

  if (lastTurnEpoch == 0) { lastTurnEpoch = now; return; }
  if (now - lastTurnEpoch >= TURN_INTERVAL_S) startTurn();
}

// ----------------------------------------------------------------------------
// Alarms & status.
// ----------------------------------------------------------------------------
void evaluateAlarms() {
  if (!sensorValid) { outOfRange = true; return; }
  bool tBad = fabs(temperature - tempSetpoint) > TEMP_ALARM_BAND;
  bool hBad = fabs(humidity   - humiditySetpoint) > HUM_ALARM_BAND;
  outOfRange = tBad || hBad;
}

void updateStatus() {
  String s;
  if (!sensorValid) {
    s = "SENSOR FAULT";
  } else {
    s  = autoMode ? "Auto" : "Manual";
    s += " | Day ";
    s += incubationDay;
    s += "/";
    s += TOTAL_DAYS;
    if (isLockdown()) s += " (Lockdown)";
    s += " | ";
    s += String(temperature, 1);
    s += "C ";
    s += String(humidity, 0);
    s += "%";
    if (heater) s += " | Heat";
    if (mister) s += " | Mist";
    if (turner) s += " | Turning";
    if (outOfRange) s += " | !OUT OF RANGE";
  }
  status = s;
}

// ----------------------------------------------------------------------------
// Cloud change callbacks.
//
// In auto mode the loop owns heater/mister/fan/turner, so the manual toggles
// are ignored (the loop will re-assert the correct state). In manual mode each
// toggle drives its relay directly. turnNow and resetCycle work in either mode.
// ----------------------------------------------------------------------------
void onHeaterChange() { if (!autoMode) writeRelay(PIN_HEATER, heater); }
void onMisterChange() { if (!autoMode) writeRelay(PIN_MISTER, mister); }
void onFanChange()    { if (!autoMode) writeRelay(PIN_FAN,    fan);    }

void onTurnerChange() {
  // Manual continuous control of the turner relay (auto uses timed runs).
  if (!autoMode && !turning) writeRelay(PIN_TURNER, turner);
}

void onTempSetpointChange() {
  tempSetpoint = constrain(tempSetpoint, TEMP_SP_MIN, TEMP_SP_MAX);
}

void onHumiditySetpointChange() {
  humiditySetpoint = constrain(humiditySetpoint, HUM_SP_MIN, HUM_SP_MAX);
}

void onAutoModeChange() {
  // Nothing to do here: the next loop pass takes over (auto) or the dashboard
  // toggles do (manual). Refresh the status label immediately for feedback.
  updateStatus();
}

void onTurnNowChange() {
  if (turnNow) startTurn(); // manual override, allowed even during lockdown
}

void onResetCycleChange() {
  if (resetCycle) {
    resetIncubationCycle();
    resetCycle = false; // momentary button
  }
}
