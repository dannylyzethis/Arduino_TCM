# Blynk Datastreams Configuration

Complete datastream definitions for Arduino TCM Smart Home Climate Controller.

---

## Datastream Table (CSV Format)

```csv
Pin,Name,Data Type,Min,Max,Default,Unit,Direction,Description
V0,Local Temperature,Double,0,120,70,°F,Read,Local DHT11 temperature sensor (Zone 0 - Main/Stove area)
V2,Local Humidity,Integer,0,100,50,%,Read,Local DHT11 humidity sensor
V3,Stove Power ON,Integer,0,1,0,,Write,Button to turn pellet stove ON
V4,Stove Power OFF,Integer,0,1,0,,Write,Button to turn pellet stove OFF
V5,Target Temperature,Double,60,80,70,°F,Write,Temperature setpoint for auto mode
V6,Auto Mode,Integer,0,1,0,,Read/Write,Enable/disable automatic temperature control
V7,IR Learning Mode,Integer,0,1,0,,Write,Activate IR code learning mode for stove remote
V8,Heat Level UP,Integer,0,1,0,,Write,Manually increase heat level (+1)
V9,Heat Level DOWN,Integer,0,1,0,,Write,Manually decrease heat level (-1)
V10,Current Heat Level,Integer,0,5,0,,Read,Current stove heat setting (0=OFF 1-5=heat levels)
V11,Stove Status,Integer,0,1,0,,Read,Stove power status (0=OFF 1=ON)
V12,Adjustment Cooldown,Integer,5,30,15,min,Write,Cooldown period between auto adjustments
V13,Manual Heat Sync,Integer,0,5,3,,Write,Manually sync heat level if out of sync with stove
V14,Thermostat Zone,Integer,0,3,0,,Write,Select which zone controls the thermostat (0=Main 1=Fan1 2=Fan2 3=Fan3)
V15,Remote Zone Temperature,Double,0,120,70,°F,Read,Temperature from selected remote zone
V16,Fan 1 OFF,Integer,0,1,0,,Write,Turn Fan 1 OFF
V17,Fan 1 LOW,Integer,0,1,0,,Write,Set Fan 1 to LOW speed
V18,Fan 1 MEDIUM,Integer,0,1,0,,Write,Set Fan 1 to MEDIUM speed
V19,Fan 1 HIGH,Integer,0,1,0,,Write,Set Fan 1 to HIGH speed
V20,Fan 1 Light,Integer,0,1,0,,Write,Toggle Fan 1 light
V21,Fan 2 OFF,Integer,0,1,0,,Write,Turn Fan 2 OFF
V22,Fan 2 LOW,Integer,0,1,0,,Write,Set Fan 2 to LOW speed
V23,Fan 2 MEDIUM,Integer,0,1,0,,Write,Set Fan 2 to MEDIUM speed
V24,Fan 2 HIGH,Integer,0,1,0,,Write,Set Fan 2 to HIGH speed
V25,Fan 2 Light,Integer,0,1,0,,Write,Toggle Fan 2 light
V26,Fan 3 OFF,Integer,0,1,0,,Write,Turn Fan 3 OFF
V27,Fan 3 LOW,Integer,0,1,0,,Write,Set Fan 3 to LOW speed
V28,Fan 3 MEDIUM,Integer,0,1,0,,Write,Set Fan 3 to MEDIUM speed
V29,Fan 3 HIGH,Integer,0,1,0,,Write,Set Fan 3 to HIGH speed
V30,Fan 3 Light,Integer,0,1,0,,Write,Toggle Fan 3 light
V31,RF Learning Mode,Integer,0,1,0,,Write,Activate RF code learning mode for fan remotes
V32,Auto Equalization,Integer,0,1,0,,Read/Write,Enable temperature equalization learning mode
V33,Temp Diff Threshold,Double,1,10,3,°F,Write,Temperature difference threshold for equalization
V34,Learning Status,String,,,System ready,,Read,Display learning system status and sample count
V35,Reset Learning Data,Integer,0,1,0,,Write,Reset all learned temperature patterns
```

---

## Individual Datastream Definitions

### Temperature & Humidity Sensors

#### V0 - Local Temperature
```json
{
  "pin": "V0",
  "name": "Local Temperature",
  "dataType": "double",
  "min": 0,
  "max": 120,
  "default": 70,
  "unit": "°F",
  "direction": "read",
  "updateFrequency": "Every 5 minutes",
  "description": "Main DHT11 temperature sensor reading from Zone 0 (stove/main area)"
}
```

#### V2 - Local Humidity
```json
{
  "pin": "V2",
  "name": "Local Humidity",
  "dataType": "integer",
  "min": 0,
  "max": 100,
  "default": 50,
  "unit": "%",
  "direction": "read",
  "updateFrequency": "Every 5 minutes",
  "description": "Main DHT11 humidity sensor reading"
}
```

#### V14 - Thermostat Zone Selector
```json
{
  "pin": "V14",
  "name": "Thermostat Zone",
  "dataType": "integer",
  "min": 0,
  "max": 3,
  "default": 0,
  "unit": "",
  "direction": "write",
  "options": ["Zone 0 (Main)", "Zone 1 (Fan 1)", "Zone 2 (Fan 2)", "Zone 3 (Fan 3)"],
  "description": "Select which zone temperature controls the pellet stove thermostat"
}
```

#### V15 - Remote Zone Temperature
```json
{
  "pin": "V15",
  "name": "Remote Zone Temperature",
  "dataType": "double",
  "min": 0,
  "max": 120,
  "default": 70,
  "unit": "°F",
  "direction": "read",
  "updateFrequency": "Every 5 minutes (per zone)",
  "description": "Temperature reading from remote ESP32 sensor (Zones 1-3)"
}
```

---

### Pellet Stove Control

#### V3 - Stove Power ON
```json
{
  "pin": "V3",
  "name": "Stove Power ON",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Turn pellet stove ON via IR remote command"
}
```

#### V4 - Stove Power OFF
```json
{
  "pin": "V4",
  "name": "Stove Power OFF",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Turn pellet stove OFF via IR remote command"
}
```

#### V5 - Target Temperature
```json
{
  "pin": "V5",
  "name": "Target Temperature",
  "dataType": "double",
  "min": 60,
  "max": 80,
  "default": 70,
  "unit": "°F",
  "step": 1,
  "direction": "write",
  "widgetType": "slider",
  "description": "Desired temperature setpoint for automatic mode"
}
```

#### V6 - Auto Mode
```json
{
  "pin": "V6",
  "name": "Auto Mode",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "read/write",
  "widgetType": "switch",
  "description": "Enable automatic temperature control (PID-style heat adjustment)"
}
```

#### V7 - IR Learning Mode
```json
{
  "pin": "V7",
  "name": "IR Learning Mode",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Activate IR learning to capture stove remote codes (one-time setup)"
}
```

#### V8 - Heat Level UP
```json
{
  "pin": "V8",
  "name": "Heat Level UP",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Manually increase heat level by 1 (1→2, 2→3, etc.)"
}
```

#### V9 - Heat Level DOWN
```json
{
  "pin": "V9",
  "name": "Heat Level DOWN",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Manually decrease heat level by 1 (5→4, 4→3, etc.)"
}
```

#### V10 - Current Heat Level
```json
{
  "pin": "V10",
  "name": "Current Heat Level",
  "dataType": "integer",
  "min": 0,
  "max": 5,
  "default": 0,
  "unit": "/5",
  "direction": "read",
  "description": "Current heat setting (0=OFF, 1=Low, 2-4=Medium, 5=High)"
}
```

#### V11 - Stove Status
```json
{
  "pin": "V11",
  "name": "Stove Status",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "read",
  "widgetType": "led",
  "description": "Stove power status (0=OFF, 1=ON)"
}
```

#### V12 - Adjustment Cooldown
```json
{
  "pin": "V12",
  "name": "Adjustment Cooldown",
  "dataType": "integer",
  "min": 5,
  "max": 30,
  "default": 15,
  "unit": "min",
  "step": 1,
  "direction": "write",
  "widgetType": "slider",
  "description": "Minimum time between automatic heat adjustments (prevents rapid cycling)"
}
```

#### V13 - Manual Heat Sync
```json
{
  "pin": "V13",
  "name": "Manual Heat Sync",
  "dataType": "integer",
  "min": 0,
  "max": 5,
  "default": 3,
  "unit": "",
  "step": 1,
  "direction": "write",
  "widgetType": "slider",
  "description": "Sync ESP32 heat level with actual stove if they become out of sync"
}
```

---

### Ceiling Fan Controls (433MHz RF)

#### V16 - Fan 1 OFF
```json
{
  "pin": "V16",
  "name": "Fan 1 OFF",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Turn ceiling Fan 1 OFF via RF remote code"
}
```

#### V17 - Fan 1 LOW
```json
{
  "pin": "V17",
  "name": "Fan 1 LOW",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Set ceiling Fan 1 to LOW speed via RF remote code"
}
```

#### V18 - Fan 1 MEDIUM
```json
{
  "pin": "V18",
  "name": "Fan 1 MEDIUM",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Set ceiling Fan 1 to MEDIUM speed via RF remote code"
}
```

#### V19 - Fan 1 HIGH
```json
{
  "pin": "V19",
  "name": "Fan 1 HIGH",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Set ceiling Fan 1 to HIGH speed via RF remote code"
}
```

#### V20 - Fan 1 Light
```json
{
  "pin": "V20",
  "name": "Fan 1 Light",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Toggle ceiling Fan 1 light via RF remote code"
}
```

#### V21-V25 - Fan 2 Controls
*Same structure as Fan 1 (V16-V20), controlling Fan 2*

#### V26-V30 - Fan 3 Controls
*Same structure as Fan 1 (V16-V20), controlling Fan 3*

#### V31 - RF Learning Mode
```json
{
  "pin": "V31",
  "name": "RF Learning Mode",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Activate RF learning to capture all fan remote codes (15 codes total)"
}
```

---

### Temperature Equalization Learning

#### V32 - Auto Equalization Mode
```json
{
  "pin": "V32",
  "name": "Auto Equalization",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "read/write",
  "widgetType": "switch",
  "description": "Enable automatic temperature equalization using learned fan patterns"
}
```

#### V33 - Temperature Difference Threshold
```json
{
  "pin": "V33",
  "name": "Temp Diff Threshold",
  "dataType": "double",
  "min": 1,
  "max": 10,
  "default": 3,
  "unit": "°F",
  "step": 0.5,
  "direction": "write",
  "widgetType": "slider",
  "description": "Temperature difference between zones that triggers equalization"
}
```

#### V34 - Learning Status
```json
{
  "pin": "V34",
  "name": "Learning Status",
  "dataType": "string",
  "min": null,
  "max": null,
  "default": "System ready",
  "unit": "",
  "direction": "read",
  "widgetType": "labeled_value",
  "description": "Display current learning progress and sample count"
}
```

#### V35 - Reset Learning Data
```json
{
  "pin": "V35",
  "name": "Reset Learning Data",
  "dataType": "integer",
  "min": 0,
  "max": 1,
  "default": 0,
  "unit": "",
  "direction": "write",
  "widgetType": "button",
  "buttonMode": "push",
  "description": "Clear all learned temperature patterns and start fresh"
}
```

---

## Simplified Format for Blynk AI

### Copy-paste format for AI assistants:

```
TEMPERATURE SENSORS:
- V0: Local Temperature (double, 0-120°F, read-only, updates every 5 min)
- V2: Local Humidity (integer, 0-100%, read-only, updates every 5 min)
- V14: Thermostat Zone Selector (integer, 0-3, write, options: Zone 0/1/2/3)
- V15: Remote Zone Temperature (double, 0-120°F, read-only, updates every 5 min)

STOVE CONTROLS:
- V3: Stove ON Button (integer, 0-1, write, push button)
- V4: Stove OFF Button (integer, 0-1, write, push button)
- V5: Target Temperature (double, 60-80°F, write, slider with step=1)
- V6: Auto Mode Switch (integer, 0-1, read/write, toggle switch)
- V7: IR Learning Button (integer, 0-1, write, push button)
- V8: Heat UP Button (integer, 0-1, write, push button)
- V9: Heat DOWN Button (integer, 0-1, write, push button)
- V10: Current Heat Level (integer, 0-5, read-only, display)
- V11: Stove Status LED (integer, 0-1, read-only, LED indicator)
- V12: Adjustment Cooldown (integer, 5-30 min, write, slider with step=1)
- V13: Manual Heat Sync (integer, 0-5, write, slider with step=1)

CEILING FANS (RF 433MHz):
Fan 1:
- V16: OFF (integer, 0-1, write, push button)
- V17: LOW (integer, 0-1, write, push button)
- V18: MEDIUM (integer, 0-1, write, push button)
- V19: HIGH (integer, 0-1, write, push button)
- V20: Light (integer, 0-1, write, push button)

Fan 2:
- V21: OFF (integer, 0-1, write, push button)
- V22: LOW (integer, 0-1, write, push button)
- V23: MEDIUM (integer, 0-1, write, push button)
- V24: HIGH (integer, 0-1, write, push button)
- V25: Light (integer, 0-1, write, push button)

Fan 3:
- V26: OFF (integer, 0-1, write, push button)
- V27: LOW (integer, 0-1, write, push button)
- V28: MEDIUM (integer, 0-1, write, push button)
- V29: HIGH (integer, 0-1, write, push button)
- V30: Light (integer, 0-1, write, push button)

RF Learning:
- V31: RF Learning Button (integer, 0-1, write, push button)

TEMPERATURE LEARNING:
- V32: Auto Equalization Switch (integer, 0-1, read/write, toggle switch)
- V33: Temp Diff Threshold (double, 1-10°F, write, slider with step=0.5)
- V34: Learning Status (string, read-only, text display)
- V35: Reset Learning Button (integer, 0-1, write, push button)
```

---

## Update Frequencies (Message Optimization)

| Datastream Type | Frequency | Daily Messages |
|----------------|-----------|----------------|
| Temperature sensors (V0, V2, V15) | Every 5 minutes | 288 per zone |
| Stove status updates (V10, V11) | On change only | ~20-50 |
| User button presses (V3-V9, V16-V35) | Immediate | ~50-100 |
| Auto adjustments | Every 15+ min | ~20-50 |
| **TOTAL** | **—** | **~600-800/day** |

---

## Data Direction Summary

**Read-only (ESP32 → Blynk):**
- V0, V2, V10, V11, V15, V34

**Write-only (Blynk → ESP32):**
- V3, V4, V7, V8, V9, V12, V13, V14, V16-V31, V33, V35

**Read/Write (Bidirectional):**
- V5, V6, V32

---

## Notes for Blynk AI Configuration

1. **All buttons use PUSH mode**, not SWITCH mode
2. **Sliders should "send on release"** to reduce message usage
3. **Temperature displays** show 1 decimal place
4. **Humidity displays** show 0 decimal places
5. **Heat level** is displayed as "X/5" format
6. **LED widget** (V11) uses threshold=1, color=red for ON, gray for OFF
7. **Update interval** for all read pins: 1 second (app display refresh)
8. **Data is pushed from ESP32**, not polled by Blynk

---

**Last Updated**: November 2024
**Firmware Version**: Arduino_TCM v4.0
**Total Datastreams**: 36 (V0-V35)
**Estimated Message Usage**: 600-800/day
