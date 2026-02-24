#include "Buzzer.h"

static const Note MEL_SCALE_UP[] = {
    {262, 200},
    {294, 200},
    {330, 200},
    {349, 200},
    {392, 200},
    {440, 200},
    {494, 200},
    {523, 400},
};
static const Note MEL_SCALE_DOWN[] = {
    {523, 200},
    {494, 200},
    {440, 200},
    {392, 200},
    {349, 200},
    {330, 200},
    {294, 200},
    {262, 400},
};
static const Note MEL_TWINKLE[] = {

    {262, 300},
    {262, 300},
    {392, 300},
    {392, 300},
    {440, 300},
    {440, 300},
    {392, 600},
    {349, 300},
    {349, 300},
    {330, 300},
    {330, 300},
    {294, 300},
    {294, 300},
    {262, 600},
};

static const Note MEL_BEEP_BEEP[] = {
    {0, 80},
    {780, 120},
    {0, 80},
    {900, 200},
    {0, 80},
};

static const Note BOOT[] = {
    // C5–E5–G5 burst
    {523, 60},
    {0, 20},
    {659, 60},
    {0, 20},
    {784, 60},
    {0, 60},
    // Hit C6, then fall back
    {1046, 140},
    {0, 40},
    {784, 60},
    {659, 60},
    {523, 120},
    {0, 100},
    // Little gliss up to B4 then land on C5
    {392, 40},
    {440, 40},
    {466, 40},
    {494, 90},
    {0, 60},
    {523, 240},
};

static void copyMelody(std::vector<Note> &dst, const Note *src, size_t n)
{
    dst.clear();
    dst.reserve(n);
    for (size_t i = 0; i < n; i++)
        dst.push_back(src[i]);
}

void Buzzer::init(uint8_t pin, uint8_t channel, uint8_t timer, uint8_t resolutionBits)
{
    _pin = pin;
    _channel = channel;
    _timer = timer;
    _resBits = resolutionBits;
    _dutyMax = (1u << _resBits) - 1u;

    ledcSetup(_channel, 1000 /*Hz placeholder*/, _resBits);
    ledcAttachPin(_pin, _channel);

    setVolume(50); // 50%
    stop();
}

void Buzzer::setVolume(uint8_t pct)
{
    if (pct > 100)
        pct = 100;
    // Avoid 0 duty when a tone should sound (mute uses _silence())
    _duty = (uint16_t)((_dutyMax * (uint32_t)pct) / 100u);
    if (_duty == 0 && pct > 0)
        _duty = 1;
}

void Buzzer::setTempoFactor(float factor)
{
    if (factor < 0.2f)
        factor = 0.2f;
    if (factor > 5.0f)
        factor = 5.0f;
    _tempo = factor;
}

void Buzzer::play(BuiltInMelody m, bool repeat)
{
    switch (m)
    {
    case BuiltInMelody::SCALE_UP:
        copyMelody(_seq, MEL_SCALE_UP, sizeof(MEL_SCALE_UP) / sizeof(Note));
        break;
    case BuiltInMelody::SCALE_DOWN:
        copyMelody(_seq, MEL_SCALE_DOWN, sizeof(MEL_SCALE_DOWN) / sizeof(Note));
        break;
    case BuiltInMelody::TWINKLE:
        copyMelody(_seq, MEL_TWINKLE, sizeof(MEL_TWINKLE) / sizeof(Note));
        break;
    case BuiltInMelody::BEEP_BEEP:
        copyMelody(_seq, MEL_BEEP_BEEP, sizeof(MEL_BEEP_BEEP) / sizeof(Note));
        break;
    case BuiltInMelody::BOOT:
        copyMelody(_seq, BOOT, sizeof(BOOT) / sizeof(Note));
        break;
    }
    _repeat = repeat;
    _idx = 0;
    _paused = false;
    _playing = true;
    _startIfNeeded();
}

void Buzzer::play(const std::vector<Note> &seq, bool repeat)
{
    _seq = seq; // copy
    _repeat = repeat;
    _idx = 0;
    _paused = false;
    _playing = true;
    _startIfNeeded();
}

void Buzzer::beep(uint16_t freqHz, uint16_t durMs)
{
    _seq = {{freqHz, durMs}};
    _repeat = false;
    _idx = 0;
    _paused = false;
    _playing = true;
    _startIfNeeded();
}

void Buzzer::stop()
{
    _playing = false;
    _paused = false;
    _idx = 0;
    _seq.clear();
    _silence();
}

void Buzzer::pause()
{
    if (_playing)
    {
        _paused = true;
        _silence();
    }
}

void Buzzer::resume()
{
    if (_playing && _paused)
    {
        _paused = false;
        _noteStartMs = millis(); // restart current note timing
        _startIfNeeded();
    }
}

void Buzzer::_startIfNeeded()
{
    if (!_playing || _paused || _seq.empty() || _idx >= _seq.size())
    {
        _silence();
        return;
    }
    const Note &n = _seq[_idx];
    _applyNote(n);
    _noteStartMs = millis();
    _curNoteDurMs = (uint32_t)(n.durMs * _tempo);
    if (_curNoteDurMs < 5)
        _curNoteDurMs = 5;
}

void Buzzer::update()
{
    if (!_playing || _paused || _seq.empty())
        return;

    uint32_t now = millis();
    if (now - _noteStartMs >= _curNoteDurMs)
    {
        // advance to next note
        _idx++;
        if (_idx >= _seq.size())
        {
            if (_repeat)
            {
                _idx = 0;
            }
            else
            {
                stop();
                return;
            }
        }
        _startIfNeeded();
    }
}

void Buzzer::_applyNote(const Note &n)
{
    if (n.freq == 0)
    {
        _silence();
        return;
    }
    // Set frequency and duty
    ledcWriteTone(_channel, (double)n.freq);
    ledcWrite(_channel, _duty);
}

void Buzzer::_silence()
{
    // Either set duty to 0 or detach tone
    ledcWrite(_channel, 0);
}
