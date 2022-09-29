#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "prefs.h"

extern PubSubClient client;

bool connect_wifi();

bool connect_mqtt();

void disconnect_mqtt();

#endif