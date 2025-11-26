# Blynk Dashboard Setup Guide

Complete guide for setting up your Blynk dashboard to control the Arduino TCM Smart Home Climate Controller.

---

## Table of Contents
- [Blynk Account Setup](#blynk-account-setup)
- [Virtual Pin Reference](#virtual-pin-reference)
- [Dashboard Widgets](#dashboard-widgets)
- [Widget Configuration Details](#widget-configuration-details)
- [Message Usage Optimization](#message-usage-optimization)
- [Troubleshooting](#troubleshooting)

---

## Blynk Account Setup

### 1. Create Blynk Account
1. Download **Blynk Legacy** app (not Blynk IoT) from App Store or Google Play
2. Create a free account
3. Create a new project:
   - **Project Name**: "Pellet Stove Control" (or your choice)
   - **Device**: ESP32
   - **Connection Type**: WiFi
4. You'll receive an **Auth Token** via email - this is already in your code

### 2. Get Auth Token
Your current Auth Token is already configured in the code:
```
39B95_5S2gbtY8hGCdPOEz4XOiyVqlpz
```

### 3. Free Tier Limits
- **Energy**: 2000 points (each widget costs energy)
- **Messages**: Unlimited on Blynk Legacy (but optimized to ~600/day in this project)

---

## Virtual Pin Reference

### Temperature & Humidity Sensors
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V0  | Value Display | Local temperature (°F) | Every 5 minutes |
| V2  | Value Display | Local humidity (%) | Every 5 minutes |
| V14 | Segmented Switch | Zone selector (0-3) | On change |
| V15 | Value Display | Remote zone temperature | Every 5 minutes |

### Pellet Stove Controls
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V3  | Button | Turn stove ON | On press |
| V4  | Button | Turn stove OFF | On press |
| V5  | Slider | Target temperature (60-80°F) | On change |
| V6  | Switch | Auto mode ON/OFF | On change |
| V8  | Button | Manual heat UP | On press |
| V9  | Button | Manual heat DOWN | On press |
| V10 | Value Display | Current heat level (1-5) | On change |
| V11 | LED | Stove status (On/Off) | On change |
| V12 | Slider | Cooldown period (5-30 min) | On change |
| V13 | Slider | Manual heat sync (0-5) | On change |

### IR Learning
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V7  | Button | IR learning mode | On press |

### Ceiling Fan Controls (433MHz RF)

#### Fan 1
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V16 | Button | Fan 1 OFF | On press |
| V17 | Button | Fan 1 LOW speed | On press |
| V18 | Button | Fan 1 MEDIUM speed | On press |
| V19 | Button | Fan 1 HIGH speed | On press |
| V20 | Button | Fan 1 Light toggle | On press |

#### Fan 2
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V21 | Button | Fan 2 OFF | On press |
| V22 | Button | Fan 2 LOW speed | On press |
| V23 | Button | Fan 2 MEDIUM speed | On press |
| V24 | Button | Fan 2 HIGH speed | On press |
| V25 | Button | Fan 2 Light toggle | On press |

#### Fan 3
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V26 | Button | Fan 3 OFF | On press |
| V27 | Button | Fan 3 LOW speed | On press |
| V28 | Button | Fan 3 MEDIUM speed | On press |
| V29 | Button | Fan 3 HIGH speed | On press |
| V30 | Button | Fan 3 Light toggle | On press |

#### RF Learning
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V31 | Button | RF learning mode | On press |

### Temperature Equalization Learning
| Pin | Widget Type | Purpose | Update Frequency |
|-----|-------------|---------|------------------|
| V32 | Switch | Auto-Equalization mode | On change |
| V33 | Slider | Temp difference threshold (1-10°F) | On change |
| V34 | Labeled Value | Learning status display | On change |
| V35 | Button | Reset all learning data | On press |

---

## Dashboard Widgets

Here's the recommended dashboard layout organized by function:

### Tab 1: Main Control (Home Screen)

#### Temperature Display Section
1. **Value Display** → V0
   - **Title**: "Room Temp"
   - **Units**: °F
   - **Decimals**: 1
   - **Size**: 2x1
   - **Font Size**: Large

2. **Value Display** → V2
   - **Title**: "Humidity"
   - **Units**: %
   - **Decimals**: 0
   - **Size**: 2x1

3. **Value Display** → V15
   - **Title**: "Remote Zone"
   - **Units**: °F
   - **Decimals**: 1
   - **Size**: 2x1

#### Stove Control Section
4. **LED** → V11
   - **Title**: "Stove Status"
   - **Size**: 1x1
   - **Color**: Red when ON, Gray when OFF

5. **Value Display** → V10
   - **Title**: "Heat Level"
   - **Units**: /5
   - **Size**: 2x1
   - **Font Size**: Large

6. **Slider** → V5
   - **Title**: "Target Temp"
   - **Range**: 60-80
   - **Step**: 1
   - **Size**: 3x1
   - **Send on release**: ON

7. **Switch** → V6
   - **Title**: "Auto Mode"
   - **Size**: 2x1

#### Quick Controls
8. **Button** → V3
   - **Label**: "STOVE ON"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Green

9. **Button** → V4
   - **Label**: "STOVE OFF"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Red

10. **Button** → V8
    - **Label**: "Heat +"
    - **Mode**: Push
    - **Size**: 1x1

11. **Button** → V9
    - **Label**: "Heat -"
    - **Mode**: Push
    - **Size**: 1x1

### Tab 2: Ceiling Fans

#### Fan 1 Controls
1. **Button** → V16
   - **Label**: "Fan 1 OFF"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Gray

2. **Button** → V17
   - **Label**: "Fan 1 LOW"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Green

3. **Button** → V18
   - **Label**: "Fan 1 MED"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Yellow

4. **Button** → V19
   - **Label**: "Fan 1 HIGH"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Red

5. **Button** → V20
   - **Label**: "Fan 1 Light"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Orange

#### Fan 2 Controls
6-10. **Same as Fan 1** but using V21-V25

#### Fan 3 Controls
11-15. **Same as Fan 1** but using V26-V30

### Tab 3: Advanced Settings

#### Zone Selection
1. **Segmented Switch** → V14
   - **Title**: "Thermostat Zone"
   - **Options**:
     - "Zone 0 (Main)"
     - "Zone 1 (Fan 1)"
     - "Zone 2 (Fan 2)"
     - "Zone 3 (Fan 3)"
   - **Size**: 4x1

#### Learning System
2. **Switch** → V32
   - **Title**: "Auto-Equalization"
   - **Size**: 3x1

3. **Slider** → V33
   - **Title**: "Temp Diff Threshold"
   - **Range**: 1-10
   - **Units**: °F
   - **Step**: 0.5
   - **Size**: 4x1

4. **Labeled Value** → V34
   - **Title**: "Learning Status"
   - **Size**: 4x1

5. **Button** → V35
   - **Label**: "Reset Learning"
   - **Mode**: Push
   - **Size**: 2x1
   - **Color**: Orange

#### System Settings
6. **Slider** → V12
   - **Title**: "Adjustment Cooldown"
   - **Range**: 5-30
   - **Units**: min
   - **Step**: 1
   - **Size**: 4x1
   - **Send on release**: ON

7. **Slider** → V13
   - **Title**: "Manual Heat Sync"
   - **Range**: 0-5
   - **Step**: 1
   - **Size**: 4x1
   - **Info**: Use if ESP32 heat level gets out of sync with actual stove

### Tab 4: Learning Mode (Setup Only)

#### IR Learning (One-time setup)
1. **Button** → V7
   - **Label**: "Learn IR Codes"
   - **Mode**: Push
   - **Size**: 3x1
   - **Color**: Blue
   - **Info**: Press, then follow Serial Monitor instructions

#### RF Learning (One-time setup)
2. **Button** → V31
   - **Label**: "Learn RF Codes"
   - **Mode**: Push
   - **Size**: 3x1
   - **Color**: Purple
   - **Info**: Press, then follow Serial Monitor for all 15 codes

#### Instructions Display
3. **Terminal** or **Labeled Value**
   - **Size**: 4x3
   - **Text**:
   ```
   IR LEARNING:
   1. Press "Learn IR Codes" button
   2. Point stove remote at receiver
   3. Press: Power ON, Power OFF,
      Heat UP, Heat DOWN
   4. Codes saved automatically

   RF LEARNING:
   1. Press "Learn RF Codes" button
   2. Press all buttons on each fan remote:
      Fan 1: OFF, LOW, MED, HIGH, LIGHT
      Fan 2: OFF, LOW, MED, HIGH, LIGHT
      Fan 3: OFF, LOW, MED, HIGH, LIGHT
   3. Codes saved automatically
   ```

---

## Widget Configuration Details

### Important Widget Settings

#### Buttons (V3, V4, V8, V9, V16-V30, V35)
- **Output**: 1
- **Mode**: PUSH (not SWITCH)
- **Send on release**: OFF
- **Write interval**: 100ms (default)

#### Switches (V6, V32)
- **Output**: 1 (ON) / 0 (OFF)
- **Mode**: SWITCH
- **Write interval**: 100ms

#### Sliders (V5, V12, V13, V33)
- **Send on release**: ON (recommended to reduce messages)
- **Write interval**: 100ms

#### Value Displays (V0, V2, V10, V15)
- **Reading frequency**: 1 second (display update)
- **Data comes from ESP32** (not polled by app)
- **Decimals**: 1 for temperature, 0 for others

#### LED (V11)
- **Color**: Red for ON, Gray for OFF
- **Threshold**: 1 (ON when value ≥ 1)

---

## Message Usage Optimization

### Current Configuration (Optimized)

Your system is configured to minimize Blynk message usage:

#### Message Frequency
- **Zone 0 (Local sensor)**: Every 5 minutes = **288 messages/day**
- **Zone 1 (Remote sensor)**: Every 5 minutes = **288 messages/day**
- **Zone 2 (Remote sensor)**: Every 5 minutes = **288 messages/day**
- **Zone 3 (Remote sensor)**: Every 5 minutes = **288 messages/day**
- **User actions** (buttons, sliders): ~50-100/day
- **Auto adjustments** (heat level changes): ~20-50/day

**Total: ~600-800 messages/day** ✅

### Previous Configuration (Before Optimization)
- Zone 0: Every 2 minutes = 720/day
- Zones 1-3: Every 30 seconds = 8,640/day
- **Total: ~10,000 messages/day** ❌

### Rate Limiting Details

The code implements smart rate limiting:
```cpp
// Blynk updates limited to every 5 minutes (300,000 ms)
const unsigned long BLYNK_UPDATE_INTERVAL = 300000;
```

- **ESP-NOW data**: Received every 30 seconds, but only sent to Blynk every 5 minutes
- **Local sensor**: Read and sent to Blynk every 5 minutes
- **LCD display**: Updated every 4 seconds (local only, no Blynk messages)
- **User actions**: Immediate (necessary for control)

### Adjusting Update Frequency

If you want to change the update frequency, edit this line in the code:
```cpp
const unsigned long BLYNK_UPDATE_INTERVAL = 300000;  // 5 minutes
```

Options:
- **1 minute**: `60000` (1,440 messages/day)
- **2 minutes**: `120000` (720 messages/day)
- **5 minutes**: `300000` (288 messages/day) ← Current
- **10 minutes**: `600000` (144 messages/day)

**Recommendation**: Keep at 5 minutes for good responsiveness without excessive usage.

---

## Troubleshooting

### Dashboard Not Updating

**Problem**: Temperature or status not updating in Blynk

**Solutions**:
1. Check WiFi connection on ESP32 (Serial Monitor shows WiFi status)
2. Use `WIFI` serial command to check connection
3. Verify Auth Token matches in code and app
4. Check Serial Monitor for "→ Blynk updated for Zone X" messages
5. Blynk Legacy servers occasionally have issues - wait a few minutes

### Buttons Not Working

**Problem**: Pressing buttons doesn't control stove/fans

**Solutions**:
1. Verify button mode is **PUSH** not **SWITCH**
2. Check IR/RF codes are learned (use V7/V31 learning mode)
3. Verify codes saved (Serial Monitor shows "✓ IR/RF codes loaded")
4. Check hardware connections (IR LED, RF transmitter)
5. Use serial commands to test: `ON`, `OFF`, `FAN1 LOW`, etc.

### Learning Mode Not Working

**Problem**: Can't learn IR or RF codes

**Solutions**:
1. Open Serial Monitor (115200 baud) to see learning progress
2. For IR: Point remote directly at receiver (GPIO14), very close (<6 inches)
3. For RF: May need RF receiver module connected to GPIO17
4. Press remote buttons firmly and hold for 1 second
5. If code shows as `0xFFFFFFFFFFFFFFFF`, press button again (repeat code)

### Out of Sync Heat Level

**Problem**: Blynk shows wrong heat level compared to actual stove

**Solutions**:
1. Use **V13 slider** (Manual Heat Sync) to sync ESP32 with actual stove level
2. After syncing, auto mode will work correctly
3. Alternatively, use serial command: `HEAT 3` (sets to level 3)

### Zone Temperatures Not Showing

**Problem**: Remote zones show 0.0°F or "OFFLINE"

**Solutions**:
1. Check remote sensor ESP32 is powered on
2. Verify `ZONE_ID` is set correctly in remote sensor code (1, 2, or 3)
3. Check main controller MAC address matches in remote sensor code
4. Serial Monitor shows "ESP-NOW: Received from Zone X" when working
5. Use `ZONES` serial command to check all zone status
6. Zones timeout after 5 minutes - check remote sensor DHT11 connections

### Auto Mode Won't Enable

**Problem**: Auto mode switch turns off immediately

**Possible Causes**:
1. **IR codes not learned** - Use V7 to learn codes first
2. **Stove is OFF** - Turn stove ON (V3) before enabling auto mode
3. **Sensor failure** - Check DHT11 sensor (use `TEMP` serial command)

### High Blynk Message Usage

**Problem**: Running out of message quota

**Solutions**:
1. Verify code has rate limiting (check `BLYNK_UPDATE_INTERVAL = 300000`)
2. Increase update interval to 10 minutes (`600000`)
3. Check for excessive button presses (each press = 1 message)
4. Use Serial commands for testing instead of Blynk app

---

## Energy Budget (Widget Costs)

Blynk Legacy free tier: **2000 energy points**

### Estimated Widget Costs
- Value Display: 100 energy
- Button: 200 energy
- Switch: 200 energy
- Slider: 200 energy
- LED: 100 energy
- Segmented Switch: 300 energy
- Labeled Value: 100 energy

### Recommended Dashboard: ~1800 energy

**Tab 1 (Main Control)**: ~1100 energy
- 3× Value Display (V0, V2, V15): 300
- 1× LED (V11): 100
- 2× Button (V3, V4): 400
- 2× Small Button (V8, V9): 400
- 2× Slider (V5, V10): 400
- 1× Switch (V6): 200

**Tab 2 (Fans)**: ~600 energy
- 15× Button (all fan controls): 600

**Tab 3 (Advanced)**: ~900 energy
- 1× Segmented Switch (V14): 300
- 3× Slider (V12, V13, V33): 600
- 1× Switch (V32): 200
- 1× Labeled Value (V34): 100
- 1× Button (V35): 200

**Total**: ~2600 energy (exceeds free tier)

### Budget-Friendly Option (~1400 energy)
- Use only Tab 1 and Tab 2
- Skip advanced settings (adjust via serial commands)
- Remove some decorative displays

---

## Quick Start Checklist

- [ ] 1. Install Blynk Legacy app
- [ ] 2. Create project with ESP32 device
- [ ] 3. Note Auth Token (already in code)
- [ ] 4. Create Tab 1: Main Control
  - [ ] Add temperature displays (V0, V2, V15)
  - [ ] Add stove controls (V3, V4, V5, V6, V11)
  - [ ] Add heat adjustment (V8, V9, V10)
- [ ] 5. Create Tab 2: Ceiling Fans
  - [ ] Add Fan 1 controls (V16-V20)
  - [ ] Add Fan 2 controls (V21-V25)
  - [ ] Add Fan 3 controls (V26-V30)
- [ ] 6. Upload code to ESP32
- [ ] 7. Open Serial Monitor (115200 baud)
- [ ] 8. Learn IR codes (V7 button + Serial Monitor)
- [ ] 9. Learn RF codes (V31 button + Serial Monitor)
- [ ] 10. Test stove control with V3/V4 buttons
- [ ] 11. Test fan controls
- [ ] 12. Set target temperature (V5)
- [ ] 13. Enable auto mode (V6)

---

## Support & Serial Commands

If Blynk isn't working, you can control everything via Serial Monitor (115200 baud):

### Essential Commands
```
HELP          - Show all commands
TEMP          - Test temperature sensor
STATUS        - Show system status
ZONES         - Show all zone temperatures
ON            - Turn stove on
OFF           - Turn stove off
HEAT 3        - Set heat level to 3
AUTO          - Enable auto mode
MANUAL        - Disable auto mode
TARGET 72     - Set target to 72°F
FAN1 LOW      - Set Fan 1 to LOW
WIFI          - Check WiFi status
```

For complete command list, type `HELP` in Serial Monitor.

---

**Last Updated**: November 2024
**Firmware Version**: Arduino_TCM v4.0 (4-Zone with Rate Limiting)
**Compatible with**: Blynk Legacy (not Blynk IoT)
