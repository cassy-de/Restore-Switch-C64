#include <Arduino.h>

/************************************************************************

  Restore_Kernal_Switch_W27E257

    "64 Tuning Board" - Kernal Switch, ATTiny84A firmware, internal clock 8Mhz (hfuse 0xdf | lfuse 0xe2)

    Designed by Matthias Lorenz

    changes 2021 by cassy

    - power on reset added, changed PIN defnitions for PlatformIO, typos 
    - a manual reset WITHOUT switching Kernal can be performed by pressing the Restore key for 800 - 2000 ms
    - true WHITE LED multiplexing added during RESET procedure

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
               XTAL/D10 o 0    10 o D0/A0  -> OUT KERNAL "PIN A15" (J2)     Vpp = 27E257 = always HIGH
               XTAL/D9  o 1     9 o D1/A1  -> OUT KERNAL "PIN A14" (J15)    A14 = 27E257
            (ICSP) RES  o 11    8 o D2/A2  <- IN "RESTORE KEY"
  (ROT)             D8  o 2     7 o D3/A3  -> _RESET Pin C64
  (GRÜN)         A7/D7  o 3     6 o D4/A4  SCK
  (BLAU)   MOSI  A6/D6  o 4     5 o D5/A5  MISO

*/

#include <EEPROM.h>

int kernalPIN14 = 9;   // was 1
int kernalPIN15 = 10;   // was 0   
int restoreKey = 8;   // was 2
int resetPIN   = 7;   // was 3

int ledR =  2;    // 6
int ledG =  3;    // 7
int ledB =  4;    // 8


int state = 0;

boolean doReset = false;

long restoreHoldTime      = millis();
long lastStateSwitchTime  = millis();


void red() {
  analogWrite(ledR, 255);
  analogWrite(ledG, 0);
  analogWrite(ledB, 0);
}

void green() {
  analogWrite(ledR, 0);
  analogWrite(ledG, 255);
  analogWrite(ledB, 0);
}

void blue() {
  analogWrite(ledR, 0);
  analogWrite(ledG, 0);
  analogWrite(ledB, 255);
}

void violet() {
  analogWrite(ledR, 180);
  analogWrite(ledG, 20);
  analogWrite(ledB, 180);
}

void doState() {

  if (state == 0) {
    blue();
    digitalWrite(kernalPIN14, LOW);    // Kernal: LOW Address
    digitalWrite(kernalPIN15, HIGH);   // Vpp must be STAY HIGH
  }
  if (state == 1) {
    violet();
    digitalWrite(kernalPIN14, HIGH);  // Kernal: HIGH Address
    digitalWrite(kernalPIN15, HIGH);  // Vpp must be STAY HIGH
  }

  EEPROM.put(10, state);
}


boolean isRestoreLongPressed() {

  while (digitalRead(restoreKey) == LOW) {

    delay(10); // 10ms
    if (millis() - restoreHoldTime > 2000) return true; // After holding 2s
  }
  return false;
}

void execReset() {
  pinMode(resetPIN, OUTPUT);
  digitalWrite(resetPIN, LOW);
  // due to its design (single R for three LED's), colors can't be mixed in regular basis
  // mixing WHITE by multiplexing RGB
  for (int i = 0; i < 16; i++) {
     red(); delay(10); green(); delay(10); blue(); delay(10);
  } 
  digitalWrite(resetPIN, HIGH);
  pinMode(resetPIN, INPUT);
  doState();
}

void setup() {

  TCCR1A  = _BV(COM1A1) | _BV(WGM10);
  TCCR1B  = _BV(CS10) | _BV(WGM12);
  OCR1A   = 127;


  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);
  analogWrite(ledR, 0);
  analogWrite(ledG, 0);
  analogWrite(ledB, 0);


  pinMode(kernalPIN14, OUTPUT);
  pinMode(kernalPIN15, OUTPUT);
  pinMode(restoreKey, INPUT);
  pinMode(resetPIN, INPUT);

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

  delay(10); // 10ms
}

