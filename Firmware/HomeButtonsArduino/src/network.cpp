#include "network.h"
#include "utils.h"

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

void callback(const char* topic, uint8_t* payload, uint32_t length) {
  char buff[length+1]; 
  memcpy(buff, payload, (size_t)length);
  buff[length] = '\0'; // required so it can be converted to String
  String topic_str = topic;
  String payload_str = (char*)buff;
  log_i("MQTT Callback on topic: %s, payload %s", topic_str.c_str(), payload_str.c_str());

  if (length < 1) {
    return;
  }

  if (topic_str == topic_s.sensor_interval_cmd) {
    uint16_t mins = payload_str.toInt();
    if (mins >= SEN_INTERVAL_MIN && mins <= SEN_INTERVAL_MAX) {
      user_s.sensor_interval = mins;
      log_i("Setting sensor interval to %d minutes.", mins);
      client.publish(topic_s.sensor_interval_state.c_str(), String(user_s.sensor_interval).c_str(), true);
    }
    else {
      log_i("cmd sensor_interval out of range");
    }
    client.publish(topic_s.sensor_interval_cmd.c_str(), nullptr, true);
  }

  else if (topic_str == topic_s.btn_1_label_cmd) {
    user_s.btn_1_label = check_button_label(payload_str);
    log_i("button 1 label changed to: %s", user_s.btn_1_label.c_str());
    client.publish(topic_s.btn_1_label_state.c_str(), user_s.btn_1_label.c_str(), true);
    client.publish(topic_s.btn_1_label_cmd.c_str(), nullptr, true);
    flags_s.buttons_redraw = true;
  }

  else if (topic_str == topic_s.btn_2_label_cmd) {
    user_s.btn_2_label = check_button_label(payload_str);
    log_i("button 2 label changed to: %s",  user_s.btn_2_label.c_str());
    client.publish(topic_s.btn_2_label_state.c_str(), payload_str.c_str(), true);
    client.publish(topic_s.btn_2_label_cmd.c_str(), nullptr, true);
    flags_s.buttons_redraw = true;
  }

  else if (topic_str == topic_s.btn_3_label_cmd) {
    user_s.btn_3_label = check_button_label(payload_str);
    log_i("button 3 label changed to: %s", user_s.btn_3_label.c_str());
    client.publish(topic_s.btn_3_label_state.c_str(), user_s.btn_3_label.c_str(), true);
    client.publish(topic_s.btn_3_label_cmd.c_str(), nullptr, true);
    flags_s.buttons_redraw = true;
  }

  else if (topic_str == topic_s.btn_4_label_cmd) {
    user_s.btn_4_label = check_button_label(payload_str);
    log_i("button 4 label changed to: %s", user_s.btn_4_label.c_str());
    client.publish(topic_s.btn_4_label_state.c_str(), user_s.btn_4_label.c_str(), true);
    client.publish(topic_s.btn_4_label_cmd.c_str(), nullptr, true);
    flags_s.buttons_redraw = true;
  }

  else if (topic_str == topic_s.btn_5_label_cmd) {
    user_s.btn_5_label = check_button_label(payload_str);
    log_i("button 5 label changed to: %s", user_s.btn_5_label.c_str());
    client.publish(topic_s.btn_5_label_state.c_str(), user_s.btn_5_label.c_str(), true);
    client.publish(topic_s.btn_5_label_cmd.c_str(), nullptr, true);
    flags_s.buttons_redraw = true;
  }

  else if (topic_str == topic_s.btn_6_label_cmd) {
    user_s.btn_6_label = check_button_label(payload_str);
    log_i("button 6 label changed to: %s", user_s.btn_6_label.c_str());
    client.publish(topic_s.btn_6_label_state.c_str(), user_s.btn_6_label.c_str(), true);
    client.publish(topic_s.btn_6_label_cmd.c_str(), nullptr, true);
    flags_s.buttons_redraw = true;
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

  // subscribe
  log_i("sub to: %s", topic_s.sensor_interval_cmd.c_str());
  client.subscribe(topic_s.sensor_interval_cmd.c_str());
  client.subscribe(topic_s.btn_1_label_cmd.c_str());
  client.subscribe(topic_s.btn_2_label_cmd.c_str());
  client.subscribe(topic_s.btn_3_label_cmd.c_str());
  client.subscribe(topic_s.btn_4_label_cmd.c_str());
  client.subscribe(topic_s.btn_5_label_cmd.c_str());
  client.subscribe(topic_s.btn_6_label_cmd.c_str());
  client.setCallback(callback);

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