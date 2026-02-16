#ifndef TIMER_CONTROL_H
#define TIMER_CONTROL_H

#include <Arduino.h>

// This allows other files to see the BBCounter defined in main
extern uint32_t BBCounter; 

// The 'extern' keyword tells the compiler the timer is defined elsewhere
extern NRF_TIMER_Type *timer;


void TimerSetup();
void TimerReset();

//float getTicksConvertToMicroseconds();
//void TimerCheckAndEvaluate();

#endif