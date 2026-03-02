#include <Arduino.h>

/** * HARDWARE CONFIGURATION
 * Adjust these based on your specific board's specs
 */
const int   ADC_RESOLUTION_BITS = 10;             // nRF52 supports up to 12
const float ADC_MAX_VALUE       = 1023.0;         // (2^10) - 1
const float V_REF               = 3.3;            // ADC Reference Voltage
const float V_DIVIDER_RATIO     = 1510.0 / 510.0; // (R_high + R_low) / R_low

/**
 * Reads the battery voltage using the configured hardware constants.
 * @return The battery voltage in Volts.
 */
float readBatteryVoltage() {
    // 1. Enable the divider (Specific to XIAO nRF52840)
    pinMode(VBAT_ENABLE, OUTPUT);
    digitalWrite(VBAT_ENABLE, LOW);
    
    // Give the hardware a moment to settle
    //delay(5);

    // 2. Perform the analog read
    uint32_t rawADC = analogRead(PIN_VBAT);

    // 3. Disable the divider to stop battery drain (optional)
    //digitalWrite(VBAT_ENABLE, HIGH);

    // 4. Calculate Voltage: 
    // Voltage = (Raw / Max_Resolution) * V_Ref * Divider_Ratio
    float voltage = (rawADC / ADC_MAX_VALUE) * V_REF * V_DIVIDER_RATIO;

    return voltage;
}