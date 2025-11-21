#include <TSL2572.h>      // For TSL2572 ambient light sensor
#include <Wire.h>
#include <WiFi101.h>
#include <TinyScreen.h>
#include <TimeLib.h>

#if defined(ARDUINO_ARCH_SAMD)
  #define SerialMonitorInterface SerialUSB
#else
  #define SerialMonitorInterface Serial
#endif

// Wi-Fi Credentials
const char* ssid = ""; // your network SSID (name)
const char* pass = ""; // your network password
char *debug_source = "";

// API server (Host or IP) and path
const char* server = "134.185.93.17"; // your server ip (no "http://")
const int serverPort = 8080; // your server port
const char* apiPath = "/api/room-vacancy";


// Room details
#define ROOM_ID "1"

uint8_t room_occupancy_status_updated = false;
uint8_t room_occupancy_status = NULL;
float AmbientLightLuxPrev = NULL;
float AmbientLightLuxCur = NULL;
TSL2572 light_sensor;

void sensor_loop() {
  if (AmbientLightLuxPrev) {
    AmbientLightLuxPrev = AmbientLightLuxCur;
  }
  AmbientLightLuxCur = light_sensor.readAmbientLight();

  if (AmbientLightLuxPrev == NULL) {
    AmbientLightLuxPrev = AmbientLightLuxCur;
  }

  float ratio = (AmbientLightLuxCur - AmbientLightLuxPrev) / AmbientLightLuxPrev;
  if (ratio < -0.1) {
    room_occupancy_status_updated = true;
    room_occupancy_status = 0;
  } else if (ratio > 2) {
    room_occupancy_status_updated = true;
    room_occupancy_status = 1;
  }
  SerialMonitorInterface.print("Lux Cur: ");
  SerialMonitorInterface.println(AmbientLightLuxCur);
  SerialMonitorInterface.print("Lux Prev: ");
  SerialMonitorInterface.println(AmbientLightLuxPrev);

  SerialMonitorInterface.print("Current state:");
  if (room_occupancy_status == -1) {
    SerialMonitorInterface.println("Unknown");
  } else if (room_occupancy_status == 1) {
    SerialMonitorInterface.println("Occupied");
  } else {
    SerialMonitorInterface.println("Vacant");
  }
  SerialMonitorInterface.print("room_occupancy_status: ");
  SerialMonitorInterface.println(room_occupancy_status);
  SerialMonitorInterface.print("ratio: ");
  SerialMonitorInterface.println(ratio);
}

void debug(char* debug_msg) {
  SerialMonitorInterface.println(String("[") + String(debug_source) + "] " + debug_msg);
}

int8_t api_init() {
  debug_source = "api_init";
  WiFi.setPins(8, 2, A3, -1); // VERY IMPORTANT FOR TINYDUINO
  SerialMonitorInterface.println();
  SerialMonitorInterface.println(String("Connecting to ") + ssid);

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
    return -1;
  }
  // END Wi-Fi setup
  SerialMonitorInterface.println("Initializing...");
}

void setup() {
  // Serial setup
  SerialMonitorInterface.begin(9600);
  delay(500);

  Wire.begin();

  // Light sensor setup
  light_sensor.init(GAIN_120X);
  api_init();

  SerialMonitorInterface.println();
  SerialMonitorInterface.println("Initializing...");
}

void api_loop() {
  if (room_occupancy_status_updated) {
    SerialMonitorInterface.println(String("Room occupancy status changed to ") + room_occupancy_status);

    if (room_occupancy_status == -1) {
      SerialMonitorInterface.println("Unknown");
    } else if (room_occupancy_status == 1) {
      sendApiPatch("OCCUPIED");
    } else {
      sendApiPatch("VACANT");
    }
  }
  room_occupancy_status_updated = false;
}


void sendApiPatch(char* room_status) {
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
  String payload = String("{\"roomId\":" ROOM_ID ",\"status\":\"") + room_status + "\"}";
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

void loop() {
  sensor_loop();
  api_loop();
  delay(500);
}