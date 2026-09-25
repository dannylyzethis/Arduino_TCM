# A2 component and assembly plan

Engineering selection draft, not an orderable production BOM. Package selections below are design choices; exact resistor/capacitor manufacturer part numbers remain to be qualified. All 14 discrete components have installed KiCad footprints. The eight connector footprints remain unassigned.

| References | Quantity per fully populated board | Electrical specification target | Package | Stove | Fan |
| --- | ---: | --- | --- | --- | --- |
| R1 | 1 | 100 ohm, 1%, at least 0.125 W | 0805 | Fit | Omit |
| R2 | 1 | 100 kohm, 1%, at least 0.125 W | 0805 | Fit | Omit |
| R3 | 1 | 100 ohm, 1%, at least 0.5 W after ambient derating | 2010 | Fit | Omit |
| R4, R5 | 2 | 4.7 kohm, 1%, at least 0.125 W | 0805 | Only if I2C sensor needs pull-ups | Same |
| R6 | 1 | 4.7 kohm, 1%, at least 0.125 W | 0805 | Only for compatible single-data-wire sensor | Same |
| R7 | 1 | Zero-ohm link, current rating matched to selected radio | 0805 | Omit | Normally omit; populate only for approved 5 V adapter power |
| R8 | 1 | 100 kohm, 1%, at least 0.125 W | 0805 | Omit | Only for simple active-high RF TX; omit for SPI |
| C1 | 1 | 100 nF, X7R, 16 V minimum, 10% | 0805 | Fit | Fit |
| C2 | 1 | 10 uF nominal, X7R, 16 V minimum, 20% | 1210 | Fit | Fit |
| C3, C5 | 2 | 100 nF, X7R, 16 V minimum, 10% | 0805 | Fit | Omit |
| C4 | 1 | 10 uF nominal, X7R, 16 V minimum, 20% | 1210 | Fit | Omit |
| Q1 | 1 | AO3400A; gate pad 1, source 2, drain 3 | SOT-23-3 | Fit | Omit |

Capacitor nominal value and package do not guarantee effective capacitance at DC bias. Qualify exact capacitor parts against their curves. A 2010 footprint does not itself guarantee 0.5 W: choose a resistor whose rated and derated power satisfies the IR calculation in node-circuit-draft.md. R7 must not be selected solely by package size.

## Connector and external-module requirements

| Reference | Positions | Purpose | Selection status |
| --- | ---: | --- | --- |
| J1, J8 | 15 each | Left/right ideaspark sockets | Pin order mapped from original product image; pitch, spacing, mating height and footprint not frozen |
| J2 | 4 | I2C sensor: 3.3 V, GND, SDA, SCL | Connector/cable and sensor part pending |
| J3 | 3 | Alternative sensor: 3.3 V, GND, DATA | Populate only if selected sensor uses this interface |
| J4 | 2 | IR LED anode / switched cathode | Polarized connector desired; cable and part pending |
| J5 | 3 | IR receiver: 3.3 V, GND, output | Receiver part, protocol carrier and cable pending |
| J6 | 8 | RF adapter power and logic | Radio module, voltage conversion and connector pending |
| J7 | 2 | Optional RF 5 V branch | Populate only with approved R7/radio configuration |

External parts are not included in the 22 schematic components: one ideaspark board per node, selected temperature sensor, USB wall adapter/cable, stove IR emitter (TSAL6200 candidate) and receiver, fan radio adapter/antenna, enclosure and mounting hardware. The CrowPanel is a separate purchased assembly.

## Assembly control

DNP means do not populate. Conditional populations are specified here; the generic connector generator does not encode independent KiCad stove/fan assembly variants. Do not send an unfiltered schematic BOM to an assembler. Produce separate released BOMs after RF and sensor selection.

Default assumptions for first bench work: no R7 or J7; no R8 until the radio's idle behavior is known; no optional sensor pull-ups until the selected module is checked. Use current-limited bench checks of the verified power path before fitting powered peripherals.

## Remaining release gates

1. Confirm socket geometry and carrier enclosure clearance.
2. Verify USB-to-VIN behavior and available 3.3 V peripheral current on the actual ideaspark board; do not equate the VIN label with a verified USB output.
3. Select sensor and IR receiver; confirm voltage limits and stove protocol.
4. Identify fan remotes and verify RF frequency, modulation, addressing and replay compatibility.
5. Complete connector selection, footprint assignment, PCB placement, routing and DRC.
6. Build separate stove/fan assembly BOMs and validate startup, reset, communication loss and command deduplication.
