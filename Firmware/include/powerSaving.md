# OpenAirSoftChrono - reduce current consumption


## Focus
Even though the nrf52 could make a lot of current saving techniques like different low power modes, with and without RAM retention, 
we shall keep the paretti principle in mind.

Compared to the nrf52 the current consumption is much bigger and/or easier to reduce:
- time of flight sensors (7 mA each)
- NRF52 DCDC converter
- Bluetooth
- external FLASH chip

<br>

## Time of flight sensors
- With data from the IMU sensor, we can detect movement and use that to activate/deactivate the sensors.
  - This may have the biggest impact, due to the high current cunsomption compared to the other functions

## NRF52 dcdc converter
The NRF52 has both internal LDO and dcdc convertert, of course latter is more efficient. 
But caution, the application board needs to bring external components to use it, otherwise the NRF may be briked.
```
// Add this at the very top of setup()
  NRF_POWER->DCDCEN = 1;
```
<br>

## Bluetooth
Even though it is low energy, we can reduce current consumption with some easy configs

### BLE transmit power
```void setup() {
  Bluefruit.begin();
  
  // Options: -40, -20, -16, -12, -8, -4, 0, +2, +3, +4, +8
  Bluefruit.setTxPower(0); // 0 dBm is a good balance for indoor use
}
```

### BLE advertising intervalls
```
void startAdv(void) {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(myService);
  Bluefruit.Advertising.addName();

  /* * ADV INTERVALS (Unit: 0.625 ms)
   * Fast: 32 (20 ms) - Very responsive, high power
   * Slow: 1600 (1 second) - Harder to find, extremely low power
   */
  Bluefruit.Advertising.setInterval(32, 1600);

  /*
   * TIMEOUTS
   * Stay in 'Fast' mode for 30 seconds, then switch to 'Slow' mode.
   * 0 = never stop advertising (keep slow mode forever).
   */
  Bluefruit.Advertising.setFastTimeout(30);

  // Start Advertising
  Bluefruit.Advertising.start(0); 
}
```

### BLE Connection parameters
```
// These values are in 1.25ms units
// 160 * 1.25 = 200ms interval
Bluefruit.configPrphConn(BLE_GATT_ATT_MTU_DEFAULT, BLE_GAP_EVENT_LENGTH_DEFAULT, 160, 160);
```

### BLE LED indicator
- The BLE stack seems to use the blue LED. Probably after BLE setup/start, the blue LED needs to be overwritten again.
- Or maybe
```
// Disable the Bluefruit status/connection LED
  Bluefruit.autoConnLed(false);
```


<br>
## external FLASH memory
It is meantioned that the external FLASH memory needs a command to go to low power mode (and the need to wake it up later).
```
#include <Adafruit_FlashTransport.h>

Adafruit_FlashTransport_QSPI flashTransport;

void setup() {
  flashTransport.begin();
  flashTransport.runCommand(0xB9); // Hex command for Deep Power Down
}
```
