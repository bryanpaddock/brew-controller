// Copyright (C) 2022 Bryan Paddock

// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.

// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.

// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include <Arduino.h>
#include "elements.h"
#include "probes.h"
#include "menu.h"
#include "mqtt.h"
#include "wifi.h"

bool DEBUG = false;
unsigned long currentMillis;
const char version[] = "v2.0.0";

bool isWifiEnabled = false;
bool isMqttEnabled = false;
bool isTempProbesEnabled = false;

void setup()
{
  if (DEBUG)
    Serial.begin(115200);

  if (isWifiEnabled)
    setupWifi();

  if (isMqttEnabled)
    setupMqtt();

  if (isTempProbesEnabled)
    setupTempProbes();

  setupElements();
  setupMenu();
}

void loop()
{
  currentMillis = millis();

  if (isMqttEnabled)
    loopMqtt();

  if (isTempProbesEnabled)
    loopTempProbes();

  loopElements();
  loopMenu();

  yield();
}