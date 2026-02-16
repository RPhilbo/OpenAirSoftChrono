#include <Arduino.h>

#define TestCase1
//#define TestCase2

// We'll use a single timer and manual "ticks" to ensure 1us = 1 tick.
HardwareTimer *MyTim;

const uint16_t BBCounterLinear = 420;
uint16_t BBCountertemp = 1;

void shootLinearTimingSlow();
void shootLinearTimingFast();

void shootWithMissingSensor();


void setup() {
  //Serial.begin(9600);
  Serial.begin(115200);
  delay(1000);
  
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(A3, OUTPUT);
  digitalWrite(A1, LOW);
  digitalWrite(A2, LOW);
  digitalWrite(A3, LOW);

  MyTim = new HardwareTimer(TIM2);

  // MANUALLY calculation for the L452RE to force 1 tick = 1 microsecond.
  // If your timing is 4x too fast, we will force the prescaler to be 4x larger.
  uint32_t inputFreq = MyTim->getTimerClkFreq();
  
  // Most STM32s double the bus clock for timers. 
  // We divide the actual frequency by 1,000,000 to get a 1MHz (1us) tick rate.
  MyTim->setPrescaleFactor(inputFreq / 1000000); 
  
  // We don't need the timer to loop, just run once per trigger
  MyTim->setCount(0);
  
  Serial.print("Detected Timer Clock: ");
  Serial.print(inputFreq / 1000000);
  Serial.println(" MHz");
}


void ShootFakeSensor() {
  // Total timeline in microseconds:
  // 0:               A1 HIGH
  // 10:              A1 LOW
  // 156 (10 + 133):  A2 HIGH
  // 166 (156 + 10):  A2 LOW
  // 383              A3 HIGH  (327 total elapsed from start of A1 + pulse width)
  // 393 (383 + 10):  A3 LOW

  // Using a simple blocking sequence but with "Timer-Verified" polling 
  // for absolute precision without interrupt jitter.
  
  uint32_t start = 0;
  MyTim->setCount(0);
  MyTim->resume();

  // Pulse 1
  digitalWrite(A1, HIGH);
  while(MyTim->getCount() < 10);
  digitalWrite(A1, LOW);

  // Gap 1 + Pulse 2
  while(MyTim->getCount() < 160);
  digitalWrite(A2, HIGH);
  while(MyTim->getCount() < 170);
  digitalWrite(A2, LOW);

  // Gap 2 + Pulse 3
  while(MyTim->getCount() < 383); 
  digitalWrite(A3, HIGH);
  while(MyTim->getCount() < 393);
  digitalWrite(A3, LOW);

  MyTim->pause();
}


void ShootFakeSensorModular(uint32_t Pulse, uint32_t Pause) {
  
  uint32_t start = 0;
  MyTim->setCount(0);
  MyTim->resume();

  // Pulse 1
  digitalWrite(A1, HIGH);
  while(MyTim->getCount() < (Pulse)); // Pulse
  digitalWrite(A1, LOW);

  // Pulse 2
  while(MyTim->getCount() < (Pulse+Pause)); // Pulse+Pause
  digitalWrite(A2, HIGH);
  while(MyTim->getCount() < (Pulse+Pause+Pulse)); // Pulse+Pause+Pulse
  digitalWrite(A2, LOW);

  MyTim->pause();
}


void loop() {
  #ifdef TestCase1
    if (BBCountertemp < BBCounterLinear+1)
    {
      Serial.print("Shooting... ");
      Serial.print("BBCounter: "); Serial.println(BBCountertemp);
      ShootFakeSensor();
      BBCountertemp++;
    }
    
    //delay(1000);  // 1 Hz = 1 shot/s = 60 shot/min      works :-)
    //delay(100);  // 10 Hz = 10 shot/s = 600 shot/min      works :-)
    delay(10);  // 100 Hz = 100 shot/s = 6000 shot/min      works :-) But misreadings increase
    //delay(20);  // 50 Hz = 50 shot/s = 3000 shot/min      works :-) But misreadings increase
    //delay(40);

  #endif

  #ifdef TestCase2
    for (size_t i = 1; i < (BBCounterLinear+1); i++)
    {
      shootLinearTimingSlow();
    }
      
  

  #endif
}


void shootLinearTimingSlow() {
  if (BBCountertemp < BBCounterLinear+1)
  {
    ShootFakeSensorModular(10, 150);
    BBCountertemp++;

  // Pause between BBs
  //delay(1000);  // 1 Hz = 1 shot/s = 60 shot/min      works :-)
  //delay(100);  // 10 Hz = 10 shot/s = 600 shot/min      works :-)
  delay(10);  // 100 Hz = 100 shot/s = 6000 shot/min      works :-) But misreadings increase
  //delay(20);  // 50 Hz = 50 shot/s = 3000 shot/min      works :-) But misreadings increase
  //delay(40);
  }
};


void shootLinearTimingFast() {

};


void shootWithMissingSensor() {

};