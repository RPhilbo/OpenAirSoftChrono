# OpenAirSoftChrono - reduce current consumption


## Focus
Even though the nrf52 could make a lot of current saving techniques like different low power modes, with and without RAM retention, 
we shall keep the paretti principle in mind.

Compared to the nrf52 the current consumption is much bigger and/or easier to reduce:
- time of flight sensors (2x 7mA)
- NRF52 DCDC converter
- Bluetooth
- external FLASH chip

<br><br>

## Time of flight sensors
- With data from the IMU sensor, we can detect movement and use that to activate/deactivate the sensors.
  - This may have the biggest impact, due to the high current cunsomption compared to the other functions

## NRF52 dcdc converter
The NRF52 has both internal LDO and dcdc convertert, of course latter is more efficient. 
But caution, the application board needs to bring external components to use it, otherwise the NRF may be briked.

## Bluetooth
Even though it is low energy, we can reduce current consumption with some easy configs
- transmit power
- advertising intervalls
