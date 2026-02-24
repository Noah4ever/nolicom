#include "SevenSegmentDisplay.h"
#include <string.h>

void SevenSegmentDisplay::init(uint8_t PIN_DATA, uint8_t PIN_CLOCK, uint8_t PIN_LATCH,
                               uint8_t PIN_DIG1, uint8_t PIN_DIG2)
{
    _PIN_DATA = PIN_DATA;
    _PIN_CLOCK = PIN_CLOCK;
    _PIN_LATCH = PIN_LATCH;
    _PIN_DIG1 = PIN_DIG1;
    _PIN_DIG2 = PIN_DIG2;

    pinMode(_PIN_DATA, OUTPUT);
    pinMode(_PIN_CLOCK, OUTPUT);
    pinMode(_PIN_LATCH, OUTPUT);
    pinMode(_PIN_DIG1, OUTPUT);
    pinMode(_PIN_DIG2, OUTPUT);

    // Known idle states to avoid glitches
    digitalWrite(_PIN_DATA, LOW);
    digitalWrite(_PIN_CLOCK, LOW);
    digitalWrite(_PIN_LATCH, LOW);
    _digitOff(_PIN_DIG1);
    _digitOff(_PIN_DIG2);

    clearDisplay();
}

void SevenSegmentDisplay::setString(const char *s)
{
    if (!s || s[0] == '\0')
    {
        setPair(' ', ' ');
        return;
    }
    char l = s[0];
    char r = (s[1] != '\0') ? s[1] : ' ';
    setPair(l, r);
}

void SevenSegmentDisplay::refresh()
{
    // Decide which characters to show
    char lChar = _left;
    char rChar = _right;

    if (_blinkActive)
    {
        // When blinking is active, show the buffer’s 2 chars (or spaces),
        // unless we are in the "off" phase where both are blanked.
        if (_blinkVisible)
        {
            lChar = (_blinkBuffer.length() >= 1) ? _blinkBuffer[0] : ' ';
            rChar = (_blinkBuffer.length() >= 2) ? _blinkBuffer[1] : ' ';
        }
        else
        {
            lChar = ' ';
            rChar = ' ';
        }
    }

    // LEFT digit slice
    _digitOff(_PIN_DIG1);
    _digitOff(_PIN_DIG2);
    uint8_t rawL = buildRawFromLogical(getCharSegments(lChar));
    shift595(rawL);
    delayMicroseconds(2);
    _digitOn(_PIN_DIG1);
    delayMicroseconds(_onMicros);
    _digitOff(_PIN_DIG1);

    // RIGHT digit slice
    uint8_t rawR = buildRawFromLogical(getCharSegments(rChar));
    shift595(rawR);
    delayMicroseconds(2);
    _digitOn(_PIN_DIG2);
    delayMicroseconds(_onMicros);
    _digitOff(_PIN_DIG2);
}

void SevenSegmentDisplay::clearDisplay()
{
    _digitOff(_PIN_DIG1);
    _digitOff(_PIN_DIG2);
    // All segments off according to polarity
    shift595(_segmentsActiveLow ? 0xFF : 0x00);
}

void SevenSegmentDisplay::setBlinkingText(const char *s, uint16_t periodMs)
{
    // Accept nullptr -> stop
    if (!s || s[0] == '\0')
    {
        stopBlinking();
        return;
    }

    // store up to two chars; anything longer is ignored for blink mode
    _blinkBuffer = s;
    if (_blinkBuffer.length() > 2)
    {
        _blinkBuffer.remove(2); // keep only first two
    }

    // timebase: full on+off cycle
    _blinkPeriodMs = (periodMs == 0) ? 1 : periodMs;
    _blinkActive = true;
    _blinkVisible = true; // start visible
    _lastBlinkToggle = millis();
}

void SevenSegmentDisplay::stopBlinking()
{
    _blinkActive = false;
    _blinkVisible = true;
    _blinkBuffer = "";
    // no further action needed; refresh() will use normal _left/_right
}

void SevenSegmentDisplay::updateBlinking()
{
    if (!_blinkActive)
        return;

    uint32_t now = millis();
    // toggle visibility every half period (on half, off half)
    uint32_t halfPeriod = _blinkPeriodMs / 2;
    if (halfPeriod == 0)
        halfPeriod = 1;

    if (now - _lastBlinkToggle >= halfPeriod)
    {
        _lastBlinkToggle = now;
        _blinkVisible = !_blinkVisible;
    }
}

uint8_t SevenSegmentDisplay::getCharSegments(char c)
{
    switch (c)
    {
    // digits
    case '0':
        return (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F);
    case '1':
        return (SEG_B | SEG_C);
    case '2':
        return (SEG_A | SEG_B | SEG_D | SEG_E | SEG_G);
    case '3':
        return (SEG_A | SEG_B | SEG_C | SEG_D | SEG_G);
    case '4':
        return (SEG_F | SEG_G | SEG_B | SEG_C);
    case '5':
        return (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);
    case '6':
        return (SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);
    case '7':
        return (SEG_A | SEG_B | SEG_C);
    case '8':
        return (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G);
    case '9':
        return (SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G);

    // uppercase defaults
    case 'A':
        return (SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G);
    case 'B':
        return (SEG_C | SEG_D | SEG_E | SEG_F | SEG_G);
    case 'C':
        return (SEG_A | SEG_F | SEG_E | SEG_D);
    case 'D':
        return (SEG_B | SEG_C | SEG_D | SEG_E | SEG_G);
    case 'E':
        return (SEG_A | SEG_F | SEG_E | SEG_D | SEG_G);
    case 'F':
        return (SEG_A | SEG_F | SEG_E | SEG_G);
    case 'G':
        return (SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);
    case 'H':
        return (SEG_B | SEG_C | SEG_E | SEG_F | SEG_G);
    case 'I':
        return (SEG_B | SEG_C);
    case 'J':
        return (SEG_B | SEG_C | SEG_D | SEG_E);
    case 'L':
        return (SEG_F | SEG_E | SEG_D);
    case 'N':
        return (SEG_C | SEG_E | SEG_G);
    case 'O':
        return (SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F);
    case 'P':
        return (SEG_A | SEG_B | SEG_E | SEG_F | SEG_G);
    case 'Q':
        return (SEG_A | SEG_B | SEG_C | SEG_F | SEG_G);
    case 'R':
        return (SEG_E | SEG_G);
    case 'S':
        return (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);
    case 'T':
        return (SEG_F | SEG_E | SEG_D | SEG_G);
    case 'U':
        return (SEG_B | SEG_C | SEG_D | SEG_E | SEG_F);
    case 'V':
        return (SEG_C | SEG_D | SEG_E);
    case 'Y':
        return (SEG_B | SEG_C | SEG_D | SEG_F | SEG_G);
    case 'Z':
        return (SEG_A | SEG_B | SEG_D | SEG_E | SEG_G);

    // requested lowercase forms
    case 'b':
        return (SEG_C | SEG_D | SEG_E | SEG_F | SEG_G);
    case 'c':
        return (SEG_D | SEG_E | SEG_G);
    case 'd':
        return (SEG_B | SEG_C | SEG_D | SEG_E | SEG_G);
    case 'h':
        return (SEG_C | SEG_E | SEG_F | SEG_G);
    case 'o':
        return (SEG_C | SEG_D | SEG_E | SEG_G);
    case 'u':
        return (SEG_C | SEG_D | SEG_E);

    // aliases: accept lowercase by mapping to uppercase
    case 'a':
        return (SEG_A | SEG_B | SEG_C | SEG_E | SEG_F | SEG_G);
    case 'e':
        return (SEG_A | SEG_F | SEG_E | SEG_D | SEG_G);
    case 'f':
        return (SEG_A | SEG_F | SEG_E | SEG_G);
    case 'g':
        return (SEG_A | SEG_F | SEG_E | SEG_D | SEG_C | SEG_G);
    case 'i':
        return (SEG_B | SEG_C);
    case 'j':
        return (SEG_B | SEG_C | SEG_D | SEG_E);
    case 'l':
        return (SEG_F | SEG_E | SEG_D);
    case 'n':
        return (SEG_C | SEG_E | SEG_G);
    case 'p':
        return (SEG_A | SEG_B | SEG_E | SEG_F | SEG_G);
    case 'q':
        return (SEG_A | SEG_B | SEG_C | SEG_F | SEG_G);
    case 'r':
        return (SEG_E | SEG_G);
    case 's':
        return (SEG_A | SEG_F | SEG_G | SEG_C | SEG_D);
    case 't':
        return (SEG_F | SEG_E | SEG_D | SEG_G);
    case 'v':
        return (SEG_C | SEG_D | SEG_E);
    case 'y':
        return (SEG_B | SEG_C | SEG_D | SEG_F | SEG_G);
    case 'z':
        return (SEG_A | SEG_B | SEG_D | SEG_E | SEG_G);

    case '!':
        return (SEG_B | SEG_DP);
    case '-':
        return SEG_G;
    case '_':
        return SEG_D;
    case '.':
        return SEG_DP;
    case ' ':
        return 0;

    default:
        return 0;
    }
}

uint8_t SevenSegmentDisplay::buildRawFromLogical(uint8_t logicalMask)
{
    // Convert logical segment ON bits to raw 595 byte based on polarity & mapping
    uint8_t raw = _segmentsActiveLow ? 0xFF : 0x00;
    for (int seg = 0; seg < 8; ++seg)
    {
        bool on = (logicalMask & (1 << seg));
        uint8_t bit = (1 << _QOF_SEG[seg]);
        if (_segmentsActiveLow)
        {
            if (on)
                raw &= ~bit; // drive LOW to turn on
        }
        else
        {
            if (on)
                raw |= bit; // drive HIGH to turn on
        }
    }
    return raw;
}

void SevenSegmentDisplay::shift595(uint8_t data)
{
    digitalWrite(_PIN_LATCH, LOW);
    shiftOut(_PIN_DATA, _PIN_CLOCK, MSBFIRST, data);
    digitalWrite(_PIN_LATCH, HIGH);
}

void SevenSegmentDisplay::setScrollingString(const char *s, uint16_t intervalMs)
{
    if (!s)
    {
        _scrollBuffer = "";
        _scrollingActive = false;
        return;
    }
    _scrollBuffer = s;
    _scrollInterval = intervalMs;
    _lastScroll = millis();
    _scrollIndex = 0;
    _scrollingActive = (_scrollBuffer.length() > 2);
    if (!_scrollingActive)
    {
        // if only 1–2 chars, just set directly
        setString(_scrollBuffer.c_str());
    }
}

void SevenSegmentDisplay::updateScrolling()
{
    if (!_scrollingActive)
        return;

    uint32_t now = millis();
    if (now - _lastScroll >= _scrollInterval)
    {
        _lastScroll = now;
        if (_scrollBuffer.length() == 0)
        {
            setPair(' ', ' ');
            return;
        }

        // Wrap around once finished
        if (_scrollIndex > (int)_scrollBuffer.length())
        {
            _scrollIndex = 0;
        }

        char left = (_scrollIndex < (int)_scrollBuffer.length()) ? _scrollBuffer[_scrollIndex] : ' ';
        char right = (_scrollIndex + 1 < (int)_scrollBuffer.length()) ? _scrollBuffer[_scrollIndex + 1] : ' ';
        setPair(left, right);

        _scrollIndex++;
    }
}
