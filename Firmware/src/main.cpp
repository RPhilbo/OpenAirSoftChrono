#include <Arduino.h>
#include <variant.h>
#include <Wire.h>
#include <bluefruit.h>
#include <RTClib.h>

#include "battery.h"
#include "config.h"
#include "timer_control.h"

#define LED_OFF HIGH
#define LED_ON LOW

#define DEBUGxTaskGetStack

/* ============================================================
 * ======================= GLOBALS ============================
 * ============================================================ */

uint32_t part     = NRF_FICR->INFO.PART;
uint32_t variant  = NRF_FICR->INFO.VARIANT;
uint64_t UID      = (NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];
DateTime TimeNow;
float tempMCU;    // NRF52 internal Die temp. Expect an offset of 2-5K

/* ============================================================
 * ======================= Logging ============================
 * ============================================================ */

uint32_t  BBCounter = 0;
uint32_t  FakeCounter = 0;
#define MAX_LOG_ENTRIES 1000

// --- Packed Data Structure (Total: 9+2+6 Bytes) ---
struct __attribute__((packed)) LogEntry {
  uint32_t bbCounterAbsolute;   // 4 bytes
  uint16_t speed;               // 2 bytes
  uint8_t  weight;              // 1 byte
  int8_t   temperature;         // 1 byte
  uint8_t  battery;             // 1 byte
  uint16_t energy;              // 2 bytes    Can be reducted later, because calc out of two other log entries.
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hr;
  uint8_t min;
  uint8_t sec;
};

// Storage
LogEntry dataLog[MAX_LOG_ENTRIES];
int head = 0;           // Next write position
bool BLEaskForFullSync    = false; // Flag to manage bulk transfer
bool BLEaskForPartialSync = false; // Flag to manage partial transfer

uint32_t BLEliveSyncCounter = 0;

/* ============================================================
 * ======================= BLE ================================
 * ============================================================ */
// Defining Bluetooth low energy device name and characteristics UUIDs
#define BLE_NAME "OpenAirsoftChrono "
//const char BLEname = 'OAC Hello 2';
BLEService        BLE_oacService    = BLEService("19b10000-e8f2-537e-4f6c-d104768a1214");

// BLE characters with downlink possibility (getting data from smartphone)
BLECharacteristic BLE_commandChar   = BLECharacteristic("4242"); // Write 0x01 to sync
BLECharacteristic BLE_bbWeightChar  = BLECharacteristic("4243"); // bbWeight to be changed via smartphone

// BLE characters upnlink only (smartphone is limited to read)
BLECharacteristic BLE_liveDataChar  = BLECharacteristic("4244"); // live update per shot
BLECharacteristic BLE_syncDataChar  = BLECharacteristic("4245"); // sync updates per smartphone request
BLECharacteristic BLE_syncTimeChar  = BLECharacteristic("4246"); // sync date and time

BLECharacteristic BLE_fakeChar      = BLECharacteristic("4249");

/* ============================================================
 * ======================= Pins ===============================
 * ============================================================ */

#define LED_OFF HIGH
#define LED_ON LOW

#define ToF_Sensor1_Enable D9
#define ToF_Sensor2_Enable D8
#define ToF_Sensor3_Enable D19

#define ToF_Sensor1_Output D10
#define ToF_Sensor2_Output D7
#define ToF_Sensor3_Output D18


/* ============================================================
 * ======================= RTOS ===============================
 * ============================================================ */
// Task handlers to use multitasking
TaskHandle_t HeartbeatTaskHandle;
TaskHandle_t TofSensorCheckHandle;
TaskHandle_t TimerCheckAndEvaluateTaskHandle;
TaskHandle_t BLEsyncFakeTaskHandle;


// A "Start Pistol" to prevent running the tasks by using "xTaskCreate" in setup()
SemaphoreHandle_t startTasksSignal;




/* ============================================================
 * ======================= TIMER ==============================
 * ============================================================ */

extern NRF_TIMER_Type *timer;

// Create a software RTC instance
RTC_Millis rtc;

/* ============================================================
 * ======================= PHYSICS ============================
 * ============================================================ */

static const float TofSensorDistance = 0.02f;
uint8_t BBweight = 36;
float BBWeight_kg = (float)BBweight / 100000.0f;

uint8_t BatteryVoltage;   // The uint8_t value of battery voltage for logging and BLE.

/* ============================================================
 * ======================= PROTOTYPES =========================
 * ============================================================ */
// Function Prototypes
void HeartbeatTask(void *pvParameters);
void TofSensorCheckTask(void *pvParameters);
void BLEsyncFakeTask(void *pvParameters);

void TofSensorsEnableAll();

void TimerCheckAndEvaluate();

void BLEsetup();
void BLEstartAdv(void);
void BLE_commandCharCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void BLE_bbWeightCharCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void BLE_syncTimeCharCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void BLEperformFullSync();
void BLEperformPartialSync();

void CheckxTaskWatermark();
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

void getTimeNow();
void printTimeNow();

float getTempNRF();


/* ============================================================
 * ======================= SETUP ==============================
 * ============================================================ */

void setup() {
  pinMode(VBAT_ENABLE, OUTPUT);   digitalWrite(VBAT_ENABLE, LOW);   // Important when battery is charging, otherwise ADC_Pin may burn.

  delay(2000);
  Serial.begin(115200);
  //while(!Serial);
  delay(500);
  Serial.println("\nSetup Start");

  // Read the MCU part, variant and unique ID
  part = NRF_FICR->INFO.PART;
  variant = NRF_FICR->INFO.VARIANT;
  UID = (NRF_FICR->DEVICEID[1] << 32) | NRF_FICR->DEVICEID[0];
  Serial.printf("[HW] Part: %lx | Variant: %lx | UID: %lx \n", part, variant, UID);

  // Initialize RTC with a default time (Jan 1 2000)
    rtc.begin(DateTime(2000, 1, 1, 0, 0, 0));

  // Setup LED pins as OUTPUT and OFF
  pinMode(LED_RED, OUTPUT);   digitalWrite(LED_RED,   LED_OFF);
  pinMode(LED_GREEN, OUTPUT); digitalWrite(LED_GREEN, LED_OFF);
  pinMode(LED_BLUE, OUTPUT);  digitalWrite(LED_BLUE,  LED_OFF);

  // Setup the ToF sensor pins
  pinMode(ToF_Sensor1_Enable, OUTPUT);   digitalWrite(ToF_Sensor1_Enable, LOW);
  pinMode(ToF_Sensor2_Enable, OUTPUT);   digitalWrite(ToF_Sensor2_Enable, LOW);
  pinMode(ToF_Sensor3_Enable, OUTPUT);   digitalWrite(ToF_Sensor3_Enable, LOW);

  pinMode(ToF_Sensor1_Output, INPUT_PULLDOWN);
  pinMode(ToF_Sensor2_Output, INPUT_PULLDOWN);
  pinMode(ToF_Sensor3_Output, INPUT_PULLDOWN);


  // Enabling the ToF sensor pins.
  Serial.println("Enabling the ToF sensor pins");
  TofSensorsEnableAll();
  delay(50);

  // Preparing the timer
  Serial.println("Timer Setup");
  TimerSetup();
  delay(50);

  BLEsetup();
  BLEstartAdv();

  startTasksSignal = xSemaphoreCreateBinary(); // Create the "Pistol"

  // Create Task: Blinking1
  xTaskCreate(
      HeartbeatTask,        // Function name
      "Blinky",             // Name for debugging
      1024,                 // Stack size (in words)
      NULL,                 // Parameter to pass
      1,                    // Priority (Low)
      &HeartbeatTaskHandle  // Task handle
  );

  // Create Task: TofSensorCheckTask
  xTaskCreate(
      TofSensorCheckTask,   // Function name
      "TofPoll",            // Name for debugging
      1024,                 // Stack size (in words)
      NULL,                 // Parameter to pass
      2,                    // Priority
      &TofSensorCheckHandle // Task handle
  );

  // Create Task: BLEsyncFakeTask
  xTaskCreate(
      BLEsyncFakeTask,   // Function name
      "BLEsyncFake",        // Name for debugging
      1024,                 // Stack size (in words)
      NULL,                 // Parameter to pass
      4,                    // Priority
      &BLEsyncFakeTaskHandle // Task handle
  );

  Serial.println("Setup Pre End");

  

   

  Serial.println(" ");
  Serial.println("Setup End");
  Serial.println(" ");

  // Give the signal - this wakes up everyone waiting for it
  Serial.println("Pre startTasksSignal");
  xSemaphoreGive(startTasksSignal);
  Serial.println("Post startTasksSignal");
}


/* ============================================================
 * ======================= LOOP ===============================
 * ============================================================ */

void loop() {
  // put your main code here, to run repeatedly:
  TimerCheckAndEvaluate();
  vTaskDelay(pdMS_TO_TICKS(1)); // Non-blocking delay
}


/* ============================================================
 * ======================= RTOS Tasks =========================
 * ============================================================ */

// Task: Heartbeat with an LED and serial print
void HeartbeatTask(void *pvParameters) {
  // Wait here forever until setup gives the signal
  xSemaphoreTake(startTasksSignal, portMAX_DELAY);
  
  // Put the signal back so OTHER tasks can also pass the gate
  xSemaphoreGive(startTasksSignal);

  while (1) {
    digitalWrite(LED_RED, LED_ON);
    //Serial.println("Heartbeat Task is alive");

    // Measure the battery voltage
    //float BatteryVoltageFloat = readBatteryVoltage();
    //BatteryVoltage = uint8_t(BatteryVoltageFloat * 50);

    getTimeNow();

    if (FakeCounter == 10)
    {
      // Measure the battery voltage
      float BatteryVoltageFloat = readBatteryVoltage();
      BatteryVoltage = uint8_t(BatteryVoltageFloat * 50);

      // read the temperature from nrf
      tempMCU = getTempNRF();
      
      Serial.printf("Heartbeat Task - Battery Voltage: %.2f V | %.1f °C \n", BatteryVoltageFloat, tempMCU);

      //getTimeNow();
      printTimeNow();

      FakeCounter = 0;
    }
    
    //Serial.printf("Heartbeat Task - Battery Voltage: %.2f V \n", BatteryVoltageFloat);
    

    // Update the characteristic and NOTIFY the connected app
    /*if (Bluefruit.connected()) {
      fakeChar.notify32(FakeCounter);
      Serial.printf("Sent value: %d\n", FakeCounter);
    }*/

    FakeCounter++;
      vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
    digitalWrite(LED_RED, LED_OFF);
      vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  }
}

// RTOS task to check the sensor via polling (for debug purpose)
void TofSensorCheckTask(void *pvParameters) {
  // Wait here forever until setup gives the signal
  xSemaphoreTake(startTasksSignal, portMAX_DELAY);
  
  // Put the signal back so OTHER tasks can also pass the gate
  xSemaphoreGive(startTasksSignal);

  while (1) {
    if (digitalRead(ToF_Sensor1_Output)) {
      digitalWrite(LED_RED, LED_ON);
      Serial.println("Sensor 1 output is high");
    }
    else digitalWrite(LED_RED, LED_OFF);

    if (digitalRead(ToF_Sensor2_Output)) {
      digitalWrite(LED_GREEN, LED_ON);
      Serial.println("Sensor 2 output is high");
    }
    else digitalWrite(LED_GREEN, LED_OFF);

    if (digitalRead(ToF_Sensor3_Output)) {
      digitalWrite(LED_BLUE, LED_ON);
      Serial.println("Sensor 3 output is high");
    }
    else digitalWrite(LED_BLUE, LED_OFF);
    
    vTaskDelay(pdMS_TO_TICKS(300)); // Non-blocking delay
  }
}


void BLEsyncFakeTask(void *pvParameters) {
  // Wait here forever until setup gives the signal
  xSemaphoreTake(startTasksSignal, portMAX_DELAY);
  
  // Put the signal back so OTHER tasks can also pass the gate
  xSemaphoreGive(startTasksSignal);
  
  while (1) {

  // Wait here forever until setup gives the signal
  xSemaphoreTake(startTasksSignal, portMAX_DELAY);
  
  // Put the signal back so OTHER tasks can also pass the gate
  xSemaphoreGive(startTasksSignal);

  // Simulate Sensor Reading (Replace with your actual sensor code)
  /*static uint32_t absCounter = 0;

  LogEntry currentRead;
  currentRead.bbCounterAbsolute = ++absCounter;
  currentRead.speed             = (uint16_t)random(5000, 25000); 
  currentRead.weight            = BBweight;             
  currentRead.temperature       = (int8_t)random(-10, 40); 
  currentRead.battery           = (uint8_t)random(42, 100);
  
  // Save to RAM Buffer
  dataLog[head] = currentRead;
  head = (head + 1) % MAX_LOG_ENTRIES;

  // Serial Debug (UART)
  Serial.printf("[DEBUG] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u%%\n", 
                currentRead.bbCounterAbsolute, 
                currentRead.speed, 
                currentRead.weight, 
                currentRead.temperature, 
                currentRead.battery);*/

  // Real-time BLE Update (Notify if phone is listening)
  if (Bluefruit.connected() && !BLEaskForFullSync) {
      if (BBCounter > BLEliveSyncCounter) {
        BLE_liveDataChar.notify(&dataLog[BLEliveSyncCounter], sizeof(LogEntry));
        BLEliveSyncCounter++;
      }  
  //BLE_liveDataChar.notify(&currentRead, sizeof(LogEntry));
  }

  // Handle Bulk Sync (If triggered)
  if (BLEaskForFullSync) {
    BLEperformFullSync();
  }

  // Handle Partial Sync (If triggered)
  if (BLEaskForPartialSync) {
    BLEperformPartialSync();
  }


  vTaskDelay(pdMS_TO_TICKS(100)); // Non-blocking delay
  //delay((uint32_t)random(5000, 9000)); // Sample every x seconds
  }
}


/* ============================================================
 * ======================= METHODS ============================
 * ============================================================ */

// Enables the ToF sensors (and their current consumption)
void TofSensorsEnableAll() {
  Serial.println("\nEnabling the ToF sensor 1 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(ToF_Sensor1_Enable, HIGH);
  Serial.println("                ToF sensor 1 is enabled");

  Serial.println("\nEnabling the ToF sensor 2 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(ToF_Sensor2_Enable, HIGH);
  Serial.println("                ToF sensor 2 is enabled");

  Serial.println("\nEnabling the ToF sensor 3 after delay");
  vTaskDelay(pdMS_TO_TICKS(500)); // Non-blocking delay
  digitalWrite(ToF_Sensor3_Enable, HIGH);
  Serial.println("                ToF sensor 3 is enabled");
}


// Read Timer value, then calc and print value in microseconds
void TimerCheckAndEvaluate() {
  uint32_t ticks = timer->CC[0];
  
  if (ticks > 1000) {
    float timerMicroseconds = (float)ticks / 16.0f;
    //float timerMilliseconds = timerMicroseconds / 1000;

    //float velocity12 = ((float)20.0f / timerMicroseconds) * 1000.0f;
    float velocity12 = TofSensorDistance / (timerMicroseconds/1000000.0f);
    float energy12 = 0.5f * BBWeight_kg * velocity12 * velocity12;
    
    ++BBCounter;

    LogEntry currentRead;
    currentRead.bbCounterAbsolute = BBCounter;
    currentRead.speed             = (uint16_t)roundf(100 * velocity12);
    currentRead.weight            = BBweight;
    //currentRead.temperature       = (int8_t)random(-10, 40);
    currentRead.temperature       = (int8_t)roundf(tempMCU);
    //currentRead.battery           = (uint8_t)random(42, 100);
    currentRead.battery           = (uint8_t)BatteryVoltage;
    currentRead.energy            = (uint16_t)roundf(1000 * energy12);
    currentRead.year              = (uint8_t)(TimeNow.year() - 2000);
    currentRead.month             = TimeNow.month();
    currentRead.day               = TimeNow.day();
    currentRead.hr                = TimeNow.hour();
    currentRead.min               = TimeNow.minute();
    currentRead.sec               = TimeNow.second();
    
    // write into RAM buffer
    dataLog[head] = currentRead;
    head = (head + 1) % MAX_LOG_ENTRIES;

    // Serial Debug (UART)
    Serial.printf("[RAM DEBUG] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u | E: %u\n", 
                  currentRead.bbCounterAbsolute, 
                  currentRead.speed, 
                  currentRead.weight, 
                  currentRead.temperature, 
                  currentRead.battery,
                  currentRead.energy);

    Serial.printf("[Serial DEBUG] bbC: %u | %.2f us | v: %.2f m/s | %.2f g | E: %.3f J | Bat: %.2f V | 20%02d-%02d-%02d %02d:%02d:%02d \n",
                  BBCounter,
                  timerMicroseconds,
                  velocity12,
                  (float)(BBweight / 100.0f),
                  energy12,
                  (float)(BatteryVoltage / 50.f),
                  currentRead.year,
                  currentRead.month,
                  currentRead.day,
                  currentRead.hr,
                  currentRead.min,
                  currentRead.sec
                );

    // Timer reset for next measurement
    TimerReset();

    // Reset the timer for the next shot.
    void TimerReset();
  }  
}

void BLEsetup(void) {
  // Initialize Bluefruit
  Bluefruit.begin();
  Bluefruit.setTxPower(0);
  //Bluefruit.setName(BLE_NAME);

  // Using UID for unique BLE device name
  // XOR all 8 bytes of the UID together into one byte
  uint8_t hashedUID = 0;
  for (int i = 0; i < 64; i += 8) {
      hashedUID ^= (uint8_t)(UID >> i);
  }

  char bleName[24];
  sprintf(bleName, "OpenAirsoftChrono-#%02X", hashedUID);
  Bluefruit.setName(bleName);

  // 1. Set the PIN (Must be a 6-digit string)
  // If setPIN isn't recognized, check your Bluefruit version (Update via PlatformIO)
  Bluefruit.Security.setPIN("420815");
  Bluefruit.Security.setMITM(1);

  // 2. Configure IO Capabilities 
  // DISPLAY_ONLY tells the phone "I will show you a PIN, you type it in"
  Bluefruit.Security.setIOCaps(true, false, false);
  
  // 3. Enable Bonding (Saves the pairing so you don't type the PIN every time)
  //Bluefruit.Security.setPairingEnabled(true);



  // Setup Service & Characteristic
  BLE_oacService.begin();

  // Set callbacks for every BLE connect and disconnect
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Setup the Characteristic for WRITING
  // CHR_PROPS_WRITE allows the phone to send data to the nRF52
  BLE_fakeChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  BLE_fakeChar.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  BLE_fakeChar.setFixedLen(4);
  BLE_fakeChar.setPresentationFormatDescriptor(BLE_GATT_CPF_FORMAT_UINT32,
                                            0x0,    // exponent: 0 (Value * 10^0)
                                            0x2700, // unit: 2700 "unitless"
                                            BLE_GATT_CPF_NAMESPACE_BTSIG,
                                            0x0000);  // description: 0 (None)
  BLE_fakeChar.begin();

  // Live Data: For real-time updates
  BLE_liveDataChar.setProperties(CHR_PROPS_NOTIFY);
  BLE_liveDataChar.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  BLE_liveDataChar.setFixedLen(sizeof(LogEntry));
  BLE_liveDataChar.begin();

  // Command Char: Phone writes here to start Sync
  BLE_commandChar.setProperties(CHR_PROPS_WRITE);
  BLE_commandChar.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  BLE_commandChar.setWriteCallback(BLE_commandCharCallback);
  BLE_commandChar.begin();

  // BB Weight Char: Phone writes here to change the bbWeight
  BLE_bbWeightChar.setProperties(CHR_PROPS_WRITE);
  BLE_bbWeightChar.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  BLE_bbWeightChar.setWriteCallback(BLE_bbWeightCharCallback);
  BLE_bbWeightChar.begin();

  // sync Data: To read the the RAM buffer to smartphone
  BLE_syncDataChar.setProperties(CHR_PROPS_NOTIFY);
  BLE_syncDataChar.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  BLE_syncDataChar.setFixedLen(sizeof(LogEntry));
  BLE_syncDataChar.begin();

  // sync Date and Time
  BLE_syncTimeChar.setProperties(CHR_PROPS_WRITE);
  BLE_syncTimeChar.setPermission(SECMODE_ENC_WITH_MITM, SECMODE_ENC_WITH_MITM);
  BLE_syncTimeChar.setWriteCallback(BLE_syncTimeCharCallback);
  BLE_syncTimeChar.begin();
}

void BLEstartAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(BLE_oacService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
}


// Callback when phone writes to the Command Characteristic
void BLE_commandCharCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  Serial.printf(">>> BLE new command received: 0x%02X\n", data[0]);
  /*if (len > 0 && data[0] == 0x42) {
    Serial.println(">>> BLE bulk sync requested by phone!");
    BLEaskForFullSync = true;
  }*/
  if (len > 0) {
      switch(data[0]) {
    case 0x42:
      // full sync
      Serial.println(">>> BLE bulk sync requested by phone!");
      BLEaskForFullSync = true;
      break;

    case 0x43:
      // partial sync
      Serial.println(">>> BLE partial sync requested by phone!");
      BLEaskForPartialSync = true;
      break;

    default:
      Serial.println(">>> Faulty code?");
    }
  }
}

// Callback when phone writes to the bbWeight Characteristic
void BLE_bbWeightCharCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  Serial.printf(">>> BLE new BB Weight received: 0x%02X\n", data[0]);
  if (len == 0) return;

  // Define valid weights
  static const uint8_t validWeights[] = {12, 20, 23, 25, 28, 30, 32, 33, 35, 36, 40, 43, 45, 46};

    uint8_t val = data[0];
    bool found = false;

    for (uint8_t w : validWeights) {
      if (val == w) {
        BBweight = val; BBWeight_kg = (float)BBweight / 100000.0f;
        found = true;
        break;
      }
    }

    if (found) {
      Serial.printf(">>> BLE: Set BB weight to %.2f g\n", (float)(BBweight / 100.0f));
    } else {
      Serial.println(">>> Faulty weight code");
    }
}


void BLEperformFullSync() {
  Serial.println(">>> BLE starting bulk sync...");
  for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
    int index = (head + i) % MAX_LOG_ENTRIES;
    
    // Check if entry exists (if buffer isn't full yet)
    if (dataLog[index].bbCounterAbsolute == 0) continue;

    while (!BLE_syncDataChar.notify(&dataLog[index], sizeof(LogEntry))) {
      delay(2); // Wait for BLE stack to clear
    }

    // Serial Debug (UART)
    Serial.printf("[BLE bulk sync] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u%% | E: %u\n", 
                  dataLog[index].bbCounterAbsolute, 
                  dataLog[index].speed, 
                  dataLog[index].weight, 
                  dataLog[index].temperature, 
                  dataLog[index].battery,
                  dataLog[index].energy);
  }
  Serial.println(">>> BLE bulk sync complete.");
  BLEaskForFullSync = false;
}


// sync the last n log entries
void BLEperformPartialSync() {
  Serial.println(">>> BLE: starting partial sync...");
  for (uint32_t i = BBCounter-20; i < BBCounter; i++) {    
    // Check if entry exists (if buffer isn't full yet)
    if (dataLog[i].bbCounterAbsolute == 0) continue;

    while (!BLE_syncDataChar.notify(&dataLog[i], sizeof(LogEntry))) {
      delay(2); // Wait for BLE stack to clear
    }

    // Serial Debug (UART)
    Serial.printf("[BLE partial sync] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u | E: %u\n", 
                  dataLog[i].bbCounterAbsolute, 
                  dataLog[i].speed, 
                  dataLog[i].weight, 
                  dataLog[i].temperature, 
                  dataLog[i].battery,
                  dataLog[i].energy);
  }
  Serial.println(">>> BLE partial sync complete.");
  BLEaskForPartialSync = false;
}


void connect_callback(uint16_t conn_handle) {
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

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.printf(">>> BLE Disconnected, reason = 0x%02X\n", reason);
}

void BLE_syncTimeCharCallback(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  Serial.println(">>> BLE Time sync");

  if (len == 6)
  {
    uint16_t year = 2000 + data[0];
    uint8_t month = data[1];
    uint8_t day   = data[2];
    uint8_t hr    = data[3];
    uint8_t min   = data[4];
    uint8_t sec   = data[5];

    // Adjust the RTC to the phone's time
    rtc.adjust(DateTime(year, month, day, hr, min, sec));
    Serial.println("RTC Synced!");

    DateTime now = rtc.now();
    char buf[50];
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d \n", 
            now.year(), now.month(), now.day(), 
            now.hour(), now.minute(), now.second() );
    Serial.println(buf);
    //getTimeNow();
    //printTimeNow();
  }
}

void getTimeNow() {
  //DateTime now = rtc.now();
  TimeNow = rtc.now();

  /*char buf[50];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d \n", 
          TimeNow.year(), TimeNow.month(), TimeNow.day(), 
          TimeNow.hour(), TimeNow.minute(), TimeNow.second() );
  Serial.println(buf);*/
}

void printTimeNow() {
  //DateTime TimeNow = rtc.now();

  char buf[50];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d \n", 
          TimeNow.year(), TimeNow.month(), TimeNow.day(), 
          TimeNow.hour(), TimeNow.minute(), TimeNow.second() );
  Serial.println(buf);
}

// Function to read internal nRF52 temperature
float getTempNRF() {
    int32_t temp;
    // The internal sensor returns temperature in 0.25°C increments
    // sd_temp_get is used when Bluetooth is active
    if (sd_temp_get(&temp) == NRF_SUCCESS) {
        return ((float)(temp / 4.0) - 4.5f); 
    }
    return -99.0; // Error
}