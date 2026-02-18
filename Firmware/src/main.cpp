#include <Arduino.h>
#include <variant.h>
#include <Wire.h>
#include <bluefruit.h>

#include "config.h"
#include "timer_control.h"

#define LED_OFF HIGH
#define LED_ON LOW

#define DEBUGxTaskGetStack


/* ============================================================
 * ======================= Logging ============================
 * ============================================================ */
uint32_t  BBCounter = 0;
uint32_t  FakeCounter = 0;
#define MAX_LOG_ENTRIES 1000

// --- Packed Data Structure (Total: 9 Bytes) ---
struct __attribute__((packed)) LogEntry {
  uint32_t bbCounterAbsolute;   // 4 bytes
  uint16_t speed;               // 2 bytes
  uint8_t  weight;              // 1 byte
  int8_t   temperature;         // 1 byte
  uint8_t  battery;             // 1 byte
  
};

// Storage
LogEntry dataLog[MAX_LOG_ENTRIES];
int head = 0;           // Next write position
bool isSyncing = false; // Flag to manage bulk transfer


/* ============================================================
 * ======================= BLE ================================
 * ============================================================ */
// Defining Bluetooth low energy device name and characteristics UUIDs
#define BLE_NAME "OAC Hello 2"
//const char BLEname = 'OAC Hello 2';
BLEService        oacService    = BLEService("19b10000-e8f2-537e-4f6c-d104768a1214");
BLECharacteristic fakeChar      = BLECharacteristic("19b10042-e8f2-537e-4f6c-d104768a1214");
BLECharacteristic liveDataChar  = BLECharacteristic("2A58"); // Live updates
BLECharacteristic commandChar   = BLECharacteristic("2A59"); // Write 0x01 to sync


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


/* ============================================================
 * ======================= PHYSICS ============================
 * ============================================================ */
static const float TofSensorDistance = 0.02f;   // [m]
float BBWeight_kg = 0.00036f;   // [kg]
uint8_t BBWeight = 36;          // BBWeight_kg * 100.000 to use uint8_t

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
void onWriteCommand(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len);
void performFullSync();

void CheckxTaskWatermark();

/* ============================================================
 * ======================= SETUP ==============================
 * ============================================================ */

void setup() {
  Serial.begin(115200);
  while(!Serial);
  delay(500);
  Serial.println("\nSetup Start");

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
  /*xTaskCreate(
      BLEsyncFakeTask,   // Function name
      "BLEsyncFake",        // Name for debugging
      1024,                 // Stack size (in words)
      NULL,                 // Parameter to pass
      4,                    // Priority
      &BLEsyncFakeTaskHandle // Task handle
  );*/

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
    Serial.println("Heartbeat Task is alive");

    // Update the characteristic and NOTIFY the connected app
    if (Bluefruit.connected()) {
      fakeChar.notify32(FakeCounter);
      Serial.printf("Sent value: %d\n", FakeCounter);
    }

    FakeCounter++;
      vTaskDelay(pdMS_TO_TICKS(1000)); // Non-blocking delay
    digitalWrite(LED_RED, LED_OFF);
      vTaskDelay(pdMS_TO_TICKS(10000)); // Non-blocking delay
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
  static uint32_t absCounter = 0;

  LogEntry LogCurrentSession;
  LogCurrentSession.bbCounterAbsolute = ++absCounter;
  LogCurrentSession.speed             = (uint16_t)random(5000, 25000); 
  LogCurrentSession.weight            = (uint8_t)40;             
  LogCurrentSession.temperature       = (int8_t)random(-10, 40); 
  LogCurrentSession.battery           = (uint8_t)random(42, 100);
  
  // Save to RAM Buffer
  dataLog[head] = LogCurrentSession;
  head = (head + 1) % MAX_LOG_ENTRIES;

  // Serial Debug (UART)
  Serial.printf("[DEBUG] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u%%\n", 
                LogCurrentSession.bbCounterAbsolute, 
                LogCurrentSession.speed, 
                LogCurrentSession.weight, 
                LogCurrentSession.temperature, 
                LogCurrentSession.battery);

  // Real-time BLE Update (Notify if phone is listening)
  if (Bluefruit.connected() && !isSyncing) {
    liveDataChar.notify(&LogCurrentSession, sizeof(LogEntry));
  }

  // Handle Bulk Sync (If triggered)
  if (isSyncing) {
    performFullSync();
  }


  vTaskDelay(pdMS_TO_TICKS(5000)); // Non-blocking delay
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
    float timerMilliseconds = timerMicroseconds / 1000;

    //float velocity12 = ((float)20.0f / timerMicroseconds) * 1000.0f;
    float velocity12 = TofSensorDistance / (timerMicroseconds/1000000.0f);
    float energy12 = 0.5f * BBWeight_kg * velocity12 * velocity12;
    
    ++BBCounter;

    Serial.printf("BBC: %u | %.2f us | %.2f ms | v: %.2f m/s | E: %.3f J\n", BBCounter, timerMicroseconds, timerMilliseconds, velocity12, energy12);

    // New RAM Logging Start
    LogEntry LogCurrentSession;
    LogCurrentSession.bbCounterAbsolute = BBCounter;
    LogCurrentSession.speed             = (uint16_t)roundf(velocity12 * 100);
    LogCurrentSession.weight            = (uint8_t)BBWeight;
    LogCurrentSession.temperature       = (int8_t)random(-10, 40);
    LogCurrentSession.battery           = (uint8_t)random(42, 100);
    
    // Save to RAM Buffer
    dataLog[head] = LogCurrentSession;
    head = (head + 1) % MAX_LOG_ENTRIES;

    // Serial Debug (UART)
    Serial.printf("[DEBUG] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u%%\n", 
                  LogCurrentSession.bbCounterAbsolute, 
                  LogCurrentSession.speed, 
                  LogCurrentSession.weight, 
                  LogCurrentSession.temperature, 
                  LogCurrentSession.battery);
      // New RAM Logging END


      // Timer reset for next measurement
      TimerReset();

      // Reset tht timer for the next shot.
      void TimerReset();
  }  
}

void BLEsetup(void) {
  // Initialize Bluefruit
  Bluefruit.begin();
  Bluefruit.setTxPower(4);
  Bluefruit.setName(BLE_NAME);

  // Setup Service & Characteristic
  oacService.begin();

  // Setup the Characteristic for WRITING
  // CHR_PROPS_WRITE allows the phone to send data to the nRF52
  fakeChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  fakeChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS); // Open read/write
  fakeChar.setFixedLen(4);
  fakeChar.setPresentationFormatDescriptor(BLE_GATT_CPF_FORMAT_UINT32,
                                            0x0,    // exponent: 0 (Value * 10^0)
                                            0x2700, // unit: 2700 "unitless"
                                            BLE_GATT_CPF_NAMESPACE_BTSIG,
                                            0x0000);  // description: 0 (None)
  fakeChar.begin();

  // Live Data: For real-time updates
  liveDataChar.setProperties(CHR_PROPS_NOTIFY);
  liveDataChar.setFixedLen(sizeof(LogEntry));
  liveDataChar.begin();

  // Command Char: Phone writes here to start Sync
  commandChar.setProperties(CHR_PROPS_WRITE);
  commandChar.setWriteCallback(onWriteCommand);
  commandChar.begin();
}

void BLEstartAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(oacService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
}


// Callback when phone writes to the Command Characteristic
void onWriteCommand(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  Serial.printf("New Command Received: 0x%02X\n", data[0]);
  if (len > 0 && data[0] == 0x01) {
    Serial.println(">>> Sync Requested by Phone!");
    isSyncing = true;
  }
}


void performFullSync() {
  Serial.println(">>> Starting Bulk Sync...");
  for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
    int index = (head + i) % MAX_LOG_ENTRIES;
    
    // Check if entry exists (if buffer isn't full yet)
    if (dataLog[index].bbCounterAbsolute == 0) continue;

    while (!liveDataChar.notify(&dataLog[index], sizeof(LogEntry))) {
      delay(2); // Wait for BLE stack to clear
    }
  }
  Serial.println(">>> Sync Complete.");
  isSyncing = false;
}
