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
// Wi-Fi Credentials
// ==============================
const char* ssid = ""; // your network SSID (name)
const char* pass = ""; // your network password

// ==============================
// Student Credentials
// ==============================
const int studentID = ; // studentID
const char* studentName = ""; // studentName

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
// Pages
// ==============================
enum Page
{
    HOME,
    ATTENDANCE,
    VACANCY,
    KAHOOT_DIGIT,
    KAHOOT_CONFIRM,
    KAHOOT_QUESTION,
    KAHOOT_WAIT,
    KAHOOT_END
};
Page currentPage = HOME;
bool needsRedraw = true;

// ==============================
// Home Menu
// ==============================
int menuIndex = 0;
const char *menuItems[] = {"Attendance", "Room Vacancy", "Kahoot"};

// ==============================
// Kahoot variables
// ==============================
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
// Main Loop
// ==============================
void loop()
{
    unsigned int b = display.getButtons();

    switch (currentPage)
    {
    case HOME:
        if (needsRedraw)
            showHome();
        handleHomeButtons(b);
        break;
    case ATTENDANCE:
        if (needsRedraw)
            showAttendance();
        handleBackButton(b);
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
// HOME SCREEN
// ==============================
void showHome()
{
    display.clearScreen();
    for (int i = 0; i < 3; i++)
    {
        int y = 10 + i * 20;

        if (i == menuIndex)
        {
            display.drawRect(0, y - 2, 78, 12, 1, TS_16b_White);
            display.fontColor(TS_16b_Black, TS_16b_White);
        }
        else
        {
            display.fontColor(TS_16b_White, TS_16b_Black);
        }

        // Convert const char* to mutable char[]
        char buffer[20];
        strncpy(buffer, menuItems[i], sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        int w = display.getPrintWidth(buffer);
        display.setCursor((80 - w) / 2, y);
        display.print(buffer);
    }
    needsRedraw = false;
}

// ==============================
// ATTENDANCE & VACANCY PAGES
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
// KAHOOT CONFIRM
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
// KAHOOT QUESTION SCREEN
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
// KAHOOT SUBMIT ANSWER
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
// KAHOOT QUESTION WAIT SCREEN
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
// KAHOOT GAME END SCREEN
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
// HOME BUTTONS
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
// BACK BUTTON
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
// KAHOOT ENTER ROOM BUTTON
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
// KAHOOT ENTER ROOM CONFIRMATION BUTTON
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

            String payload = "{\"gameRoomCode\":\"" + String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"}";

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
            int currQuestion = startDoc["questionIndex"];
            questionCount = totalQuestions;

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
// KAHOOT WAIT QUESTION BUTTONS
// ==============================
void handleWaitScreen(unsigned int b)
{
    if (b & TSButtonLowerRight)
    {
        WiFiClient client;
        if (!client.connect(server, serverPort))
            return;

        String payload = "{\"gameRoomCode\":\"" + String(digits[0]) + String(digits[1]) + String(digits[2]) + "\"}";

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

        const char *status = startDoc["status"];
        int currentQuestion = startDoc["questionIndex"];

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
// KAHOOT ANSWER QUESTION BUTTONS
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
// KAHOOT GAME END BUTTON
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