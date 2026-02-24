#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

// ---------- Pins ----------
constexpr int LED_PIN = 16; // External LED -> 220Ω -> GND
constexpr int BTN_PIN = 33; // Button -> GND (INPUT_PULLUP)
constexpr int ROLE_PIN = 4; // Role jumper: GND = Device A, HIGH/3V3 = Device B
constexpr int WIFI_CH = 1;  // Keep both boards on same channel

#ifndef LED_BUILTIN
#define LED_BUILTIN 2 // Most ESP32 devkits: onboard LED on GPIO2
#endif
constexpr int ONBOARD_LED = LED_BUILTIN;

// ---------- Your MACs ----------
uint8_t macA[6] = {0x8C, 0x4F, 0x00, 0x0F, 0xD8, 0x54}; // Device A
uint8_t macB[6] = {0xF8, 0xB3, 0xB7, 0x45, 0x35, 0x00}; // Device B
uint8_t peerMac[6], myMac[6];

// ---------- Message ----------
struct __attribute__((packed)) Msg
{
    uint8_t cmd;
    uint8_t seconds;
}; // cmd=1 -> LED on

// ---------- State (runtime) ----------
volatile bool irqBtn = false;
void IRAM_ATTR isrBtn() { irqBtn = true; }

volatile bool recvPulse = false;
volatile uint32_t recvLedOffAt = 0;

bool btnLatched = false;
uint32_t debounceAt = 0;

// ---------- Pairing persistence ----------
Preferences prefs;
bool seenRx = false;     // have we ever received a packet (on this device)?
bool seenTxOK = false;   // have we ever gotten an ESP-NOW send OK (to peer)?
bool pairedEver = false; // seenRx && seenTxOK from past or this boot

// ---------- Utils ----------
static String macToStr(const uint8_t *m)
{
    char b[18];
    sprintf(b, "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
    return String(b);
}

// flash onboard LED once at boot if already paired before (or becomes paired during this boot before flashing)
static void bootFlashIfPairedOnce()
{
    if (!pairedEver)
        return; // only flash at boot, once, and only if already paired *in the past*
    pinMode(ONBOARD_LED, OUTPUT);
    digitalWrite(ONBOARD_LED, HIGH);
    delay(150);
    digitalWrite(ONBOARD_LED, LOW);
}

// persist current pair flags
static void savePairFlags()
{
    prefs.putBool("seenRx", seenRx);
    prefs.putBool("seenTxOK", seenTxOK);
    prefs.putBool("paired", (seenRx && seenTxOK));
}

// ---------- ESP-NOW callbacks (NON-BLOCKING) ----------
static void onRecv(const uint8_t *, const uint8_t *data, int len)
{
    if (len < (int)sizeof(Msg))
        return;
    Msg m;
    memcpy(&m, data, sizeof(Msg));
    if (m.cmd == 1)
    {
        recvPulse = true;
        recvLedOffAt = millis() + (uint32_t)m.seconds * 1000UL;
    }
    // mark RX seen, persist
    if (!seenRx)
    {
        seenRx = true;
        savePairFlags();
    }
}

static void onSent(const uint8_t *, esp_now_send_status_t s)
{
    if (s == ESP_NOW_SEND_SUCCESS && !seenTxOK)
    {
        seenTxOK = true;
        savePairFlags();
    }
}

// ---------- ESP-NOW helpers ----------
static void forceChannel(int ch)
{
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

static void addPeer(const uint8_t *mac)
{
    if (esp_now_is_peer_exist(mac))
        return;
    esp_now_peer_info_t p{};
    memcpy(p.peer_addr, mac, 6);
    p.channel = WIFI_CH;
    p.encrypt = false;
    esp_now_add_peer(&p);
}

static void sendPulse(uint8_t seconds = 3)
{
    Msg m{1, seconds};
    // send short burst for reliability (non-blocking callbacks)
    uint32_t t0 = millis();
    while (millis() - t0 < 300) // 300 ms
    { 
        esp_now_send(peerMac, (uint8_t *)&m, sizeof(m));
        delay(25);
    }
}

void setup()
{
    // IO
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    pinMode(BTN_PIN, INPUT_PULLUP);
    pinMode(ROLE_PIN, INPUT_PULLUP); // TIP: on Device B, tie ROLE_PIN to 3V3 with a short jumper to avoid floating
    attachInterrupt(digitalPinToInterrupt(BTN_PIN), isrBtn, FALLING);

    Serial.begin(115200);
    delay(100);

    // Load pairing flags before any flash
    prefs.begin("pair", false);
    seenRx = prefs.getBool("seenRx", false);
    seenTxOK = prefs.getBool("seenTxOK", false);
    pairedEver = prefs.getBool("paired", false);

    // Boot flash ONCE if we have already paired in a previous run
    bootFlashIfPairedOnce();

    // WiFi / ESP-NOW init
    WiFi.mode(WIFI_STA);
    forceChannel(WIFI_CH);
    WiFi.macAddress(myMac);
    Serial.print("My MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.printf("WiFi channel: %d\n", WIFI_CH);

    bool isA = (digitalRead(ROLE_PIN) == LOW);
    Serial.print("Role: ");
    Serial.println(isA ? "Device A (ROLE_PIN=GND)" : "Device B (ROLE_PIN=HIGH/3V3)");

    memcpy(peerMac, isA ? macB : macA, 6);
    Serial.print("Peer MAC: ");
    Serial.println(macToStr(peerMac));
    if (memcmp(peerMac, myMac, 6) == 0)
    {
        Serial.println("ERROR: peerMac == myMac (check MACs & role jumper)");
    }

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ESP-NOW init failed");
        ESP.restart();
    }
    esp_now_register_recv_cb(onRecv);
    esp_now_register_send_cb(onSent);
    addPeer(peerMac);
}

void loop()
{
    // handle received LED pulse (non-blocking)
    if (recvPulse)
    {
        recvPulse = false;
        digitalWrite(LED_PIN, HIGH);
    }
    if (digitalRead(LED_PIN) == HIGH && millis() >= recvLedOffAt)
    {
        digitalWrite(LED_PIN, LOW);
    }

    // debounced button handling
    if (irqBtn)
    {
        irqBtn = false;
        debounceAt = millis();
    }
    if (!btnLatched && (millis() - debounceAt) >= 15)
    {
        if (digitalRead(BTN_PIN) == LOW)
        {
            btnLatched = true;

            // local short feedback blink
            digitalWrite(LED_PIN, HIGH);
            delay(60);
            digitalWrite(LED_PIN, LOW);

            sendPulse(3); // ask peer to light for 3 seconds
        }
    }
    if (btnLatched && digitalRead(BTN_PIN) == HIGH)
    {
        btnLatched = false;
    }

    delay(1);
}
