#include "bluetooth.h"

// Defining Bluetooth low energy device name and characteristics UUIDs
#define BLE_NAME "OpenAirsoftChrono "
//const char BLEname = 'OAC Hello 2';
BLEService        BLE_oacService    = BLEService("38473649-f72a-43bf-a6cd-31e0b2f7207d");

// BLE characters bidirectional (mainly to write smartphone --> NRF)
BLECharacteristic BLE_commandChar   = BLECharacteristic("3f2289d8-575c-43ea-b28c-8e2dc0074984"); // Write 0x01 to sync
BLECharacteristic BLE_bbWeightChar  = BLECharacteristic("c88b71aa-e965-4538-8b3a-67697b7cc774"); // bbWeight to be changed via smartphone
BLECharacteristic BLE_syncTimeChar  = BLECharacteristic("e49d43db-8007-4727-a239-e5143264fe4c"); // sync date and time

// BLE characters uplink only (NRF --> smartphone)
BLECharacteristic BLE_liveDataChar  = BLECharacteristic("52e57b10-74fd-42ba-8401-e00d4dc0463c"); // live update per shot
BLECharacteristic BLE_syncDataChar  = BLECharacteristic("ee39cb31-12f2-4666-b4b9-58e21ec77c34"); // sync updates per smartphone request
BLECharacteristic BLE_DeviceStatus  = BLECharacteristic("56606f53-c354-4e08-ad4e-2b74bf1bf0d0"); // Battery, temperature, IMU

// Debug purpose, will be deleted later
BLECharacteristic BLE_fakeChar      = BLECharacteristic("4249");



void BLEstartAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(BLE_oacService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
}

void BLE_connect_callback(uint16_t conn_handle) {
  // This code runs ONCE per new connection
  Serial.println(">>> BLE Client Connected!");
  
  // syncing the actual value of BBCounter at the moment of connection
  BLEliveSyncCounter = BBCounter;
  
  // Optional: You can get info about the phone
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  char peer_name[32] = { 0 };
  conn->getPeerName(peer_name, sizeof(peer_name));
  Serial.printf(">>> BLE Connected to: %s\n", peer_name);
}

void BLE_disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.printf(">>> BLE Disconnected, reason = 0x%02X\n", reason);
}