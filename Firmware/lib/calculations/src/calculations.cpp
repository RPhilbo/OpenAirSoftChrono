#include "calculations.h"
#include <stdio.h>

/* ============================================================
 * ======================= TIME ===============================
 * ============================================================ */

// Ticks to microseconds conversion
float calculateTicksToMicroseconds(uint32_t ticks) {
    return (float)ticks / (TIMERFREQ/1000000.0f);
}

// Ticks to milliseconds conversion
float calculateTicksToMilliseconds(uint32_t ticks) {
    return (float)ticks / (TIMERFREQ/1000000.0f);
}

    
/* ============================================================
 * ======================= VELOCITY ===========================
 * ============================================================ */

// calculate velocity with meter and seconds
float calculateVelocitySI(float distance_m, float time_s) {
    float velocity = distance_m / time_s; // Umrechnung in m/s
    return velocity;
}

// calculate velocity with millimeters and milliseconds
float calculateVelocityMilli(float distance_mm, float time_ms) {
    float velocity = distance_mm / time_ms;
    return velocity;
}

// calculate velocity with millimeters and microseconds
float calculateVelocityMicro(float distance_mm, float time_us) {
    float velocity = 1000 * distance_mm / time_us;
    return velocity;
}


/* ============================================================
 * ======================= ENERGY =============================
 * ============================================================ */

// calcuate Energy: E = 0.5 * m * v^2
float calculateEnergy(float velocity, float mass_kg) {
    float energy = 0.5f * (mass_kg) * velocity * velocity; // Energie in Joule
    return energy;
}


/* ============================================================
 * ======================= UNIT CONVERSION ====================
 * ============================================================ */

// convert velocity from m/s to ft/s
float convertVelocityMSToFTS(float velocity_ms) {
    return velocity_ms * 3.28084f; // conversion in ft/s
}