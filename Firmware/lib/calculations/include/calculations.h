#include <Arduino.h>

float calculateTicksToMicroseconds(uint32_t ticks);
float calculateTicksToMilliseconds(uint32_t ticks);

float calculateVelocitySI(float distance_m, float time_s);
float calculateVelocityMilli(float distance_mm, float time_ms);
float calculateVelocityMicro(float distance_mm, float time_us);

float calculateEnergy(float velocity, float mass_kg);