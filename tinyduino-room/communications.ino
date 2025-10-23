// SPDX-FileCopyrightText: 2025 SIT AIO Authors
// SPDX-License-Identifier: MIT

#include <WiFi101.h>

void communications_setup() {
  WiFi.setPins(8, 2, A3, -1); // VERY IMPORTANT FOR TINYDUINO
}

void communications_loop() {
  // #TODO
}