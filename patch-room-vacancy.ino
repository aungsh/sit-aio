#include <SPI.h>
#include <WiFi101.h>

// Wi-Fi Credentials
const char* ssid = ""; // your network SSID (name)
const char* pass = ""; // your network password

// API server (Host or IP) and path
const char* server = "172.20.61.115"; // your server ip (no "http://")
const int serverPort = 3000; // your server port
const char* apiPath = "/api/room-vacancy";

unsigned long lastSend = 0;
const unsigned long sendInterval = 60UL * 1000UL; // send every 60 seconds

#if defined(ARDUINO_ARCH_SAMD)
  #define SerialMonitorInterface SerialUSB
#else
  #define SerialMonitorInterface Serial
#endif

void setup() {
  SerialMonitorInterface.begin(9600);
  WiFi.setPins(8, 2, A3, -1); // VERY IMPORTANT FOR TINYDUINO
  while (!SerialMonitorInterface);

  SerialMonitorInterface.println();
  SerialMonitorInterface.print("Connecting to ");
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
    SerialMonitorInterface.println("WiFi connected");
    SerialMonitorInterface.print("IP: ");
    SerialMonitorInterface.println(WiFi.localIP());
  } else {
    SerialMonitorInterface.println("WiFi connection failed");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // Try reconnect
    WiFi.begin(ssid, pass);
    delay(2000);
    return;
  }

  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();
    sendApiPatch();
  }
}

void sendApiPatch() {
  WiFiClient client;
  SerialMonitorInterface.print("Connecting to ");
  SerialMonitorInterface.print(server);
  SerialMonitorInterface.print(":");
  SerialMonitorInterface.println(serverPort);

  if (!client.connect(server, serverPort)) {
    SerialMonitorInterface.println("Connection failed");
    return;
  }

  // Example JSON payload
  String payload = "{\"roomId\":1,\"status\":\"OCCUPIED\"}"; // replace accordingly with regards to light sensor data
  SerialMonitorInterface.println(payload);

  // Build HTTP PATCH
  client.print(String("PATCH ") + apiPath + " HTTP/1.1\r\n");
  client.print(String("Host: ") + server + "\r\n");
  client.print("Content-Type: application/json\r\n");
  client.print(String("Content-Length: ") + payload.length() + "\r\n");
  client.print("Connection: close\r\n");
  client.print("\r\n");
  client.print(payload);

  // Wait for response (with timeout)
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read and print response
  SerialMonitorInterface.println("Response:");
  while (client.available()) {
    String line = client.readStringUntil('\n');
    SerialMonitorInterface.println(line);
  }

  client.stop();
  SerialMonitorInterface.println("Connection closed");
}