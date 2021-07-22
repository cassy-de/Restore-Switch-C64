#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
// #ifdef __AVR__
//  #include <avr/power.h> // Required for 16 MHz Adafruit Trinket
// #endif

/************************************************************************

  Restore_Kernal_Switch_W27E257_WS2812

    "64 Tuning Board" - Kernal Switch, ATTiny84A firmware, internal clock 8Mhz (hfuse 0xdf | lfuse 0xe2)

    Designed by Matthias Lorenz

    changes 2021 by cassy

    - power on reset added, changed PIN defnitions for PlatformIO, typos 
    - a manual reset WITHOUT switching Kernal can be performed by holding the Restore key for 800 - 2000 ms
      later is anounced by LEDs blinking white twice, then switched off, during the actual Reset, LEDs are flashing white
    - LED's changed to WS2812 / 2821 NEOPixel, libs from Adafruit_Neopixel 

    burning fuses:  minipro -p attiny84 -w fuses.bin -c config
    flashing fw:    minipro -p attiny84 -w firmware.hex

 ************************************************************************


   LICENSE: There is no License, no GPL, no MIT, no BSD, ...

   ...only free software: Public Domain

   Creative freedom means 100% freedom, not pseudo free :-)


Warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Pin definition is COUNTER clock wise when using PlatformIO
See pin definition on the "inside" of the ic pinout below 
                             |
              ATTINY84A-PU   |
              ------------   |
                            / \
                    +5V o    ^    o GND
  NEU! WS2812  XTAL/D10 o 0    10 o D0/A0  -> OUT KERNAL "PIN A15" (J2)     Vpp = 27E257 = always HIGH
               XTAL/D9  o 1     9 o D1/A1  -> OUT KERNAL "PIN A14" (J15)    A14 = 27E257
            (ICSP) RES  o 11    8 o D2/A2  <- IN "RESTORE KEY"
  (war ROT)         D8  o 2     7 o D3/A3  -> _RESET Pin C64
  (war GRÜN)     A7/D7  o 3     6 o D4/A4  SCK
  (war BLAU)OSI  A6/D6  o 4     5 o D5/A5  MISO

*/


#include <EEPROM.h>

int kernalPIN14 = 9;   // was 1
int kernalPIN15 = 10;   // was 0   
int restoreKey = 8;   // was 2
int resetPIN   = 7;   // was 3

#define LED_PIN    0
#define LED_COUNT 16
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int state = 0;
unsigned int delay2show = 6000;  // after 6000ms, start the show (rainbow)
unsigned long rainbowPreviousMillis=0;  // counter for the rainbow effect
int rainbowCycles = 0;

boolean doReset = false;

long restoreHoldTime      = millis();
long lastStateSwitchTime  = millis();

void colorWipe(uint32_t color, int wait) {
  for(unsigned int i=0; i<strip.numPixels(); i++) { // For each pixel in strip...
    strip.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    strip.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

uint32_t Wheel(byte WheelPos) {  // procedure to create a R->G->B->R... color wheel
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void red() {
  colorWipe(strip.Color(255,   0,   0), 10); // Red
}

void green() {
  colorWipe(strip.Color(  0, 255,   0), 10); // Green
}

void blue() {
  colorWipe(strip.Color(  0,   0, 255), 10); // Blue
}

void violet() {
  colorWipe(strip.Color(  180,  20, 180), 10); // Violet
}

void white() {
  colorWipe(strip.Color(  255,  255, 255), 1); // White
}

void off() {
  colorWipe(strip.Color(  0,  0, 0), 1); // LEDs off
}

void rainbow() {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, Wheel(((16*i)+rainbowCycles) & 255));
  }
  strip.show();
  rainbowCycles++;
  if(rainbowCycles >= 256) rainbowCycles = 0;
}


void doState() {

  if (state == 0) {
    blue();
    digitalWrite(kernalPIN14, LOW);    // Kernal: LOW Address
    digitalWrite(kernalPIN15, HIGH);   // Vpp must be STAY HIGH
  }
  if (state == 1) {
    red();
    digitalWrite(kernalPIN14, HIGH);  // Kernal: HIGH Address
    digitalWrite(kernalPIN15, HIGH);  // Vpp must be STAY HIGH
  }

  EEPROM.put(10, state);
}


boolean isRestoreLongPressed() {

  while (digitalRead(restoreKey) == LOW) {

    delay(10); // 10ms
    if ((millis() - restoreHoldTime > 800) && (millis() - restoreHoldTime < 1000) ) { //  show the start of reset cycle
      white(); delay(100); off();  // flash white once to anounce the reset
    }
    if (millis() - restoreHoldTime > 2000) return true; // After holding 2s
  }
  return false;
}

void execReset() {
  pinMode(resetPIN, OUTPUT);
  digitalWrite(resetPIN, LOW);
  for (int i = 0; i < 3; i++) {
     white(); delay(10); off(); delay(10); // during Reset -> flash LEDs white four times
  } 
  digitalWrite(resetPIN, HIGH);
  pinMode(resetPIN, INPUT);
  lastStateSwitchTime = millis();
  doState();
}

void setup() {

  TCCR1A  = _BV(COM1A1) | _BV(WGM10);
  TCCR1B  = _BV(CS10) | _BV(WGM12);
  OCR1A   = 127;

  pinMode(kernalPIN14, OUTPUT);
  pinMode(kernalPIN15, OUTPUT);
  pinMode(restoreKey, INPUT);
  pinMode(resetPIN, INPUT);
  
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  EEPROM.get(10, state);

  if(state < 0 || state > 3) state = 0;
 
  doState();

  execReset();     // Power On reset
}



void loop() {

  if (isRestoreLongPressed()) {

    if (millis() - lastStateSwitchTime > 1000) { // Rotating Delay Time (1s)
      lastStateSwitchTime = millis();

      state++;
      if (state == 2) state = 0;

      doState();

      doReset = true;
    }
  }
  else {

    if (millis() - restoreHoldTime > 800) {    // after short press
      doReset = true;
    }
    restoreHoldTime = millis();

    // after releasing Restore key and possibly changing State, clear the flag and execute system RESET 
    if (doReset) {
      doReset = false;

      execReset();
    }
  }

  if (millis() - lastStateSwitchTime > delay2show) { // Delay to Show Time (6s)

    rainbowPreviousMillis = millis();
    rainbow(); 

  }
  delay(10); // 10ms
}


