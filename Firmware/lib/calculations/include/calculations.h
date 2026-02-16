#include <Arduino.h>

#define TIMERFREQ 16000000UL // 16 MHz

// Time
float calculateTicksToMicroseconds(uint32_t ticks);
float calculateTicksToMilliseconds(uint32_t ticks);

// Velocity
float calculateVelocitySI(float distance_m, float time_s);
float calculateVelocityMilli(float distance_mm, float time_ms);
float calculateVelocityMicro(float distance_mm, float time_us);

// Energy
float calculateEnergySI(float velocity, float mass_kg);
float calculateEnergyGramm(float velocity, float mass_g);

// Units
float convertVelocityMSToFTS(float velocity_ms);