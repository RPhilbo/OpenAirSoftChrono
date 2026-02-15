# OpenAirSoftChrono
Airsoft chronometer - open source - hardware + firmware + app

## Requierements
### Must have
- Reliable velocity measurement of airsoft BBs
    - based on a cheap and small evaluation board and custom sensor pcb for afordable diy solution
- 3D printed housing to use the device in a mock supressor (for hands free use)
- Measurement capabilities
    - BB speeds between 50 m/s and 250 m/s
    - Up to 3500 RPM (rounds per minute)

### Nice to have
- Battery powered
- Bluetooth low energy
    - to show the actual measurement result
    - to show the last x measurement results
    - to change the BB weight
- Logging of measurement results
    - persistent BB-Counter
    - persistent logging of measurements

<br><br>

# License

| Asset Type          | License     | Short Description     |
| :---                |    :----   |          :--- |
| Firmware (Code)     | CC BY-NC-SA 4.0                    | <ul><li>Attribution: You must give credit to the original author. </li><li> Non-Commercial: You may not use this material for commercial purposes. </li><li> ShareAlike: If you remix or build upon this work, you must distribute your contributions under the same license. </li></ul> |
| Hardware (PCB/CAD)  | CERN-OHL-S-2.0 - with NC notice        | <ul><li>Non-Commercial: You may not use this material for commercial purposes.</li></ul>      |
| Documentation       | CC BY-NC-SA 4.0                    | <ul><li>Attribution: You must give credit to the original author. </li><li> Non-Commercial: You may not use this material for commercial purposes. </li><li> ShareAlike: If you remix or build upon this work, you must distribute your contributions under the same license. </li></ul> |

<br><br>

# Notes for calculations and limits
## What are the numbers, we have to deal with?
### BB speed
- BB speed is assumed to be 50 m/s - 250 m/s
### BB weight
- BB weights between 0.2g - 0.5g
### BB diameter
- BB diameter 5.95mm
### Rate of Fire (normally rounds per minute)
Type | RoF [1/s] | Interval [ms]
--- | --- | ---
AEG | 1-60 | 25
GBB | 1-20 | 50

<br><br>

## Requirements derived from these numbers?
- The sensor has a time window to sense the BB, derived by the BBs diameter and speed.
- Velocity formula: $$v = \frac{d}{\Delta t}$$   rearranged to time:   $$\Delta t = \frac{d}{v}$$ 

### Velocity formula
$$v = \frac{d}{\Delta t}$$

### Rearranged to time
$$\Delta t = \frac{d}{v}$$

### With the above numbers:
- The distance between the two sensors is 20mm.

$$\Delta t = \frac{0.02\text{ m}}{250\text{ m/s}} \approx 80\text{ us}$$


### BB flyby time
- For simplification, we assume the sensor has a very narrow field of view. Therefore we assume the time to sense the BB is:

$$t = \frac{d}{v} = \frac{5.95 mm}{250 m/s} = 23.8 us$$

