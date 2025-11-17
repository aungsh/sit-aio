#include <Wire.h>
#include <TinyScreen.h>
#include <ArduinoJson.h>

TinyScreen display = TinyScreen(TinyScreenPlus);

// ==============================
// FAKE API RESPONSE
// ==============================
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


void showHome();
void handleHomeButtons();
void showAttendance();
void showKahoot();
void showVacancy();
void handleBackButton();

// Globals
enum Page { HOME, ATTENDANCE, VACANCY, KAHOOT,  KAHOOT_DIGIT, KAHOOT_CONFIRM,  KAHOOT_QUESTION};
Page currentPage = HOME;

bool needsRedraw = true;
int menuIndex = 0;
int questionCount = 0;

const char* menuItems[] = {"Attendance", "Room Vacancy", "Kahoot"};

// Kahoot digit entry
int digits[3] = {0, 0, 0};
int currentDigit = 0;

void setup() {
  Wire.begin();
  display.begin();
  display.setBrightness(10);
  display.clearScreen();
  display.setFont(thinPixel7_10ptFontInfo);
}

void loop() {
  switch (currentPage) {
    case HOME:
      if (needsRedraw) showHome();
      handleHomeButtons();
      break;
    case ATTENDANCE:
      if (needsRedraw) showAttendance();
      handleBackButton();
      break;
    case VACANCY:
      if (needsRedraw) showVacancy();
      handleBackButton();
      break;
    case KAHOOT:
      if (needsRedraw) showKahoot();
      handleBackButton();
      break;
    case KAHOOT_DIGIT:
      if (needsRedraw) showDigitEntry();
      handleDigitEntryButtons();
      break;
    case KAHOOT_CONFIRM:
      if (needsRedraw) showConfirmation();
      handleConfirmationButtons();
      break;
    case KAHOOT_QUESTION:
      if (needsRedraw) showKahootQuestion();
      handleConfirmationButtons();
      break;
  }
}

// ========================================================
// HOME SCREEN
// ========================================================
void showHome() {
  display.clearScreen();

  // Menu items
  for (int i = 0; i < 3; i++) {
    int y = 10 + i * 20;

    // Highlight selected item (80% width)
    if (i == menuIndex) {
      display.drawRect(0, y - 2, 78, 12, 1, TS_16b_White);
      display.fontColor(TS_16b_Black, TS_16b_White);
    } else {
      display.fontColor(TS_16b_White, TS_16b_Black);
    }

    // Centered text
    char text[20];
    strcpy(text, menuItems[i]);
    int textWidth = display.getPrintWidth(text);
    display.setCursor((80 - textWidth) / 2, y);
    display.print(text);
  }

  // Button hints
  display.fontColor(TS_16b_White, TS_16b_Black);

  display.drawLine(84, 7, 82, 5, TS_16b_White); 
  display.drawLine(84, 7, 86, 5, TS_16b_White); 

  display.drawLine(84, 10, 82, 8, TS_16b_White); 
  display.drawLine(84, 10, 86, 8, TS_16b_White);

  display.setCursor(80, 55);  
  display.print("->");        

  needsRedraw = false;
}


// ========================================================
// HANDLE HOME BUTTONS
// ========================================================
void handleHomeButtons() {
  unsigned int b = display.getButtons();

  if (b & TSButtonUpperRight) {
    menuIndex++;
    if (menuIndex > 2) menuIndex = 0;
    needsRedraw = true;
    delay(200);
  }

  if (b & TSButtonLowerRight) { 
    switch(menuIndex) {
        case 0: currentPage = ATTENDANCE; break;
        case 1: currentPage = VACANCY; break;
        case 2: currentPage = KAHOOT; break;
    }
    needsRedraw = true;
    delay(200);
  }
}

// ========================================================
// BACK BUTTON
// ========================================================
void handleBackButton() {
  unsigned int b = display.getButtons();
  if (b & TSButtonLowerLeft) { // back to home
    currentPage = HOME;
    needsRedraw = true;
    delay(200);
  }
}

// ========================================================
// ATTENDANCE PAGE
// ========================================================
void showAttendance() {
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


// ========================================================
// ROOM VACANCY PAGE
// ========================================================
void showVacancy() {
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
// KAHOOT REDIRECT
// ==============================
void showKahoot() {
  currentPage = KAHOOT_DIGIT;
  needsRedraw = true;
}


// ==============================
// KAHOOT DIGIT ENTRY
// ==============================
void showDigitEntry() {
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);

  display.setCursor(17, 20);
  display.print("Enter code :");

  display.setCursor(23, 40);

  for (int i = 0; i < 3; i++) {
    if (i == currentDigit) {
      display.print("[" + String(digits[i]) + "]");
    } else {
      display.print(" " + String(digits[i]) + " ");
    }
  }

  // Buttons Hints
  display.setCursor(2,3);
  display.print(">>");

  display.drawLine(84, 7, 82, 5, TS_16b_White); 
  display.drawLine(84, 7, 86, 5, TS_16b_White); 

  display.drawLine(84, 10, 82, 8, TS_16b_White); 
  display.drawLine(84, 10, 86, 8, TS_16b_White);

  display.setCursor(80, 55);  
  display.print("->");


  needsRedraw = false;
}


void handleDigitEntryButtons() {
  unsigned int b = display.getButtons();

  if (b & TSButtonUpperRight) {  // increase digit
    digits[currentDigit]++;
    if (digits[currentDigit] > 9) digits[currentDigit] = 0;
    needsRedraw = true;
    delay(200);
  }

  if (b & TSButtonUpperLeft) {   
    currentDigit++;
    if (currentDigit > 2) currentDigit = 0;
    needsRedraw = true;
    delay(200);
  }

  if (b & TSButtonLowerRight) {  
    currentPage = KAHOOT_CONFIRM;
    needsRedraw = true;
    delay(200);
  }

  if (b & TSButtonLowerLeft) {  
    currentPage = HOME;
    needsRedraw = true;
    delay(200);
  }
}


// ==============================
// KAHOOT CONFIRM SCREEN
// ==============================
void showConfirmation() {
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);

  display.setCursor(5, 10);
  display.print("You entered :");

  display.setCursor(70, 10);
  for (int i = 0; i < 3; i++) display.print(digits[i]);

  // Parse fake response
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, FAKE_JOIN_RESPONSE);

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

  needsRedraw = false;
}

// ==============================
// HANDLE CONFIRMATION BUTTONS
// ==============================
void handleConfirmationButtons() {
  unsigned int b = display.getButtons();

  // Parse fake join response
  StaticJsonDocument<512> joinDoc;
  DeserializationError joinErr = deserializeJson(joinDoc, FAKE_JOIN_RESPONSE);

  const char* message = nullptr;
  if (!joinErr) message = joinDoc["message"];

  // Back button only if room not found
  if (message && strcmp(message, "Game room not found") == 0) {
    if (b & TSButtonLowerLeft) {
      currentPage = KAHOOT_DIGIT;
      currentDigit = 0;
      needsRedraw = true;
      delay(200);
    }
    return;
  }

  // If student successfully joined → Start button
  if (message && strcmp(message, "Student joined the game room") == 0) {
    if (b & TSButtonLowerRight) {

      // Parse FAKE_START_RESPONSE
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

      // WAITING → still show confirm page
      if (strcmp(status, "WAITING") == 0) {
        currentPage = KAHOOT_CONFIRM;
      }

      // ONGOING → move to question page
      else if (strcmp(status, "ONGOING") == 0) {
        currentPage = KAHOOT_QUESTION;
      }

      needsRedraw = true;
      delay(200);
    }
  }
}

// ==============================
// NEW SCREEN AFTER START
// ==============================
void showKahootQuestion() {
  display.clearScreen();
  display.fontColor(TS_16b_White, TS_16b_Black);

  display.setCursor(5, 10);
  display.print("Question Screen");

  display.setCursor(5, 30);
  display.print("Total: ");
  display.print(questionCount);

  needsRedraw = false;
}


