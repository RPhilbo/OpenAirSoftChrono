#include "config.h"
#include "timer_control.h"

NRF_TIMER_Type *timer = NRF_TIMER2;


/* ============================================================
 * ======================= METHODS ============================
 * ============================================================ */

// Setting up the timer, with direct Pin numbers!!! ToDo: Use defines/Variables.
void TimerSetup() {
  timer->TASKS_STOP = 1; 
  timer->TASKS_CLEAR = 1;
  timer->CC[0] = 0; 
  timer->PRESCALER = 0;
  timer->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;

  #ifdef bbDirectionFromUSB
  NRF_GPIOTE->CONFIG[0] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) | (47 << GPIOTE_CONFIG_PSEL_Pos) | (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos);
  NRF_GPIOTE->CONFIG[1] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) | (44 << GPIOTE_CONFIG_PSEL_Pos) | (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos);
  //NRF_GPIOTE->CONFIG[3] = (GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos) | (D8 << GPIOTE_CONFIG_PSEL_Pos) | (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos) | (GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos);
  #else
  NRF_GPIOTE->CONFIG[0] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) | (44 << GPIOTE_CONFIG_PSEL_Pos) | (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos);
  NRF_GPIOTE->CONFIG[1] = (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) | (47 << GPIOTE_CONFIG_PSEL_Pos) | (GPIOTE_CONFIG_POLARITY_LoToHi << GPIOTE_CONFIG_POLARITY_Pos);
  #endif

  NRF_PPI->CH[0].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[0];
  NRF_PPI->CH[0].TEP = (uint32_t)&timer->TASKS_START;
  NRF_PPI->FORK[0].TEP = (uint32_t)&timer->TASKS_CLEAR;

  NRF_PPI->CH[1].EEP = (uint32_t)&NRF_GPIOTE->EVENTS_IN[1];
  NRF_PPI->CH[1].TEP = (uint32_t)&timer->TASKS_CAPTURE[0];

  NRF_PPI->CHENSET = (1 << 0) | (1 << 1);

  Serial.println("Timer is set up");
}


// Reset the timer
void TimerReset() {
  NRF_PPI->CHENCLR = (1 << 0) | (1 << 1);
  timer->TASKS_STOP = 1;
  timer->TASKS_CLEAR = 1;
  timer->CC[0] = 0;
  NRF_GPIOTE->EVENTS_IN[0] = 0;
  NRF_GPIOTE->EVENTS_IN[1] = 0;
  (void)NRF_GPIOTE->EVENTS_IN[0]; // ToDo: check if redundant
  (void)NRF_GPIOTE->EVENTS_IN[1]; // ToDo: check if redundant
  while(digitalRead(D10) == HIGH || digitalRead(D7) == HIGH);
  NRF_PPI->CHENSET = (1 << 0) | (1 << 1);
};
