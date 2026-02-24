// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <esp_wifi.h>

// #define LED_PIN 14
// #define BUTTON_PIN 33 // tied to GND, use INPUT_PULLUP

// // MAC of the *other* ESP32 (the one you want to send to)
// static const uint8_t RECEIVER_MAC[6] = {0xF8, 0xB3, 0xB7, 0x45, 0x35, 0x00};

// // Payload structure
// struct __attribute__((packed)) Msg
// {
//     uint8_t cmd;         // 1 = LED pulse
//     uint16_t durationMs; // how long LED should stay on
// };

// volatile unsigned long ledOffAt = 0;

// static void onRecv(const uint8_t *mac, const uint8_t *data, int len)
// {
//     if (len < (int)sizeof(Msg))
//         return;
//     Msg m{};
//     memcpy(&m, data, sizeof(Msg));

//     Serial.printf("RX cmd=%u durationMs=%u from %02X:%02X:%02X:%02X:%02X:%02X\n",
//                   m.cmd, m.durationMs,
//                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

//     if (m.cmd == 1)
//     {
//         digitalWrite(LED_PIN, HIGH);
//         uint16_t d = m.durationMs ? m.durationMs : 1;
//         if (d > 60000)
//             d = 60000; // safety clamp
//         ledOffAt = millis() + d;
//     }
// }

// static void onSent(const uint8_t *dstMac, esp_now_send_status_t status)
// {
//     Serial.printf("Sent -> %s\n",
//                   status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
// }

// void setup()
// {
//     Serial.begin(115200);
//     pinMode(LED_PIN, OUTPUT);
//     digitalWrite(LED_PIN, LOW);

//     pinMode(BUTTON_PIN, INPUT_PULLUP);

//     WiFi.mode(WIFI_STA);

//     if (esp_now_init() != ESP_OK)
//     {
//         Serial.println("ESP-NOW init failed");
//         return;
//     }
//     esp_now_register_recv_cb(onRecv);
//     esp_now_register_send_cb(onSent);

//     // Add peer we want to send to
//     esp_now_peer_info_t peer{};
//     memcpy(peer.peer_addr, RECEIVER_MAC, 6);
//     peer.channel = 0; // current Wi-Fi channel
//     peer.encrypt = false;
//     if (!esp_now_is_peer_exist(RECEIVER_MAC))
//     {
//         if (esp_now_add_peer(&peer) != ESP_OK)
//         {
//             Serial.println("Failed to add peer");
//         }
//     }

//     Serial.println("Receiver+Sender ready");
//     digitalWrite(LED_PIN, HIGH);
//     delay(500);
//     digitalWrite(LED_PIN, LOW);
// }

// void loop()
// {
//     // Handle LED timeout
//     if (ledOffAt && millis() >= ledOffAt)
//     {
//         digitalWrite(LED_PIN, LOW);
//         ledOffAt = 0;
//         Serial.println("LED OFF (timeout)");
//     }

//     // Handle button press -> send
//     static bool lastState = HIGH;
//     bool state = digitalRead(BUTTON_PIN); // LOW = pressed
//     if (state == LOW && lastState == HIGH)
//     {
//         Serial.println("Button pressed -> sending");
//         Msg m{1, 2000}; // LED on for 2000ms at the other ESP
//         esp_now_send(RECEIVER_MAC, (uint8_t *)&m, sizeof(m));
//     }
//     lastState = state;
// }
