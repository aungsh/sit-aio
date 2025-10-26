// SPDX-FileCopyrightText: 2025 SIT AIO Authors
// SPDX-License-Identifier: MIT

#include <WiFi101.h>

// Wi-Fi Credentials
const char* ssid = ""; // your network SSID (name)
const char* pass = ""; // your network password

// API server (Host or IP) and path
const char* server = "172.20.61.115"; // your server ip (no "http://")
const int serverPort = 3000; // your server port
const char* apiPath = "/api/room-vacancy";


// Send interval
unsigned long lastSend = 0;
const unsigned long sendInterval = 60UL * 1000UL; // send every 60 seconds

void wait_for_wifi_connection() {
  int status = WL_IDLE_STATUS;
  unsigned long start = millis();
  
  SerialMonitorInterface.print("[WiFI Communications] Connecting to ");
  SerialMonitorInterface.println(ssid);
  
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

void send_api_patch() {
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

void communications_setup() {
  WiFi.setPins(8, 2, A3, -1); // VERY IMPORTANT FOR TINYDUINO
  wait_for_wifi_connection();
}

void communications_loop() {
  if (millis() - lastSend >= sendInterval) {
    lastSend = millis();
    send_api_patch();
  }
}
