#include "config.h"
#include "pins.h"

/* ============================================================
 * ======================= DEFINITIONS ========================
 * ============================================================ */

#define LED_OFF HIGH
#define LED_ON LOW

/* ============================================================
 * ======================= VARIABLES ==========================
 * ============================================================ */

TargetIdentifier TargetID;


/* ============================================================
 * ======================= METHODS ============================
 * ============================================================ */

// Get the complete targer MCU identifier as a struct
TargetIdentifier getTargetIdentifier() {
    TargetIdentifier TargetID;
    
    TargetID.part       = NRF_FICR->INFO.PART;
    TargetID.variant    = NRF_FICR->INFO.VARIANT;
    TargetID.UID        = ((uint64_t)NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];

    return TargetID;
}


// Enables the ToF sensors (and their current consumption)
void TofSensorsEnableAll() {
  Serial.println("\nEnabling the ToF sensor 1 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(TOF_SENSOR1_ENABLE, HIGH);
  Serial.println("                ToF sensor 1 is enabled");

  Serial.println("\nEnabling the ToF sensor 2 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(TOF_SENSOR2_ENABLE, HIGH);
  Serial.println("                ToF sensor 2 is enabled");

  Serial.println("\nEnabling the ToF sensor 3 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(TOF_SENSOR3_ENABLE, HIGH);
  Serial.println("                ToF sensor 3 is enabled");
}

// method to check the sensor outputs via polling (for debug purpose)
void TofSensorCheckTask() {
  
  if (digitalRead(TOF_SENSOR1_OUTPUT)) {
    digitalWrite(LED_RED, LED_ON);
    Serial.println("Sensor 1 output is high");
  }
  else digitalWrite(LED_RED, LED_OFF);

  if (digitalRead(TOF_SENSOR2_OUTPUT)) {
    digitalWrite(LED_GREEN, LED_ON);
    Serial.println("Sensor 2 output is high");
  }
  else digitalWrite(LED_GREEN, LED_OFF);

  if (digitalRead(TOF_SENSOR3_OUTPUT)) {
    digitalWrite(LED_BLUE, LED_ON);
    Serial.println("Sensor 3 output is high");
  }
  else digitalWrite(LED_BLUE, LED_OFF);
}