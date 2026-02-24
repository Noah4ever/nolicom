#include "TriLeds.h"

void TriLeds::init(uint8_t pinG, uint8_t pinY, uint8_t pinR,
                   bool activeHigh, bool usePwm,
                   uint8_t chG, uint8_t chY, uint8_t chR,
                   uint8_t timer, uint8_t resBits)
{
    _pinG = pinG;
    _pinY = pinY;
    _pinR = pinR;
    _activeHigh = activeHigh;
    _usePwm = usePwm;
    _chG = chG;
    _chY = chY;
    _chR = chR;
    _timer = timer;
    _res = resBits;
    _dutyMax = (1u << _res) - 1u;

    pinMode(_pinG, OUTPUT);
    pinMode(_pinY, OUTPUT);
    pinMode(_pinR, OUTPUT);

    if (_usePwm)
    {
        ledcSetup(_chG, 1000, _res);
        ledcSetup(_chY, 1000, _res);
        ledcSetup(_chR, 1000, _res);
        ledcAttachPin(_pinG, _chG);
        ledcAttachPin(_pinY, _chY);
        ledcAttachPin(_pinR, _chR);
    }
    off();
}

void TriLeds::playLEDAnim(Anim a, uint16_t periodMs, uint16_t gMs, uint16_t yMs, uint16_t rMs)
{
    _anim = a;
    _period = periodMs;
    _gMs = gMs;
    _yMs = yMs;
    _rMs = rMs;
    _t0 = millis();
    _kittIdx = 0;
    _kittDir = 1;
    _trafIdx = 0;
    _phase = 0;
    _dPhase = 4;
    _blink = false;

    switch (a)
    {
    case Anim::Off:
        off();
        break;
    case Anim::SolidG:
        solid(true, false, false);
        break;
    case Anim::SolidY:
        solid(false, true, false);
        break;
    case Anim::SolidR:
        solid(false, false, true);
        break;
    case Anim::BlinkAll:
        if (_usePwm)
            _pwm(0, 0, 0);
        else
            _digital(false, false, false);
        break;
    case Anim::ChaseGYR:
        if (_usePwm)
            _pwm(255, 0, 0);
        else
            _digital(true, false, false);
        break;
    case Anim::Kitt:
        if (_usePwm)
            _pwm(255, 0, 0);
        else
            _digital(true, false, false);
        break;
    case Anim::Traffic:
    {                 // render immediately
        _trafIdx = 0; // start on Green
        if (_usePwm)
            _pwm(255, 0, 0);
        else
            _digital(true, false, false);
        break;
    }
    case Anim::PulseGreen:
        _pulseWhich = 0;
        if (_usePwm)
            _pwm(0, 0, 0);
        else
            _digital(false, false, false);
        break;
    case Anim::PulseYellow:
        _pulseWhich = 1;
        if (_usePwm)
            _pwm(0, 0, 0);
        else
            _digital(false, false, false);
        break;
    case Anim::PulseRed:
        _pulseWhich = 2;
        if (_usePwm)
            _pwm(0, 0, 0);
        else
            _digital(false, false, false);
        break;
    }
}

void TriLeds::solid(bool g, bool y, bool r)
{
    if (_usePwm)
        _pwm(g ? 255 : 0, y ? 255 : 0, r ? 255 : 0);
    else
        _digital(g, y, r);
}
void TriLeds::off()
{
    if (_usePwm)
        _pwm(0, 0, 0);
    else
        _digital(false, false, false);
    _anim = Anim::Off;
}

void TriLeds::update()
{
    uint32_t now = millis();
    switch (_anim)
    {
    case Anim::Off:
    case Anim::SolidG:
    case Anim::SolidY:
    case Anim::SolidR:
        return;
    case Anim::BlinkAll:
        _tickBlink(now);
        break;
    case Anim::ChaseGYR:
        _tickChase(now);
        break;
    case Anim::Kitt:
        _tickKitt(now);
        break; // smooth here
    case Anim::Traffic:
        _tickTraffic(now);
        break; // smooth here
    case Anim::PulseGreen:
        _tickPulse(now, 0);
        break;
    case Anim::PulseYellow:
        _tickPulse(now, 1);
        break;
    case Anim::PulseRed:
        _tickPulse(now, 2);
        break;
    }
}

// ---- IO ----
void TriLeds::_digital(bool g, bool y, bool r)
{
    auto w = [&](uint8_t pin, bool on)
    {
        digitalWrite(pin, (_activeHigh ? (on ? HIGH : LOW) : (on ? LOW : HIGH)));
    };
    w(_pinG, g);
    w(_pinY, y);
    w(_pinR, r);
}

void TriLeds::_pwm(uint8_t g, uint8_t y, uint8_t r)
{
    if (!_usePwm)
    {
        _digital(g > 0, y > 0, r > 0);
        return;
    }
    auto mapd = [&](uint8_t v)
    { return (uint16_t)v * _dutyMax / 255u; };
    uint16_t dg = mapd(g), dy = mapd(y), dr = mapd(r);
    if (_activeHigh)
    {
        ledcWrite(_chG, dg);
        ledcWrite(_chY, dy);
        ledcWrite(_chR, dr);
    }
    else
    {
        ledcWrite(_chG, _dutyMax - dg);
        ledcWrite(_chY, _dutyMax - dy);
        ledcWrite(_chR, _dutyMax - dr);
    }
}

// ---- Animations ----
void TriLeds::_tickBlink(uint32_t now)
{
    if (now - _t0 >= _period)
    {
        _t0 = now;
        _blink = !_blink;
        if (_usePwm)
            _pwm(_blink ? 255 : 0, _blink ? 255 : 0, _blink ? 255 : 0);
        else
            _digital(_blink, _blink, _blink);
    }
}

void TriLeds::_tickChase(uint32_t now)
{
    if (now - _t0 >= _period)
    {
        _t0 = now;
        uint8_t idx = ((now / _period) % 3);
        if (_usePwm)
            _pwm(idx == 0 ? 255 : 0, idx == 1 ? 255 : 0, idx == 2 ? 255 : 0);
        else
            _digital(idx == 0, idx == 1, idx == 2);
    }
}

void TriLeds::_tickKitt(uint32_t now)
{
    if (!_usePwm)
    { // fallback to step if no PWM
        if (now - _t0 >= _kittStep)
        {
            _t0 = now;
            _kittIdx += _kittDir;
            if (_kittIdx >= 2)
            {
                _kittIdx = 2;
                _kittDir = -1;
            }
            else if (_kittIdx == 0)
            {
                _kittDir = 1;
            }
            _digital(_kittIdx == 0, _kittIdx == 1, _kittIdx == 2);
        }
        return;
    }

    // Smooth within-step blend: brightness moves from current to next over _kittStep
    uint32_t t = now - _t0;
    if (t >= _kittStep)
    {
        // advance hop
        _t0 = now - (t % _kittStep);
        _kittIdx += _kittDir;
        if (_kittIdx >= 2)
        {
            _kittIdx = 2;
            _kittDir = -1;
        }
        else if (_kittIdx == 0)
        {
            _kittDir = 1;
        }
        t = now - _t0;
    }

    float f = (float)t / (float)_kittStep; // 0..1
    uint8_t nextIdx = _kittIdx + _kittDir;
    if (_kittDir < 0 && _kittIdx == 2)
        nextIdx = 1;
    if (_kittDir > 0 && _kittIdx == 0)
        nextIdx = 1;

    uint8_t bCur = _mix255(1.0f - f);
    uint8_t bNext = _mix255(f);

    uint8_t g = 0, y = 0, r = 0;
    if (_kittIdx == 0)
        g = bCur;
    if (_kittIdx == 1)
        y = bCur;
    if (_kittIdx == 2)
        r = bCur;
    if (nextIdx == 0)
        g = bNext;
    if (nextIdx == 1)
        y = bNext;
    if (nextIdx == 2)
        r = bNext;

    _pwm(g, y, r);
}

void TriLeds::_tickTraffic(uint32_t now)
{
    uint16_t phaseDur = (_trafIdx == 0) ? _gMs : (_trafIdx == 1) ? _yMs
                                                                 : _rMs;
    uint32_t elapsed = now - _t0;

    // If we fell behind (long loop), advance across multiple phases
    while (elapsed >= phaseDur)
    {
        elapsed -= phaseDur;
        _t0 = now - elapsed;           // keep elapsed within current phase
        _trafIdx = (_trafIdx + 1) % 3; // next phase
        phaseDur = (_trafIdx == 0) ? _gMs : (_trafIdx == 1) ? _yMs
                                                            : _rMs;
    }

    if (!_usePwm || _trafficXfade == 0)
    {
        // Step mode: set solid LED for current phase
        _digital(_trafIdx == 0, _trafIdx == 1, _trafIdx == 2);
        return;
    }

    // Smooth cross-fade in the last _trafficXfade ms of the phase
    uint32_t xfadeStart = (phaseDur > _trafficXfade) ? (phaseDur - _trafficXfade) : 0;
    bool inBlend = (elapsed >= xfadeStart);
    float f = inBlend ? (float)(elapsed - xfadeStart) / (float)_trafficXfade : 0.0f;
    if (f < 0)
        f = 0;
    if (f > 1)
        f = 1;

    auto mix255 = [](float v) -> uint8_t
    {
        if (v <= 0)
            return 0;
        if (v >= 1)
            return 255;
        return (uint8_t)(v * 255.0f + 0.5f);
    };

    uint8_t curB = inBlend ? mix255(1.0f - f) : 255;
    uint8_t nxtB = inBlend ? mix255(f) : 0;
    uint8_t next = (uint8_t)((_trafIdx + 1) % 3);

    uint8_t g = 0, y = 0, r = 0;
    // current phase at full/decaying brightness
    if (_trafIdx == 0)
        g = curB;
    if (_trafIdx == 1)
        y = curB;
    if (_trafIdx == 2)
        r = curB;
    // next phase rising
    if (next == 0)
        g = max(g, nxtB);
    if (next == 1)
        y = max(y, nxtB);
    if (next == 2)
        r = max(r, nxtB);

    _pwm(g, y, r); // always drive outputs every update
}

void TriLeds::_tickPulse(uint32_t now, uint8_t which)
{
    if (now - _t0 >= _period)
    {
        _t0 = now;
        int v = (int)_phase + _dPhase;
        if (v >= 255)
        {
            v = 255;
            _dPhase = -_dPhase;
        }
        if (v <= 0)
        {
            v = 0;
            _dPhase = -_dPhase;
        }
        _phase = (uint8_t)v;

        uint8_t g = (which == 0) ? _phase : 0;
        uint8_t y = (which == 1) ? _phase : 0;
        uint8_t r = (which == 2) ? _phase : 0;
        _pwm(g, y, r);
    }
}
