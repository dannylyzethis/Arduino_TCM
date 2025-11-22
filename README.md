# Arduino TCM - Smart Pellet Stove Controller

Automated pellet stove control system using ESP32 with DHT11 temperature/humidity monitoring and IR remote control. Features temperature-based automation and remote control via Blynk IoT platform.

## Features

✅ **Temperature & Humidity Monitoring** - Real-time DHT11 sensor readings
✅ **IR Learning** - Capture commands from your pellet stove's remote
✅ **PID-Style Heat Control** - Automatically adjusts heat level (1-5) based on temperature
✅ **Manual Control** - Remote stove on/off and heat adjustment via Blynk app
✅ **Smart Heat Adjustment** - Uses 5 heat levels instead of on/off cycling
✅ **Adjustable Cooldown** - User-configurable interval (5-30 min) between auto adjustments
✅ **Real-time Status** - Monitor stove state, heat level, and temperature from anywhere
✅ **Persistent Storage** - IR codes saved to flash memory (survive reboots)
✅ **Sensor Protection** - Auto-disables if temperature sensor fails
✅ **Heat Level Sync** - Manually sync software state with actual stove level

## Hardware Requirements

### Core Components
- **ESP32 development board**
- **DHT11** temperature and humidity sensor
- **VS1838B** IR receiver module (38kHz)
- **940nm IR LED** transmitter
- **2N2222 NPN transistor** (for IR LED)
- **100Ω resistor** (for IR LED current limiting)
- **10kΩ resistor** (for transistor base)
- Jumper wires
- Breadboard

### Optional
- Enclosure/case for permanent installation
- Power supply (USB or 5V adapter)

## Wiring Diagram

### DHT11 Temperature Sensor
```
DHT11          ESP32
─────────────────────
VCC     →      3.3V
GND     →      GND
DATA    →      GPIO4
```

### VS1838B IR Receiver
```
VS1838B        ESP32
─────────────────────
VCC     →      3.3V
GND     →      GND
OUT     →      GPIO14
```

### IR LED Transmitter Circuit
```
ESP32 GPIO15 ──┬─── 10kΩ ───┬─── Transistor Base (2N2222)
               │             │
              GND           Emitter → GND
                             │
                          Collector
                             │
                             ├─── IR LED Anode (+)
                             │
                           100Ω
                             │
                          5V/VIN

IR LED Cathode (-) → Transistor Collector
```

**Important Notes:**
- IR LED must be connected through a transistor (ESP32 GPIO can't provide enough current)
- Use 100Ω resistor to limit IR LED current (~40-50mA)
- Point IR LED directly at your pellet stove's IR receiver

## Software Requirements

### Arduino IDE Setup

1. Install [Arduino IDE](https://www.arduino.cc/en/software) (1.8.19 or later)
2. Add ESP32 board support:
   - Go to **File → Preferences**
   - Add to **Additional Board Manager URLs**:
     `https://dl.espressif.com/dl/package_esp32_index.json`
   - Go to **Tools → Board → Boards Manager**
   - Search for "ESP32" and install **"esp32 by Espressif Systems"**

### Required Libraries

Install via Arduino IDE (**Sketch → Include Library → Manage Libraries**):

| Library | Author | Purpose |
|---------|--------|---------|
| **Blynk** | Volodymyr Shymanskyy | IoT platform connectivity |
| **DHT sensor library** | Adafruit | DHT11 temperature/humidity |
| **Adafruit Unified Sensor** | Adafruit | DHT library dependency |
| **IRremoteESP8266** | David Conran, Sebastien Warin | IR transmit/receive |

## Blynk Setup

### 1. Create Blynk Project

1. Download [Blynk app](https://blynk.io/) (iOS/Android) or use [Blynk.Console](https://blynk.cloud/)
2. Create a new **Template** or **Project**
3. Note your **Template ID** and **Auth Token**

### 2. Add Datastreams

Configure these virtual pins in your Blynk template:

| Pin | Name | Type | Range | Purpose |
|-----|------|------|-------|---------|
| **V0** | Temperature | Value | 32-120°F | Current temperature display |
| **V2** | Humidity | Value | 0-100% | Current humidity display |
| **V3** | Stove ON | Button | 0-1 | Turn stove on |
| **V4** | Stove OFF | Button | 0-1 | Turn stove off |
| **V5** | Target Temp | Slider | 60-80°F | Temperature setpoint |
| **V6** | Auto Mode | Switch | 0-1 | Enable heat auto-adjustment |
| **V7** | Learn IR | Button | 0-1 | Enter learning mode |
| **V8** | Heat UP | Button | 0-1 | Manual heat increase |
| **V9** | Heat DOWN | Button | 0-1 | Manual heat decrease |
| **V10** | Heat Level | Value | 0-5 | Current heat level display |
| **V11** | Stove Status | LED/Value | 0-1 | Stove on/off indicator |
| **V12** | Cooldown Period | Slider | 5-30 min | Auto adjustment interval |
| **V13** | Heat Sync | Slider | 0-5 | Manual heat level sync |

### 3. Add Dashboard Widgets

**Recommended Dashboard Layout:**
- **Gauge** widget → V0 (Temperature in °F)
- **Gauge** widget → V2 (Humidity %)
- **Button** widget → V3 (Stove ON)
- **Button** widget → V4 (Stove OFF)
- **Slider** widget → V5 (Target Temperature: 60-80°F)
- **Switch** widget → V6 (Auto Mode)
- **Button** widget → V7 (Learn IR Codes)
- **Button** widget → V8 (Heat UP ↑)
- **Button** widget → V9 (Heat DOWN ↓)
- **Value Display** widget → V10 (Heat Level: 0-5)
- **LED** widget → V11 (Stove Status: On/Off)
- **Slider** widget → V12 (Cooldown Period: 5-30 minutes)
- **Slider** widget → V13 (Heat Level Sync: 0-5)

## Configuration

### Update Credentials

Edit `Arduino_TCM_Blynk_DHT11.ino` with your details:

```cpp
// Blynk credentials
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
char auth[] = "YOUR_AUTH_TOKEN";

// WiFi credentials
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";
```

### Adjust Pin Assignments (if needed)

```cpp
#define DHTPIN 4        // DHT11 sensor data pin
#define IR_RECV_PIN 14  // IR receiver output
#define IR_SEND_PIN 15  // IR LED control pin
```

### Set Temperature Preferences

```cpp
float tempSetpoint = 70.0;          // Default target temperature (°F)
float tempHysteresis = 2.0;         // Temperature buffer for heat adjustments
unsigned long adjustmentCooldown = 900000; // 15 minutes between adjustments (ms)
```

**How PID-Style Control Works:**
- Stove stays **ON** (you control on/off manually)
- Heat level **increases** when temp < `setpoint - 2°F` (e.g., < 68°F)
- Heat level **decreases** when temp > `setpoint + 2°F` (e.g., > 72°F)
- Adjustments happen at configurable intervals (default: 15 minutes)
- Cooldown period can be adjusted via Blynk slider (5-30 minutes)
- Heat levels range from 1 (minimum) to 5 (maximum)

## Installation & Setup

### Step 1: Upload Code

1. Open `Arduino_TCM_Blynk_DHT11/Arduino_TCM_Blynk_DHT11.ino` in Arduino IDE
2. Update credentials (see Configuration section)
3. Select board: **Tools → Board → ESP32 Arduino → ESP32 Dev Module**
4. Select COM port: **Tools → Port → (your port)**
5. Click **Upload** ⬆️
6. Open **Serial Monitor** (115200 baud) to verify connection

### Step 2: Learn IR Codes

**You must teach the system your pellet stove's remote commands:**

1. In **Blynk app**, tap the **"Learn IR"** button (V7)
2. Watch the **Serial Monitor** for instructions
3. When prompted, press these buttons on your **pellet stove remote**:
   - **POWER ON** button (point remote at VS1838B receiver)
   - Wait for confirmation ✓
   - **POWER OFF** button
   - Wait for confirmation ✓
   - **HEAT UP** button
   - Wait for confirmation ✓
   - **HEAT DOWN** button
   - Wait for confirmation ✓

4. Serial Monitor will display captured codes:
   ```
   === IR CODES SAVED ===
   Power ON: 0x1234ABCD
   Power OFF: 0x5678EF01
   Heat Up: 0x9ABC2345
   Heat Down: 0xDEF67890
   Learning mode complete!
   ```

5. **Codes are automatically saved to flash memory!**
   - IR codes persist through power cycles and reboots
   - No need to re-learn codes after restarting
   - Codes are loaded automatically on startup

**Note:** If you want to re-learn codes, just press the "Learn IR" button again - new codes will overwrite the old ones.

### Step 3: Test Manual Control

1. In Blynk app, tap **"Stove ON"** button (V3)
2. Watch your pellet stove turn on (starts at heat level 3)
3. Check **Stove Status** LED (V11) turns on
4. Check **Heat Level** display (V10) shows "3"
5. Test **Heat UP** (V8) and **Heat DOWN** (V9) buttons
6. Verify heat level changes (should range 1-5)
7. Test **"Stove OFF"** button (V4) to turn off

### Step 4: Enable Heat Automation

1. **Turn stove ON** manually using V3 button
2. Set desired **Target Temperature** using slider (V5) - e.g., 70°F
3. **Optional:** Adjust **Cooldown Period** slider (V12) - default is 15 minutes
4. Enable **Auto Mode** switch (V6)
5. System will automatically adjust heat level at your chosen interval:
   - **Increase heat** when temp < 68°F (up to level 5)
   - **Decrease heat** when temp > 72°F (down to level 1)
   - **Maintain** heat level when temp is 68-72°F

### Step 5: Sync Heat Level (If Needed)

If the system's heat level gets out of sync with your actual stove (e.g., after a reboot or manual remote changes):

1. Check your stove's actual heat level (look at the display)
2. Use **Heat Sync slider** (V13) to set the correct level (0-5)
3. System now knows the true stove state
4. Auto mode will work correctly from this point

**When to use:**
- After ESP32 reboots while stove is running
- If you manually changed heat using the physical remote
- If an IR command failed and states are mismatched

## Usage

### Manual Mode (Default)
- Use **Stove ON** (V3) and **Stove OFF** (V4) buttons to control power
- Use **Heat UP** (V8) and **Heat DOWN** (V9) to manually adjust heat (1-5)
- Monitor temperature, humidity, and current heat level in real-time
- Full manual control without any automation

### Auto Mode (Smart Heat Adjustment)
1. **Turn stove ON** manually first (V3)
2. Set target temperature with slider (V5)
3. Adjust cooldown period with slider (V12) - default 15 min, range 5-30 min
4. Enable Auto Mode switch (V6)
5. System automatically adjusts heat level at your chosen interval:
   - Too cold → Increases heat (up to level 5)
   - Too hot → Decreases heat (down to level 1)
   - Just right → Maintains current heat level
6. Can adjust cooldown period anytime (even while auto mode is running)
7. Can still manually adjust heat or turn off stove anytime
8. Auto mode disables when you turn stove OFF

### Monitoring
- **Temperature** updates every 2 minutes
- **Humidity** updates every 2 minutes
- **Heat level** (1-5) displays current setting
- **Stove status** LED shows on/off state
- Serial Monitor provides detailed automation logs

## How It Works

### PID-Style Heat Level Control

Unlike simple on/off thermostats, this system uses **proportional control** by adjusting heat levels:

```
Current Temp < Target - 2°F  →  Increase heat level (1→2→3→4→5)
Current Temp > Target + 2°F  →  Decrease heat level (5→4→3→2→1)
Within ±2°F of target       →  Maintain current heat level
```

**Example (Target = 70°F, Cooldown = 15 min):**
- Room at 65°F, Heat Level 2 → **Increases to Level 3** (15 min later)
- Room at 67°F, Heat Level 3 → **Increases to Level 4** (15 min later)
- Room at 69°F, Heat Level 4 → **Maintains Level 4** (in range)
- Room reaches 72°F → **Decreases to Level 3** (15 min later)
- System continuously adjusts to maintain comfort

**Why This is Better:**
- Pellet stoves are designed for continuous operation, not on/off cycling
- Smoother temperature control without temperature swings
- More efficient fuel usage
- Extends stove lifespan (less wear from start/stop cycles)

### Safety Features

- **Manual on/off control** - You control when stove turns on/off (not automated)
- **Sensor failure protection** - Auto mode disables after 3 consecutive sensor failures
- **IR code validation** - Won't send commands if codes not learned
- **Heat level limits** - Won't exceed min (1) or max (5) settings
- **Adjustable cooldown** - User-configurable intervals (5-30 min) prevent command spam
- **Manual override** - Can disable auto mode or adjust heat anytime
- **Auto-disable on OFF** - Auto mode turns off when stove is manually shut off
- **Persistent storage** - IR codes survive power outages and reboots

## Troubleshooting

### DHT11 Sensor Issues

**Symptom:** "Failed to read from DHT sensor!"

**Solutions:**
- ✓ Check wiring (VCC, GND, DATA)
- ✓ Ensure sensor has power (3.3V or 5V)
- ✓ Verify `DHTPIN` matches GPIO4
- ✓ Try adding 10kΩ pull-up resistor on data line

### IR Learning Fails

**Symptom:** No codes captured / "IR Signal Received!" not appearing

**Solutions:**
- ✓ Check VS1838B wiring (VCC, GND, OUT)
- ✓ Point remote **directly** at receiver (2-6 inches away)
- ✓ Replace remote batteries
- ✓ Verify receiver pin is GPIO14
- ✓ Check Serial Monitor is open (115200 baud)

### IR Commands Not Working

**Symptom:** Stove doesn't respond to commands

**Solutions:**
- ✓ Check IR LED wiring and transistor circuit
- ✓ Verify 100Ω resistor in series with IR LED
- ✓ Point IR LED directly at stove's receiver
- ✓ Reduce distance (try 1-3 feet initially)
- ✓ Check LED polarity (long leg = anode/+)
- ✓ Some stoves use different protocols - try adjusting:
  ```cpp
  irsend.sendNEC(code, 32);  // Try: sendSony, sendRC5, sendSamsung
  ```

### WiFi Connection Issues

**Symptom:** Can't connect to WiFi

**Solutions:**
- ✓ Verify SSID and password are correct
- ✓ ESP32 only supports **2.4GHz** networks (not 5GHz)
- ✓ Check WiFi signal strength
- ✓ Restart router if necessary

### Blynk Connection Issues

**Symptom:** "Not connected to Blynk"

**Solutions:**
- ✓ Verify Auth Token is correct (copy from Blynk.Console)
- ✓ Check Template ID matches
- ✓ Ensure virtual pins match datastream configuration
- ✓ Check Blynk server status at [status.blynk.cc](https://status.blynk.cc)

### Automation Not Adjusting Heat

**Symptom:** Auto mode enabled but heat level doesn't change

**Solutions:**
- ✓ **Stove must be ON** - Auto mode only works when stove is running
- ✓ Verify IR codes were learned successfully (Power, Heat Up, Heat Down)
- ✓ Check temperature is outside hysteresis range (±2°F)
- ✓ **Wait for cooldown period** - default is 15 minutes between adjustments
- ✓ Check Cooldown Period slider (V12) - may be set too high
- ✓ Ensure Auto Mode switch (V6) is ON
- ✓ Monitor Serial output for automation messages
- ✓ Check Heat Level display (V10) - should be 1-5
- ✓ Manually test Heat UP/DOWN buttons first
- ✓ If at max heat (5) and still cold, system won't increase further
- ✓ If at min heat (1) and still hot, system won't decrease further

## Advanced Customization

### Change Update Frequency

```cpp
// Default: 2 minutes (120,000 ms)
timer.setInterval(120000L, sendData);

// Change to 1 minute:
timer.setInterval(60000L, sendData);
```

### Adjust Temperature Control Parameters

```cpp
// Default: ±2°F buffer before adjusting heat
float tempHysteresis = 2.0;

// Tighter control (±1°F) - more frequent adjustments:
float tempHysteresis = 1.0;

// Wider buffer (±3°F) - less frequent adjustments:
float tempHysteresis = 3.0;

// Adjustment cooldown (default: 15 minutes)
unsigned long adjustmentCooldown = 900000; // milliseconds

// Note: Cooldown can also be adjusted via Blynk slider (V12)
// Range: 5-30 minutes (300000-1800000 ms)
```

**Recommended Cooldown Settings:**
- **5-10 minutes** - Quick response, smaller spaces, well-insulated
- **15 minutes** - Default, balanced for most pellet stoves
- **20-30 minutes** - Conservative, larger spaces, slow-response stoves

### Add More IR Commands

```cpp
// In setup, learn additional buttons:
uint64_t irCode_FanSpeed = 0;
uint64_t irCode_Timer = 0;

// Create new Blynk handlers:
BLYNK_WRITE(V8) {
  if (param.asInt() == 1) {
    sendIRCommand(irCode_FanSpeed);
  }
}
```

### Change IR Protocol

If your stove uses a different IR protocol, modify:

```cpp
// Line 70 - Change from NEC to your protocol:
irsend.sendSony(code, 12);      // Sony 12-bit
// or
irsend.sendRC5(code, 13);       // RC5
// or
irsend.sendSamsung(code, 32);   // Samsung
```

## Safety Disclaimer

⚠️ **IMPORTANT SAFETY INFORMATION** ⚠️

This is an automation project for educational purposes. When using with heating appliances:

- **Never leave automated stove unattended for extended periods**
- **Install smoke detectors and CO detectors near stove**
- **Ensure stove has proper clearances from combustible materials**
- **Follow all manufacturer safety guidelines**
- **Use at your own risk** - author assumes no liability
- **Test thoroughly** before relying on automation
- **Manual override should always be accessible**

## Project Structure

```
Arduino_TCM/
├── Arduino_TCM_Blynk_DHT11/
│   └── Arduino_TCM_Blynk_DHT11.ino    # Main sketch
└── README.md                           # This file
```

## Future Enhancements

Potential additions (contributions welcome!):
- [ ] EEPROM storage for learned IR codes (persist across reboots)
- [ ] Weekly temperature schedule
- [ ] Multiple temperature zones
- [ ] Push notifications (low temp alerts)
- [ ] Historical data logging
- [ ] Web dashboard
- [ ] Voice control integration (Alexa/Google Home)

## License

Open source - MIT License. Free to modify and use for personal projects.

## Support

For issues or questions:
- Check troubleshooting section above
- Review serial monitor output for debugging
- Verify all wiring matches diagrams
- Test components individually

---

**Built with:** ESP32 | DHT11 | VS1838B | IR LED | Blynk IoT
**Author:** Arduino TCM Project
**Version:** 2.1 (PID-style heat level control)
