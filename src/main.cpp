#include <Arduino.h>

/************************************************************************

  Restore_Kernel_Switch_W27E257

    "64 Tuning Board" - Kernel Switch, ATTiny84A Firmware, Internal 8Mhz (hfuse 0xdf | lfuse 0xe2)

    Designed by Matthias Lorenz

 ************************************************************************


   LICENSE: There is no License, no GPL, no MIT, no BSD, ...

   ...only free software: Public Domain

   Creative freedom means 100% freedom, not pseudo free :-)


Warning !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Pin definition is COUNTER clock wise when using PlatformIO
See pin definition on the "inside" of the ic pinout below 

              ATTINY84A-PU
              ------------

                    +5V o    ^    o GND
               XTAL/D10 o 0    10 o D0/A0  -> OUT KERNEL "PIN A15" (J2)     Vpp = 27E257 = always HIGH
               XTAL/D9  o 1     9 o D1/A1  -> OUT KERNEL "PIN A14" (J15)    A14 = 27E257
            (ICSP) RES  o 11    8 o D2/A2  <- IN "RESTORE KEY"
  (ROT)             D8  o 2     7 o D3/A3  -> _RESET Pin C64
  (GRÜN)         A7/D7  o 3     6 o D4/A4  SCK
  (BLAU)   MOSI  A6/D6  o 4     5 o D5/A5  MISO

*/

#include <EEPROM.h>

int kernePIN14 = 9;   // was 1
int kernePIN15 = 10;   // was 0   
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
  analogWrite(ledG, 155);
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
    digitalWrite(kernePIN14, LOW);    // Kernel: LOW Address
    digitalWrite(kernePIN15, HIGH);   // Vpp must be STAY HIGH
  }
  if (state == 1) {
    violet();
    digitalWrite(kernePIN14, HIGH);  // Kernel: HIGH Address
    digitalWrite(kernePIN15, HIGH);  // Vpp must be STAY HIGH
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


  pinMode(kernePIN14, OUTPUT);
  pinMode(kernePIN15, OUTPUT);
  pinMode(restoreKey, INPUT);
  pinMode(resetPIN, INPUT);

  EEPROM.get(10, state);

  if(state < 0 || state > 3) state = 0;
 
  doState();
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

    restoreHoldTime = millis();

    // * After Released Restore Key and State changed... RESET System *
    if (doReset) {
      doReset = false;

      pinMode(resetPIN, OUTPUT);
      digitalWrite(resetPIN, LOW);
      delay(500);
      digitalWrite(resetPIN, HIGH);
      pinMode(resetPIN, INPUT);
    }
  }


  delay(10); // 10ms
}

