#include <Wire.h>
#include <TinyScreen.h>
#include <ArduinoJson.h>
#include <WiFi101.h>

#if defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#else
#define SerialMonitorInterface Serial
#endif

// ==============================
// Project metadata / display
// ==============================
#define PROJECT_DISPLAY_NAME "CLASSHOOT"

TinyScreen display = TinyScreen(TinyScreenPlus);
#define SCREEN_WIDTH 96

// ==============================
// Screen Drawing Helpers
// ==============================
uint8_t menuTextY[8] = {
  1 * 12 - 1, 2 * 12 - 1, 3 * 12 - 1, 4 * 12 - 1,
  5 * 12 - 1, 6 * 12 - 1, 7 * 12 - 3, 8 * 12 - 3
};

void leftArrow(int x, int y) {
  display.drawLine(x + 2, y - 2, x + 2, y + 2, 0xFFFF);
  display.drawLine(x + 1, y - 1, x + 1, y + 1, 0xFFFF);
  display.drawLine(x + 0, y - 0, x + 0, y + 0, 0xFFFF);
}

void rightArrow(int x, int y) {
  display.drawLine(x + 0, y - 2, x + 0, y + 2, 0xFFFF);
  display.drawLine(x + 1, y - 1, x + 1, y + 1, 0xFFFF);
  display.drawLine(x + 2, y - 0, x + 2, y + 0, 0xFFFF);
}

void upArrow(int x, int y) {
  display.drawLine(x + 0, y - 0, x + 4, y - 0, 0xFFFF);
  display.drawLine(x + 1, y - 1, x + 3, y - 1, 0xFFFF);
  display.drawLine(x + 2, y - 2, x + 2, y - 2, 0xFFFF);
}

void downArrow(int x, int y) {
  display.drawLine(x + 0, y + 0, x + 4, y + 0, 0xFFFF);
  display.drawLine(x + 1, y + 1, x + 3, y + 1, 0xFFFF);
  display.drawLine(x + 2, y + 2, x + 2, y + 2, 0xFFFF);
}

// ==============================
// Wi-Fi Credentials
// ==============================
// TODO: fill in your campus Wi-Fi here
const char* ssid = "";   // your network SSID (name)
const char* pass = "";   // your network password

// ==============================
// Student Credentials
// ==============================
// TODO: fill in your own student details
const int   studentID   = 2500000;        // student ID
const char* studentName = "testttes";       // student name

// ==============================
// Room Module / Attendance config
// ==============================
const char* ROOM_PREFIX = "Room-";
const char* ROOM_PSK    = "room12345";
const char* ROOM_IP     = "192.168.4.1";

const uint8_t PREFIX_LEN = 3;
const uint8_t PREFIX[PREFIX_LEN] = { 3, 0, 4 };  // YELLOW-BLUE-BLACK indexes

// ==============================
// API server (Host or IP) and path (Kahoot backend)
// ==============================
const char *server      = "134.185.93.17";
const int   serverPort  = 8080;
const char *apiPath     = "/api/classhoot/join";
const char *startApiPath= "/api/classhoot/start";
const char *checkApiPath= "/api/classhoot/check";

// ==============================
// Real API responses
// ==============================
String joinResponse  = "";
String startResponse = "";

// ==============================
// Pages
// ==============================
enum Page {
  STANDBY,
  HOME,
  ATTENDANCE,
  VACANCY,
  KAHOOT_DIGIT,
  KAHOOT_CONFIRM,
  KAHOOT_QUESTION,
  KAHOOT_WAIT,
  KAHOOT_END
};

Page currentPage = STANDBY;
bool needsRedraw = true;

// ==============================
// Home Menu
// ==============================
int menuIndex = 0;
const char *menuItems[] = {"Attendance", "Room Vacancy", "Kahoot"};
size_t numMenuItems = 3;

// ==============================
// Kahoot variables
// ==============================
int digits[3]         = {0, 0, 0};
int currentDigit      = 0;
int answer            = 1;
int questionIndex     = 0;
int questionCount     = 0;
int lastQuestionIndex = -1;
int totalScore        = 0;

// ==============================
// Attendance helpers (TinyScreen side)
// ==============================

void clearScreen() {
  display.clearScreen();
  display.setCursor(0, 0);
}

void drawCentered(const char* text, uint8_t y) {
  uint8_t w = display.getPrintWidth((char*)text);
  uint8_t x = (96 - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

uint8_t readButtons() {
  return display.getButtons();
}

// Button helpers for attendance flow
bool btnA(uint8_t b)    { return b & TSButtonUpperLeft;  }   // UL
bool btnB(uint8_t b)    { return b & TSButtonUpperRight; }   // UR
bool btnDown(uint8_t b) { return b & TSButtonLowerLeft;  }   // LL
bool btnUp(uint8_t b)   { return b & TSButtonLowerRight; }   // LR

// Connect to the selected Room- AP
bool connectToAP(const char* ssidRoom) {
  WiFi.disconnect();

  clearScreen();
  drawCentered("Connecting:", 12);
  drawCentered(ssidRoom, 24);

  WiFi.begin(ssidRoom, ROOM_PSK);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    display.setCursor(0, 48);
    display.print(".");
    delay(500);
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    clearScreen();
    drawCentered("Connected!", 24);
    delay(800);
    return true;
  }

  clearScreen();
  drawCentered("WiFi Failed", 20);
  drawCentered("Check Room AP", 34);
  delay(1500);
  return false;
}

// Minimal JSON string parsing helpers (for /session)
String extractJsonString(const String& json, const char* key) {
  String pattern = "\"" + String(key) + "\"";
  int idx = json.indexOf(pattern);
  if (idx < 0) return "";
  idx = json.indexOf(':', idx);
  if (idx < 0) return "";
  idx = json.indexOf('\"', idx);
  if (idx < 0) return "";
  int end = json.indexOf('\"', idx + 1);
  if (end < 0) return "";
  return json.substring(idx + 1, end);
}

int extractJsonInt(const String& json, const char* key, int defVal) {
  String pattern = "\"" + String(key) + "\"";
  int idx = json.indexOf(pattern);
  if (idx < 0) return defVal;
  idx = json.indexOf(':', idx);
  if (idx < 0) return defVal;
  int end = json.indexOf(',', idx);
  if (end < 0) end = json.indexOf('}', idx);
  if (end < 0) return defVal;
  String s = json.substring(idx + 1, end);
  s.trim();
  return s.toInt();
}

// HTTP GET /session from the room module
String fetchSessionJson() {
  WiFiClient client;
  if (!client.connect(ROOM_IP, 80)) {
    return "";
  }

  client.println("GET /session HTTP/1.1");
  client.println("Host: 192.168.4.1");
  client.println("Connection: close");
  client.println();

  String resp;
  unsigned long start = millis();
  while (millis() - start < 3000) {
    while (client.available()) {
      resp += (char)client.read();
    }
    if (!client.connected()) break;
  }
  client.stop();

  int idx = resp.indexOf("\r\n\r\n");
  if (idx >= 0) return resp.substring(idx + 4);
  return resp;
}

// /check result codes
enum {
  CHECK_ERROR = 0,
  CHECK_TAKEN = 1,
  CHECK_FREE  = 2
};

// Call room module /check which proxies to backend /api/attendance/check
uint8_t callRoomCheck(const String& sidStr, const String& snameStr) {
  WiFiClient client;
  if (!client.connect(ROOM_IP, 80)) {
    return CHECK_ERROR;
  }

  String encodedName = snameStr;
  encodedName.replace(" ", "%20");

  String body = String("studentId=") + sidStr +
                "&studentName=" + encodedName;

  client.println("POST /check HTTP/1.1");
  client.print("Host: "); client.println(ROOM_IP);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: "); client.println(body.length());
  client.println("Connection: close");
  client.println();
  client.print(body);

  String resp;
  unsigned long start = millis();
  const unsigned long TIMEOUT_MS = 4000;

  while (millis() - start < TIMEOUT_MS) {
    while (client.available()) {
      resp += (char)client.read();
    }
    if (!client.connected()) break;
  }
  client.stop();

  int idx = resp.indexOf("\r\n\r\n");
  String json = (idx >= 0) ? resp.substring(idx + 4) : resp;

  if (json.indexOf("\"taken\":true")  >= 0) return CHECK_TAKEN;
  if (json.indexOf("\"taken\":false") >= 0) return CHECK_FREE;
  return CHECK_ERROR;
}

// PRNG for colour sequence generation (same as room module)
uint32_t fnv1a(const String& s) {
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

// Colour index -> TinyScreen 8b colour
uint8_t idxToColor8(uint8_t idx) {
  switch (idx) {
    case 0: return TS_8b_Blue;
    case 1: return TS_8b_Green;
    case 2: return TS_8b_Red;
    case 3: return TS_8b_Yellow;
    case 4: return TS_8b_Black;   // reserved prefix colour
  }
  return TS_8b_Black;
}

void fillColor(uint8_t c8) {
  display.clearScreen();
  display.drawRect(0, 0, 96, 64, TSRectangleFilled, c8);
}

// Generate the personal code from nonce + studentID
void generateCode(const String& nonce, uint8_t seqLen, uint8_t* outCode, const String& sidStr) {
  uint32_t st = fnv1a(nonce + ":" + sidStr);
  for (uint8_t i = 0; i < seqLen; i++) {
    outCode[i] = nextRand(st) % 4;  // 0..3 = RGBY
  }
}

// Show colour sequence, call /begin on room module, and display result
void showSequence(const String& moduleCode,
                  const String& startTime,
                  const String& endTime,
                  const String& sessionNonce,
                  uint8_t codeLen,
                  const String& sidStr,
                  const String& snameStr) {
  if (codeLen > 16) codeLen = 16;

  // Generate CODE from nonce + student ID
  uint8_t code[16];
  generateCode(sessionNonce, codeLen, code, sidStr);

  // Build full sequence = PREFIX + CODE
  uint8_t full[32];
  uint8_t total = PREFIX_LEN + codeLen;
  for (uint8_t i = 0; i < PREFIX_LEN; i++) full[i] = PREFIX[i];
  for (uint8_t i = 0; i < codeLen;   i++) full[PREFIX_LEN + i] = code[i];

  // Instruction screen
  clearScreen();
  drawCentered("Show to sensor", 10);
  drawCentered(moduleCode.c_str(), 24);
  // bottom-right = OK for consistency
  drawCentered("BR to start", 40);

  // Wait for bottom-right (LR) to start
  while (true) {
    uint8_t b = readButtons();
    if (btnUp(b)) break;   // LR = OK
    delay(80);
  }
  delay(200);

  // 1) Send POST /begin with studentId + name
  WiFiClient client;
  if (!client.connect(ROOM_IP, 80)) {
    clearScreen();
    drawCentered("Begin failed", 20);
    drawCentered("No connection", 34);
    delay(1500);
    return;
  }

  String encodedName = snameStr;
  encodedName.replace(" ", "%20");

  String body = String("studentId=") + sidStr +
                "&studentName=" + encodedName;

  client.println("POST /begin HTTP/1.1");
  client.print("Host: "); client.println(ROOM_IP);
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: "); client.println(body.length());
  client.println("Connection: close");
  client.println();
  client.print(body);

  // 2) Play colour sequence while room is capturing
  for (uint8_t i = 0; i < total; i++) {
    fillColor(idxToColor8(full[i]));
    delay(600);                        // MUST match room's SLOT_MS
    fillColor(TS_8b_Black);
    delay(150);
  }

  // 3) Read HTTP response from /begin
  String resp;
  unsigned long startMs = millis();
  const unsigned long TIMEOUT_MS = 12000;   // room takes ~9s to capture

  while (millis() - startMs < TIMEOUT_MS) {
    while (client.available()) {
      resp += (char)client.read();
    }
    if (!client.connected()) break;
  }
  client.stop();

  int idx = resp.indexOf("\r\n\r\n");
  String json = (idx >= 0) ? resp.substring(idx + 4) : resp;

  bool markedOK      = false;
  bool codeMismatch  = false;
  bool notWhite      = false;
  bool alreadyMarked = false;
  bool prefixFail    = false;
  bool busy          = false;

  if (json.indexOf("\"status\":\"ok\"") >= 0) {
    if (json.indexOf("\"match\":true") >= 0) markedOK = true;
    else                                     codeMismatch = true;
  } else if (json.indexOf("\"status\":\"reject\"") >= 0) {
    if (json.indexOf("not_whitelisted") >= 0) notWhite = true;
    if (json.indexOf("already_marked")  >= 0) alreadyMarked = true;
  } else if (json.indexOf("\"status\":\"fail\"") >= 0) {
    if (json.indexOf("prefix_not_found") >= 0) prefixFail = true;
  } else if (json.indexOf("\"status\":\"busy\"") >= 0) {
    busy = true;
  }

  clearScreen();
  if (markedOK) {
    drawCentered("Attendance", 18);
    drawCentered("MARKED PRESENT", 32);
  } else if (codeMismatch) {
    drawCentered("Code mismatch", 18);
    drawCentered("Try again", 32);
  } else if (notWhite) {
    drawCentered("Not whitelisted", 24);
  } else if (alreadyMarked) {
    drawCentered("Already marked", 24);
  } else if (prefixFail) {
    drawCentered("Sync failed", 18);
    drawCentered("Try again", 32);
  } else if (busy) {
    drawCentered("Room in use", 18);
    drawCentered("Try again", 32);
  } else {
    drawCentered("No response", 18);
    drawCentered("from room", 32);
  }
  delay(2500);
}

// Reconnect to campus WiFi (ssid/pass globals)
void reconnectCampusWiFi() {
  WiFi.disconnect();
  delay(100);

  SerialMonitorInterface.println();
  SerialMonitorInterface.print("Reconnecting to ");
  SerialMonitorInterface.println(ssid);

  int status = WL_IDLE_STATUS;
  status = WiFi.begin(ssid, pass);

  unsigned long start = millis();
  while (status != WL_CONNECTED && millis() - start < 20000) {
    delay(500);
    SerialMonitorInterface.print(".");
    status = WiFi.status();
  }
  SerialMonitorInterface.println();

  if (WiFi.status() == WL_CONNECTED) {
    SerialMonitorInterface.println("WiFi reconnected");
    SerialMonitorInterface.print("IP: ");
    SerialMonitorInterface.println(WiFi.localIP());
  } else {
    SerialMonitorInterface.println("WiFi reconnection failed");
  }
}

// Main attendance flow (called once when you enter ATTENDANCE)
void runAttendance() {
  String sidStr   = String(studentID);
  String snameStr = String(studentName);
  bool userCancelled = false;

  clearScreen();
  drawCentered("Scanning WiFi...", 24);

  // We'll always reconnect to campus WiFi at the end
  do {
    int n = WiFi.scanNetworks();
    if (n <= 0) {
      clearScreen();
      drawCentered("No networks", 20);
      drawCentered("found.", 34);
      delay(1500);
      break;
    }

    String rooms[10];
    int roomCount = 0;
    for (int i = 0; i < n && roomCount < 10; i++) {
      String ssidRoom = WiFi.SSID(i);
      if (ssidRoom.startsWith(ROOM_PREFIX)) {
        rooms[roomCount++] = ssidRoom;
      }
    }

    if (roomCount == 0) {
      clearScreen();
      drawCentered("No Room-", 20);
      drawCentered("networks.", 34);
      delay(1500);
      break;
    }

    int sel = 0;
    while (true) {
      clearScreen();
      drawCentered("Select Room", 6);

      for (int i = 0; i < roomCount; i++) {
        if (i == sel) {
          display.setCursor(4, 20 + i * 10);
          display.print(">");
        }
        display.setCursor(14, 20 + i * 10);
        display.print(rooms[i]);
      }

      display.setCursor(4, 56);
      // Bottom-left = Back, Bottom-right = OK
      display.print("BL=Back  BR=OK");

      uint8_t b = readButtons();
      // Top row = navigation (keep behaviour intuitive)
      if (btnB(b)) sel = (sel + 1) % roomCount;                     // UR = next
      if (btnA(b)) sel = (sel - 1 + roomCount) % roomCount;         // UL = prev
      // Bottom row = actions
      if (btnUp(b))    break;                                       // LR = OK
      if (btnDown(b)) { userCancelled = true; break; }              // LL = Back
      delay(100);
    }

    if (userCancelled) break;

    String chosen = rooms[sel];
    if (!connectToAP(chosen.c_str())) break;

    clearScreen();
    drawCentered("Fetching", 20);
    drawCentered("session...", 34);

    String json = fetchSessionJson();
    if (json.length() == 0) {
      clearScreen();
      drawCentered("Failed to get", 20);
      drawCentered("/session", 34);
      delay(1500);
      break;
    }

    String moduleCode   = extractJsonString(json, "moduleCode");
    String startTime    = extractJsonString(json, "startTime");
    String endTime      = extractJsonString(json, "endTime");
    String sessionNonce = extractJsonString(json, "sessionNonce");
    int    seqLen       = extractJsonInt(json, "seqLen", 6);

    // Check with backend if attendance already taken
    clearScreen();
    drawCentered("Checking", 20);
    drawCentered("status...", 34);

    uint8_t cr = callRoomCheck(sidStr, snameStr);

    if (cr == CHECK_TAKEN) {
      clearScreen();
      drawCentered("Attendance", 20);
      drawCentered("already taken", 34);
      delay(2000);
      break;
    } else if (cr == CHECK_ERROR) {
      clearScreen();
      drawCentered("Check failed", 20);
      drawCentered("Try again", 34);
      delay(2000);
      break;
    }

    // Show session summary + Start screen
    clearScreen();
    drawCentered(moduleCode.c_str(), 6);

    display.setCursor(6, 20);
    display.print("Time: ");
    display.print(startTime);
    display.print("-");
    display.print(endTime);

    display.setCursor(6, 32);
    display.print("SID: ");
    display.print(sidStr);

    display.setCursor(6, 50);
    // Bottom-left = Back, Bottom-right = Start
    display.print("BL=Back  BR=Start");

    while (true) {
      uint8_t b = readButtons();
      if (btnUp(b)) {   // LR = Start
        showSequence(moduleCode, startTime, endTime, sessionNonce,
                     (uint8_t)seqLen, sidStr, snameStr);
        break;
      }
      if (btnDown(b)) break; // LL = Back
      delay(80);
    }

  } while (false);

  // Reconnect to campus WiFi after attendance flow
  reconnectCampusWiFi();
}

// ==============================
// Setup
// ==============================
void setup()
{
  Wire.begin();
  display.begin();
  display.setBrightness(15);
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);

  SerialMonitorInterface.begin(9600);
  WiFi.setPins(8, 2, A3, -1); // VERY IMPORTANT FOR TINYDUINO
  while (!SerialMonitorInterface)
    ;

  SerialMonitorInterface.println();
  SerialMonitorInterface.print("Connecting to ");
  SerialMonitorInterface.println(ssid);

  int status = WL_IDLE_STATUS;
  status = WiFi.begin(ssid, pass);

  unsigned long start = millis();
  while (status != WL_CONNECTED && millis() - start < 20000)
  {
    delay(500);
    SerialMonitorInterface.print(".");
    status = WiFi.status();
  }
  SerialMonitorInterface.println();

  if (WiFi.status() == WL_CONNECTED)
  {
    SerialMonitorInterface.println("WiFi connected");
    SerialMonitorInterface.print("IP: ");
    SerialMonitorInterface.println(WiFi.localIP());
  }
  else
  {
    SerialMonitorInterface.println("WiFi connection failed");
  }
}

// ==============================
// STANDBY SCREEN
// ==============================
void showStandby() {
  display.clearScreen();

  // Title
  int width = display.getPrintWidth(PROJECT_DISPLAY_NAME);
  display.fontColor(0xFFFF, 0x0000);
  display.setCursor(SCREEN_WIDTH / 2 - width / 2 - 1, menuTextY[0] - 1);
  display.print(PROJECT_DISPLAY_NAME);
  display.setCursor(SCREEN_WIDTH / 2 - width / 2 + 1, menuTextY[0] + 1);
  display.print(PROJECT_DISPLAY_NAME);

  // Name
  width = display.getPrintWidth((char *)studentName);
  display.setCursor(SCREEN_WIDTH / 2 - width / 2, menuTextY[1] + 1);
  display.print(studentName);

  // SID
  String sidStr = String(studentID);
  width = display.getPrintWidth((char *)sidStr.c_str());
  display.setCursor(SCREEN_WIDTH / 2 - width / 2, menuTextY[2] + 1);
  display.print(sidStr.c_str());

  // Hint: BL = Menu
  leftArrow(0, 45 + 2);
  display.drawLine(1, 54, 1, 56, 0xFFFF);
  display.drawLine(1, 56, 6, 56, 0xFFFF);
  display.setCursor(8, 52);
  display.print("Menu");

  needsRedraw = false;
}

void handleStandbyButtons(unsigned int b) {
  if (b & TSButtonLowerLeft) {  // BL -> Home
    currentPage = HOME;
    needsRedraw = true;
    delay(200);
  }
}

// ==============================
// Main Loop
// ==============================
void loop()
{
  unsigned int b = display.getButtons();

  switch (currentPage)
  {
    case STANDBY:
      if (needsRedraw) showStandby();
      handleStandbyButtons(b);
      break;

    case HOME:
      if (needsRedraw)
        showHome();
      handleHomeButtons(b);
      break;

    case ATTENDANCE:
      // Run attendance flow once, then return to HOME
      runAttendance();
      currentPage = HOME;
      needsRedraw = true;
      break;

    case VACANCY:
      if (needsRedraw)
        showVacancy();
      handleBackButton(b);
      break;

    case KAHOOT_DIGIT:
      if (needsRedraw)
        showDigitEntry();
      handleDigitEntryButtons(b);
      break;

    case KAHOOT_CONFIRM:
      if (needsRedraw)
        showConfirmation();
      handleConfirmationButtons();
      break;

    case KAHOOT_QUESTION:
      if (needsRedraw)
        showQuestionScreen();
      handleQuestionButtons(b);
      break;

    case KAHOOT_WAIT:
      if (needsRedraw)
        showWaitScreen();
      handleWaitScreen(b);
      break;

    case KAHOOT_END:
      if (needsRedraw)
        showGameEnd();
      handleGameEndButtons(b);
      break;
  }

  display.flush();
}

// ==============================
// ORIGINAL GOLDEN HOME SCREEN
// (button mapping unchanged)
// ==============================
void showHome()
{
    display.clearScreen();
    display.fontColor(TS_16b_White, NULL);
    downArrow(90, 10+2);
    rightArrow(90,45+2);
    leftArrow(0, 45+2);
    display.setCursor(8, 52);
    //display.print("Back");

    display.fontColor(TS_16b_White, TS_16b_Black);

    int w = display.getPrintWidth(PROJECT_DISPLAY_NAME);
    display.setCursor((SCREEN_WIDTH - w) / 2, 5);
    display.print(PROJECT_DISPLAY_NAME);

    for (int i = 0; i < numMenuItems; i++)
    {
        w = display.getPrintWidth((char *)menuItems[i]);
        int y = 20 + i * 15;
        if (i == menuIndex)
        {
            w += display.getPrintWidth(">  <");
            display.setCursor((SCREEN_WIDTH-w)/2, y);
            display.fontColor(TS_16b_White, TS_16b_Black);
            display.print("> ");
        } else {
            display.fontColor(TS_8b_Gray, NULL);
            display.setCursor((SCREEN_WIDTH-w)/2, y);
        }
        display.print(menuItems[i]);

        if (i == menuIndex)
            display.print(" <");
    }

    needsRedraw = false;
}

// ==============================
// ATTENDANCE & VACANCY PAGES (legacy)
// ==============================
void showAttendance()
{
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  char text[] = "ATTENDANCE PAGE";
  int w = display.getPrintWidth(text);
  display.setCursor((100 - w) / 2, 20);
  display.print(text);
  display.setCursor(2, 55);
  display.print("<-");
  needsRedraw = false;
}

void showVacancy()
{
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  char text[] = "ROOM VACANCY";
  int w = display.getPrintWidth(text);
  display.setCursor((100 - w) / 2, 20);
  display.print(text);
  display.setCursor(2, 55);
  display.print("<-");
  needsRedraw = false;
}

// ==============================
// KAHOOT DIGIT ENTRY (golden)
// ==============================
void showDigitEntry()
{
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(17, 20);
  display.print("Enter code:");
  display.setCursor(23, 40);
  for (int i = 0; i < 3; i++)
  {
    if (i == currentDigit)
      display.print("[" + String(digits[i]) + "]");
    else
      display.print(" " + String(digits[i]) + " ");
  }

  // Buttons Hints
  display.setCursor(2, 3);
  display.print(">>");

  display.drawLine(84, 7, 82, 5, TS_16b_White);
  display.drawLine(84, 7, 86, 5, TS_16b_White);

  display.drawLine(84, 10, 82, 8, TS_16b_White);
  display.drawLine(84, 10, 86, 8, TS_16b_White);

  display.setCursor(80, 55);
  display.print("->");

  needsRedraw = false;
}

// ==============================
// KAHOOT CONFIRM (golden)
// ==============================
void showConfirmation()
{
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(5, 10);
  display.print("You entered:");
  display.setCursor(70, 10);
  for (int i = 0; i < 3; i++)
    display.print(digits[i]);

  // Simulate join response
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, joinResponse);
  const char *msg = doc["message"];
  const char *status = doc["gameRoom"]["status"];

  if (error)
  {
    display.setCursor(10, 30);
    display.print("Error parsing JSON");
  }
  else
  {
    const char *message = doc["message"];

    if (strcmp(message, "Game room not found") == 0)
    {
      display.setCursor(10, 30);
      display.print("Room not found");
      display.setCursor(2, 55);
      display.print("<-"); // back button
    }
    else if (strcmp(message, "Student joined the game room") == 0)
    {
      const char *status = doc["gameRoom"]["status"];
      display.setCursor(20, 30);
      display.print(status);
      display.setCursor(2, 55);
      display.print("<-"); // back button
      if (!strcmp(status, "ENDED") == 0)
      {
        display.setCursor(60, 55);
        display.print("Start->"); // start game button
      }
    }
  }

  needsRedraw = false;
}

// ==============================
// KAHOOT QUESTION SCREEN (golden)
// ==============================
void showQuestionScreen()
{
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);

  // Question header
  display.setCursor(2, 2);
  display.print("Question " + String(questionIndex + 1));

  // Option positions (y-coordinates)
  int ys[4] = {2, 16, 30, 44};

  // Draw all 4 options (1,2,3,4)
  for (int i = 0; i < 4; i++)
  {
    int y = ys[i];

    if (answer == i + 1)
    {
      // HIGHLIGHTED OPTION
      display.drawRect(60, y - 1, 18, 12, 1, TS_16b_White);
      display.fontColor(TS_16b_Black, TS_16b_White); // inverted
    }
    else
    {
      // Normal
      display.fontColor(TS_16b_White, TS_16b_Black);
    }

    display.setCursor(72, y);
    display.print(String(i + 1));
  }

  // Submit button
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(55, 55);
  display.print("Submit->");

  needsRedraw = false;
}

// ==============================
// KAHOOT SUBMIT ANSWER (golden)
// ==============================
void submitAnswer()
{
  WiFiClient client;

  Serial.print("Connecting to ");
  Serial.print(server);
  Serial.print(":");
  Serial.println(serverPort);

  if (!client.connect(server, serverPort))
  {
    Serial.println("Connection failed");
    display.clearScreen();
    display.setCursor(5, 28);
    display.print("NET ERROR");
    delay(1000);
    return;
  }

  String payload =
      String("{\"studentId\":" + String(studentID) + ",") +
      "\"gameRoomCode\":\"" + String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"," +
      "\"questionIndex\":" + String(questionIndex) + "," +
      "\"optionSelected\":" + String(answer) + "," +
      "\"timeTaken\":5}";

  // --------- HTTP HEADER -----------
  client.print(String("POST ") + checkApiPath + " HTTP/1.1\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print(String("Content-Length: ") + payload.length() + "\r\n");
  client.print("Connection: close\r\n\r\n");
  client.print(payload);

  // --------- Show feedback ----------
  display.clearScreen();
  display.setCursor(20, 28);
  display.print("Submitted!");
  display.flush();

  // --------- Wait for response -------
  unsigned long timeout = millis();
  while (client.available() == 0)
  {
    if (millis() - timeout > 5000)
    {
      SerialMonitorInterface.println(">>> Client Timeout");
      client.stop();
      return;
    }
  }

  // --------- Read response ----------
  String response = "";
  while (client.available())
  {
    response += (char)client.read();
  }

  // --------- Extract score ----------
  StaticJsonDocument<256> doc;
  int jsonStart = response.indexOf("{");
  if (jsonStart >= 0)
  {
    String json = response.substring(jsonStart);

    DeserializationError err = deserializeJson(doc, json);
    if (!err)
    {
      totalScore = doc["score"] | 0;
    }
    else
    {
      SerialMonitorInterface.println("JSON parse failed!");
    }
  }
}

// ==============================
// KAHOOT QUESTION WAIT SCREEN (golden)
// ==============================
void showWaitScreen()
{
  display.clearScreen();
  display.setCursor(10, 28);
  display.print("Waiting...");
  display.setCursor(60, 55);
  display.print("Next->"); // start game button
  needsRedraw = false;
}

// ==============================
// KAHOOT GAME END SCREEN (golden)
// ==============================
void showGameEnd()
{
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(10, 20);
  display.print("Game Ended");
  display.setCursor(10, 35);
  display.print("Score: ");
  display.print(totalScore);
  display.setCursor(55, 52);
  display.print("Back->");

  needsRedraw = false;
}

// ==============================
// HOME BUTTONS (golden mapping)
// ==============================
void handleHomeButtons(unsigned int b)
{
  if (b & TSButtonUpperRight)
  {
    menuIndex++;
    if (menuIndex > 2)
      menuIndex = 0;
    needsRedraw = true;
    delay(200);
  }
  if (b & TSButtonLowerRight)
  {
    switch (menuIndex)
    {
      case 0:
        currentPage = ATTENDANCE;
        break;
      case 1:
        currentPage = VACANCY;
        break;
      case 2:
        currentPage = KAHOOT_DIGIT;
        break;
    }
    needsRedraw = true;
    delay(200);
  }
}

// ==============================
// BACK BUTTON (golden mapping)
// ==============================
void handleBackButton(unsigned int b)
{
  if (b & TSButtonLowerLeft)
  {
    currentPage = HOME;
    needsRedraw = true;
    delay(200);
  }
}

// ==============================
// KAHOOT ENTER ROOM BUTTONS (golden mapping)
// ==============================
void handleDigitEntryButtons(unsigned int b)
{
  if (b & TSButtonUpperRight)
  {
    digits[currentDigit]++;
    if (digits[currentDigit] > 9)
      digits[currentDigit] = 0;
    needsRedraw = true;
    delay(200);
  }
  if (b & TSButtonUpperLeft)
  {
    currentDigit++;
    if (currentDigit > 2)
      currentDigit = 0;
    needsRedraw = true;
    delay(200);
  }
  if (b & TSButtonLowerRight)
  {
    WiFiClient client;

    if (!client.connect(server, serverPort))
    {
      SerialMonitorInterface.println("Connection failed");
      display.clearScreen();
      display.setCursor(5, 28);
      display.print("NET ERROR");
      delay(1000);
      return;
    }

    String payload =
        String("{\"studentId\":" + String(studentID) + ",") +
        "\"studentName\":\"" + studentName + "\"," +
        "\"gameRoomCode\":\"" + String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"}";

    // --------- HTTP HEADER -----------
    client.print(String("POST ") + apiPath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);

    // --------- Show feedback ----------
    display.clearScreen();
    display.setCursor(20, 28);
    currentPage = KAHOOT_CONFIRM;
    needsRedraw = true;
    delay(200);
    display.flush();

    // --------- Wait for response -------
    unsigned long timeout = millis();
    while (client.available() == 0)
    {
      if (millis() - timeout > 5000)
      {
        SerialMonitorInterface.println(">>> Client Timeout");
        client.stop();
        return;
      }
    }

    // --------- Read response ----------
    joinResponse = ""; // clear old response
    bool jsonStartFound = false;

    while (client.available())
    {
      char c = client.read();

      if (!jsonStartFound)
      {
        if (c == '{')
        {
          jsonStartFound = true;
          joinResponse = "{";
        }
      }
      else
      {
        joinResponse += c;
      }
    }
  }
  if (b & TSButtonLowerLeft)
  {
    currentPage = HOME;
    needsRedraw = true;
    delay(200);
  }
}

// ==============================
// KAHOOT ENTER ROOM CONFIRMATION BUTTON (golden mapping)
// ==============================
void handleConfirmationButtons()
{
  unsigned int b = display.getButtons();

  // Parse join response
  StaticJsonDocument<512> joinDoc;
  DeserializationError joinErr = deserializeJson(joinDoc, joinResponse);

  const char *message = nullptr;
  if (!joinErr)
    message = joinDoc["message"];

  // Back button
  if (b & TSButtonLowerLeft)
  {
    // Allow back if room not found OR always back
    currentPage = KAHOOT_DIGIT;
    currentDigit = 0;
    needsRedraw = true;
    delay(200);
    return;
  }

  // Start button (only if student joined)
  if (message && strcmp(message, "Student joined the game room") == 0)
  {
    if (b & TSButtonLowerRight)
    {

      WiFiClient client;

      if (!client.connect(server, serverPort))
      {
        SerialMonitorInterface.println("Connection failed");
        display.clearScreen();
        display.setCursor(5, 28);
        display.print("NET ERROR");
        delay(1000);
        return;
      }

      String payload = "{\"gameRoomCode\":\"" +
                       String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"}";

      // --------- HTTP HEADER -----------
      client.print(String("POST ") + startApiPath + " HTTP/1.1\r\n");
      client.print(String("Host: ") + server + "\r\n");
      client.print("Content-Type: application/json\r\n");
      client.print(String("Content-Length: ") + payload.length() + "\r\n");
      client.print("Connection: close\r\n\r\n");
      client.print(payload);

      // --------- Show feedback ----------
      display.clearScreen();
      display.setCursor(20, 28);
      display.flush();

      // --------- Wait for response -------
      unsigned long timeout = millis();
      while (client.available() == 0)
      {
        if (millis() - timeout > 5000)
        {
          SerialMonitorInterface.println(">>> Client Timeout");
          client.stop();
          return;
        }
      }

      // --------- Read response ----------
      startResponse = ""; // clear old response
      bool jsonStartFound = false;

      while (client.available())
      {
        char c = client.read();

        if (!jsonStartFound)
        {
          if (c == '{')
          {
            jsonStartFound = true;
            startResponse = "{";
          }
        }
        else
        {
          startResponse += c;
        }
      }

      // Parse start response
      StaticJsonDocument<512> startDoc;
      DeserializationError startErr = deserializeJson(startDoc, startResponse);

      if (startErr)
      {
        currentPage = KAHOOT_CONFIRM;
        needsRedraw = true;
        return;
      }

      const char *status = startDoc["status"];
      int totalQuestions = startDoc["totalQuestions"];
      int currQuestion   = startDoc["questionIndex"];
      questionCount      = totalQuestions;

      if (strcmp(status, "WAITING") == 0)
      {
        currentPage = KAHOOT_CONFIRM;
      }
      else if (strcmp(status, "ONGOING") == 0)
      {
        currentPage = KAHOOT_QUESTION;
        questionIndex = currQuestion;
        answer = 1;
        lastQuestionIndex = -1;
      }

      needsRedraw = true;
      delay(200);
    }
  }
}

// ==============================
// KAHOOT WAIT QUESTION BUTTONS (golden mapping)
// ==============================
void handleWaitScreen(unsigned int b)
{
  if (b & TSButtonLowerRight)
  {
    WiFiClient client;
    if (!client.connect(server, serverPort))
      return;

    String payload = "{\"gameRoomCode\":\"" +
                     String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"}";

    client.print(String("POST ") + startApiPath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);

    unsigned long timeout = millis();
    while (client.available() == 0)
    {
      if (millis() - timeout > 3000)
      { // 3s timeout
        client.stop();
        return;
      }
    }

    startResponse = ""; // clear old response
    bool jsonStartFound = false;

    while (client.available())
    {
      char c = client.read();

      if (!jsonStartFound)
      {
        if (c == '{')
        {
          jsonStartFound = true;
          startResponse = "{";
        }
      }
      else
      {
        startResponse += c;
      }
    }

    // Parse start response
    StaticJsonDocument<512> startDoc;
    DeserializationError startErr = deserializeJson(startDoc, startResponse);

    if (startErr)
    {
      currentPage = KAHOOT_CONFIRM;
      needsRedraw = true;
      return;
    }

    const char *status    = startDoc["status"];
    int currentQuestion   = startDoc["questionIndex"];

    if (strcmp(status, "ONGOING") == 0 && questionIndex < currentQuestion)
    {
      // Teacher moved to next question
      currentPage = KAHOOT_QUESTION;
      questionIndex = currentQuestion;
      answer = 1;
      needsRedraw = true;
    }
    else if (strcmp(status, "ENDED") == 0)
    {
      currentPage = KAHOOT_END;
      needsRedraw = true;
      answer = 1;
    }
  }
}

// ==============================
// KAHOOT ANSWER QUESTION BUTTONS (golden mapping)
// ==============================
void handleQuestionButtons(unsigned int b)
{
  // Navigate answer
  if (b & TSButtonUpperRight)
  {
    answer++;
    if (answer > 4)
      answer = 1;
    needsRedraw = true;
    delay(200);
  }
  if (b & TSButtonUpperLeft)
  {
    answer--;
    if (answer < 1)
      answer = 4;
    needsRedraw = true;
    delay(200);
  }

  // Submit
  if (b & TSButtonLowerRight)
  {
    submitAnswer();
    currentPage = KAHOOT_WAIT; // new page
    needsRedraw = true;
    delay(200);
  }
}

// ==============================
// KAHOOT GAME END BUTTON (golden mapping)
// ==============================
void handleGameEndButtons(unsigned int b)
{
  if (b & TSButtonLowerRight)
  { // Back button
    currentPage = HOME;
    totalScore = 0; // optional: reset score
    needsRedraw = true;
    delay(200);
  }
}
