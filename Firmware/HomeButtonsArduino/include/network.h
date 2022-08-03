#ifndef NETWORK_H
#define NETWORK_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include "prefs.h"

extern PubSubClient client;

bool connect_wifi();

bool connect_mqtt();

void disconnect_mqtt();

#endif