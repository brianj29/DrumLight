#include "FastLED.h" // For CRGB/CHSV functions

const int redPin = 0;
const int greenPin = 1;
const int bluePin = 4;

const uint8_t PROGMEM gamma8[] = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
    1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
    2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
    5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
   10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
   17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
   25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
   37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
   51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
   69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
   90, 92, 93, 95, 96, 98, 99,101,102,104,105,107,109,110,112,114,
  115,117,119,120,122,124,126,127,129,131,133,135,137,138,140,142,
  144,146,148,150,152,154,156,158,160,162,164,167,169,171,173,175,
  177,180,182,184,186,189,191,193,196,198,200,203,205,208,210,213,
  215,218,220,223,225,228,231,233,236,239,241,244,247,249,252,255 };

#if 1 // use gamma correction
void WriteLed(const CRGB& Led) {
  analogWrite(redPin,   255 - pgm_read_byte(&gamma8[Led.r]));
  analogWrite(greenPin, 255 - pgm_read_byte(&gamma8[Led.g]));
  analogWrite(bluePin,  255 - pgm_read_byte(&gamma8[Led.b]));
}

#elif 0 // direct port access... needs fixing
void WriteLed(const CRGB& Led) {
  volatile uint8_t* Port[] = {&OCR0A, &OCR0B, &OCR1B};
  *Port[0] = 255 - pgm_read_byte(&gamma8[Led.r]);
  *Port[1] = 255 - pgm_read_byte(&gamma8[Led.g]);
  *Port[2] = 255 - pgm_read_byte(&gamma8[Led.b]);
}

#else
void WriteLed(const CRGB& Led) {
  analogWrite(redPin,   255 - Led.r);
  analogWrite(greenPin, 255 - Led.g);
  analogWrite(bluePin,  255 - Led.b);
}
#endif

void setup() {
#if 1
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
#else
  // Configure counter/timer0 for fast PWM on PB0 and PB1
  TCCR0A = 3<<COM0A0 | 3<<COM0B0 | 3<<WGM00;
  TCCR0B = 0<<WGM02 | 3<<CS00; // Optional; already set
  // Configure counter/timer1 for fast PWM on PB4
  GTCCR = 1<<PWM1B | 3<<COM1B0;
  TCCR1 = 3<<COM1A0 | 7<<CS10;
#endif
  WriteLed(CRGB::Black);
}

uint8_t Hues[] = {HUE_RED, HUE_GREEN, HUE_BLUE};

void loop() {
  CRGB Led;
  uint8_t Idx;
  uint8_t Idx2;

  Idx = 0;
  while (1) {
#if 0
    int d = 500;
    WriteLed(CRGB::Red);
    delay(d);
    WriteLed(CRGB::Black);
    delay(d);
    WriteLed(CRGB::Green);
    delay(d);
    WriteLed(CRGB::Black);
    delay(d);
    WriteLed(CRGB::Blue);
    delay(d);
    WriteLed(CRGB::Black);
    delay(d);
#elif 0
    analogWrite(redPin,   255 - (Idx & 3)); // Test 4 lowest values
    delay(500);
    Idx++; // 8-bit wraparound
#elif 1
    Led = CHSV(Idx, 255, 255); // max out saturation, brightness
    WriteLed(Led);
    Idx++; // 8-bit wraparound
    delay(12);
#elif 0
    Led = CHSV(HUE_GREEN, Idx, 255); // vary saturation
    WriteLed(Led);
    Idx++; // 8-bit wraparound
    delay(8);
#elif 0
    for (Idx2 = 0; Idx2 < 255; Idx2++) {
      Led = CHSV(Hues[Idx], 255, Idx2); // vary value (brightness)
      WriteLed(Led);
      delay(8);
    }
    Idx = (Idx + 1) % (sizeof(Hues) / sizeof(Hues[0]));
#elif 0
    Led = ColorFromPalette(HeatColors_p, 240 - Idx, 255, LINEARBLEND);
    WriteLed(Led);
    Idx = (Idx + 1) % 241; // See note in colorpallets.cpp.  Use only 0-240
    delay(8);
#else
    Led = HeatColor(Idx);
    WriteLed(Led);
    Idx--; // 8-bit wraparound
    delay(5);
#endif
  }
}

// See the blend() function in colorutils.h to interpolate between 2 colors
