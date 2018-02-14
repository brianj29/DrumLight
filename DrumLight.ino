/*
 * Drum light
 * by Brian J. Johnson  January, 2018
 *
 * Circuit:
 *  ATTiny85, powered by 3V to 5V (I used 2xAAA batteries)
 *  Common-anode RGB LED on redPin, greenPin, bluePin w/appropriate resistors
 *  Piezo disc between knockPin and ground, with a 10k pulldown
 *  Pushbutton between buttonPin and ground
 *
 * Tap the piezo to flash the LED!  Push the button to select the next
 * LED flash pattern from the list.  The current selection is stored
 * in NVRAM.
 *
 * The pulldown on knockPin forms a voltage divider with the internal
 * pullup (about 40k.)  That produces a bias voltage of about 0.2 x
 * Vcc, which is enough to get the voltage produced by the piezo up
 * into the range which digitalRead() can sense, while still reading
 * as 0 in the normal case.  Adjust the resistance to vary the
 * sensitivity.  Having a path to ground also helps limit the high
 * voltage peaks which piezos can produce.
 *
 * #defining CALIBRATE puts the sketch in a mode where it colors the
 * LED based on the voltage ranges for digital pins defined on the
 * datasheet.  A "low" input should be less than 0.3 x Vcc, and an
 * "high" input should be greater than 0.6 x Vcc.
 */

#include "FastLED.h" // For CRGB/CHSV functions
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/power.h>

// Pin definitions

const int redPin    = 0;
const int greenPin  = 1;
const int bluePin   = 4;
const int knockPin  = 3;
const int buttonPin = 2;
const int knockPinInt = PCINT3; // interrupt bit numbers
const int buttonPinInt = PCINT2;
#ifdef CALIBRATE
const int knockPinAnalog = A3;
#endif

// EEPROM layout
const int effectOffset = 0;

// The current effect in use -- backed up to EEPROM
uint8_t effect;

#if 1 // use gamma correction
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

void WriteLed(const CRGB& Led) {
  analogWrite(redPin,   255 - pgm_read_byte(&gamma8[Led.r]));
  analogWrite(greenPin, 255 - pgm_read_byte(&gamma8[Led.g]));
  analogWrite(bluePin,  255 - pgm_read_byte(&gamma8[Led.b]));
}

#else
void WriteLed(const CRGB& Led) {
  analogWrite(redPin,   255 - Led.r);
  analogWrite(greenPin, 255 - Led.g);
  analogWrite(bluePin,  255 - Led.b);
}
#endif

#ifdef CALIBRATE
// Use the LED to indicate the level on the knock sensor input pin,
// so I can calibrate the voltage divider or trimpot.
#define ARANGE 1024 // analogRead() range
void calibrate() {
  int val;
  int peak = 0;
  int d = 500; // ms
  unsigned long peakMillis;
  unsigned long curMillis;

  while (1) {
    val = analogRead(knockPinAnalog);

    // Hold the highest value seen in the last d milliseconds
    curMillis = millis();
    if (val > peak + 10) {
      peak = val;
      peakMillis = curMillis;
    }
    else if (curMillis - peakMillis > d) {
      peak = val;
    }

    if (peak < 0.25 * ARANGE) { // < 0.3 x Vcc reads as LOW
      WriteLed(CRGB::Red);
    }
    else if (peak < 0.65 * ARANGE) { // 0.3 to 0.6 x Vcc is indeterminate
      WriteLed(CRGB::Blue);
    }
    else if (peak < 0.95 * ARANGE) { // > 0.6 x Vcc reads as HIGH
      WriteLed(CRGB::Green);
    }
    else { // Nearing the top of the range
      WriteLed(CRGB::White);
    }
  }
}
#endif

// interrupt service routine, called when waking from sleep
ISR (PCINT0_vect)
{
  sleep_disable ();         // first thing after waking from sleep:
}

void setup() {
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(knockPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  WriteLed(CRGB::Black);

  // Enable the pin change interrupts
  PCMSK |= bit (knockPinInt) | bit (buttonPinInt);
  GIFR  |= bit (PCIF);   // clear any outstanding interrupts
  GIMSK |= bit (PCIE);   // enable pin change interrupts

#ifndef CALIBRATE
  // Turn off the ADC -- we don't use it
  ADCSRA = 0;
#endif

  // Initialize the effect index
#define MAX_EFFECT 5 // see below
  effect = EEPROM.read(effectOffset);
  if (effect > MAX_EFFECT) {
    effect = 0;
  }
}

uint8_t Hues[] = {HUE_RED, HUE_GREEN, HUE_BLUE};

void loop() {
  CRGB Led;
  uint8_t Idx;
  uint8_t Idx2;

#ifdef CALIBRATE
  calibrate();
  // does not return
#endif

  // Play the current effect

  const int delay1 = 80; // For flash effects
  const int delay2 = 80;

  switch (effect) {
  case 0:
    // Triple flash - patriotic
    WriteLed(CRGB::Blue);  delay(delay1);
    WriteLed(CRGB::Black); delay(delay2);
    WriteLed(CRGB::White); delay(delay1);
    WriteLed(CRGB::Black); delay(delay2);
    WriteLed(CRGB::Red);   delay(delay1);
    break;
  case 1:
    // Triple flash - white
    WriteLed(CRGB::White); delay(delay1);
    WriteLed(CRGB::Black); delay(delay2);
    WriteLed(CRGB::White); delay(delay1);
    WriteLed(CRGB::Black); delay(delay2);
    WriteLed(CRGB::White); delay(delay1);
    break;
  case 2:
    // Triple flash - violet
    WriteLed(CRGB::Violet); delay(delay1);
    WriteLed(CRGB::Black);  delay(delay2);
    WriteLed(CRGB::Violet); delay(delay1);
    WriteLed(CRGB::Black);  delay(delay2);
    WriteLed(CRGB::Violet); delay(delay1);
    break;
  case 3:
    // Rainbow
#if 0
    for (Idx = 0; Idx < 240; Idx++) {
      Led = CHSV(Idx, 255, 255); // vary hue, max out saturation, brightness
      WriteLed(Led);
      delay(4);
    }
#endif
    for (Idx = 0; Idx < 255; Idx++) {
      Led = blend(CHSV(HUE_RED, 255, 255), CHSV(HUE_PURPLE, 255, 255), Idx, FORWARD_HUES);
      WriteLed(Led);
      delay(4);
    }
    break;
  case 4:
    // Heat flare
    for (Idx = 0; Idx < 220; Idx++) {
      Led = ColorFromPalette(HeatColors_p, 220 - Idx, 255, LINEARBLEND);
      WriteLed(Led);
      delay(5);
    }
    break;
  case 5:
    // basic R/G/B, mainly for testing the circuit
    {
      int d = 500;
      WriteLed(CRGB::Red);
      delay(d);
      WriteLed(CRGB::Green);
      delay(d);
      WriteLed(CRGB::Blue);
      delay(d);
    }
    break;
  default:
    break;
  }

  // Turn off the light
  WriteLed(CRGB::Black);

  // Sleep until something happens
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);   
  power_all_disable ();    // power off ADC, Timer 0 and 1, serial interface
  noInterrupts ();         // make sure we don't get interrupted before we sleep
  sleep_enable ();         // enables the sleep bit in the mcucr register
  interrupts ();           // interrupts allowed now, next instruction WILL be executed
  sleep_cpu ();            // here the device is put to sleep
  power_all_enable();      // power everything back on

  // We're back after sleeping
  if (digitalRead(buttonPin) == 0) {
    // Advance to the next effect
    effect += 1;
    if (effect > MAX_EFFECT) {
      effect = 0;
    }
    // Save it to non-volatile storage
    EEPROM.write(effectOffset, effect);
  }
}

// See the blend() function in colorutils.h to interpolate between 2 colors
