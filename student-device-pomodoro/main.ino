/*
 * Pomodoro Timer Module for TinyScreen + TinyZero
 * 
 * Button Layout:
 * [TL]  [TR]    TL = Top Left, TR = Top Right
 * [BL]  [BR]    BL = Bottom Left, BR = Bottom Right
 * 
 * Navigation:
 * - Init Screen: BL to continue, any other button quits
 * - Mode Select: TL = 25/5 mode, BL = 50/10 mode, right buttons quit
 * - Timer Running: BR to quit (shows summary)
 * - Flash Alert: Any button to acknowledge and continue to rest/study
 * - Summary: Shows total study time, BR to quit
 */

#include <Wire.h>
#include <TinyScreen.h>
#include <ArduinoJson.h>
#include <WiFi101.h>

#if defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#else
#define SerialMonitorInterface Serial
#endif

JsonDocument api_res_deserialized;

TinyScreen display = TinyScreen(TinyScreenDefault);

// Module states
enum PomodoroState {
  POMO_INIT,
  POMO_MODE_SELECT,
  POMO_STUDY,
  POMO_BREAK,
  POMO_FLASHING,
  POMO_SUMMARY
};

// Timer configuration (using seconds for testing, change to minutes for production)
#define USE_SECONDS_FOR_TESTING true

struct TimerMode {
  int studyTime;
  int breakTime;
};

// Wifi
const char* ssid = "Stephen Hawking's Penis";
const char* pass = "420not369";

const int studentID = 67 ;
const char* studentName = "Tristan";

const char* server = "134.185.93.17"; // your server ip (no "http://")
const int serverPort = 8080;
const char* startApiPath = "/api/strady/start";
const char* endApiPath = "/api/strady/end";

String stradyResponse = "";

// Module variables
PomodoroState currentState = POMO_INIT;
PomodoroState stateBeforeFlash = POMO_STUDY;
TimerMode modes[] = {{5, 3}, {10, 5}};  // For testing: 5s/3s and 10s/5s
int selectedMode = 0;
unsigned long timerStartMillis = 0;
int currentMinutes = 0;

// Study time tracking
unsigned long totalStudyTimeMs = 0;  // Total accumulated study time in milliseconds
unsigned long currentSessionStartMs = 0;  // When current study session started

// Flash notification variables
unsigned long lastFlashMillis = 0;
bool flashState = false;

// Button state tracking (for proper debouncing)
bool lastButtonState = false;
unsigned long lastButtonChangeTime = 0;
const unsigned long debounceDelay = 50;

// Flag to signal module should exit
bool shouldExit = false;

// Exported study time (in seconds) - can be read by main program
unsigned long exportedStudyTimeSeconds = 0;

void connectToWifi(char *api_path, char *study_session_id) {
  WiFiClient client;
  SerialMonitorInterface.print("Connecting to ");
  SerialMonitorInterface.print(server);
  SerialMonitorInterface.print(":");
  SerialMonitorInterface.print(serverPort);
  SerialMonitorInterface.println(api_path);

  if (!client.connect(server, serverPort)) {
        SerialMonitorInterface.println("Connection failed");
        display.clearScreen();
        display.setCursor(5,28);
        display.print("NET ERROR");
        delay(1000);
        return;
    }

    
    String payload;
    if (study_session_id == NULL) {
      payload = 
      String("{\"studentId\":") + studentID + 
      ",\"studentName\":\"" + studentName + "\"}";
    } else {
      payload = 
      String("{\"studySessionId\":") + study_session_id  + "}";
    }

    SerialMonitorInterface.println(payload);

    // --------- HTTP HEADER -----------
    client.print(String("POST ") + api_path + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);

    // --------- Show feedback ----------
    display.clearScreen();
    display.setCursor(20,28);
    display.flush();

    // --------- Wait for response -------
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            SerialMonitorInterface.println(">>> Client Timeout");
            client.stop();
            return;
        }
    }

    // --------- Read response ----------
    stradyResponse = String("");          // clear old response
    bool jsonStartFound = false;
    int json_start = false;
    while (client.available()) {
      String line = client.readStringUntil('\n');
      SerialMonitorInterface.println(line);
      if (json_start) {
        stradyResponse += line;
      }
      if (line == "\r\n") {
        json_start = true;
      }
    }
    SerialMonitorInterface.println(stradyResponse);
    deserializeJson(api_res_deserialized, stradyResponse);
/*
    while (client.available()) {
      char c = client.read();

      if (!jsonStartFound) {
          if (c == '{') {
              jsonStartFound = true;
              stradyResponse = "{";
          }
      } else {
          stradyResponse += c;
      }
      SerialMonitorInterface.print(stradyResponse);
    }*/
}

void setupPomodoro() {
  connectToWifi((char *)startApiPath, NULL);
  
  Wire.begin();
  display.begin();
  display.setBrightness(10);
  display.setFlip(true);
  
  currentState = POMO_INIT;
  shouldExit = false;
  totalStudyTimeMs = 0;
  lastButtonState = false;
  
  // Wait for all buttons to be released before starting
  while (display.getButtons()) {
    delay(10);
  }
  delay(100);  // Extra delay to ensure clean start
  
  drawInitScreen();
}

// Returns true if module should continue, false if it should exit
bool loopPomodoro() {
  if (shouldExit) {
    return false;
  }
  
  handleButtons();
  updateTimer();
  handleFlashNotification();
  
  return !shouldExit;
}

// Returns true if a button was newly pressed (edge detection)
bool getButtonPress(uint8_t buttonMask) {
  bool currentState = display.getButtons(buttonMask);
  return currentState;
}

// Check if any button is pressed and handle debouncing with edge detection
// Returns the button pressed only on the rising edge (new press)
uint8_t getNewButtonPress() {
  uint8_t buttons = display.getButtons();
  bool anyPressed = (buttons != 0);
  
  // Debounce
  if (millis() - lastButtonChangeTime < debounceDelay) {
    return 0;
  }
  
  // Edge detection: only return button on transition from not pressed to pressed
  if (anyPressed && !lastButtonState) {
    lastButtonState = true;
    lastButtonChangeTime = millis();
    return buttons;
  } else if (!anyPressed && lastButtonState) {
    lastButtonState = false;
    lastButtonChangeTime = millis();
  }
  
  return 0;
}

void handleButtons() {
  uint8_t pressed = getNewButtonPress();
  
  if (pressed == 0) {
    return;
  }
  
  bool tlPressed = pressed & TSButtonUpperLeft;
  bool trPressed = pressed & TSButtonUpperRight;
  bool blPressed = pressed & TSButtonLowerLeft;
  bool brPressed = pressed & TSButtonLowerRight;
  
  switch(currentState) {
    case POMO_INIT:
      if (blPressed) {
        currentState = POMO_MODE_SELECT;
        drawModeSelect();
      } else {
        shouldExit = true;
      }
      break;
      
    case POMO_MODE_SELECT:
      if (tlPressed) {
        selectedMode = 0; // 25/5
        startStudySession();
      } else if (blPressed) {
        selectedMode = 1; // 50/10
        startStudySession();
      } else {
        goToSummary();
      }
      break;
      
    case POMO_STUDY:
    case POMO_BREAK:
      if (brPressed) {
        goToSummary();
      }
      break;
      
    case POMO_SUMMARY:
      if (brPressed) {
        shouldExit = true;
      }
      break;
      
    default:
      break;
  }
}

void updateTimer() {
  if (currentState != POMO_STUDY && currentState != POMO_BREAK) {
    return;
  }
  
  unsigned long elapsed = millis() - timerStartMillis;
  unsigned long totalMillis;
  
  #if USE_SECONDS_FOR_TESTING
    totalMillis = (unsigned long)currentMinutes * 1000UL;  // seconds
  #else
    totalMillis = (unsigned long)currentMinutes * 60000UL; // minutes
  #endif
  
  if (elapsed >= totalMillis) {
    // Timer finished
    if (currentState == POMO_STUDY) {
      // Add completed study time
      totalStudyTimeMs += totalMillis;
    }
    stateBeforeFlash = currentState;
    currentState = POMO_FLASHING;
    lastFlashMillis = millis();
    flashState = false;
  } else {
    // Update display every second
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate >= 1000) {
      lastUpdate = millis();
      drawTimerScreen(totalMillis - elapsed);
    }
  }
}

void startStudySession() {
  currentState = POMO_STUDY;
  currentMinutes = modes[selectedMode].studyTime;
  timerStartMillis = millis();
  currentSessionStartMs = millis();
  
  #if USE_SECONDS_FOR_TESTING
    drawTimerScreen((unsigned long)currentMinutes * 1000UL);
  #else
    drawTimerScreen((unsigned long)currentMinutes * 60000UL);
  #endif
}

void startBreakSession() {
  currentState = POMO_BREAK;
  currentMinutes = modes[selectedMode].breakTime;
  timerStartMillis = millis();
  
  #if USE_SECONDS_FOR_TESTING
    drawTimerScreen((unsigned long)currentMinutes * 1000UL);
  #else
    drawTimerScreen((unsigned long)currentMinutes * 60000UL);
  #endif
}

void goToSummary() {
  // If quitting during study, add partial study time
  if (currentState == POMO_STUDY) {
    unsigned long partialTime = millis() - currentSessionStartMs;
    totalStudyTimeMs += partialTime;
  }
  
  // Calculate exported time in seconds
  exportedStudyTimeSeconds = totalStudyTimeMs / 1000;
  
  currentState = POMO_SUMMARY;
  drawSummaryScreen();
}

void handleFlashNotification() {
  if (currentState != POMO_FLASHING) return;
  
  // Check for any button press to acknowledge
  uint8_t pressed = getNewButtonPress();
  if (pressed != 0) {
    display.setBrightness(10);
    if (stateBeforeFlash == POMO_STUDY) {
      startBreakSession();
    } else {
      startStudySession();
    }
    return;
  }
  
  const unsigned long flashInterval = 400;
  
  if (millis() - lastFlashMillis >= flashInterval) {
    lastFlashMillis = millis();
    flashState = !flashState;
    
    display.clearScreen();
    
    if (flashState) {
      display.drawRect(0, 0, 96, 64, TSRectangleFilled, TS_8b_Red);
    } else {
      display.drawRect(0, 0, 96, 64, TSRectangleFilled, TS_8b_Green);
    }
    
    // Draw message on top
    display.setFont(liberationSansNarrow_12ptFontInfo);
    display.fontColor(TS_8b_White, flashState ? TS_8b_Red : TS_8b_Green);
    
    char msg1[] = "Time's Up!";
    int x = (96 - display.getPrintWidth(msg1)) / 2;
    display.setCursor(x, 15);
    display.print(msg1);
    
    char msg2[] = "Press any key";
    x = (96 - display.getPrintWidth(msg2)) / 2;
    display.setCursor(x, 40);
    display.print(msg2);
  }
}

// Drawing functions
void drawInitScreen() {
  display.clearScreen();
  display.setFont(liberationSansNarrow_16ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);
  
  char text1[] = "Pomodoro";
  int x = (96 - display.getPrintWidth(text1)) / 2;
  display.setCursor(x, 5);
  display.print(text1);
  
  char text2[] = "Timer";
  x = (96 - display.getPrintWidth(text2)) / 2;
  display.setCursor(x, 26);
  display.print(text2);
  
  display.setFont(liberationSansNarrow_10ptFontInfo);
  char text3[] = "<< Press to start";
  x = (96 - display.getPrintWidth(text3)) / 2;
  display.setCursor(x, 50);
  display.print(text3);
}

void drawModeSelect() {
  display.clearScreen();
  display.setFont(liberationSansNarrow_12ptFontInfo);
  display.fontColor(TS_8b_White, TS_8b_Black);
  
  char title[] = "Select Mode:";
  int x = (96 - display.getPrintWidth(title)) / 2;
  display.setCursor(x, 2);
  display.print(title);
  
  display.setFont(liberationSansNarrow_10ptFontInfo);
  
  display.setCursor(2, 22);
  display.print("<< 25min/5min");
  
  display.setCursor(2, 44);
  display.print("<< 50min/10min");
}

void drawTimerScreen(unsigned long remainingMillis) {
  int minutes = remainingMillis / 60000;
  int seconds = (remainingMillis % 60000) / 1000;
  
  #if USE_SECONDS_FOR_TESTING
    // For testing, show total seconds instead
    int totalSecs = remainingMillis / 1000;
    minutes = totalSecs / 60;
    seconds = totalSecs % 60;
  #endif
  
  display.clearScreen();
  display.fontColor(TS_8b_White, TS_8b_Black);
  
  // Draw status text
  display.setFont(liberationSansNarrow_12ptFontInfo);
  int x;
  if (currentState == POMO_STUDY) {
    char modeText[] = "Study!!";
    x = (96 - display.getPrintWidth(modeText)) / 2;
    display.setCursor(x, 5);
    display.print(modeText);
  } else {
    char modeText[] = "Rest...";
    x = (96 - display.getPrintWidth(modeText)) / 2;
    display.setCursor(x, 5);
    display.print(modeText);
  }
  
  // Draw timer
  display.setFont(liberationSansNarrow_16ptFontInfo);
  char timeStr[10];
  sprintf(timeStr, "%02d:%02d", minutes, seconds);
  x = (96 - display.getPrintWidth(timeStr)) / 2;
  display.setCursor(x, 28);
  display.print(timeStr);
  
  // Quit
  display.setFont(liberationSansNarrow_10ptFontInfo);
  display.setCursor(60, 52);
  display.print("Quit >>");
}

void drawSummaryScreen() {
  display.clearScreen();
  display.fontColor(TS_8b_White, TS_8b_Black);
  
  // Title
  display.setFont(liberationSansNarrow_12ptFontInfo);
  char title[] = "Session Over";
  int x = (96 - display.getPrintWidth(title)) / 2;
  display.setCursor(x, 2);
  display.print(title);
  
  // Study time label
  display.setFont(liberationSansNarrow_8ptFontInfo);
  char label[] = "Time spent studying:";
  x = (96 - display.getPrintWidth(label)) / 2;
  display.setCursor(x, 18);
  display.print(label);
  
  // Format time as HH:MM:SS or MM:SS
  unsigned long totalSecs = exportedStudyTimeSeconds;
  int hours = totalSecs / 3600;
  int mins = (totalSecs % 3600) / 60;
  int secs = totalSecs % 60;
  
  display.setFont(liberationSansNarrow_16ptFontInfo);
  char timeStr[12];
  if (hours > 0) {
    sprintf(timeStr, "%d:%02d:%02d", hours, mins, secs);
  } else {
    sprintf(timeStr, "%02d:%02d", mins, secs);
  }
  x = (96 - display.getPrintWidth(timeStr)) / 2;
  display.setCursor(x, 35);
  display.print(timeStr);
  
  // Quit button (bottom right)
  display.setFont(liberationSansNarrow_10ptFontInfo);
  char quit[] = "Quit >>";
  x = 96 - display.getPrintWidth(quit) - 2;
  display.setCursor(x, 52);
  display.print(quit);
}

// Getter for external access to study time
// not needed since strady auto calculates
unsigned long getStudyTimeSeconds() {
  return exportedStudyTimeSeconds;
}

// Main Arduino functions
void setup() {
    Wire.begin();

    display.begin();
    display.setBrightness(15);
    display.clearScreen();
    display.setFont(thinPixel7_10ptFontInfo);

    SerialMonitorInterface.begin(9600);
    delay(500);

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
  setupPomodoro();
}

void loop() {
  if (!loopPomodoro()) {
    const int studySessionId = api_res_deserialized["studySessionId"];
    SerialMonitorInterface.println(studySessionId); // test print
    // Module exited - show exit message then stop
    connectToWifi((char *)endApiPath, (char *)String(studySessionId).c_str());
    display.clearScreen();
    display.setFont(liberationSansNarrow_10ptFontInfo);
    display.fontColor(TS_8b_White, TS_8b_Black);
    char exitMsg[] = "Goodbye!";
    int x = (96 - display.getPrintWidth(exitMsg)) / 2;
    display.setCursor(x, 28);
    display.print(exitMsg);
    
    // Study time is available in exportedStudyTimeSeconds
    // or via getStudyTimeSeconds() for the main program to use
    
    while(1); // Stop here (or return to main menu in your full program)
  }
}