// SPDX-FileCopyrightText: 2025 SIT AIO Authors
// SPDX-License-Identifier: MIT

#include <TSL2572.h>      // For TSL2572 ambient light sensor
int room_occupancy_status = -1;
float AmbientLightLuxPrev = NULL;
float AmbientLightLuxCur = NULL;
TSL2572 light_sensor;

void occupancy_setup() {
  light_sensor.init(GAIN_16X);
}

void occupancy_loop() {
  if (AmbientLightLuxPrev) {
    AmbientLightLuxPrev = AmbientLightLuxCur;
  }
  AmbientLightLuxCur = light_sensor.readAmbientLight();
  
  if (AmbientLightLuxPrev == NULL) {
    AmbientLightLuxPrev = AmbientLightLuxCur;
  }

  float ratio = (AmbientLightLuxCur - AmbientLightLuxPrev) / AmbientLightLuxPrev;
  if (ratio < -0.1) {
    room_occupancy_status = 0;
  } else if (ratio > 2) {
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
