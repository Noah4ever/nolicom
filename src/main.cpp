#include <Arduino.h>
#include "SevenSegmentDisplay.h"
#include "Buzzer.h"
#include "TriLeds.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ---- Pins ----
// 74HC595
static const uint8_t PIN_DATA = 23;  // SER/DS
static const uint8_t PIN_CLOCK = 21; // SRCLK
static const uint8_t PIN_LATCH = 22; // RCLK
// 7-Segment
static const uint8_t PIN_DIG_LEFT = 25;
static const uint8_t PIN_DIG_RIGHT = 26;
// Buzzer
static const uint8_t PIN_BUZZER = 17;
// Tri LEDs
static const uint8_t PIN_LED_G = 16;
static const uint8_t PIN_LED_Y = 5;
static const uint8_t PIN_LED_R = 19;
// Button (to GND, needs internal pull-up)
static const uint8_t PIN_BTN = 33;

// ---- Peer MAC (receiver) ----
static const uint8_t RECEIVER_MAC[6] = {0x8C, 0x4F, 0x00, 0x0F, 0xD8, 0x54};
// If you want to send to the other one instead, swap to:
// static const uint8_t RECEIVER_MAC[6] = {0xF8, 0xB3, 0xB7, 0x45, 0x35, 0x00};

// ---- App state ----
SevenSegmentDisplay disp;
Buzzer buzz;
TriLeds leds;

struct __attribute__((packed)) Msg
{
    uint8_t cmd;         // 1 = LED pulse
    uint16_t durationMs; // how long the LED should stay ON on the receiver
};

volatile bool recvPulse = false;
static uint32_t recvLedOffAt = 0; // set to millis()+2000 when a packet arrives

// ---- Helpers ----
static void forceChannel(int ch)
{
    // Set Wi-Fi primary channel (both ends must match)
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
}

static bool addPeer(const uint8_t *mac, uint8_t channel)
{
    if (esp_now_is_peer_exist(mac))
        return true;
    esp_now_peer_info_t p{};
    memcpy(p.peer_addr, mac, 6);
    p.channel = channel; // must match Wi-Fi channel
    p.encrypt = false;
    esp_err_t err = esp_now_add_peer(&p);
    if (err != ESP_OK)
    {
        Serial.printf("esp_now_add_peer failed: 0x%02X\n", err);
        return false;
    }
    return true;
}

// ---- ESPNOW Callbacks ----
static void onRecv(const uint8_t *srcMac, const uint8_t *data, int len)
{
    if (len < (int)sizeof(Msg))
        return;
    Msg m{};
    memcpy(&m, data, sizeof(Msg));
    if (m.cmd == 1)
    {
        recvPulse = true;
    }
}

static void onSent(const uint8_t *dstMac, esp_now_send_status_t status)
{
    Serial.printf("Send to %02X:%02X:%02X:%02X:%02X:%02X -> %s\n",
                  dstMac[0], dstMac[1], dstMac[2], dstMac[3], dstMac[4], dstMac[5],
                  status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ---- Setup ----
void setup()
{
    Serial.begin(115200);

    // Peripherals
    disp.init(PIN_DATA, PIN_CLOCK, PIN_LATCH, PIN_DIG_LEFT, PIN_DIG_RIGHT);
    disp.setDigitActiveHigh(true);   // enabling digit = HIGH
    disp.setSegmentsActiveLow(true); // segment ON = LOW
    disp.setBrightnessMicros(250);

    buzz.init(PIN_BUZZER);
    buzz.setVolume(95);
    buzz.setTempoFactor(1.5);
    buzz.play(BuiltInMelody::BOOT, false);

    leds.init(PIN_LED_G, PIN_LED_Y, PIN_LED_R, true, true);

    pinMode(PIN_BTN, INPUT_PULLUP);

    // Wi-Fi / ESP-NOW
    WiFi.mode(WIFI_STA);
    const uint8_t CHANNEL = 1; // <<< set this to the channel your receiver uses
    forceChannel(CHANNEL);

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("ERROR: esp_now_init() failed!");
        // don't return; allow other parts to run but sending will fail
    }
    else
    {
        esp_now_register_recv_cb(onRecv);
        esp_now_register_send_cb(onSent);
        addPeer(RECEIVER_MAC, CHANNEL);
    }

    Serial.println("Setup done.");
}

// ---- Sending ----
static void sendPulseOnce()
{
    Msg m{1, 2000};
    esp_err_t err = esp_now_send(RECEIVER_MAC, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
    if (err != ESP_OK)
    {
        Serial.printf("esp_now_send error: 0x%02X\n", err);
    }
}

// ---- Loop ----
void loop()
{
    // housekeeping
    disp.refresh();
    disp.updateScrolling();
    disp.updateBlinking();
    buzz.update();
    leds.update();

    // handle received pulse -> play animation for 2 seconds
    if (recvPulse)
    {
        recvPulse = false;
        leds.playLEDAnim(TriLeds::Anim::ChaseGYR);
        buzz.play(BuiltInMelody::BEEP_BEEP, false);
        disp.setString("HI");
        disp.setBlinkingText("HI", 300);
        recvLedOffAt = millis() + 2000; // start 2s window NOW
    }
    if (recvLedOffAt && millis() >= recvLedOffAt)
    {
        recvLedOffAt = 0;
        leds.playLEDAnim(TriLeds::Anim::Off);
        leds.off();
        buzz.stop();
        disp.setString("  ");
        disp.stopBlinking();
    }

    // button press -> send pulse
    static uint32_t lastPressMs = 0;
    const bool pressed = (digitalRead(PIN_BTN) == LOW);
    if (pressed && millis() - lastPressMs > 250)
    { // simple debounce
        lastPressMs = millis();
        Serial.println("Button pressed -> sending pulse");
        // local blink on RED
        digitalWrite(PIN_LED_R, HIGH);
        delay(60);
        digitalWrite(PIN_LED_R, LOW);

        sendPulseOnce();
    }
}
