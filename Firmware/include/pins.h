#ifndef PINS_H
#define PINS_H

#include <Arduino.h>
#include <variant.h>

// ToF sensor enable pins
constexpr uint8_t TOF_SENSOR1_ENABLE = D9;
constexpr uint8_t TOF_SENSOR2_ENABLE = D8;
constexpr uint8_t TOF_SENSOR3_ENABLE = D19;

// ToF sensor output pins
constexpr uint8_t TOF_SENSOR1_OUTPUT = D10;
constexpr uint8_t TOF_SENSOR2_OUTPUT = D7;
constexpr uint8_t TOF_SENSOR3_OUTPUT = D18;

#endif