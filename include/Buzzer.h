#pragma once
#include <Arduino.h>
#include <vector>

struct Note
{
    uint16_t freq;  // Hz (0 = rest)
    uint16_t durMs; // milliseconds
};

enum class BuiltInMelody : uint8_t
{
    SCALE_UP,
    SCALE_DOWN,
    TWINKLE,
    BEEP_BEEP,
    BOOT
};

class Buzzer
{
public:
    Buzzer() {}

    // pin: GPIO to drive the (passive) piezo buzzer (default GPIO 5)
    // channel: LEDC PWM channel 0..15 (default 0)
    // timer: LEDC timer 0..3 (default 0)
    // resolutionBits: 8..15 (duty resolution; default 10 bits => 0..1023)
    void init(uint8_t pin = 5, uint8_t channel = 0, uint8_t timer = 0, uint8_t resolutionBits = 10);

    // Play a built-in melody
    void play(BuiltInMelody m, bool repeat = false);

    // Play a custom sequence (copied into internal buffer)
    void play(const std::vector<Note> &seq, bool repeat = false);

    // Stop / pause / resume
    void stop();
    void pause();
    void resume();

    // Call often in loop() to advance playback
    void update();

    // Status
    bool isPlaying() const { return _playing && !_paused; }
    bool isPaused() const { return _paused; }

    // Options
    void setVolume(uint8_t pct);       // 0..100 (% duty)
    void setTempoFactor(float factor); // multiply note durations (e.g. 0.8 faster, 1.2 slower)
    void setRepeat(bool rep) { _repeat = rep; }

    // Convenience one-shot beep (non-blocking fire-and-forget)
    void beep(uint16_t freqHz, uint16_t durMs);

private:
    void _applyNote(const Note &n);
    void _silence();
    void _startIfNeeded();

    // LEDC
    uint8_t _pin = 5;
    uint8_t _channel = 0;
    uint8_t _timer = 0;
    uint8_t _resBits = 10;
    uint32_t _dutyMax = 1023; // (1<<_resBits)-1

    // Playback state
    std::vector<Note> _seq;
    size_t _idx = 0;
    bool _repeat = false;
    bool _playing = false;
    bool _paused = false;
    uint32_t _noteStartMs = 0;
    float _tempo = 1.0f; // 1.0 = original speed
    uint32_t _curNoteDurMs = 0;

    // Volume (duty)
    uint16_t _duty = 512; // ~50%
};
