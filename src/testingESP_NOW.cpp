// #include <Arduino.h>
// #include <WiFi.h>
// #include <esp_now.h>
// #include <esp_wifi.h>

// // -------- Pins --------
// constexpr int LED_PIN = 16; // external LED -> 220Ω -> GND
// constexpr int BTN_PIN = 33; // button -> GND (INPUT_PULLUP)
// constexpr int ROLE_PIN = 4; // jumper: GND = Device A, HIGH/open = Device B
// constexpr int WIFI_CH = 1;  // keep both on same channel

// // -------- Your MACs --------
// uint8_t macA[6] = {0x8C, 0x4F, 0x00, 0x0F, 0xD8, 0x54}; // Device A
// uint8_t macB[6] = {0xF8, 0xB3, 0xB7, 0x45, 0x35, 0x00}; // Device B
// uint8_t peerMac[6], myMac[6];

// // -------- Message --------
// struct __attribute__((packed)) Msg
// {
//     uint8_t cmd;
//     uint8_t seconds;
// }; // cmd=1 -> LED on

// // simple flags
// volatile bool irqBtn = false;
// volatile bool recvPulse = false;
// volatile uint32_t recvLedOffAt = 0;

// void IRAM_ATTR isrBtn() { irqBtn = true; }

// static void forceChannel(int ch)
// {
//     esp_wifi_set_promiscuous(true);
//     esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
//     esp_wifi_set_promiscuous(false);
// }

// static void addPeer(const uint8_t *mac)
// {
//     if (esp_now_is_peer_exist(mac))
//         return;
//     esp_now_peer_info_t p{};
//     memcpy(p.peer_addr, mac, 6);
//     p.channel = WIFI_CH;
//     p.encrypt = false;
//     esp_now_add_peer(&p);
// }

// // we don't care about receiving while sending, but the callback is light anyway
// static void onRecv(const uint8_t *, const uint8_t *data, int len)
// {
//     if (len < (int)sizeof(Msg))
//         return;
//     Msg m;
//     memcpy(&m, data, sizeof(Msg));
//     if (m.cmd == 1)
//     {
//         recvPulse = true;
//         recvLedOffAt = millis() + (uint32_t)m.seconds * 1000UL; // e.g. 3s
//     }
// }

// // fire-and-forget: spam packets for ~1s
// static void sendPulse(uint8_t seconds)
// {
//     Msg m{1, seconds};
//     uint32_t t0 = millis();
//     while (millis() - t0 < 1000)
//     { // 1 second burst
//         esp_now_send(peerMac, (uint8_t *)&m, sizeof(m));
//         delay(25);
//     }
// }

// void setup()
// {
//     pinMode(LED_PIN, OUTPUT);
//     digitalWrite(LED_PIN, LOW);
//     pinMode(BTN_PIN, INPUT_PULLUP);
//     pinMode(ROLE_PIN, INPUT_PULLUP); // on the "B" board, tie to 3V3 to be solid

//     attachInterrupt(digitalPinToInterrupt(BTN_PIN), isrBtn, FALLING);

//     Serial.begin(115200);
//     delay(100);
//     WiFi.mode(WIFI_STA);
//     forceChannel(WIFI_CH);
//     WiFi.macAddress(myMac);

//     bool isA = (digitalRead(ROLE_PIN) == LOW);
//     memcpy(peerMac, isA ? macB : macA, 6);

//     esp_now_register_recv_cb(onRecv);
//     addPeer(peerMac);
// }

// void loop()
// {
//     // remote LED control from received pulses
//     if (recvPulse)
//     {
//         recvPulse = false;
//         digitalWrite(LED_PIN, HIGH);
//     }
//     if (digitalRead(LED_PIN) == HIGH && millis() >= recvLedOffAt)
//     {
//         digitalWrite(LED_PIN, LOW);
//     }

//     // button press -> blast packets for 1s, simple local feedback blink
//     if (irqBtn && digitalRead(BTN_PIN) == LOW)
//     {
//         irqBtn = false;              // clear immediately; no debounce on purpose
//         digitalWrite(LED_PIN, HIGH); // short local blink so you know it registered
//         delay(80);
//         digitalWrite(LED_PIN, LOW);

//         sendPulse(3); // ask peer to light for 3 seconds

//         // optional: wait for release so holding the button doesn't retrigger right away
//         while (digitalRead(BTN_PIN) == LOW)
//             delay(1);
//     }

//     delay(1);
// }
