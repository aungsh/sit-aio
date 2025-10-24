// SPDX-FileCopyrightText: 2025 SIT AIO Authors
// SPDX-License-Identifier: MIT

#include <Wire.h>

#if defined (ARDUINO_ARCH_AVR)
#define SerialMonitorInterface Serial
#include <SoftwareSerial.h>
#elif defined(ARDUINO_ARCH_SAMD)
#define SerialMonitorInterface SerialUSB
#endif

void setup()
{
  SerialMonitorInterface.print("== PROGRAM START ==");
  Wire.begin();
  SerialMonitorInterface.begin(9600);
  communications_setup();
  occupancy_setup();
} 

void loop()
{
  communications_loop();
  occupancy_loop();
  delay(1000);
}