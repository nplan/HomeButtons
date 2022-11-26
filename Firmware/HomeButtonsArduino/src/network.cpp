#include "network.h"

// ------ global variables ------
WiFiClient wifi_client;
PubSubClient client(wifi_client);
uint32_t wifi_start_time = 0;
uint32_t mqtt_start_time = 0;

bool connect_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);

  // Try to connect with settings stored by ESP32
  // Must not use quick connect on first try after new Wi-Fi settings,
  // because BSSID and channel will not be saved
  if (persisted_s.wifi_quick_connect) {
    WiFi.begin();
    wifi_start_time = millis();
    while (true) {
      delay(50);
      if (WiFi.status() == WL_CONNECTED) {
        return true;
      } else if (millis() - wifi_start_time > QUICK_WIFI_TIMEOUT) {
        break;  // proceed with normal connection retry
      }
    }
  }

  // If not successful, connect only with SSID and password, and then save new
  // channel & BSSID info
  WiFi.begin(); // must be called before WiFi.SSID() otherwise returns empty
  delay(500); // does not work without this delay
  String ssid = WiFi.SSID();
  String psk = WiFi.psk();
  WiFi.begin(ssid.c_str(), psk.c_str());
  wifi_start_time = millis();
  while (true) {
    delay(50);
    if (WiFi.status() == WL_CONNECTED) {
      uint8_t* bssid = WiFi.BSSID();
      int32_t ch = WiFi.channel();
      WiFi.disconnect();
      WiFi.begin(ssid.c_str(), psk.c_str(), ch, bssid, true);
      wifi_start_time = millis();
      while (true) {
        delay(50);
        if (WiFi.status() == WL_CONNECTED) {
          persisted_s.wifi_quick_connect =
              true;  // can connect next time with params saved by esp
          return true;
        } else if (millis() - wifi_start_time > WIFI_TIMEOUT) {
          persisted_s.wifi_quick_connect =
              false;  // make sure next time quick connect is disabled
          return false;
        }
      }
    } else if (millis() - wifi_start_time > WIFI_TIMEOUT) {
      persisted_s.wifi_quick_connect =
          false;  // make sure next time quick connect is disabled
      return false;
    }
  }
}

bool connect_mqtt() {
  client.setServer(user_s.mqtt_server.c_str(), user_s.mqtt_port);
  client.setBufferSize(2048);
  mqtt_start_time = millis();
  while (!client.connected()) {
    if (user_s.mqtt_user.length() > 0 && user_s.mqtt_password.length() > 0) {
      client.connect(factory_s.unique_id.c_str(), user_s.mqtt_user.c_str(),
                     user_s.mqtt_password.c_str());
    } else {
      client.connect(factory_s.unique_id.c_str());
    }
    delay(50);
    if (millis() - mqtt_start_time > MQTT_TIMEOUT) {
      return false;
    }
  }
  return true;
}

void disconnect_mqtt() {
  // Wait a bit
  unsigned long tm = millis();
  while (millis() - tm < MQTT_DISCONNECT_TIMEOUT) {
    client.loop();
  }
  // disconnect and wait until closed
  client.disconnect();
  wifi_client.flush();
  // wait until connection is closed completely
  while (client.state() != -1) {
    client.loop();
    delay(10);
  }
}