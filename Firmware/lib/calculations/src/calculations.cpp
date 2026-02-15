#include "calculations.h"

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

//calcuate Energie: E = 0.5 * m * v^2
float calculateEnergy(float velocity, float mass_kg) {
    float energy = 0.5f * (mass_kg) * velocity * velocity; // Energie in Joule
    return energy;
}
