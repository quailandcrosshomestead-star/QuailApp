// =====================================================================
//  Phone-Controlled ESP32 Blimp  (single board, no transmitter)
// ---------------------------------------------------------------------
//  The ESP32 on the gondola creates its own WiFi hotspot and serves a
//  touchscreen controller to your phone's browser. Control messages
//  come back over a WebSocket ~20x/second and drive three brushed DC
//  motors through two DRV8833 H-bridges:
//
//     * Left thruster   (horizontal)
//     * Right thruster  (horizontal)
//     * Vertical motor  (up / down)
//
//  Differential thrust:  left = forward + yaw,  right = forward - yaw
//  so both thrusters together = go forward/back, opposite = turn.
//
//  ---- REQUIRED LIBRARIES (install via Arduino Library Manager) -------
//     * "ESP Async WebServer"  by ESP32Async  (me-no-dev fork)
//     * "Async TCP"            by ESP32Async
//  Target: ESP32 dev module, Arduino-ESP32 core 3.x.
// =====================================================================

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ----------------------- WIFI HOTSPOT --------------------------------
// Your phone joins this network, then you open http://192.168.4.1
// Password must be >= 8 chars (WPA2). Change these to whatever you like.
static const char *AP_SSID = "Blimp-01";
static const char *AP_PASS = "flyblimp";   // >= 8 characters

// ----------------------- MOTOR WIRING --------------------------------
// Each brushed motor uses TWO pins (an H-bridge half each): PWM one and
// hold the other LOW to spin one way; swap for reverse.
//
// DRV8833 #1  ->  Left + Right thrusters
// DRV8833 #2  ->  Vertical motor
static const int LEFT_A  = 25;   // DRV8833 #1  AIN1
static const int LEFT_B  = 26;   // DRV8833 #1  AIN2
static const int RIGHT_A = 27;   // DRV8833 #1  BIN1
static const int RIGHT_B = 14;   // DRV8833 #1  BIN2
static const int VERT_A  = 32;   // DRV8833 #2  AIN1
static const int VERT_B  = 33;   // DRV8833 #2  AIN2

// Optional hardware failsafe: tie both DRV8833 nSLEEP pins here
// (HIGH = enabled, LOW = coast). Set to -1 and wire nSLEEP to 3V3 to skip.
static const int SLEEP_PIN  = 13;
static const int STATUS_LED = 2;    // onboard LED on most dev modules

// If a motor spins the wrong way, flip its flag (no re-soldering needed).
static const bool LEFT_REVERSE  = false;
static const bool RIGHT_REVERSE = false;
static const bool VERT_REVERSE  = false;

// ----------------------- PWM / TUNING --------------------------------
static const int      PWM_FREQ = 20000;           // 20 kHz = silent
static const int      PWM_RES  = 10;              // 10-bit -> 0..1023
static const int      PWM_MAX  = (1 << PWM_RES) - 1;

static const int      AXIS_FS  = 1000;            // axis range from the page
static const int      MOTOR_DEADSTART = 60;       // min duty a small motor turns at
static const int      SLEW_PER_UPDATE = 40;       // softer = gentler ramp
static const uint32_t FAILSAFE_MS = 500;          // cut motors if link goes quiet

// ----------------------- BATTERY SENSE -------------------------------
// Read the 1S LiPo through a resistor divider on an ADC1 pin (ADC2 is
// unusable while WiFi is on). Battery+ -> R1 -> BATT_PIN -> R2 -> GND.
// With R1 = R2 = 100k, 4.2 V at the battery reads ~2.1 V at the pin.
static const int    BATT_PIN     = 34;            // ADC1, input-only pin
static const float  BATT_R1      = 100000.0;      // top resistor (to battery +)
static const float  BATT_R2      = 100000.0;      // bottom resistor (to GND)
static const float  BATT_CAL     = 1.00;          // fine-tune vs. a multimeter
static const float  BATT_DIVIDER = (BATT_R1 + BATT_R2) / BATT_R2;  // = 2.0
static const float  BATT_EMA     = 0.15;          // smoothing (0..1, lower = smoother)

// ----------------------- STATE ---------------------------------------
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

volatile int      inFwd = 0, inYaw = 0, inVert = 0;   // latest command axes
volatile bool     inArmed = false;
volatile uint32_t lastCmdMs = 0;

int curLeft = 0, curRight = 0, curVert = 0;           // slew-limited duty
float battVolts = 0.0;                                 // smoothed battery voltage

// =====================================================================
//  Control page (served to the phone). Two touch joysticks + ARM.
// =====================================================================
static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<title>Blimp</title>
<style>
:root{--bg:#0e1116;--fg:#e6edf3;--accent:#3b82f6;--danger:#ef4444;--ok:#22c55e;--pad:#1c2430;}
*{box-sizing:border-box;-webkit-user-select:none;user-select:none;-webkit-tap-highlight-color:transparent;}
html,body{margin:0;height:100%;background:var(--bg);color:var(--fg);
  font-family:-apple-system,system-ui,Segoe UI,Roboto,sans-serif;overflow:hidden;
  touch-action:none;overscroll-behavior:none;}
#top{position:fixed;top:0;left:0;right:0;height:58px;display:flex;align-items:center;
  justify-content:space-between;padding:0 16px;z-index:10;}
#left-info{display:flex;align-items:center;gap:12px;}
#status{font-size:14px;opacity:.85;}
#status b{color:var(--ok);} #status.off b{color:var(--danger);}
#batt{font-size:14px;font-weight:700;padding:5px 11px;border-radius:9px;
  background:var(--pad);border:1px solid #2b3542;color:var(--fg);white-space:nowrap;}
#batt.ok{color:var(--ok);} #batt.warn{color:#f59e0b;}
#batt.crit{color:#fff;background:var(--danger);border-color:var(--danger);}
#arm{border:0;border-radius:12px;padding:14px 26px;font-size:17px;font-weight:800;
  color:#fff;background:var(--ok);letter-spacing:.5px;}
#arm.armed{background:var(--danger);}
.zone{position:fixed;top:58px;bottom:0;width:50%;}
#left{left:0;} #right{right:0;border-left:1px solid #1b2430;}
.stick{position:absolute;width:150px;height:150px;margin:-75px 0 0 -75px;border-radius:50%;
  background:var(--pad);border:2px solid #2b3542;}
.thumb{position:absolute;left:50%;top:50%;width:74px;height:74px;margin:-37px 0 0 -37px;
  border-radius:50%;background:var(--accent);opacity:.92;}
.label{position:absolute;bottom:16px;width:100%;text-align:center;font-size:12px;opacity:.45;}
</style></head><body>
<div id="top">
  <div id="left-info">
    <div id="status" class="off">status: <b>connecting…</b></div>
    <div id="batt">–.–– V</div>
  </div>
  <button id="arm">ARM</button>
</div>
<div class="zone" id="left"><div class="label">throttle (up/down) &nbsp;·&nbsp; turn (left/right)</div></div>
<div class="zone" id="right"><div class="label">altitude (up/down)</div></div>
<script>
var armed=false, fwd=0, yaw=0, vert=0;
var arm=document.getElementById('arm'), st=document.getElementById('status');
var bt=document.getElementById('batt');
// 1S LiPo thresholds: land by ~3.5 V, critical by ~3.4 V.
function showBatt(v){
  bt.textContent=v.toFixed(2)+' V';
  bt.className=(v>=3.6)?'ok':(v>=3.4)?'warn':'crit';
}
function setArm(a){armed=a;arm.classList.toggle('armed',a);arm.textContent=a?'DISARM':'ARM';}
arm.addEventListener('click',function(){setArm(!armed);});

function expo(n){var e=0.40;return (1-e)*n+e*n*n*n;}

function zone(id,cb){
  var z=document.getElementById(id),stick=null,thumb=null,tid=null,cx=0,cy=0,R=62;
  function make(x,y){
    stick=document.createElement('div');stick.className='stick';
    thumb=document.createElement('div');thumb.className='thumb';
    stick.appendChild(thumb);z.appendChild(stick);
    stick.style.left=x+'px';stick.style.top=y+'px';cx=x;cy=y;
  }
  function clear(){if(stick){stick.remove();stick=null;}tid=null;cb(0,0);}
  z.addEventListener('touchstart',function(e){
    for(var i=0;i<e.changedTouches.length;i++){var t=e.changedTouches[i];
      if(tid===null){tid=t.identifier;var r=z.getBoundingClientRect();
        make(t.clientX-r.left,t.clientY-r.top);}}
    e.preventDefault();},{passive:false});
  z.addEventListener('touchmove',function(e){
    for(var i=0;i<e.changedTouches.length;i++){var t=e.changedTouches[i];
      if(t.identifier===tid){var r=z.getBoundingClientRect();
        var dx=(t.clientX-r.left)-cx, dy=(t.clientY-r.top)-cy;
        var d=Math.hypot(dx,dy); if(d>R){dx=dx/d*R;dy=dy/d*R;}
        thumb.style.transform='translate('+dx+'px,'+dy+'px)';
        cb(dx/R,-dy/R);}}   // up = positive
    e.preventDefault();},{passive:false});
  z.addEventListener('touchend',function(e){
    for(var i=0;i<e.changedTouches.length;i++){if(e.changedTouches[i].identifier===tid)clear();}
    e.preventDefault();},{passive:false});
  z.addEventListener('touchcancel',function(e){
    for(var i=0;i<e.changedTouches.length;i++){if(e.changedTouches[i].identifier===tid)clear();}},{passive:false});
}
zone('left', function(x,y){yaw=x;fwd=y;});
zone('right',function(x,y){vert=y;});

var ws;
function connect(){
  ws=new WebSocket('ws://'+location.host+'/ws');
  ws.onopen =function(){st.className='';st.innerHTML='status: <b>connected</b>';};
  ws.onclose=function(){st.className='off';st.innerHTML='status: <b>reconnecting…</b>';
    bt.className='';bt.textContent='–.–– V';setArm(false);setTimeout(connect,600);};
  ws.onerror=function(){ws.close();};
  ws.onmessage=function(ev){
    if(typeof ev.data==='string'&&ev.data.charAt(0)==='v')showBatt(parseFloat(ev.data.slice(2)));
  };
}
connect();
setInterval(function(){
  if(!ws||ws.readyState!==1)return;
  var f=Math.round(expo(fwd)*1000),y=Math.round(expo(yaw)*1000),v=Math.round(expo(vert)*1000);
  ws.send(f+','+y+','+v+','+(armed?1:0));
},50);
</script></body></html>
)HTML";

// =====================================================================
//  Motor helpers
// =====================================================================
void driveMotor(int pinA, int pinB, int value, bool reverse) {
  value = constrain(value, -PWM_MAX, PWM_MAX);
  if (reverse) value = -value;
  if (value >= 0) { ledcWrite(pinA, value);  ledcWrite(pinB, 0); }
  else            { ledcWrite(pinA, 0);      ledcWrite(pinB, -value); }
}

int axisToDuty(int axis) {
  int duty = (int)((long)axis * PWM_MAX / AXIS_FS);
  duty = constrain(duty, -PWM_MAX, PWM_MAX);
  if (abs(duty) < MOTOR_DEADSTART) {
    if (abs(duty) < MOTOR_DEADSTART / 3) return 0;
    duty = (duty > 0) ? MOTOR_DEADSTART : -MOTOR_DEADSTART;
  }
  return duty;
}

int slew(int cur, int target) {
  if (target > cur) return min(cur + SLEW_PER_UPDATE, target);
  if (target < cur) return max(cur - SLEW_PER_UPDATE, target);
  return cur;
}

// Read the battery through the divider and smooth it (EMA).
void sampleBattery() {
  float mv = analogReadMilliVolts(BATT_PIN);          // calibrated pin millivolts
  float v = (mv / 1000.0) * BATT_DIVIDER * BATT_CAL;  // scale back to real voltage
  if (battVolts <= 0.01) battVolts = v;               // seed on first read
  else battVolts += BATT_EMA * (v - battVolts);
}

// =====================================================================
//  WebSocket: parse "f,y,v,a" text frames from the phone
// =====================================================================
void onWsEvent(AsyncWebSocket *srv, AsyncWebSocketClient *client,
               AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    char buf[48];
    size_t n = len < sizeof(buf) - 1 ? len : sizeof(buf) - 1;
    memcpy(buf, data, n);
    buf[n] = 0;
    int f, y, v, a;
    if (sscanf(buf, "%d,%d,%d,%d", &f, &y, &v, &a) == 4) {
      inFwd  = constrain(f, -AXIS_FS, AXIS_FS);
      inYaw  = constrain(y, -AXIS_FS, AXIS_FS);
      inVert = constrain(v, -AXIS_FS, AXIS_FS);
      inArmed = (a != 0);
      lastCmdMs = millis();
    }
  } else if (type == WS_EVT_DISCONNECT) {
    inArmed = false;   // stop the instant the browser drops
  }
}

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\n[blimp] booting");

  pinMode(STATUS_LED, OUTPUT);
  if (SLEEP_PIN >= 0) { pinMode(SLEEP_PIN, OUTPUT); digitalWrite(SLEEP_PIN, HIGH); }

  const int pins[] = {LEFT_A, LEFT_B, RIGHT_A, RIGHT_B, VERT_A, VERT_B};
  for (int pin : pins) ledcAttach(pin, PWM_FREQ, PWM_RES);

  // Battery ADC: 12-bit, full 0..~3.1 V input range on the sense pin.
  analogReadResolution(12);
  analogSetPinAttenuation(BATT_PIN, ADC_11db);
  sampleBattery();

  // Bring up the WiFi hotspot.
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("[blimp] AP \"");  Serial.print(AP_SSID);
  Serial.print("\"  ->  http://"); Serial.println(WiFi.softAPIP());  // 192.168.4.1

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
    req->send_P(200, "text/html", INDEX_HTML);
  });
  // Any other path (phone "captive portal" probes) -> the control page.
  server.onNotFound([](AsyncWebServerRequest *req) {
    req->send_P(200, "text/html", INDEX_HTML);
  });
  server.begin();
  Serial.println("[blimp] ready - join the WiFi and press ARM");
}

// =====================================================================
void loop() {
  ws.cleanupClients();

  uint32_t now = millis();

  // Sample the battery a few times a second and push it to the phone once
  // a second (as a "v:3.85" text frame the control page parses).
  static uint32_t lastBattSample = 0, lastBattSend = 0;
  if (now - lastBattSample >= 250) {
    lastBattSample = now;
    sampleBattery();
  }
  if (now - lastBattSend >= 1000) {
    lastBattSend = now;
    ws.textAll("v:" + String(battVolts, 2));
  }

  static uint32_t lastUpdate = 0;
  if (now - lastUpdate < 10) return;      // ~100 Hz control loop
  lastUpdate = now;

  bool linkFresh = (now - lastCmdMs < FAILSAFE_MS);
  bool armed = linkFresh && inArmed;

  int tLeft = 0, tRight = 0, tVert = 0;
  if (armed) {
    int left  = inFwd + inYaw;
    int right = inFwd - inYaw;
    int m = max(abs(left), abs(right));       // keep turn ratio if saturated
    if (m > AXIS_FS) { left = (int)((long)left * AXIS_FS / m);
                       right = (int)((long)right * AXIS_FS / m); }
    tLeft  = axisToDuty(left);
    tRight = axisToDuty(right);
    tVert  = axisToDuty(inVert);
  }

  curLeft  = slew(curLeft,  tLeft);
  curRight = slew(curRight, tRight);
  curVert  = slew(curVert,  tVert);

  driveMotor(LEFT_A,  LEFT_B,  curLeft,  LEFT_REVERSE);
  driveMotor(RIGHT_A, RIGHT_B, curRight, RIGHT_REVERSE);
  driveMotor(VERT_A,  VERT_B,  curVert,  VERT_REVERSE);

  // LED: solid = armed, slow blink = phone connected/safe, fast = no link.
  if (armed)          digitalWrite(STATUS_LED, HIGH);
  else if (linkFresh) digitalWrite(STATUS_LED, (now / 500) % 2);
  else                digitalWrite(STATUS_LED, (now / 100) % 2);
}
