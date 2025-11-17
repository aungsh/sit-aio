#include <Wire.h>
#include <TinyScreen.h>
#include <WiFi101.h>

#if defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#else
#define SerialMonitorInterface Serial
#endif

const char* apiPath = "/api/classhoot/check";
TinyScreen display = TinyScreen(2);  // I2C address
uint8_t lastButtons = 0;  // Store initial button state
int answer = 1; // Store base answer value
int totalQuestions = 5; // Interim value
int questionIndex = 0; //Interim value
int lastQuestionIndex = -1; // -1 means nothing drawn yet
int totalScore = 0;

enum ScreenState { QUESTION,
                   SUBMITTED,
                   GAME_ENDED };

ScreenState screenState = QUESTION;

void setup() {
    Wire.begin();
    display.begin();
    Serial.begin(9600);  // For debugging
    display.setBrightness(15);

    display.setFont(liberationSans_8ptFontInfo);
    display.fontColor(0xFFFF, 0x0000);  // White on black
}

void loop() {
    // Read current button state
    uint8_t buttons = display.getButtons();
    // Detect new presses only
    uint8_t pressed = buttons & ~lastButtons;

    if (pressed & 0x01) {
        if (answer == 1) {
            answer = 4;
        } else {
            answer--;
        }
        SerialMonitorInterface.println(answer);
    }

    if (pressed & 0x02) {
        if (answer == 4) {
            answer = 1;
        } else {
            answer++;
        }
        SerialMonitorInterface.println(answer);
    }

    if (pressed & 0x08) {
        if (screenState == QUESTION) {
            display.clearScreen();
            submit("134.185.93.17", 8080);
            screenState = SUBMITTED;
        } else if (screenState == SUBMITTED) {
            questionIndex++;
            if (questionIndex == totalQuestions) {
                screenState = GAME_ENDED;
                display.clearScreen();
                drawEnded();
            } else {
                screenState = QUESTION;
                answer = 1;
                display.clearScreen();
            }
        }
    }

    if (screenState == QUESTION) {
        if (questionIndex != lastQuestionIndex) {
            display.clearScreen(); // clear only when question changes
            question(questionIndex);
            lastQuestionIndex = questionIndex; // update tracker
        }
    }

    lastButtons = buttons;

    display.flush();
}

void question(int questionIndex) {
    display.clearScreen();
    display.setCursor(2, 2);
    display.print("Question " + String(questionIndex + 1));

    display.setCursor(2, 16);
    display.print("10:00");

    // Options
    display.setCursor(72, 2);
    display.print("1");
    display.setCursor(72, 16);
    display.print("2");
    display.setCursor(72, 30);
    display.print("3");
    display.setCursor(72, 44);
    display.print("4");

    // Submit button
    display.setCursor(28, 52);
    display.print("Submit");
}

void submit(const char *server, const int serverPort) {
    WiFiClient client;
    SerialMonitorInterface.print("Connecting to ");
    SerialMonitorInterface.print(server);
    SerialMonitorInterface.print(":");
    SerialMonitorInterface.println(serverPort);

    if (!client.connect(server, serverPort)) {
        SerialMonitorInterface.println("Connection failed");
        //return;
    }

    String payload = "{\"studentId\":2501909,"
                      "\"gameRoomCode\":\"000\","
                      "\"questionIndex\":" + String(questionIndex) + "," +
                      "\"optionSelected\":" + String(answer) + "," +
                      "\"timeTaken\":5}";

    // Build HTTP POST
    client.print(String("POST ") + apiPath + " HTTP/1.1\r\n");
    client.print(String("Host: ") + server + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print(String("Content-Length: ") + payload.length() + "\r\n");
    client.print("Connection: close\r\n");
    client.print("\r\n");
    client.print(payload);

    display.setCursor(20, 28);
    display.print("Submitted!");

    display.setCursor(65, 52);
    display.print("Next");

    // Wait for response (with timeout)
    unsigned long timeout = millis();

    while (client.available() == 0) {
        if (millis() - timeout > 5000) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        return;
        }
    }
        
    // Read the response
    String response = "";

    while (client.available()) {
        char c = client.read();
        response += c;
    }

    SerialMonitorInterface.println("Response:");
    SerialMonitorInterface.println(response);
    // Extract "score"
    int sStart = response.indexOf("\"score\":");

    if (sStart != -1) {
        sStart += 8; // move past "score":
        int sEnd = response.indexOf(",", sStart);
        if (sEnd == -1) sEnd = response.indexOf("}", sStart);
        totalScore += response.substring(sStart, sEnd).toInt();
    }
}

void drawEnded() {
    display.setCursor(20, 28);
    display.print("Game Ended");

    display.setCursor(65, 52);
    display.print("Back");
}