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