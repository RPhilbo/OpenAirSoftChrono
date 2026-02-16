#include <Arduino.h>
#include <variant.h>
#include <Wire.h>

#include "config.h"
#include "timer_control.h"

#define LED_OFF HIGH
#define LED_ON LOW


/* ============================================================
 * ======================= Pins ===============================
 * ============================================================ */

#define LED_OFF HIGH
#define LED_ON LOW

#define ToF_Sensor1_Enable D9
#define ToF_Sensor2_Enable D8
#define ToF_Sensor3_Enable D19

#define ToF_Sensor1_Output D10
#define ToF_Sensor2_Output D7
#define ToF_Sensor3_Output D18


/* ============================================================
 * ======================= RTOS ===============================
 * ============================================================ */
// Task handlers to use multitasking
TaskHandle_t HeartbeatTaskHandle;
TaskHandle_t TofSensorCheckHandle;
TaskHandle_t TimerCheckAndEvaluateTaskHandle;


// A "Start Pistol" to prevent running the tasks by using "xTaskCreate" in setup()
SemaphoreHandle_t startTasksSignal;


uint32_t BBCounter = 0;

/* ============================================================
 * ======================= TIMER ==============================
 * ============================================================ */

NRF_TIMER_Type *timer = NRF_TIMER2;


/* ============================================================
 * ======================= PHYSICS ============================
 * ============================================================ */
static const float TofSensorDistance = 0.02f;
float BBWeight = 0.00036f;


/* ============================================================
 * ======================= PROTOTYPES =========================
 * ============================================================ */
// Function Prototypes
void HeartbeatTask(void *pvParameters);
void TofSensorCheckTask(void *pvParameters);

void TofSensorsEnableAll();
//void TimerSetup();
void TimerCheckAndEvaluate();

/* ============================================================
 * ======================= SETUP ==============================
 * ============================================================ */

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(500);
  Serial.println("\nSetup Start");

  // Setup LED pins as OUTPUT and OFF
  pinMode(LED_RED, OUTPUT);   digitalWrite(LED_RED,   LED_OFF);
  pinMode(LED_GREEN, OUTPUT); digitalWrite(LED_GREEN, LED_OFF);
  pinMode(LED_BLUE, OUTPUT);  digitalWrite(LED_BLUE,  LED_OFF);

  // Setup the ToF sensor pins
  pinMode(ToF_Sensor1_Enable, OUTPUT);   digitalWrite(ToF_Sensor1_Enable, LOW);
  pinMode(ToF_Sensor2_Enable, OUTPUT);   digitalWrite(ToF_Sensor2_Enable, LOW);
  pinMode(ToF_Sensor3_Enable, OUTPUT);   digitalWrite(ToF_Sensor3_Enable, LOW);

  pinMode(ToF_Sensor1_Output, INPUT_PULLDOWN);
  pinMode(ToF_Sensor2_Output, INPUT_PULLDOWN);
  pinMode(ToF_Sensor3_Output, INPUT_PULLDOWN);


  // Enabling the ToF sensor pins.
  Serial.println("Enabling the ToF sensor pins");
  TofSensorsEnableAll();
  delay(50);

  // Preparing the timer
  Serial.println("Timer Setup");
  TimerSetup();
  delay(50);


  startTasksSignal = xSemaphoreCreateBinary(); // Create the "Pistol"

  // Create Task: Blinking1
  xTaskCreate(
      HeartbeatTask,        // Function name
      "Blinky",             // Name for debugging
      1024,                 // Stack size (in words)
      NULL,                 // Parameter to pass
      1,                    // Priority (Low)
      &HeartbeatTaskHandle  // Task handle
  );

  // Create Task: TofSensorCheckTask
  xTaskCreate(
      TofSensorCheckTask,   // Function name
      "TofPoll",            // Name for debugging
      1024,                 // Stack size (in words)
      NULL,                 // Parameter to pass
      2,                    // Priority
      &TofSensorCheckHandle // Task handle
  );


  Serial.println("Setup Pre End");

  

   

  Serial.println(" ");
  Serial.println("Setup End");
  Serial.println(" ");

  // Give the signal - this wakes up everyone waiting for it
  Serial.println("Pre startTasksSignal");
  xSemaphoreGive(startTasksSignal);
  Serial.println("Post startTasksSignal");
}


/* ============================================================
 * ======================= LOOP ===============================
 * ============================================================ */

void loop() {
  // put your main code here, to run repeatedly:
  TimerCheckAndEvaluate();
  vTaskDelay(pdMS_TO_TICKS(1)); // Non-blocking delay
}


/* ============================================================
 * ======================= RTOS Tasks =========================
 * ============================================================ */

// Task: Heartbeat with an LED and serial print
void HeartbeatTask(void *pvParameters) {
  // Wait here forever until setup gives the signal
  xSemaphoreTake(startTasksSignal, portMAX_DELAY);
  
  // Put the signal back so OTHER tasks can also pass the gate
  xSemaphoreGive(startTasksSignal);

  while (1) {
    digitalWrite(LED_RED, LED_ON);
    Serial.println("Heartbeat Task is alive");
      vTaskDelay(pdMS_TO_TICKS(1000)); // Non-blocking delay
    digitalWrite(LED_RED, LED_OFF);
      vTaskDelay(pdMS_TO_TICKS(10000)); // Non-blocking delay
  }
}

// RTOS task to check the sensor via polling (for debug purpose)
void TofSensorCheckTask(void *pvParameters) {
  // Wait here forever until setup gives the signal
  xSemaphoreTake(startTasksSignal, portMAX_DELAY);
  
  // Put the signal back so OTHER tasks can also pass the gate
  xSemaphoreGive(startTasksSignal);

  while (1) {
    if (digitalRead(ToF_Sensor1_Output)) {
      digitalWrite(LED_RED, LED_ON);
      Serial.println("Sensor 1 output is high");
    }
    else digitalWrite(LED_RED, LED_OFF);

    if (digitalRead(ToF_Sensor2_Output)) {
      digitalWrite(LED_GREEN, LED_ON);
      Serial.println("Sensor 2 output is high");
    }
    else digitalWrite(LED_GREEN, LED_OFF);

    if (digitalRead(ToF_Sensor3_Output)) {
      digitalWrite(LED_BLUE, LED_ON);
      Serial.println("Sensor 3 output is high");
    }
    else digitalWrite(LED_BLUE, LED_OFF);
    
    vTaskDelay(pdMS_TO_TICKS(300)); // Non-blocking delay
  }
}




/* ============================================================
 * ======================= METHODS ============================
 * ============================================================ */

// Enables the ToF sensors (and their current consumption)
void TofSensorsEnableAll() {
  Serial.println("\nEnabling the ToF sensor 1 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(ToF_Sensor1_Enable, HIGH);
  Serial.println("                ToF sensor 1 is enabled");

  Serial.println("\nEnabling the ToF sensor 2 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(ToF_Sensor2_Enable, HIGH);
  Serial.println("                ToF sensor 2 is enabled");

  Serial.println("\nEnabling the ToF sensor 3 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(ToF_Sensor3_Enable, HIGH);
  Serial.println("                ToF sensor 3 is enabled");
}

/*void TimerSetup() {
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
}*/


// Read Timer value, then calc and print value in microseconds
void TimerCheckAndEvaluate() {
  uint32_t ticks = timer->CC[0];
  
  if (ticks > 1000) {
    float timerMicroseconds = (float)ticks / 16.0f;
    float timerMilliseconds = timerMicroseconds / 1000;

    //float velocity12 = ((float)20.0f / timerMicroseconds) * 1000.0f;
    float velocity12 = TofSensorDistance / (timerMicroseconds/1000000.0f);
    float energy12 = 0.5f * BBWeight * velocity12 * velocity12;
    
    BBCounter++;

    Serial.printf("BBCounter: %u | %.2f us | %.2f ms | v12: %.2f m/s | E12: %.3f J\n", BBCounter, timerMicroseconds, timerMilliseconds, velocity12, energy12);

    // Hardware Reset
    NRF_PPI->CHENCLR = (1 << 0) | (1 << 1);
    timer->TASKS_STOP = 1; timer->TASKS_CLEAR = 1; timer->CC[0] = 0;
    NRF_GPIOTE->EVENTS_IN[0] = 0; NRF_GPIOTE->EVENTS_IN[1] = 0;
    (void)NRF_GPIOTE->EVENTS_IN[0]; (void)NRF_GPIOTE->EVENTS_IN[1];
    while(digitalRead(D10) == HIGH || digitalRead(D7) == HIGH);
    NRF_PPI->CHENSET = (1 << 0) | (1 << 1);
  }  
}