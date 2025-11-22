# Arduino TCM - Smart Pellet Stove Controller

Automated pellet stove control system using ESP32 with DHT11 temperature/humidity monitoring and IR remote control. Features temperature-based automation and remote control via Blynk IoT platform.

## Features

✅ **Temperature & Humidity Monitoring** - Real-time DHT11 sensor readings
✅ **IR Learning** - Capture commands from your pellet stove's remote
✅ **Automatic Climate Control** - Turn stove on/off based on temperature setpoint
✅ **Manual Control** - Remote stove operation via Blynk app
✅ **Smart Hysteresis** - Prevents rapid on/off cycling (±2°F buffer)
✅ **Real-time Status** - Monitor stove state and temperature from anywhere

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
| **V3** | Stove Power | Button | 0-1 | Manual power on/off |
| **V4** | Stove Status | Value | 0-1 | Stove state (on/off) |
| **V5** | Target Temp | Slider | 60-80°F | Temperature setpoint |
| **V6** | Auto Mode | Switch | 0-1 | Enable automation |
| **V7** | Learn IR | Button | 0-1 | Enter learning mode |

### 3. Add Dashboard Widgets

**Recommended Dashboard Layout:**
- **Gauge** widget → V0 (Temperature)
- **Gauge** widget → V2 (Humidity)
- **Button** widget → V3 (Stove Power - Manual)
- **LED** widget → V4 (Stove Status)
- **Slider** widget → V5 (Target Temperature: 60-80°F)
- **Switch** widget → V6 (Auto Mode)
- **Button** widget → V7 (Learn IR Codes)

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
float tempSetpoint = 70.0;      // Default target temperature (°F)
float tempHysteresis = 2.0;     // Temperature buffer (prevents flickering)
```

**How Hysteresis Works:**
- Stove turns **ON** when temp drops below `setpoint - 2°F` (e.g., 68°F)
- Stove turns **OFF** when temp rises above `setpoint + 2°F` (e.g., 72°F)
- Prevents constant on/off cycling

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
   - **POWER** button (point remote at VS1838B receiver)
   - Wait for confirmation ✓
   - **HEAT UP** button
   - Wait for confirmation ✓
   - **HEAT DOWN** button
   - Wait for confirmation ✓

4. Serial Monitor will display captured codes:
   ```
   === IR CODES SAVED ===
   Power: 0x1234ABCD
   Heat Up: 0x5678EF01
   Heat Down: 0x9ABC2345
   Learning mode complete!
   ```

5. **Optional:** Copy these codes and paste them in the sketch for permanent storage:
   ```cpp
   uint64_t irCode_Power = 0x1234ABCD;
   uint64_t irCode_HeatUp = 0x5678EF01;
   uint64_t irCode_HeatDown = 0x9ABC2345;
   ```

### Step 3: Test Manual Control

1. In Blynk app, tap **"Stove Power"** button (V3)
2. Watch your pellet stove respond
3. Check **Stove Status** LED (V4) updates
4. Test multiple times to ensure reliability

### Step 4: Enable Automation

1. Set desired **Target Temperature** using slider (V5) - e.g., 70°F
2. Enable **Auto Mode** switch (V6)
3. System will now automatically:
   - Turn stove **ON** when temp < 68°F
   - Turn stove **OFF** when temp > 72°F

## Usage

### Manual Mode
- Use **Stove Power** button in Blynk to control stove
- Monitor temperature and humidity in real-time
- Override automation at any time

### Auto Mode
- Set target temperature with slider
- Enable Auto Mode switch
- System maintains temperature automatically
- Can still use manual controls (auto mode temporarily pauses)

### Monitoring
- **Temperature** updates every 2 minutes
- **Humidity** updates every 2 minutes
- **Stove status** shows current state (On/Off)
- Serial Monitor provides detailed logs

## How It Works

### Temperature Control Logic

```
Current Temp < Target - 2°F  →  Turn stove ON
Current Temp > Target + 2°F  →  Turn stove OFF
Otherwise                    →  No change (maintain state)
```

**Example (Target = 70°F):**
- Room at 67°F → Stove turns **ON**
- Room heats to 72°F → Stove turns **OFF**
- Room cools to 68°F → Stove turns **ON** again

### Safety Features

- **Sensor failure detection** - Alerts if DHT11 stops responding
- **IR code validation** - Won't send if codes not learned
- **Manual override** - Can disable automation anytime
- **Hysteresis buffer** - Prevents rapid cycling

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

### Automation Not Triggering

**Symptom:** Auto mode enabled but stove doesn't turn on/off

**Solutions:**
- ✓ Verify IR codes were learned successfully
- ✓ Check temperature is outside hysteresis range
- ✓ Ensure Auto Mode switch (V6) is ON
- ✓ Monitor Serial output for automation messages
- ✓ Manually test stove power button first

## Advanced Customization

### Change Update Frequency

```cpp
// Default: 2 minutes (120,000 ms)
timer.setInterval(120000L, sendData);

// Change to 1 minute:
timer.setInterval(60000L, sendData);
```

### Adjust Hysteresis

```cpp
// Default: ±2°F buffer
float tempHysteresis = 2.0;

// Tighter control (±1°F):
float tempHysteresis = 1.0;

// Wider buffer (±3°F):
float tempHysteresis = 3.0;
```

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
**Version:** 2.0 (with IR automation)
