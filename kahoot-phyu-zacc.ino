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
// Display
// ==============================
TinyScreen display = TinyScreen(TinyScreenPlus);

// ==============================
// Screen Drawing Helpers
// ==============================
uint8_t menuTextY[8] = {1 * 12 - 1, 2 * 12 - 1, 3 * 12 - 1, 4 * 12 - 1, 5 * 12 - 1, 6 * 12 - 1, 7 * 12 - 3, 8 * 12 - 3};

char *student_name = "Aungsh";
char *student_id = "2501909";

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
// Fake API responses
// ==============================
const char* apiPath = "/api/classhoot/check";
const String FAKE_JOIN_RESPONSE = R"rawliteral(
{
    "message": "Student joined the game room",
    "gameRoom": {
        "id": 9,
        "code": "612",
        "title": "maths Test",
        "status": "WAITING",
        "currentIndex": 0,
        "totalQuestions": 1,
        "createdAt": "2025-11-17T07:30:44.048Z",
        "updatedAt": "2025-11-17T07:30:44.048Z"
    },
    "player": {
        "id": 2501235,
        "name": "Ei",
        "createdAt": "2025-11-17T07:37:20.176Z"
    },
    "playerRoom": {
        "id": 22,
        "playerId": 2501235,
        "roomId": 9,
        "score": 0,
        "joinedAt": "2025-11-17T07:37:20.466Z"
    }
}
)rawliteral";

const String FAKE_START_RESPONSE = R"rawliteral(
{
    "status": "ONGOING",
    "questionIndex": 1,
    "totalQuestions": 5,
    "question": "Is foogle clean",
    "option1": "Not clean",
    "option2": "clean",
    "option3": "Rat",
    "option4": "Caterpillar",
    "correctOption": 3,
    "timeLimit": 10
}
)rawliteral";

// ==============================
// Pages
// ==============================
enum Page { STANDBY, HOME, ATTENDANCE, POMODORO, KAHOOT_DIGIT, KAHOOT_CONFIRM, KAHOOT_QUESTION, KAHOOT_END};
Page currentPage = STANDBY;
bool needsRedraw = true;

// ==============================
// Home Menu
// ==============================
int menuIndex = 0;
const char* menuItems[] = {
  "Attendance",
  "Kahoot",
  "Pomodoro",
};

// ==============================
// Kahoot variables
// ==============================
int digits[3] = {0,0,0};
int currentDigit = 0;

int answer = 1;
int questionIndex = 0;
int questionCount = 0;
int lastQuestionIndex = -1;
int totalScore = 0;

// ==============================
// Setup
// ==============================
void setup() {
  Wire.begin();
  display.begin();
  display.setBrightness(15);
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
  Serial.begin(9600);
}

// ==============================
// Main Loop
// ==============================
void loop() {
  unsigned int b = display.getButtons();

  switch(currentPage) {
    case STANDBY:
      if (needsRedraw) showStandby();
      handleStandbyButtons(b);
      break;
    case HOME: 
      if (needsRedraw) showHome();
      handleHomeButtons(b);
      break;
    case ATTENDANCE:
      if (needsRedraw) showAttendance();
      handleBackButton(b);
      break;
    case POMODORO:
      if (needsRedraw) showPomodoro();
      handleBackButton(b);
      break;
    case KAHOOT_DIGIT:
      if (needsRedraw) showDigitEntry();
      handleDigitEntryButtons(b);
      break;
    case KAHOOT_CONFIRM:
      if (needsRedraw) showConfirmation();
      handleConfirmationButtons();
      break;
    case KAHOOT_QUESTION:
      if (needsRedraw) showQuestionScreen();
      handleQuestionButtons(b);
      break;
    case KAHOOT_END:
      if(needsRedraw) showGameEnd();
      handleGameEndButtons(b);
      break;
  }

  display.flush();
}

void handleStandbyButtons(unsigned int b){
  if(b & TSButtonLowerLeft) {
    currentPage = HOME;
    needsRedraw=true;
    delay(200);
  }
}

void showStandby() {
  display.clearScreen();
  display.setCursor(9, menuTextY[6]);
  char intName[20];
  int width = display.getPrintWidth("SIT AIO");
  display.setCursor(96 / 2 - width / 2 - 1, menuTextY[0] - 1);
  display.fontColor(0xFFFF, NULL);
  display.print("SIT AIO");
  display.setCursor(96 / 2 - width / 2 + 1, menuTextY[0] + 1);
  display.print("SIT AIO");
  display.setCursor(70, menuTextY[6]);
  width = display.getPrintWidth(student_name);
  display.setCursor(96 / 2 - width / 2, menuTextY[1]+1);
  display.print(student_name);
  width = display.getPrintWidth(student_id);
  display.setCursor(96 / 2 - width / 2, menuTextY[2]+1);
  display.print(student_id);
  
  leftArrow(0, 57);
  display.setCursor(8, 52);
  display.print("Menu");
  needsRedraw = false;
}

// ==============================
// HOME SCREEN
// ==============================
void showHome() {
  display.clearScreen();
  display.fontColor(TS_16b_White, NULL);
  downArrow(90, 10+2);
  rightArrow(90,45+2);
  leftArrow(0, 57);
  display.setCursor(8, 52);
  display.print("Back");
  for(int i = 0; i<3; i++){
    int y = 10 + i*12;

    if(i == menuIndex) {
      display.drawRect(10, y-2, 76, 12, 1, TS_16b_White);
      display.fontColor(TS_16b_Black, TS_16b_White);
    } else {
      display.fontColor(TS_16b_White, TS_16b_Black);
    }

    // Convert const char* to mutable char[]
    char buffer[20];
    strncpy(buffer, menuItems[i], sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';

    int w = display.getPrintWidth(buffer);
    display.setCursor((96 - w)/2, y);
    display.print(buffer);
  }
  needsRedraw = false;
}

void handleHomeButtons(unsigned int b){
  if(b &TSButtonLowerLeft) {
    currentPage = STANDBY;
    needsRedraw = true;
    delay(200);
  }
  if(b & TSButtonUpperRight){ menuIndex++; if(menuIndex>2) menuIndex=0; needsRedraw=true; delay(200);}
  if(b & TSButtonLowerRight){ 
    switch(menuIndex){
      case 0: currentPage=ATTENDANCE; break;
      case 1: currentPage=KAHOOT_DIGIT; break;
      case 2: currentPage=POMODORO; break;
    }
    needsRedraw=true; delay(200);
  }
}

// ==============================
// BACK BUTTON
// ==============================
void handleBackButton(unsigned int b){
  if(b & TSButtonLowerLeft){ currentPage=HOME; needsRedraw=true; delay(200); }
}

// ==============================
// ATTENDANCE & VACANCY
// ==============================
void showAttendance(){
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  char text[]="ATTENDANCE PAGE";
  int w=display.getPrintWidth(text);
  display.setCursor((100-w)/2,20); display.print(text);
  display.setCursor(2,55); display.print("<-");
  needsRedraw=false;
}

void showPomodoro(){
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  char text[]="POMODORO";
  int w=display.getPrintWidth(text);
  display.setCursor((100-w)/2,20); display.print(text);
  display.setCursor(2,55); display.print("<-");
  needsRedraw=false;
}

// ==============================
// KAHOOT DIGIT ENTRY
// ==============================
void showDigitEntry(){
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(17,20); display.print("Enter code:");
  display.setCursor(23,40);
  for(int i=0;i<3;i++){
    if(i==currentDigit) display.print("[" + String(digits[i]) + "]");
    else display.print(" " + String(digits[i]) + " ");
  }
  needsRedraw=false;
}

void handleDigitEntryButtons(unsigned int b){
  if(b & TSButtonUpperRight){ digits[currentDigit]++; if(digits[currentDigit]>9) digits[currentDigit]=0; needsRedraw=true; delay(200);}
  if(b & TSButtonUpperLeft){ currentDigit++; if(currentDigit>2) currentDigit=0; needsRedraw=true; delay(200);}
  if(b & TSButtonLowerRight){ currentPage=KAHOOT_CONFIRM; needsRedraw=true; delay(200);}
  if(b & TSButtonLowerLeft){ currentPage=HOME; needsRedraw=true; delay(200);}
}

// ==============================
// KAHOOT CONFIRM
// ==============================
void showConfirmation(){
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(5,10); display.print("You entered:");
  display.setCursor(70,10);
  for(int i=0;i<3;i++) display.print(digits[i]);
  
  // Simulate join response
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, FAKE_JOIN_RESPONSE);
  const char* msg = doc["message"];
  const char* status = doc["gameRoom"]["status"];
  
  if (error) {
    display.setCursor(10, 30);
    display.print("Error parsing JSON");
  } else {
    const char* message = doc["message"];

    if (strcmp(message, "Game room not found") == 0) {
      display.setCursor(10, 30);
      display.print("Room not found");
      display.setCursor(2, 55);
      display.print("<-"); // back button
    } else if (strcmp(message, "Student joined the game room") == 0) {
      const char* status = doc["gameRoom"]["status"];
      display.setCursor(20, 30);
      display.print(status); 
      display.setCursor(60, 55);
      display.print("Start->"); // start game button
    }
  }
  
  needsRedraw=false;
}

void handleConfirmationButtons() {
    unsigned int b = display.getButtons();

    // Parse fake join response
    StaticJsonDocument<512> joinDoc;
    DeserializationError joinErr = deserializeJson(joinDoc, FAKE_JOIN_RESPONSE);

    const char* message = nullptr;
    if (!joinErr) message = joinDoc["message"];

    // Back button
    if (b & TSButtonLowerLeft) {
        // Allow back if room not found OR always back
        currentPage = KAHOOT_DIGIT;
        currentDigit = 0;
        needsRedraw = true;
        delay(200);
        return;
    }

    // Start button (only if student joined)
    if (message && strcmp(message, "Student joined the game room") == 0) {
        if (b & TSButtonLowerRight) {

            // Parse start response
            StaticJsonDocument<512> startDoc;
            DeserializationError startErr = deserializeJson(startDoc, FAKE_START_RESPONSE);

            if (startErr) {
                currentPage = KAHOOT_CONFIRM;
                needsRedraw = true;
                return;
            }

            const char* status = startDoc["status"];
            int totalQuestions = startDoc["totalQuestions"];
            questionCount = totalQuestions;

            if (strcmp(status, "WAITING") == 0) {
                currentPage = KAHOOT_CONFIRM;
            } else if (strcmp(status, "ONGOING") == 0) {
                currentPage = KAHOOT_QUESTION;
                questionIndex = 0;
                answer = 1;
                lastQuestionIndex = -1;
            }

            needsRedraw = true;
            delay(200);
        }
    }
}

// ==============================
// KAHOOT QUESTION SCREEN
// ==============================
void showQuestionScreen() {
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);

  // Question header
  display.setCursor(2, 2);
  display.print("Question " + String(questionIndex + 1));

  // Option positions (y-coordinates)
  int ys[4] = { 2, 16, 30, 44 };

  // Draw all 4 options (1,2,3,4)
  for (int i = 0; i < 4; i++) {
    int y = ys[i];

    if (answer == i + 1) {
      // HIGHLIGHTED OPTION
      display.drawRect(60, y - 1, 18, 12, 1, TS_16b_White);
      display.fontColor(TS_16b_Black, TS_16b_White);  // inverted
    } else {
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
// KAHOOT SUBMIT ANSWER
// ==============================
void submitAnswer() {
    const char* server = "134.185.93.17";
    int serverPort = 8080;

    WiFiClient client;

    Serial.print("Connecting to ");
    Serial.print(server);
    Serial.print(":");
    Serial.println(serverPort);

    if (!client.connect(server, serverPort)) {
        Serial.println("Connection failed");
        display.clearScreen();
        display.setCursor(5,28);
        display.print("NET ERROR");
        delay(1000);
        return;
    }

    String payload =
        String("{\"studentId\":2501909,") +
        "\"gameRoomCode\":\"" + String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"," +
        "\"questionIndex\":" + String(questionIndex) + "," +
        "\"optionSelected\":" + String(answer) + "," +
        "\"timeTaken\":5}";

    // --------- HTTP HEADER -----------
    client.print(String("POST ") + apiPath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(payload);

    // --------- Show feedback ----------
    display.clearScreen();
    display.setCursor(20,28);
    display.print("Submitted!");
    display.flush();

    // --------- Wait for response -------
    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
            Serial.println(">>> Client Timeout");
            client.stop();
            return;
        }
    }

    // --------- Read response ----------
    String response = "";
    while (client.available()) {
        response += (char)client.read();
    }

    Serial.println("Response:");
    Serial.println(response);

    // --------- Extract score ----------
    int sStart = response.indexOf("\"score\":");
    if (sStart != -1) {
        sStart += 8;
        int sEnd = response.indexOf(",", sStart);
        if (sEnd == -1) sEnd = response.indexOf("}", sStart);

        int gained = response.substring(sStart, sEnd).toInt();
        totalScore += gained;

        Serial.print("Score gained: ");
        Serial.println(gained);
        Serial.print("Total score: ");
        Serial.println(totalScore);
    }
}

void handleQuestionButtons(unsigned int b){
  // Navigate answer
  if(b & TSButtonUpperRight){ answer++; if(answer>4) answer=1; needsRedraw=true; delay(200); SerialMonitorInterface.println(answer);} 
  if(b & TSButtonUpperLeft){ answer--; if(answer<1) answer=4; needsRedraw=true; delay(200); SerialMonitorInterface.println(answer);}
  
  // Submit
  if(b & TSButtonLowerRight){
    submitAnswer();
    questionIndex++;
    if(questionIndex>=questionCount){
      currentPage = KAHOOT_END;
      needsRedraw = true;
    } else {
      answer = 1;
      needsRedraw = true;
    }
    delay(200);
  }
}

void showGameEnd() {
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);
  display.setCursor(10, 20); display.print("Game Ended");
  display.setCursor(10, 35); display.print("Score: "); display.print(totalScore);
  display.setCursor(55, 52); display.print("Back->");

  needsRedraw = false;
}

void handleGameEndButtons(unsigned int b){
  if(b & TSButtonLowerRight){   // Back button
    currentPage = HOME;
    totalScore = 0;            // optional: reset score
    needsRedraw = true;
    delay(200);
  }
}