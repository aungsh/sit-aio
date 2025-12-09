/*
 * Room Module – Minimal:
 * - Hosts AP (e.g. "Room-E503") for student TinyScreen devices
 * - Also connects as STA to campus Wi-Fi for internet (WIFI_AP_STA)
 * - sessionNonce auto-rotates every 10 minutes
 * - /session: returns module info + sessionNonce + seqLen
 * - /check:   proxies to backend /api/attendance/check
 * - /begin:   verifies color code, then POSTs /api/attendance
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <Adafruit_TCS34725.h>
#include <math.h>

// ---------- I2C / Sensor ----------
#define SDA_PIN      D2
#define SCL_PIN      D1
#define TCS_LED_PIN  D7   // LED control

Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_154MS,
  TCS34725_GAIN_4X
);

// ---------- STA reconnect state ----------
bool staConnecting = false;
unsigned long lastStaAttemptMs = 0;
const unsigned long STA_RETRY_INTERVAL_MS = 30000;  // 30s between retries

wl_status_t lastStaStatus = WL_IDLE_STATUS;


// ---------- WiFi / HTTP ----------
// AP for student devices (also used as classroomName)
const char* AP_SSID     = "Room-E503";
const char* AP_PASSWORD = "room12345";

// STA (internet) to hotspot or campus WiFi
const char* STA_SSID     = "";
const char* STA_PASSWORD = "";

// Backend server info
const char* BACKEND_HOST = "134.185.93.17";
const uint16_t BACKEND_PORT = 8080;
const char* BACKEND_PATH = "/api/attendance";
const char* BACKEND_CHECK_PATH = "/api/attendance/check";

ESP8266WebServer server(80);

// ---------- Session / Config ----------

// Number of colour slots in the verification code
const uint8_t CODE_LEN = 6;

// These are just for display to students (backend is real source of truth)
String moduleCode   = "PM101";
String startTimeStr = "09:00";
String endTimeStr   = "10:00";

// Dynamic session nonce – auto-rotated every 10 minutes
String sessionNonce = "";

// Nonce rotation timer (10 minutes)
unsigned long lastNonceMs = 0;
const unsigned long NONCE_PERIOD_MS = 10UL * 60UL * 1000UL; // 10 min

// Concurrency control for /begin
bool   isCapturing   = false;
String activeStudent = "";

// ---------- Color classification ----------

// Convert scaled RGB [0..255] to HSV
void rgbToHsv(uint16_t r, uint16_t g, uint16_t b,
              float &h, float &s, float &v) {
  float rf = r / 255.0f;
  float gf = g / 255.0f;
  float bf = b / 255.0f;

  float maxVal = max(rf, max(gf, bf));
  float minVal = min(rf, min(gf, bf));
  float delta  = maxVal - minVal;

  v = maxVal;

  if (maxVal <= 0.0f) {
    s = 0.0f; h = 0.0f; return;
  }

  s = (delta <= 0.0f) ? 0.0f : (delta / maxVal);
  if (delta <= 0.0f) { h = 0.0f; return; }

  if (maxVal == rf) {
    h = 60.0f * fmod(((gf - bf) / delta), 6.0f);
  } else if (maxVal == gf) {
    h = 60.0f * (((bf - rf) / delta) + 2.0f);
  } else {
    h = 60.0f * (((rf - gf) / delta) + 4.0f);
  }
  if (h < 0.0f) h += 360.0f;
}

// returns label: "BLUE", "GREEN", "RED", "YELLOW", "GRAY", "BLACK", "WHITE", ...
String classifyColor(uint16_t r, uint16_t g, uint16_t b, uint16_t c) {
  // Very low light = black
  if (c < 80) return "BLACK";

  // Scale raw RGB by clear channel to 0..255
  float scale = 255.0f / max(1, (int)c);
  uint16_t rs = (uint16_t)min(255, (int)(r * scale));
  uint16_t gs = (uint16_t)min(255, (int)(g * scale));
  uint16_t bs = (uint16_t)min(255, (int)(b * scale));

  // Primary-like thresholds
  if (rs > 220 && gs > 220 && bs < 80) return "YELLOW";
  if (bs > 220 && rs < 80 && gs < 80) return "BLUE";
  if (rs > 220 && gs < 80 && bs < 80) return "RED";
  if (gs > 220 && rs < 80 && bs < 80) return "GREEN";

  // HSV fallback
  float h, s, v;
  rgbToHsv(rs, gs, bs, h, s, v);

  if (v < 0.15f) return "BLACK";

  if (s < 0.20f) {
    if (v > 0.75f) return "WHITE";
    if (v > 0.25f) return "GRAY";
    return "BLACK";
  }

  if (h < 20.0f)  return "RED";
  if (h < 80.0f)  return "YELLOW";
  if (h < 160.0f) return "GREEN";
  if (h < 260.0f) return "BLUE";

  return "UNKNOWN";
}

// label -> index (0..4 or 255)
uint8_t labelToIndex(const String& s) {
  if (s == "BLUE")   return 0;
  if (s == "GREEN")  return 1;
  if (s == "RED")    return 2;
  if (s == "YELLOW") return 3;
  if (s == "BLACK")  return 4;  // prefix bucket
  return 255;
}

// ---------- Prefix ----------
const uint8_t PREFIX_LEN = 3;
const uint8_t PREFIX[PREFIX_LEN] = { 3, 0, 4 };  // YELLOW-BLUE-BLACK/WHITE-ish

const uint8_t EXTRA_MARGIN_SLOTS = 6;
const uint8_t TOTAL_SLOTS =
  PREFIX_LEN + CODE_LEN + EXTRA_MARGIN_SLOTS;

// ---------- PRNG ----------

uint32_t fnv1aHash(const String& s) {
  uint32_t h = 2166136261UL;
  for (size_t i = 0; i < s.length(); i++) {
    h ^= (uint8_t)s[i];
    h *= 16777619UL;
  }
  return h;
}

uint32_t nextRand(uint32_t& st) {
  st = st * 1664525UL + 1013904223UL;
  return st;
}

// Generate random nonce (16 hex chars)
void randomizeNonce() {
  const char hexChars[] = "0123456789ABCDEF";
  char buf[17];
  for (int i = 0; i < 16; i++) {
    int v = random(16);
    buf[i] = hexChars[v];
  }
  buf[16] = '\0';
  sessionNonce = String(buf);
  Serial.print(F("New sessionNonce = "));
  Serial.println(sessionNonce);
}

void generateExpectedCode(const String& sid, uint8_t* outCode) {
  String seed = sessionNonce + ":" + sid;
  uint32_t st = fnv1aHash(seed);
  for (uint8_t i = 0; i < CODE_LEN; i++) {
    outCode[i] = nextRand(st) % 4;  // 0..3 = RGBY
  }
}

// ---------- Capture ----------

uint8_t captureOneSlot(uint16_t slotMs) {
  uint32_t start = millis();
  const uint16_t SAMPLE_MS = 40;

  uint16_t counts[5] = {0,0,0,0,0};

  while (millis() - start < slotMs) {
    uint16_t r, g, b, c;
    tcs.getRawData(&r, &g, &b, &c);
    String label = classifyColor(r, g, b, c);
    uint8_t idx  = labelToIndex(label);
    if (idx <= 4) counts[idx]++;
    delay(SAMPLE_MS);
  }

  uint8_t bestIdx = 255;
  uint16_t bestCount = 0;
  for (uint8_t i = 0; i < 5; i++) {
    if (counts[i] > bestCount) {
      bestCount = counts[i];
      bestIdx   = i;
    }
  }
  return (bestCount == 0) ? 255 : bestIdx;
}

void captureStream(uint8_t* stream) {
  Serial.println(F("Capturing stream..."));
  const uint16_t SLOT_MS = 600;

  for (uint8_t i = 0; i < TOTAL_SLOTS; i++) {
    uint8_t idx = captureOneSlot(SLOT_MS);
    stream[i] = idx;
  }
}

// Find prefix and return index where CODE starts
int findPrefixStart(const uint8_t* s) {
  for (int i = 0; i + PREFIX_LEN + CODE_LEN <= TOTAL_SLOTS; i++) {
    bool ok = true;
    for (int j = 0; j < PREFIX_LEN; j++) {
      if (s[i + j] != PREFIX[j]) { ok = false; break; }
    }
    if (ok) return i + PREFIX_LEN;
  }
  return -1;
}

int hammingDistance(const uint8_t* a, const uint8_t* b) {
  int d = 0;
  for (uint8_t i = 0; i < CODE_LEN; i++) {
    if (a[i] != b[i]) d++;
  }
  return d;
}

// ---------- HTTP Helpers ----------

String makeSessionJson() {
  String json = "{\n";
  json += "  \"moduleCode\":  \"" + moduleCode + "\",\n";
  json += "  \"startTime\":   \"" + startTimeStr + "\",\n";
  json += "  \"endTime\":     \"" + endTimeStr + "\",\n";
  json += "  \"sessionId\":   \"" + moduleCode + "-" + startTimeStr + "\",\n";
  json += "  \"sessionNonce\":\"" + sessionNonce + "\",\n";
  json += "  \"seqLen\":      " + String(CODE_LEN) + "\n";
  json += "}\n";
  return json;
}

void handleRoot() {
  server.send(200, "text/plain", "Room Module OK");
}

void handleSession() {
  String body = makeSessionJson();
  server.send(200, "application/json", body);
}

void handleNotFound() {
  server.send(404, "text/plain", "404 Not Found");
}

// ---------- Internet POST to backend (/api/attendance) ----------

bool sendAttendanceToServer(const String& sid, const String& sname) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[POST] No STA internet connection."));
    return false;
  }

  WiFiClient client;
  HTTPClient http;

  String url = String("http://") + BACKEND_HOST + ":" + BACKEND_PORT + BACKEND_PATH;
  Serial.print(F("[POST] URL: "));
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println(F("[POST] http.begin failed"));
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  int sidInt = sid.toInt();  // "2501234" -> 2501234

  // Build JSON payload
  String payload = "{";
  payload += "\"studentId\":"     + String(sidInt) + ",";
  payload += "\"studentName\":\"" + sname + "\",";
  payload += "\"classroomName\":\"" + String(AP_SSID) + "\"";
  payload += "}";

  Serial.print(F("[POST] Payload: "));
  Serial.println(payload);

  int code = http.POST(payload);
  if (code > 0) {
    Serial.print(F("[POST] HTTP code: "));
    Serial.println(code);
    String resp = http.getString();
    Serial.print(F("[POST] Response: "));
    Serial.println(resp);
  } else {
    Serial.print(F("[POST] Error: "));
    Serial.println(http.errorToString(code));
  }

  http.end();
  return (code >= 200 && code < 300);
}

// ---------- Internet POST to backend (/api/attendance/check) ----------

enum RemoteCheckResult {
  REMOTE_CHECK_ERROR,
  REMOTE_CHECK_FREE,   // taken == false
  REMOTE_CHECK_TAKEN   // taken == true
};

uint8_t checkAttendanceOnBackend(const String& sid, const String& sname) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(F("[CHECK] No STA internet connection."));
    return REMOTE_CHECK_ERROR;
  }

  WiFiClient client;
  HTTPClient http;

  String url = String("http://") + BACKEND_HOST + ":" + BACKEND_PORT + BACKEND_CHECK_PATH;
  Serial.print(F("[CHECK] URL: "));
  Serial.println(url);

  if (!http.begin(client, url)) {
    Serial.println(F("[CHECK] http.begin failed"));
    return REMOTE_CHECK_ERROR;
  }

  http.addHeader("Content-Type", "application/json");

  int sidInt = sid.toInt();

  String payload = "{";
  payload += "\"studentId\":"     + String(sidInt) + ",";
  payload += "\"studentName\":\"" + sname + "\",";
  payload += "\"classroomName\":\"" + String(AP_SSID) + "\"";
  payload += "}";

  Serial.print(F("[CHECK] Payload: "));
  Serial.println(payload);

  int code = http.POST(payload);
  if (code <= 0) {
    Serial.print(F("[CHECK] POST error: "));
    Serial.println(http.errorToString(code));
    http.end();
    return REMOTE_CHECK_ERROR;
  }

  String resp = http.getString();
  http.end();

  Serial.print(F("[CHECK] HTTP "));
  Serial.println(code);
  Serial.print(F("[CHECK] Body: "));
  Serial.println(resp);

  if (resp.indexOf("\"taken\":true")  >= 0) return REMOTE_CHECK_TAKEN;
  if (resp.indexOf("\"taken\":false") >= 0) return REMOTE_CHECK_FREE;
  return REMOTE_CHECK_ERROR;
}

// ---------- /check endpoint for TinyScreen ----------

void handleCheck() {
  if (!server.hasArg("studentId")) {
    server.send(400, "text/plain", "Missing studentId");
    return;
  }

  String sid = server.arg("studentId");
  sid.trim();

  String sname = "";
  if (server.hasArg("studentName")) {
    sname = server.arg("studentName");
  }

  uint8_t r = checkAttendanceOnBackend(sid, sname);

  if (r == REMOTE_CHECK_FREE) {
    server.send(200, "application/json",
      "{\"status\":\"ok\",\"taken\":false}");
  } else if (r == REMOTE_CHECK_TAKEN) {
    server.send(200, "application/json",
      "{\"status\":\"ok\",\"taken\":true}");
  } else {
    server.send(200, "application/json",
      "{\"status\":\"error\"}");
  }
}

// ---------- /begin with busy handling + POST ----------

void handleBegin() {
  // If we are already in the middle of a capture, reject new requests
  if (isCapturing) {
    server.send(200, "application/json",
      "{\"status\":\"busy\",\"reason\":\"room_module_in_use\"}");
    return;
  }

  if (!server.hasArg("studentId")) {
    server.send(400, "text/plain", "Missing studentId");
    return;
  }

  // Optional: studentName argument from client
  String sname = "";
  if (server.hasArg("studentName")) {
    sname = server.arg("studentName");
  }

  isCapturing = true;
  String sid = server.arg("studentId");
  sid.trim();
  activeStudent = sid;

  Serial.println(F("----- /begin called -----"));
  Serial.print(F("Student: ")); Serial.println(sid);
  Serial.print(F("Name: "));    Serial.println(sname);

  auto sendAndUnlock = [&](int code, const String &type, const String &body) {
    server.send(code, type, body);
    isCapturing = false;
    activeStudent = "";
  };

  uint8_t expected[CODE_LEN];
  generateExpectedCode(sid, expected);

  Serial.print(F("Expected CODE: "));
  for (uint8_t i = 0; i < CODE_LEN; i++) {
    Serial.print(expected[i]); Serial.print(" ");
  }
  Serial.println();

  uint8_t stream[TOTAL_SLOTS];
  captureStream(stream);

  Serial.print(F("Stream: "));
  for (uint8_t i = 0; i < TOTAL_SLOTS; i++) {
    if (stream[i] == 255) Serial.print("x ");
    else { Serial.print(stream[i]); Serial.print(" "); }
  }
  Serial.println();

  int codeStart = findPrefixStart(stream);
  if (codeStart < 0) {
    Serial.println(F("Prefix not found."));
    sendAndUnlock(200, "application/json",
      "{\"status\":\"fail\",\"reason\":\"prefix_not_found\"}");
    return;
  }

  Serial.print(F("Prefix found at slot "));
  Serial.println(codeStart - PREFIX_LEN);

  uint8_t observed[CODE_LEN];
  for (uint8_t i = 0; i < CODE_LEN; i++) {
    observed[i] = stream[codeStart + i];
  }

  Serial.print(F("Observed CODE: "));
  for (uint8_t i = 0; i < CODE_LEN; i++) {
    Serial.print(observed[i]); Serial.print(" ");
  }
  Serial.println();

  int d = hammingDistance(expected, observed);
  Serial.print(F("Hamming distance = "));
  Serial.println(d);

  const uint8_t MAX_ALLOWED_DIFF = 2;  // tolerate up to 2 misreads

  if (d <= MAX_ALLOWED_DIFF) {
    Serial.println(F("CODE MATCH (within tolerance). Posting to backend."));

    bool postOk = sendAttendanceToServer(sid, sname);
    if (!postOk) {
      Serial.println(F("[WARN] Failed to POST attendance."));
    }

    sendAndUnlock(200, "application/json",
      "{\"status\":\"ok\",\"match\":true}");
  } else {
    Serial.println(F("CODE MISMATCH."));
    sendAndUnlock(200, "application/json",
      "{\"status\":\"ok\",\"match\":false}");
  }
}

// ---------- setup / loop ----------

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("=== Room Module – Minimal + Nonce rotation + /check ==="));

  Wire.begin(SDA_PIN, SCL_PIN);

  if (!tcs.begin()) {
    Serial.println(F("No TCS34725 found!"));
    while (1) delay(1000);
  }
  Serial.println(F("TCS34725 OK."));

  pinMode(TCS_LED_PIN, OUTPUT);
  digitalWrite(TCS_LED_PIN, LOW);   // LED off

  // Let ESP8266 core handle RNG seeding (hardware RNG)
  randomizeNonce();
  lastNonceMs = millis();

  /* Seed RNG and generate first nonce
  randomSeed(analogRead(A0));
  randomizeNonce();
  lastNonceMs = millis();
  */

  // AP + STA mode: host classroom AP and connect to internet Wi-Fi
  WiFi.mode(WIFI_AP_STA);

  // Start AP for student devices
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print(F("AP ")); Serial.print(AP_SSID);
  Serial.print(F(" IP: ")); Serial.println(WiFi.softAPIP());

  // Connect STA to campus Wi-Fi for internet
  WiFi.begin(STA_SSID, STA_PASSWORD);
  Serial.print(F("Connecting STA to ")); Serial.println(STA_SSID);

  uint8_t tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 20) {
    delay(500);
    Serial.print(".");
    tries++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("STA connected, IP: "));
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(F("STA connect failed, will keep retrying in background."));
  }

  // Init reconnect state
  staConnecting    = (WiFi.status() != WL_CONNECTED);
  lastStaAttemptMs = millis();
  lastStaStatus    = WiFi.status();

  server.on("/",        HTTP_GET,  handleRoot);
  server.on("/session", HTTP_GET,  handleSession);
  server.on("/begin",   HTTP_POST, handleBegin);
  server.on("/check",   HTTP_POST, handleCheck);
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println(F("HTTP server started."));
}

void loop() {
  server.handleClient();

  unsigned long now = millis();

  // Rotate nonce every 10 minutes
  if ((unsigned long)(now - lastNonceMs) >= NONCE_PERIOD_MS) {
    randomizeNonce();
    lastNonceMs = now;
  }

  // ---------- STA reconnect (non-blocking) ----------
  if (!isCapturing) {
    wl_status_t st = WiFi.status();

    // Detect transition: was not connected, now connected
    if (st == WL_CONNECTED && lastStaStatus != WL_CONNECTED) {
      Serial.print(F("[STA] Reconnected, IP: "));
      Serial.println(WiFi.localIP());
      staConnecting = false;
    }

    // If disconnected, retry every STA_RETRY_INTERVAL_MS
    if (st != WL_CONNECTED) {
      if (!staConnecting && (now - lastStaAttemptMs) >= STA_RETRY_INTERVAL_MS) {
        Serial.println(F("[STA] Disconnected, retrying WiFi.begin()..."));
        WiFi.begin(STA_SSID, STA_PASSWORD);
        staConnecting    = true;
        lastStaAttemptMs = now;
      }
    }

    // Remember status for next loop
    lastStaStatus = st;
  }
}