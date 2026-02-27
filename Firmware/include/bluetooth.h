#ifndef BLUETOOTH_H
#define BLUETOOTH_H

#include <Arduino.h>
#include <bluefruit.h>

//const char BLEname = 'OAC Hello 2';
extern BLEService BLE_oacService;

// BLE characters bidirectional (mainly to write smartphone --> NRF)
extern BLECharacteristic BLE_commandChar;   // To send commands from App --> target MCU
extern BLECharacteristic BLE_bbWeightChar;  // bbWeight to be changed via smartphone
extern BLECharacteristic BLE_syncTimeChar;  // sync date and time

// BLE characters uplink only (NRF --> smartphone)
extern BLECharacteristic BLE_liveDataChar;  // live update per shot
extern BLECharacteristic BLE_syncDataChar;  // sync updates per smartphone request
extern BLECharacteristic BLE_DeviceStatus;  // Battery, temperature, IMU

// Debug purpose, will be deleted later
extern BLECharacteristic BLE_fakeChar;

#endif