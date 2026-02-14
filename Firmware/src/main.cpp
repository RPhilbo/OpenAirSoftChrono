#include <Arduino.h>
#include <variant.h>
#include <Wire.h>

#define LED_OFF HIGH
#define LED_ON LOW


/* ============================================================
 * ======================= RTOS ===============================
 * ============================================================ */
// Task handlers to use multitasking
TaskHandle_t HeartbeatTaskHandle;


// A "Start Pistol" to prevent running the tasks by using "xTaskCreate" in setup()
SemaphoreHandle_t startTasksSignal;


/* ============================================================
 * ======================= PROTOTYPES =========================
 * ============================================================ */
// Function Prototypes
void HeartbeatTask(void *pvParameters);


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


  Serial.println("Setup Pre End");

  // Give the signal - this wakes up everyone waiting for it
  xSemaphoreGive(startTasksSignal); 

  Serial.println("Setup End");
}


/* ============================================================
 * ======================= LOOP ===============================
 * ============================================================ */
void loop() {
  // put your main code here, to run repeatedly:

  vTaskDelay(pdMS_TO_TICKS(1000)); // Non-blocking delay
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


/* ============================================================
 * ======================= METHODS ============================
 * ============================================================ */