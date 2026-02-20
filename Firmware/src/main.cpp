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
bool BLEisSyncing = false; // Flag to manage bulk transfer


/* ============================================================
 * ======================= BLE ================================
 * ============================================================ */
// Defining Bluetooth low energy device name and characteristics UUIDs
#define BLE_NAME "OAC Hello 2"
//const char BLEname = 'OAC Hello 2';
BLEService        BLE_oacService    = BLEService("19b10000-e8f2-537e-4f6c-d104768a1214");
BLECharacteristic BLE_commandChar   = BLECharacteristic("4242"); // Write 0x01 to sync

BLECharacteristic BLE_fakeChar      = BLECharacteristic("4243");
BLECharacteristic BLE_liveDataChar  = BLECharacteristic("4244"); // live update per shot
BLECharacteristic BLE_syncDataChar  = BLECharacteristic("4245"); // sync updates per smartphone request



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
static const float TofSensorDistance = 0.02f;
float BBWeight = 0.00036f;


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
void connect_callback(uint16_t conn_handle);
void disconnect_callback(uint16_t conn_handle, uint8_t reason);

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
    Serial.println("Heartbeat Task is alive");

    // Update the characteristic and NOTIFY the connected app
    /*if (Bluefruit.connected()) {
      fakeChar.notify32(FakeCounter);
      Serial.printf("Sent value: %d\n", FakeCounter);
    }

    FakeCounter++;*/
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
  /*static uint32_t absCounter = 0;

  LogEntry currentRead;
  currentRead.bbCounterAbsolute = ++absCounter;
  currentRead.speed             = (uint16_t)random(5000, 25000); 
  currentRead.weight            = (uint8_t)40;             
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
  /*if (Bluefruit.connected() && !BLEisSyncing) {
    BLE_liveDataChar.notify(&currentRead, sizeof(LogEntry));
  }*/

  // Handle Bulk Sync (If triggered)
  if (BLEisSyncing) {
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
    float energy12 = 0.5f * BBWeight * velocity12 * velocity12;
    
    ++BBCounter;

    LogEntry currentRead;
    currentRead.bbCounterAbsolute = BBCounter;
    currentRead.speed             = (uint16_t)random(5000, 25000); 
    currentRead.weight            = (uint8_t)40;             
    currentRead.temperature       = (int8_t)random(-10, 40); 
    currentRead.battery           = (uint8_t)random(42, 100);
    
    // write into RAM Buffer
    dataLog[head] = currentRead;
    head = (head + 1) % MAX_LOG_ENTRIES;

    // Serial Debug (UART)
    Serial.printf("[DEBUG] Cnt:%lu | Spd:%u | Wt:%u | Temp:%d | Bat:%u%%\n", 
                  currentRead.bbCounterAbsolute, 
                  currentRead.speed, 
                  currentRead.weight, 
                  currentRead.temperature, 
                  currentRead.battery);

    Serial.printf("BBC: %u | %.2f us | %.2f ms | v: %.2f m/s | E: %.3f J\n", BBCounter, timerMicroseconds, timerMilliseconds, velocity12, energy12);

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
  BLE_oacService.begin();

  // Set callbacks for every BLE connect and disconnect
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Setup the Characteristic for WRITING
  // CHR_PROPS_WRITE allows the phone to send data to the nRF52
  BLE_fakeChar.setProperties(CHR_PROPS_READ | CHR_PROPS_NOTIFY);
  BLE_fakeChar.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS); // Open read/write
  BLE_fakeChar.setFixedLen(4);
  BLE_fakeChar.setPresentationFormatDescriptor(BLE_GATT_CPF_FORMAT_UINT32,
                                            0x0,    // exponent: 0 (Value * 10^0)
                                            0x2700, // unit: 2700 "unitless"
                                            BLE_GATT_CPF_NAMESPACE_BTSIG,
                                            0x0000);  // description: 0 (None)
  BLE_fakeChar.begin();

  // Live Data: For real-time updates
  BLE_liveDataChar.setProperties(CHR_PROPS_NOTIFY);
  BLE_liveDataChar.setFixedLen(sizeof(LogEntry));
  BLE_liveDataChar.begin();

  // Command Char: Phone writes here to start Sync
  BLE_commandChar.setProperties(CHR_PROPS_WRITE);
  BLE_commandChar.setWriteCallback(onWriteCommand);
  BLE_commandChar.begin();

  // sync Data: To read the the RAM buffer to smartphone
  BLE_syncDataChar.setProperties(CHR_PROPS_NOTIFY);
  BLE_syncDataChar.setFixedLen(sizeof(LogEntry));
  BLE_syncDataChar.begin();
}

void BLEstartAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addService(BLE_oacService);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.start(0);
}


// Callback when phone writes to the Command Characteristic
void onWriteCommand(uint16_t conn_hdl, BLECharacteristic* chr, uint8_t* data, uint16_t len) {
  Serial.printf(">>> BLE new command received: 0x%02X\n", data[0]);
  if (len > 0 && data[0] == 0x01) {
    Serial.println(">>> BLE bulk sync requested by phone!");
    BLEisSyncing = true;
  }
}


void performFullSync() {
  Serial.println(">>> BLE starting bulk sync...");
  for (int i = 0; i < MAX_LOG_ENTRIES; i++) {
    int index = (head + i) % MAX_LOG_ENTRIES;
    
    // Check if entry exists (if buffer isn't full yet)
    if (dataLog[index].bbCounterAbsolute == 0) continue;

    while (!BLE_syncDataChar.notify(&dataLog[index], sizeof(LogEntry))) {
      delay(2); // Wait for BLE stack to clear
    }
  }
  Serial.println(">>> BLE bulk sync complete.");
  BLEisSyncing = false;
}

void connect_callback(uint16_t conn_handle) {
  // This code runs ONCE per new connection
  Serial.println(">>> BLE Client Connected!");
  
  // Reset your counter here
  //tripCounter = 0; 
  
  // Optional: You can get info about the phone
  BLEConnection* conn = Bluefruit.Connection(conn_handle);
  char peer_name[32] = { 0 };
  conn->getPeerName(peer_name, sizeof(peer_name));
  Serial.printf(">>> BLE Connected to: %s\n", peer_name);
}

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  Serial.printf(">>> BLE Disconnected, reason = 0x%02X\n", reason);
}