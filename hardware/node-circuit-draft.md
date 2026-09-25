# Shared node circuit — A0

Electrical proposal, not a fabrication release. Native KiCad schematic and project are now generated in this directory. ERC passed with zero errors and warnings, and the exported netlist matched all model connections. Socket pin mapping is documented from the product graphic. Socket geometry, connector footprints, power capability and RF compatibility remain unverified. The model uses passive logical interfaces, so ERC cannot prove external-module electrical compatibility. Preview files are in preview/. Cosmetic text placement was adjusted after generation; regenerating from the model may require repeating that visual cleanup.

## Common circuitry

Use the user's ideaspark ESP32/ST7789 board in sockets. Power each node through its existing USB port from a wall adapter. Take peripheral power only from verified 5 V and 3.3 V header connections after checking the board's USB path and regulator capacity. No second power input is included in this draft.

Provide 100 nF plus 10 uF local bypass capacitors on the carrier's 3.3 V rail. These are provisional starting values; component tolerances and USB startup/inrush need review. Reserve all display signals. The carrier has no additional display connector.

The A1 schematic uses two physical 15-position socket symbols: J1 (left) and J8 (right). See [socket mapping](ideaspark-socket-map.md) for the original product graphic, viewing orientation and all 30 contact assignments. The earlier A0 logical 14-pin connector has been replaced. GPIO25 remains the proposed IR output; display GPIO23/18/15/2/4/32 remain reserved. Socket footprints and header-row spacing are not yet frozen.

## Sensor options

J2 provides 3.3 V, GND, SDA and SCL for a short-cable I2C sensor module. Optional 4.7 kohm pull-ups are not populated if the module already provides suitable pull-ups. J3 provides a separate 3.3 V/GND/data interface with an optional pull-up for a compatible single-data-wire sensor. Select the actual sensor before assembly; this is not a claim that every DHT11 variant is specified for 3.3 V. Keep the sensor away from processor/display heat and direct stove radiant heat when measuring room temperature.

## Stove population: IR transmitter

Circuit: verified USB 5 V -> R3 -> J4 pin 1 -> external LED anode; external LED cathode -> J4 pin 2 -> Q1 drain; Q1 source -> GND.

- Q1: AO3400A N-channel MOSFET candidate; gate=1, source=2, drain=3. Manufacturer specifies on-resistance at 2.5 V gate drive, supporting this 3.3 V logic application.
- R1: 100 ohm series gate resistor from GPIO25.
- R2: 100 kohm gate-to-ground resistor for default-off behavior during reset.
- R3: 100 ohm, 1%, 0.5 W initial LED current limit.
- External emitter candidate: Vishay TSAL6200, 940 nm. Connector polarity must be marked.
- C4/C5: 10 uF and 100 nF near the IR supply branch; route pulse-current return directly toward supply ground.

Initial estimate at 5 V with approximately 1.3 V LED drop: (5 - 1.3)/100 = 37 mA while on. This is an estimate, not a guaranteed current. At an assumed 5.25 V maximum source and 99 ohm minimum resistance, even a shorted LED limits resistor dissipation to 5.25^2/99 = 0.278 W before considering supply-path resistance; 0.5 W is the starting rating, subject to package and ambient derating. Do not increase current until LED thermal/pulse limits, driver waveform, power budget and required range are tested.

J5 provides 3.3 V/GND/receiver output with local 100 nF decoupling. The actual receiver part, carrier frequency and filtering are pending stove-remote verification. Do not assume all 3-pin receivers share pin order. The connector describes our cable contract, not a bare receiver package.

## Fan population: RF adapter

J6 is an eight-signal adapter contract, not a ready-to-plug radio footprint: 3.3 V, GND, TX-data/CS, RX-data/IRQ, SCLK, MOSI, MISO, AUX. It leaves room for simple data-controlled modules or an SPI radio, depending on the actual remote protocol. All signals must remain ESP32-compatible 3.3 V logic. Any voltage conversion belongs in the selected adapter design.

R8 is a 100 kohm pull-down for a simple active-high RF transmitter's data input only. Omit it for an SPI adapter, which needs its own correct CS/reset biasing. No universal radio biasing is assumed.

J7 optionally exposes 5 V and GND to the adapter through R7, a normally-unpopulated zero-ohm link. Populate only after confirming the radio power requirement and USB current budget. This is not a power input. Do not short this rail to the 3.3 V radio supply.

RF frequency, modulation, addressing and code format remain unknown. The existing 433 MHz/RCSwitch code does not prove compatibility. No RF module, antenna or adapter connector footprint is selected yet.

## Assembly variants

| Circuit | Stove node | Fan node |
| --- | --- | --- |
| Socketed ideaspark, local display, sensor and rail decoupling | Fit | Fit |
| Q1, R1-R3, J4/J5, C3-C5 | Fit | Omit |
| J6 RF interface | Omit | Fit after module selection |
| R8 simple-TX pull-down | Omit | Conditional |
| R7/J7 5 V RF branch | Omit | Conditional, normally omitted |

## Remaining validation

Confirm board header mapping and dimensions, verify available current, select sensor and receiver, identify fan RF protocol, assign connector parts/footprints, generate and inspect the native schematic, run ERC, and then place/route the carrier. Initial bench work should exercise transmission capture and node communication before controlling appliances.

## Sources

- AO3400A manufacturer datasheet: https://aosmd.com/res/data_sheets/AO3400A.pdf
- TSAL6200 manufacturer datasheet: https://www.vishay.com/docs/81010/tsal6200.pdf
- Board-linked display example: https://github.com/GJKJ/ESP32114LCD
