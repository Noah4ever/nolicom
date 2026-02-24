#pragma once
#include <Arduino.h>

/*
  TriLeds - 3 LED animator (G,Y,R) with smooth KITT & Traffic fades
  -----------------------------------------------------------------
  - Non-blocking (millis)
  - Built-ins: Off, Solid*, BlinkAll, ChaseGYR, Kitt (smooth), Traffic (smooth end-fade), Pulse*
  - Requires usePwm=true for smooth fades

  Quick start:
    TriLeds leds;
*/

class TriLeds
{
public:
    enum class Anim : uint8_t
    {
        Off,
        SolidG,
        SolidY,
        SolidR,
        BlinkAll, // toggles all LEDs (uses _period)
        ChaseGYR, // step G->Y->R (uses _period), no smooth blend
        Kitt,     // smooth blend G<->Y<->R (uses _kittStep)
        Traffic,  // G(_gMs)→Y(_yMs)→R(_rMs) with smooth end-fade (_trafficXfade)
        PulseGreen,
        PulseYellow,
        PulseRed
    };

    void init(uint8_t pinG, uint8_t pinY, uint8_t pinR,
              bool activeHigh = true, bool usePwm = false,
              uint8_t chG = 1, uint8_t chY = 2, uint8_t chR = 3,
              uint8_t timer = 1, uint8_t resBits = 8);

    // Animation control
    void playLEDAnim(Anim a, uint16_t periodMs = 150,
                     uint16_t gMs = 1500, uint16_t yMs = 400, uint16_t rMs = 1500);

    // Fade controls
    void setKittStep(uint16_t ms) { _kittStep = ms; }             // per-hop duration (smooth)
    void setTrafficCrossfade(uint16_t ms) { _trafficXfade = ms; } // blend window at phase end

    // Simple control
    void solid(bool g, bool y, bool r);
    void off();

    // Update
    void update();

private:
    // IO
    void _digital(bool g, bool y, bool r);
    void _pwm(uint8_t g, uint8_t y, uint8_t r);

    // Anim ticks
    void _tickBlink(uint32_t now);
    void _tickChase(uint32_t now);
    void _tickKitt(uint32_t now);    // smooth within-step blend
    void _tickTraffic(uint32_t now); // smooth phase-end cross-fade
    void _tickPulse(uint32_t now, uint8_t which);

    // Helpers
    static inline uint8_t _mix255(float v)
    {
        if (v <= 0)
            return 0;
        if (v >= 1)
            return 255;
        return (uint8_t)(v * 255.0f + 0.5f);
    }

    // Config
    uint8_t _pinG = 0, _pinY = 0, _pinR = 0;
    bool _activeHigh = true, _usePwm = false;
    uint8_t _chG = 1, _chY = 2, _chR = 3, _timer = 1, _res = 8;
    uint16_t _dutyMax = 255;

    // State
    Anim _anim = Anim::Off;
    uint32_t _t0 = 0;

    // Generic timings
    uint16_t _period = 150;                        // Blink/Chase
    uint16_t _gMs = 1500, _yMs = 400, _rMs = 1500; // Traffic

    // KITT smooth
    uint16_t _kittStep = 120; // ms per hop (smooth blend)
    uint8_t _kittIdx = 0;     // 0=G,1=Y,2=R
    int8_t _kittDir = 1;      // bounce direction

    // Traffic cross-fade
    uint8_t _trafIdx = 0;       // 0=G,1=Y,2=R
    uint16_t _trafficXfade = 0; // ms blend at end of phase

    // Pulse
    uint8_t _pulseWhich = 0, _phase = 0;
    int8_t _dPhase = 4;

    // Blink
    bool _blink = false;
};
