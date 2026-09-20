// Code generated / maintained for the Arduino IoT Cloud Thing "Quail Incubator".
//
// This header declares every Cloud variable used by the incubator sketch and
// registers it with the Arduino IoT Cloud so it shows up on the dashboard.
// It mirrors what the Arduino IoT Cloud editor generates, kept in the repo so
// the control logic in incubator.ino can be reviewed and version-controlled.

#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include "arduino_secrets.h"

const char SSID[] = SECRET_SSID;         // Network SSID (name)
const char PASS[] = SECRET_OPTIONAL_PASS; // Network password (WPA) or key (WEP)

// ---- Cloud change callbacks (implemented in incubator.ino) ----
void onHeaterChange();
void onMisterChange();
void onFanChange();
void onTurnerChange();
void onTempSetpointChange();
void onHumiditySetpointChange();
void onAutoModeChange();
void onTurnNowChange();
void onResetCycleChange();

// ---- Cloud variables ----
// READ variables are reported by the board; READWRITE variables can also be
// set from the dashboard and fire the matching callback above.
String status;            // Human-readable summary shown on the dashboard
float  temperature;       // Measured temperature (deg C)
float  humidity;          // Measured relative humidity (%)
float  tempSetpoint;      // Target temperature (deg C)
float  humiditySetpoint;  // Target relative humidity (%)
int    incubationDay;     // Current day of the incubation cycle (1-based)
bool   autoMode;          // true = board regulates automatically
bool   heater;            // Heater relay state
bool   mister;            // Humidifier/mister relay state
bool   fan;               // Circulation fan relay state
bool   turner;            // Egg-turner relay state
bool   turnNow;           // Momentary: request an immediate egg turn
bool   resetCycle;        // Momentary: restart the incubation cycle at day 1
bool   outOfRange;        // true when temp/humidity are outside the safe band

void initProperties() {
  ArduinoCloud.addProperty(status,           READ,      ON_CHANGE,  NULL);
  ArduinoCloud.addProperty(temperature,      READ,      10 * SECONDS, NULL);
  ArduinoCloud.addProperty(humidity,         READ,      10 * SECONDS, NULL);
  ArduinoCloud.addProperty(tempSetpoint,     READWRITE, ON_CHANGE,  onTempSetpointChange);
  ArduinoCloud.addProperty(humiditySetpoint, READWRITE, ON_CHANGE,  onHumiditySetpointChange);
  ArduinoCloud.addProperty(incubationDay,    READ,      ON_CHANGE,  NULL);
  ArduinoCloud.addProperty(autoMode,         READWRITE, ON_CHANGE,  onAutoModeChange);
  ArduinoCloud.addProperty(heater,           READWRITE, ON_CHANGE,  onHeaterChange);
  ArduinoCloud.addProperty(mister,           READWRITE, ON_CHANGE,  onMisterChange);
  ArduinoCloud.addProperty(fan,              READWRITE, ON_CHANGE,  onFanChange);
  ArduinoCloud.addProperty(turner,           READWRITE, ON_CHANGE,  onTurnerChange);
  ArduinoCloud.addProperty(turnNow,          READWRITE, ON_CHANGE,  onTurnNowChange);
  ArduinoCloud.addProperty(resetCycle,       READWRITE, ON_CHANGE,  onResetCycleChange);
  ArduinoCloud.addProperty(outOfRange,       READ,      ON_CHANGE,  NULL);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
