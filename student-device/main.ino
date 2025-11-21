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
// Project metadata
// ==============================
#define PROJECT_DISPLAY_NAME "CLASSHOOT"

// ==============================
// Display
// ==============================
TinyScreen display = TinyScreen(TinyScreenPlus);
#define SCREEN_WIDTH 96

// ==============================
// Screen Drawing Helpers
// ==============================
uint8_t menuTextY[8] = {1 * 12 - 1, 2 * 12 - 1, 3 * 12 - 1, 4 * 12 - 1, 5 * 12 - 1, 6 * 12 - 1, 7 * 12 - 3, 8 * 12 - 3};

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
const char* ssid = "RIZiP17"; // your network SSID (name)
const char* pass = "Tr*toPath369"; // your network password

// ==============================
// Student Credentials
// ==============================
const int studentID = 2501769; // studentID
const char* studentName = "Rifa"; // studentName

// ==============================
// Room Module / Attendance config
// ==============================
const char* ROOM_PREFIX = "Room-";
const char* ROOM_PSK    = "room12345";
const char* ROOM_IP     = "192.168.4.1";

const uint8_t PREFIX_LEN = 3;
const uint8_t PREFIX[PREFIX_LEN] = { 3, 0, 4 };

// ==============================
// API server (Host or IP) and path
// ==============================
const char *server = "134.185.93.17";
const int serverPort = 8080;
const char *apiPath = "/api/classhoot/join";
const char *startApiPath = "/api/classhoot/start";
const char *checkApiPath = "/api/classhoot/check";

// ==============================
// Real API responses
// ==============================
String joinResponse = "";
String startResponse = "";

// ==============================
// Game state
// ==============================
bool needsRedraw = true;

enum Page
{
    STANDBY,
    HOME,
    ATTENDANCE,
    VACANCY,
    KAHOOT_DIGIT,
    KAHOOT_CONFIRM,
    KAHOOT_QUESTION,
    KAHOOT_WAIT,
    KAHOOT_END,
};
Page currentPage = STANDBY;

const int numMenuItems = 3;
const char *menuItems[numMenuItems] = {"Attendance", "Vacancy", "Kahoot"};
int menuIndex = 0;

int digits[3] = {0, 0, 0};
int currentDigit = 0;
int answer = 1;
int questionIndex = 0;
int questionCount = 0;
int lastQuestionIndex = -1;
int totalScore = 0;

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

    SerialMonitorInterface.begin(115200);

    WiFi.setPins(8, 2, A3, -1); // VERY IMPORTANT FOR TINYDUINO
    
    /*
    unsigned long startWait = millis();
    while (!SerialMonitorInterface && millis() - startWait < 2000) {
        ; // wait max 2s
    }
    */

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
// Loop
// ==============================
void loop()
{
    unsigned int b = display.getButtons();
    if (b)
    {
        // Any button press triggers redraw
        needsRedraw = true;
    }

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
        // Run attendance flow (Room Module) then return to HOME
        runAttendance();
        currentPage = ATTENDANCE;
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

    delay(100);
}

// ==============================
// STANDBY PAGE
// ==============================
void showStandby() {
  display.clearScreen();
  display.setCursor(9, menuTextY[6]);
  char intName[20];
  int width = display.getPrintWidth(PROJECT_DISPLAY_NAME);
  display.setCursor(SCREEN_WIDTH / 2 - width / 2 - 1, menuTextY[0] - 1);
  display.fontColor(0xFFFF, NULL);
  display.print(PROJECT_DISPLAY_NAME);
  display.setCursor(SCREEN_WIDTH / 2 - width / 2 + 1, menuTextY[0] + 1);
  display.print(PROJECT_DISPLAY_NAME);
  display.setCursor(SCREEN_WIDTH, menuTextY[6]);
  width = display.getPrintWidth((char *)studentName);
  display.setCursor(SCREEN_WIDTH / 2 - width / 2, menuTextY[1]+1);
  display.print(studentName);
  width = display.getPrintWidth((char *)String(studentID).c_str());
  display.setCursor(SCREEN_WIDTH / 2 - width / 2, menuTextY[2]+1);
  display.print(String(studentID).c_str());
  
  leftArrow(0, 45 + 2);
  display.drawLine(1, 54,    1, 56, 0xFFFF);
  display.drawLine(1, 56,    6, 56, 0xFFFF);
  display.setCursor(8, 52);
  display.print("Menu");
  needsRedraw = false;
}

void handleStandbyButtons(unsigned int b) {
  if(b & TSButtonLowerLeft) {
    currentPage = HOME;
    needsRedraw=true;
    delay(200);
  }
}

// ==============================
// HOME PAGE
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
// Attendance via Room Module (TinyScreen)
// ==============================

// Simple display helpers for attendance screens
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

// Minimal JSON string parsing helpers
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

// PRNG for colour sequence generation
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

// Main attendance flow
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
            // Top row = navigation
            if (btnB(b)) sel = (sel + 1) % roomCount;                      // UR = down
            if (btnA(b)) sel = (sel - 1 + roomCount) % roomCount;          // UL = up
            // Bottom row = actions
            if (btnUp(b))    break;                                        // LR = OK
            if (btnDown(b)) { userCancelled = true; break; }               // LL = Back
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
            if (btnUp(b)) {   // LR = OK/Start
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
// ATTENDANCE & VACANCY PAGES (legacy Attendance page left as-is but not used)
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
    char text[] = "VACANCY PAGE";
    int w = display.getPrintWidth(text);
    display.setCursor((100 - w) / 2, 20);
    display.print(text);
    display.setCursor(2, 55);
    display.print("<-");
    needsRedraw = false;
}

// ==============================
// KAHOOT DIGIT ENTRY
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
    needsRedraw = false;
}

// ==============================
// KAHOOT CONFIRMATION
// ==============================
void showConfirmation()
{
    display.clearScreen();
    display.fontColor(TS_16b_White, TS_16b_Black);
    display.setCursor(5, 20);
    display.print("Code: ");
    display.print(digits[0]);
    display.print(digits[1]);
    display.print(digits[2]);
    display.setCursor(10, 40);
    display.print("Press upper-right");
    display.setCursor(10, 50);
    display.print("to confirm");
    needsRedraw = false;
}

// ==============================
// KAHOOT QUESTION SCREEN
// ==============================

void showQuestionScreen()
{
    display.clearScreen();
    display.fontColor(TS_16b_White, TS_16b_Black);

    if (!startResponse.length())
    {
        display.setCursor(5, 20);
        display.print("No questions.");
        needsRedraw = false;
        return;
    }

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, startResponse);
    if (error)
    {
        display.setCursor(5, 20);
        display.print("JSON error");
        needsRedraw = false;
        return;
    }

    JsonArray questions = doc["questions"].as<JsonArray>();
    questionCount = questions.size();
    if (questionIndex < 0 || questionIndex >= questionCount)
        questionIndex = 0;

    JsonObject qObj = questions[questionIndex];
    const char *questionText = qObj["question"];
    JsonArray options = qObj["options"].as<JsonArray>();

    display.setCursor(0, 5);
    display.print("Q");
    display.print(questionIndex + 1);
    display.print("/");
    display.print(questionCount);

    display.setCursor(0, 15);
    display.print(questionText);

    int y = 30;
    for (int i = 0; i < options.size(); i++)
    {
        display.setCursor(5, y);
        display.print(i + 1);
        display.print(": ");
        display.print(options[i].as<const char *>());
        y += 10;
    }

    display.setCursor(5, 60);
    display.print("Press 1/2/3/4");

    needsRedraw = false;
}

// ==============================
// KAHOOT WAIT SCREEN
// ==============================
void showWaitScreen()
{
    display.clearScreen();
    display.fontColor(TS_16b_White, TS_16b_Black);
    display.setCursor(10, 20);
    display.print("Awaiting next");
    display.setCursor(10, 30);
    display.print("question...");
    needsRedraw = false;
}

// ==============================
// KAHOOT GAME END SCREEN
// ==============================
void showGameEnd()
{
    display.clearScreen();
    display.fontColor(TS_16b_White, TS_16b_Black);
    display.setCursor(10, 20);
    display.print("Game over!");
    display.setCursor(10, 30);
    display.print("Score: ");
    display.print(totalScore);
    needsRedraw = false;
}

// ==============================
// Button Handlers
// ==============================
void handleHomeButtons(unsigned int b)
{
    if (b &TSButtonLowerLeft) {
        currentPage = STANDBY;
        needsRedraw = true;
        delay(200);
    }
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
        currentDigit--;
        if (currentDigit < 0)
            currentDigit = 2;
        needsRedraw = true;
        delay(200);
    }
    if (b & TSButtonLowerRight)
    {
        currentDigit++;
        if (currentDigit > 2)
            currentDigit = 0;
        needsRedraw = true;
        delay(200);
    }
    if (b & TSButtonLowerLeft)
    {
        currentPage = KAHOOT_CONFIRM;
        needsRedraw = true;
        delay(200);
    }
}

// ==============================
void handleConfirmationButtons()
{
    unsigned int b = display.getButtons();
    if (b & TSButtonUpperRight)
    {
        int gameCode = digits[0] * 100 + digits[1] * 10 + digits[2];
        SerialMonitorInterface.print("Sending join request for game code: ");
        SerialMonitorInterface.println(gameCode);

        WiFiClient client;
        if (client.connect(server, serverPort))
        {
            StaticJsonDocument<256> doc;
            doc["studentId"] = studentID;
            doc["studentName"] = studentName;
            doc["gameCode"] = gameCode;

            String requestBody;
            serializeJson(doc, requestBody);

            client.print("POST ");
            client.print(apiPath);
            client.println(" HTTP/1.1");
            client.print("Host: ");
            client.println(server);
            client.println("Content-Type: application/json");
            client.print("Content-Length: ");
            client.println(requestBody.length());
            client.println();
            client.print(requestBody);

            unsigned long start = millis();
            while (!client.available() && millis() - start < 5000)
            {
                delay(100);
            }

            if (client.available())
            {
                String response = client.readString();
                SerialMonitorInterface.println("Response:");
                SerialMonitorInterface.println(response);

                int idx = response.indexOf("\r\n\r\n");
                if (idx >= 0)
                    joinResponse = response.substring(idx + 4);
                else
                    joinResponse = response;

                currentPage = KAHOOT_WAIT;
            }
            else
            {
                SerialMonitorInterface.println("No response from server");
            }

            client.stop();
        }
        else
        {
            SerialMonitorInterface.println("Connection failed");
        }
        needsRedraw = true;
        delay(200);
    }
}

// ==============================
void handleQuestionButtons(unsigned int b)
{
    int selectedAnswer = 0;
    if (b & TSButtonUpperLeft)
        selectedAnswer = 1;
    else if (b & TSButtonUpperRight)
        selectedAnswer = 2;
    else if (b & TSButtonLowerLeft)
        selectedAnswer = 3;
    else if (b & TSButtonLowerRight)
        selectedAnswer = 4;

    if (selectedAnswer != 0)
    {
        answer = selectedAnswer;
        submitAnswer();
        currentPage = KAHOOT_WAIT;
        needsRedraw = true;
        delay(200);
    }
}

// ==============================
void handleWaitScreen(unsigned int b)
{
    if (b & TSButtonUpperRight)
    {
        WiFiClient client;
        if (client.connect(server, serverPort))
        {
            client.print("GET ");
            client.print(checkApiPath);
            client.println(" HTTP/1.1");
            client.print("Host: ");
            client.println(server);
            client.println("Connection: close");
            client.println();

            unsigned long start = millis();
            while (!client.available() && millis() - start < 5000)
            {
                delay(100);
            }

            if (client.available())
            {
                String response = client.readString();
                SerialMonitorInterface.println("Check response:");
                SerialMonitorInterface.println(response);

                int idx = response.indexOf("\r\n\r\n");
                String jsonString;
                if (idx >= 0)
                    jsonString = response.substring(idx + 4);
                else
                    jsonString = response;

                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, jsonString);
                if (!error)
                {
                    if (doc["status"] == "question")
                    {
                        startResponse = jsonString;
                        currentPage = KAHOOT_QUESTION;
                    }
                    else if (doc["status"] == "end")
                    {
                        totalScore = doc["finalScore"];
                        currentPage = KAHOOT_END;
                    }
                }
            }
            client.stop();
        }
        needsRedraw = true;
        delay(200);
    }
}

// ==============================
void handleGameEndButtons(unsigned int b)
{
    if (b & TSButtonLowerLeft)
    {
        currentPage = HOME;
        needsRedraw = true;
        delay(200);
    }
}

// ==============================
// Answer Submission
// ==============================
void submitAnswer()
{
    StaticJsonDocument<256> doc;
    doc["answer"] = answer;

    String requestBody;
    serializeJson(doc, requestBody);

    WiFiClient client;
    if (client.connect(server, serverPort))
    {
        client.print("POST ");
        client.print(startApiPath);
        client.println(" HTTP/1.1");
        client.print("Host: ");
        client.println(server);
        client.println("Content-Type: application/json");
        client.print("Content-Length: ");
        client.println(requestBody.length());
        client.println();
        client.print(requestBody);

        unsigned long start = millis();
        while (!client.available() && millis() - start < 5000)
        {
            delay(100);
        }

        if (client.available())
        {
            String response = client.readString();
            SerialMonitorInterface.println("Submit response:");
            SerialMonitorInterface.println(response);
        }

        client.stop();
    }
}

