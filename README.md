# Arduino TCM - Blynk DHT11 Temperature & Humidity Monitor

This project uses an ESP32 microcontroller with a DHT11 sensor to monitor temperature and humidity, and sends the data to the Blynk IoT platform for remote monitoring.

## Hardware Requirements

- ESP32 development board
- DHT11 temperature and humidity sensor
- Jumper wires
- Breadboard (optional)

## Wiring

Connect the DHT11 sensor to your ESP32:
- VCC → 3.3V or 5V
- GND → GND
- DATA → GPIO4 (or change `DHTPIN` in the code)

## Software Requirements

### Arduino IDE Setup

1. Install [Arduino IDE](https://www.arduino.cc/en/software)
2. Add ESP32 board support:
   - Go to File → Preferences
   - Add to Additional Board Manager URLs: `https://dl.espressif.com/dl/package_esp32_index.json`
   - Go to Tools → Board → Boards Manager
   - Search for "ESP32" and install "esp32 by Espressif Systems"

### Required Libraries

Install the following libraries via Arduino IDE (Sketch → Include Library → Manage Libraries):
- **Blynk** by Volodymyr Shymanskyy (search for "Blynk")
- **DHT sensor library** by Adafruit
- **Adafruit Unified Sensor** (dependency for DHT library)

## Blynk Setup

1. Download the Blynk app (iOS/Android) or use [Blynk.Console](https://blynk.cloud/)
2. Create a new project or template
3. Note your Template ID and Auth Token
4. Add two widgets:
   - Gauge or Value Display on Virtual Pin V0 (Temperature in °F)
   - Gauge or Value Display on Virtual Pin V2 (Humidity in %)

## Configuration

Before uploading the sketch, update these values in `Arduino_TCM_Blynk_DHT11.ino`:

```cpp
// Blynk credentials
#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
char auth[] = "YOUR_AUTH_TOKEN";

// WiFi credentials
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// DHT11 pin (if different)
#define DHTPIN 4  // Change if your sensor is on a different GPIO
```

## Upload and Run

1. Open `Arduino_TCM_Blynk_DHT11/Arduino_TCM_Blynk_DHT11.ino` in Arduino IDE
2. Select your ESP32 board: Tools → Board → ESP32 Arduino → (your board model)
3. Select the correct COM port: Tools → Port
4. Click Upload
5. Open Serial Monitor (115200 baud) to view debug output

## Features

- Reads temperature (°C and °F) and humidity from DHT11
- Sends data to Blynk every 5 minutes
- Serial monitor output for local debugging
- Automatic WiFi and Blynk reconnection

## Troubleshooting

**Failed to read from DHT sensor:**
- Check wiring connections
- Ensure DHT11 has power (3.3V or 5V)
- Verify `DHTPIN` matches your wiring

**WiFi connection issues:**
- Verify SSID and password are correct
- ESP32 only supports 2.4GHz WiFi networks
- Check signal strength

**Blynk connection issues:**
- Verify Auth Token is correct
- Check Template ID matches your project
- Ensure virtual pins match your Blynk dashboard widgets

## License

Open source - feel free to modify and use for your projects.
