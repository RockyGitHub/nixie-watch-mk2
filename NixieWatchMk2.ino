// Licensed by Chris Davidoff
// Uses ShiftOutSlow library for the shiftout functions, not written by me



#include <stdint.h>
#include "RTC.h" //Include RTC library included with the Aruino_Apollo3 core
#include "ShiftOutSlow.h"

//Apollo3RTC rtc; //Create instance of RTC class
ShiftOutSlow SOS(4, 8, LSBFIRST);

enum stateMachine {WatchState = 0, StopWatchState = 1, SetTimeState = 2};
int currentState = 0;

int blinkPin = 5; //13
int sdaPin = 40; //14
int sclPin = 39; //15
int HVSupplyPin = 43; //29
int ButtonPin = 42; //28
int OEPin = 23; //   !pin //5
int latchPin = 10; //26
int clockPin = 8; //24
int dataPin = 4; //3
int clearPin = 9; //   !pin //25
int redLED = 18;
int blueLED = 19;

int counter = 0;
bool isFirstPress = false;
bool HasBeenHeldDown = false;
volatile int ButtonPressCount = 0;  // volatile is required for use in the ISR
volatile bool ButtonWasPressed = false;
volatile unsigned long PressedTime = 0;

int previousMinute;
int previousSecond;

void setup()
{

  pinMode(blinkPin, OUTPUT);
  digitalWrite(blinkPin, HIGH); //so I can see it's alive when I plug it in
  pinMode(HVSupplyPin, OUTPUT);
  pinMode(ButtonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ButtonPin), myISR, FALLING);
  //attachInterruptArg(digitalPinToInterrupt(INT_PIN), myISRArg, &count, RISING);
  // Shift registers
  //pinMode(OEPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(clearPin, OUTPUT);
  pinMode(redLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  //analogReadResolution(14);
  // clear shift registers
  digitalWrite(clearPin, HIGH);
  delay(10);
  digitalWrite(clearPin, LOW);
  delay(10);
  digitalWrite(clearPin, HIGH);
  delay(10);

  //digitalWrite(HVSupplyPin, HIGH);
  //Serial.begin(115200);
  delay(100);
  //Serial.println("Hello from Artemis!");
  //digitalWrite(OEPin, LOW); // HIGH is off
  analogWriteResolution(8);
  analogWrite(OEPin, 180); // the higher the number, the dimmer it'll be

  delay(1000);
  digitalWrite(HVSupplyPin, LOW);
  digitalWrite(blinkPin, LOW);
  digitalWrite(blueLED, HIGH);
  
  rtc.setTime(00, 00, 00, 10, 4, 5, 2022);
  //rtc.setToCompilerTime(); //Easily set RTC using the system __DATE__ and __TIME__ macros from compiler
}

void loop()
{
  rtc.getTime();
  // Serial.println(ButtonPressCount);

  if (ButtonPressCount > 0)
  {
    if (digitalRead(ButtonPin) == HIGH)
    {
      HasBeenHeldDown = false;
    }
    
    // Action with 2 button presses
    if (ButtonPressCount == 2) {
      if (currentState != StopWatchState)
        currentState = StopWatchState;
      else
        currentState = WatchState;
      ButtonPressCount = 0;
      counter = 0;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - PressedTime >= 1500)
    {
      if (HasBeenHeldDown == true)
      {
        digitalWrite(HVSupplyPin, !digitalRead(HVSupplyPin)); // replace this with the HV supply pin
      }
      ButtonPressCount = 0;
    }
  }
  switch (currentState){
    case WatchState:
      Watch();
      break;
    case StopWatchState:
      StopWatch();
      break;
    case SetTimeState:
      //SetTime();
      break;
  }
  delay(50);
  
  /*for (int i=0; i<4; i++){
    for (int j=0; j<10; j++){
      delay(200);
      switch(i){
        case 0:
          SetNixieDigits(11,11,11,j);
          break;
        case 1:
          SetNixieDigits(11,11,j,11);
          break;
        case 2:
          SetNixieDigits(11,j,11,11);
          break;
        case 3:
          SetNixieDigits(j,11,11,11);
          break;
      }
    }
  }*/

  //    Serial.print(rtc.hour);
  //  Serial.print(" ");
  //  Serial.print(rtc.minute);
  //  Serial.print(" ");
  //  Serial.println(rtc.seconds);   
}

void Watch(){
  int currentSecond = rtc.seconds;
  SetNixieDigits(firstDigit(rtc.hour), rtc.hour % 10, firstDigit(rtc.minute), rtc.minute %10);
  if (currentSecond > previousSecond){
    digitalWrite(redLED, HIGH);
    delay(5);
    digitalWrite(redLED, LOW);
  }
  previousSecond = currentSecond;
}

void StopWatch(){
  int currentSecond = rtc.seconds;
  int currentMinute = rtc.minute % 10;
  if (currentMinute > previousMinute)
    counter++;
  SetNixieDigits(firstDigit(counter), counter % 10, firstDigit(rtc.seconds), rtc.seconds % 10);
  previousMinute = currentMinute;
  if (currentSecond > previousSecond){
    digitalWrite(redLED, HIGH);
    digitalWrite(blinkPin, HIGH);
    delay(5);
    digitalWrite(redLED, LOW);
    digitalWrite(blinkPin, LOW);
  }
  previousSecond = currentSecond;
}

int firstDigit(int n)
{
  if (n < 10)
    return 0;
  while (n >= 10)
    n /= 10;
  return n;
}

void PowerDown()
{
  //power_adc_disable();
  // unfinished
}

void myISR(void)
{
  //Serial.println("Hi i am an ISR!");
  ButtonPressCount++;
  PressedTime = millis();
  HasBeenHeldDown = true;
}

void SetNixieDigits(int digit1, int digit2, int digit3, int digit4)
{
  // NX1, NX2, NX3, NX4
  // 2,   10,  10,  10
  // IE: 12:59       // B10 << 30, B0010000000 << 20, B0000001000 << 10, B00000000001
  uint8_t dataMask1 = Digit1Mask(digit1);
  uint16_t dataMask2 = Digit234Mask(digit2);
  uint16_t dataMask3 = Digit234Mask(digit3);
  uint16_t dataMask4 = Digit234Mask(digit4);
  uint32_t shiftOutData = 0b00000000000000000000000000000000;
  shiftOutData = ((dataMask1 << 30) | (dataMask2 << 20) | (dataMask3 << 10) | dataMask4);
  //shiftOutData = (0b11 << 30) | (0b1000000000 << 20) | (0b0000010000 << 10) | (0b00000000001);
  //shiftOutData = 0xFFFFFFFF;
  //shiftOutData = 0;
  /*Serial.print("\t binary output ");
  Serial.print(shiftOutData & 0xFF, BIN);
  Serial.print(" ");
  Serial.print((shiftOutData >> 8) & 0xff, BIN);
  Serial.print(" ");
  Serial.print((shiftOutData >> 16) & 0xFF, BIN);
  Serial.print(" ");
  Serial.println((shiftOutData >> 24) & 0xFF, BIN);*/
  digitalWrite(latchPin, LOW);
  digitalWrite(clockPin, LOW);
  SOS.setDelay(500); // this is a random number I pulled out of my butt, but it works
  SOS.write(shiftOutData & 0xFF);
  //shiftOut(dataPin, clockPin, LSBFIRST, shiftOutData & 0xFF);
  digitalWrite(clockPin, LOW);
  SOS.write((shiftOutData >> 8) & 0xFF);
  //shiftOut(dataPin, clockPin, LSBFIRST, (shiftOutData >> 8) & 0xFF);
  digitalWrite(clockPin, LOW);
  SOS.write((shiftOutData >> 16) & 0xFF);
  //shiftOut(dataPin, clockPin, LSBFIRST, (shiftOutData >> 16) & 0xFF);
  digitalWrite(clockPin, LOW);
  SOS.write((shiftOutData >> 24) & 0xFF);
  //shiftOut(dataPin, clockPin, LSBFIRST, (shiftOutData >> 24) & 0xFF);
  digitalWrite(latchPin, HIGH);

}

uint8_t Digit1Mask(int digit)
{
  switch (digit)
  {
    case 1:
      return 0b10;
      break;
    case 2:
      return 0b01;
      break;
    default:
      return 0b00;
  }
}

uint16_t Digit234Mask(int digit)
{
  switch (digit)
  {
    case 0:
      return 0b1000000000;
      break;
    case 1:
      return 0b0100000000;
      break;
    case 2:
      return 0b0010000000;
      break;
    case 3:
      return 0b0001000000;
      break;
    case 4:
      return 0b0000100000;
      break;
    case 5:
      return 0b0000010000;
      break;
    case 6:
      return 0b0000001000;
      break;
    case 7:
      return 0b0000000100;
      break;
    case 8:
      return 0b0000000010;
      break;
    case 9:
      return 0b0000000001;
      break;
    default:
      return 0b0000000000;
  }
}


void SetNixieDigitsReversed(int digit1, int digit2, int digit3, int digit4)
{
  // NX1, NX2, NX3, NX4
  // 2,   10,  10,  10
  // IE: 12:59       // B10 << 30, B0010000000 << 20, B0000001000 << 10, B00000000001
  uint8_t dataMask1 = Digit1MaskReversed(digit1);
  uint16_t dataMask2 = Digit234MaskReversed(digit2);
  uint16_t dataMask3 = Digit234MaskReversed(digit3);
  uint16_t dataMask4 = Digit234MaskReversed(digit4);
  uint32_t shiftOutData = 0b00000000000000000000000000000000;
  shiftOutData = ((dataMask1 << 30) | (dataMask2 << 20) | (dataMask3 << 10) | dataMask4);
  //shiftOutData = (0b11 << 30) | (0b1000000000 << 20) | (0b0000010000 << 10) | (0b00000000001);
  //shiftOutData = 0xFFFFFFFF;
  //shiftOutData = 0;
  // Serial.print("\t binary output ");
  // Serial.print(shiftOutData & 0xFF, BIN);
  // Serial.print(" ");
  // Serial.print((shiftOutData >> 8) & 0xff, BIN);
  // Serial.print(" ");
  // Serial.print((shiftOutData >> 16) & 0xFF, BIN);
  // Serial.print(" ");
  // Serial.println((shiftOutData >> 24) & 0xFF, BIN);
  digitalWrite(latchPin, LOW);
  digitalWrite(clockPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, shiftOutData & 0xFF);
  digitalWrite(clockPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, (shiftOutData >> 8) & 0xFF);
  digitalWrite(clockPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, (shiftOutData >> 16) & 0xFF);
  digitalWrite(clockPin, LOW);
  shiftOut(dataPin, clockPin, LSBFIRST, (shiftOutData >> 24) & 0xFF);
  digitalWrite(latchPin, HIGH);

}
uint8_t Digit1MaskReversed(int digit)
{
  switch (digit)
  {
    case 1:
      return 0b01;
      break;
    case 2:
      return 0b10;
      break;
    default:
      return 0b00;
      break;
  }
}
uint16_t Digit234MaskReversed(int digit)
{
  switch (digit)
  {
    case 0:
      return 0b0000000001;
      break;
    case 1:
      return 0b0000000010;
      break;
    case 2:
      return 0b0010000000;
      break;
    case 3:
      return 0b0001000000;
      break;
    case 4:
      return 0b0000100000;
      break;
    case 5:
      return 0b0000010000;
      break;
    case 6:
      return 0b0000001000;
      break;
    case 7:
      return 0b0010000000;
      break;
    case 8:
      return 0b0100000000;
      break;
    case 9:
      return 0b1000000000;
      break;
    default:
      return 0b0000000000;
  }
}
