#pragma once

#include <Arduino.h>

/*
 =============================================================================
  SevenSegmentDisplay Class
  -----------------------------------------------------------------------------
  Purpose:
    Control a 2-digit 7-segment display using a 74HC595 shift register.
    Supports static display, multiplexing refresh, and scrolling strings.

    Segment labels
          a
        -----
     f |     | b
       |  g  |
        -----
       |     |
     e |     | c
        -----
          d

  Wiring:
    - 74HC595 SER (DS)   -> PIN_DATA
    - 74HC595 SRCLK      -> PIN_CLOCK
    - 74HC595 RCLK       -> PIN_LATCH
    - 74HC595 Q outputs  -> Segments A..DP (order defined by QOF_SEG mapping)
    - Digit enable pins  -> Transistors driving left/right digit commons

  Typical common-anode wiring:
    - 74HC595 sinks current through segment resistors
    - Digit common pins go HIGH to enable
    - -> use setDigitActiveHigh(true), setSegmentsActiveLow(true)

  Typical common-cathode wiring:
    - 74HC595 sources current through segment resistors
    - Digit common pins go LOW to enable
    - -> use setDigitActiveHigh(false), setSegmentsActiveLow(false)

  Usage:
    1. Create a display instance:
         SevenSegmentDisplay disp;

    2. Initialize in setup():
         disp.init(PIN_DATA, PIN_CLOCK, PIN_LATCH, PIN_DIG1, PIN_DIG2);
         disp.setDigitActiveHigh(true);    // adjust for your hardware
         disp.setSegmentsActiveLow(true);  // adjust for your hardware

    3. Choose what to show:
         disp.setPair('H','I');            // show two characters
         disp.setString("Hi");             // first two characters of string
         disp.setScrollingString("HELLO WORLD", 400); // scroll text at 400ms

    4. In loop(), call:
         disp.refresh();         // multiplex refresh, must be called very often
         disp.updateScrolling(); // advances scrolling if enabled

    5. Optional configuration:
         disp.setBrightnessMicros(1000); // adjust per-digit ON time
         disp.setSegmentMapping(map);    // override QOF_SEG mapping if needed

  Notes:
    - refresh() must run frequently; do not use long blocking delays in loop().
    - updateScrolling() uses millis() timing for non-blocking scrolling.
    - getCharSegments() includes digits, many letters, and some lowercase forms
      (b, c, d, h, o, u) with custom shapes.
 =============================================================================
*/

#define SEG_A (1 << 0)
#define SEG_B (1 << 1)
#define SEG_C (1 << 2)
#define SEG_D (1 << 3)
#define SEG_E (1 << 4)
#define SEG_F (1 << 5)
#define SEG_G (1 << 6)
#define SEG_DP (1 << 7)

class SevenSegmentDisplay
{
public:
  enum class SevenSegmentPosition
  {
    LEFT = 0,
    RIGHT
  };

  enum class SevenSegmentChar
  {
    H = (SEG_B | SEG_C | SEG_E | SEG_F | SEG_G),
    I = (SEG_B | SEG_C)
  };

  SevenSegmentDisplay()
      : _PIN_DATA(0), _PIN_CLOCK(0), _PIN_LATCH(0), _PIN_DIG1(0), _PIN_DIG2(0),
        _digitActiveHigh(true), _segmentsActiveLow(true), _onMicros(1000),
        _left(' '), _right(' ')
  {
  }

  void init(uint8_t PIN_DATA, uint8_t PIN_CLOCK, uint8_t PIN_LATCH, uint8_t PIN_DIG1, uint8_t PIN_DIG2);

  // Note: these perform one multiplex "slice"; call repeatedly (e.g., in loop()).
  void printChar(char c, SevenSegmentPosition position);
  void printChars(char l, char r);
  void printString(const char *str);

  // Store desired characters; call refresh() rapidly in loop() for a stable display.
  void setPair(char left, char right)
  {
    _left = left;
    _right = right;
  }
  void setString(const char *s); // uses first two chars of s, pads with space
  void refresh();                // paints LEFT then RIGHT each call
  void clearDisplay();

  void setDigitActiveHigh(bool activeHigh) { _digitActiveHigh = activeHigh; }
  void setSegmentsActiveLow(bool activeLow) { _segmentsActiveLow = activeLow; }
  void setBrightnessMicros(uint16_t onMicros) { _onMicros = onMicros; } // per-digit ON time (μs)
  void setSegmentMapping(const uint8_t map[8])
  {
    for (int i = 0; i < 8; i++)
      _QOF_SEG[i] = map[i];
  }

  uint8_t getCharSegments(char c);

  void setScrollingString(const char *s, uint16_t intervalMs = 400);
  void updateScrolling(); // call this in loop() along with refresh()

  void setBlinkingText(const char *s, uint16_t periodMs); // period of full on+off cycle
  void stopBlinking();
  void updateBlinking();

private:
  uint8_t buildRawFromLogical(uint8_t logicalMask);
  void shift595(uint8_t data);

  inline void _digitOn(uint8_t pin) { digitalWrite(pin, _digitActiveHigh ? HIGH : LOW); }
  inline void _digitOff(uint8_t pin) { digitalWrite(pin, _digitActiveHigh ? LOW : HIGH); }

  String _scrollBuffer;
  uint16_t _scrollInterval;
  uint32_t _lastScroll;
  int _scrollIndex;
  bool _scrollingActive;

  bool _blinkActive = false;
  bool _blinkVisible = true;
  uint16_t _blinkPeriodMs = 500; // full cycle (on+off) in ms
  uint32_t _lastBlinkToggle = 0;
  String _blinkBuffer; // up to two chars used when blinking

  uint8_t _PIN_DATA;
  uint8_t _PIN_CLOCK;
  uint8_t _PIN_LATCH;
  uint8_t _PIN_DIG1;
  uint8_t _PIN_DIG2;

  bool _digitActiveHigh;   // HIGH enables a digit (typical for common-anode)
  bool _segmentsActiveLow; // LOW lights a segment (typical when 595 sinks current)
  uint16_t _onMicros;      // per-digit ON time in microseconds (brightness)

  // Current content for refresh()
  volatile char _left;
  volatile char _right;

  // Maps segments A..DP (index 0..7) to 74HC595 bit positions Q0..Q7
  //   a->Q5, b->Q6, c->Q2, d->Q1, e->Q0, f->Q7, g->Q3, dp->Q4
  uint8_t _QOF_SEG[8] = {
      5, // a -> Q5
      6, // b -> Q6
      2, // c -> Q2
      1, // d -> Q1
      0, // e -> Q0
      7, // f -> Q7
      3, // g -> Q3
      4  // dp -> Q4
  };
};
