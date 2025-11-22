#define BLYNK_TEMPLATE_ID "TMPL24OkwKw_5"
#define BLYNK_TEMPLATE_NAME "test"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>

// Define DHT11 pin and type
#define DHTPIN 4        // GPIO4
#define DHTTYPE DHT11   // DHT11 sensor

// Define IR pins
#define IR_RECV_PIN 14  // GPIO14 for IR receiver (VS1838B)
#define IR_SEND_PIN 15  // GPIO15 for IR LED transmitter

// Blynk Auth Token
char auth[] = "39B95_5S2gbtY8hGCdPOEz4XOiyVqlpz";

// WiFi credentials
char ssid[] = "Pahouse 2.4";
char pass[] = "jennyjenny92";

// Virtual pins for Blynk
#define TEMP_VPIN V0            // Temperature (°F)
#define HUM_VPIN V2             // Humidity (%)
#define STOVE_ON_VPIN V3        // Stove ON button
#define STOVE_OFF_VPIN V4       // Stove OFF button
#define TEMP_SETPOINT_VPIN V5   // Target temperature setpoint
#define AUTO_MODE_VPIN V6       // Auto mode switch (0=Manual, 1=Auto)
#define LEARN_MODE_VPIN V7      // IR Learning mode button
#define MANUAL_HEAT_UP_VPIN V8  // Manual heat up button
#define MANUAL_HEAT_DOWN_VPIN V9 // Manual heat down button
#define HEAT_LEVEL_VPIN V10     // Current heat level display (1-5)
#define STOVE_STATUS_VPIN V11   // Stove status (On/Off)
#define COOLDOWN_VPIN V12       // Adjustment cooldown period (minutes)
#define HEAT_SYNC_VPIN V13      // Manual heat level sync (1-5)
#define ZONE_SELECT_VPIN V14    // Zone selector (0=Zone1, 1=Zone2, 2=Average)
#define REMOTE_TEMP_VPIN V15    // Remote zone temperature display

// Initialize components
DHT dht(DHTPIN, DHTTYPE);
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
BlynkTimer timer;
Preferences preferences;

// Stove control variables
bool stoveIsOn = false;
bool autoMode = false;
int currentHeatLevel = 3;           // Current heat level (1-5), starts at medium
float tempSetpoint = 70.0;          // Default target temperature in °F
float currentTemp = 0.0;
float tempHysteresis = 2.0;         // Temperature buffer for adjustment trigger
unsigned long lastAdjustmentTime = 0;
unsigned long adjustmentCooldown = 900000; // 15 minutes between auto adjustments (900000 ms)
int sensorFailCount = 0;            // Track consecutive sensor failures
const int MAX_SENSOR_FAILS = 3;     // Disable auto mode after this many failures

// Multi-zone temperature control
float localTemp = 0.0;              // Temperature from local DHT11 (Zone 1)
float remoteTemp = 0.0;             // Temperature from remote ESP32 (Zone 2)
int activeZone = 0;                 // 0=Zone1, 1=Zone2, 2=Average
unsigned long lastRemoteTempTime = 0;
const unsigned long REMOTE_TIMEOUT = 300000; // 5 minutes - consider remote offline if no update

// ESP-NOW data structure for receiving temperature
typedef struct {
  float temperature;
  float humidity;
  uint8_t sensorID;  // Identifier for multiple remote sensors
} RemoteSensorData;

RemoteSensorData incomingData;

// IR code storage (you'll capture these from your stove remote)
uint64_t irCode_PowerOn = 0;        // Store your stove's power ON button code
uint64_t irCode_PowerOff = 0;       // Store your stove's power OFF button code
uint64_t irCode_HeatUp = 0;         // Store heat increase code
uint64_t irCode_HeatDown = 0;       // Store heat decrease code
decode_type_t irProtocol = UNKNOWN; // Will be detected when learning

// Learning mode
bool learningMode = false;
int learningStep = 0;  // 0=PowerOn, 1=PowerOff, 2=HeatUp, 3=HeatDown

// Function to send IR command
void sendIRCommand(uint64_t code) {
  if (code == 0) {
    Serial.println("Error: IR code not set. Please learn codes first!");
    return;
  }

  Serial.print("Sending IR code: 0x");
  Serial.println(uint64ToString(code, HEX));

  // Send the code (adjust protocol if needed - NEC is common)
  irsend.sendNEC(code, 32);
  delay(100); // Small delay after sending
}

// Save IR codes to EEPROM/Flash
void saveIRCodes() {
  preferences.begin("stove", false);  // false = read/write mode
  preferences.putULong64("powerOn", irCode_PowerOn);
  preferences.putULong64("powerOff", irCode_PowerOff);
  preferences.putULong64("heatUp", irCode_HeatUp);
  preferences.putULong64("heatDown", irCode_HeatDown);
  preferences.end();
  Serial.println("IR codes saved to flash memory");
}

// Load IR codes from EEPROM/Flash
void loadIRCodes() {
  preferences.begin("stove", true);  // true = read-only mode
  irCode_PowerOn = preferences.getULong64("powerOn", 0);
  irCode_PowerOff = preferences.getULong64("powerOff", 0);
  irCode_HeatUp = preferences.getULong64("heatUp", 0);
  irCode_HeatDown = preferences.getULong64("heatDown", 0);
  preferences.end();

  if (irCode_PowerOn != 0 && irCode_PowerOff != 0) {
    Serial.println("✓ IR codes loaded from flash memory");
    Serial.print("  Power ON: 0x");
    Serial.println(uint64ToString(irCode_PowerOn, HEX));
    Serial.print("  Power OFF: 0x");
    Serial.println(uint64ToString(irCode_PowerOff, HEX));
    Serial.print("  Heat UP: 0x");
    Serial.println(uint64ToString(irCode_HeatUp, HEX));
    Serial.print("  Heat DOWN: 0x");
    Serial.println(uint64ToString(irCode_HeatDown, HEX));
  } else {
    Serial.println("No saved IR codes found - please use learning mode");
  }
}

// ESP-NOW callback when data is received from remote sensor
void onDataReceive(const uint8_t * mac, const uint8_t *incomingDataPtr, int len) {
  // Validate data size to prevent memory corruption
  if (len != sizeof(incomingData)) {
    Serial.print("ERROR: Received invalid ESP-NOW packet size: ");
    Serial.print(len);
    Serial.print(" bytes (expected ");
    Serial.print(sizeof(incomingData));
    Serial.println(" bytes)");
    return;
  }

  memcpy(&incomingData, incomingDataPtr, sizeof(incomingData));

  remoteTemp = incomingData.temperature;
  lastRemoteTempTime = millis();

  Serial.print("ESP-NOW: Received from Zone 2 - Temp: ");
  Serial.print(remoteTemp);
  Serial.print("°F, Humidity: ");
  Serial.print(incomingData.humidity);
  Serial.println("%");

  // Update Blynk with remote temperature
  Blynk.virtualWrite(REMOTE_TEMP_VPIN, remoteTemp);

  // Update current temp based on active zone
  updateActiveTemperature();
}

// Update current temperature based on selected zone
void updateActiveTemperature() {
  switch (activeZone) {
    case 0:  // Zone 1 (Local sensor)
      currentTemp = localTemp;
      break;
    case 1:  // Zone 2 (Remote sensor)
      // Check if remote data is recent
      if (millis() - lastRemoteTempTime < REMOTE_TIMEOUT) {
        currentTemp = remoteTemp;
      } else {
        Serial.println("WARNING: Remote sensor data stale, using local temp");
        currentTemp = localTemp;
      }
      break;
    case 2:  // Average of both zones
      if (millis() - lastRemoteTempTime < REMOTE_TIMEOUT) {
        currentTemp = (localTemp + remoteTemp) / 2.0;
      } else {
        Serial.println("WARNING: Remote sensor offline, using local temp only");
        currentTemp = localTemp;
      }
      break;
  }
}

// Manual stove power ON
void turnStoveOn() {
  if (!stoveIsOn) {
    sendIRCommand(irCode_PowerOn);
    stoveIsOn = true;
    currentHeatLevel = 3; // Start at medium heat

    Serial.println("Stove turned ON (Heat Level 3)");

    Blynk.virtualWrite(STOVE_STATUS_VPIN, 1);
    Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
  }
}

// Manual stove power OFF
void turnStoveOff() {
  if (stoveIsOn) {
    sendIRCommand(irCode_PowerOff);
    stoveIsOn = false;
    currentHeatLevel = 0;

    Serial.println("Stove turned OFF");

    Blynk.virtualWrite(STOVE_STATUS_VPIN, 0);
    Blynk.virtualWrite(HEAT_LEVEL_VPIN, 0);

    // Disable auto mode when manually turning off
    autoMode = false;
    Blynk.virtualWrite(AUTO_MODE_VPIN, 0);
  }
}

// Increase heat level
void increaseHeat() {
  if (!stoveIsOn) {
    Serial.println("Cannot increase heat - stove is OFF");
    return;
  }

  if (currentHeatLevel >= 5) {
    Serial.println("Already at maximum heat level (5)");
    return;
  }

  sendIRCommand(irCode_HeatUp);
  currentHeatLevel++;

  Serial.print("Heat level increased to: ");
  Serial.println(currentHeatLevel);

  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
}

// Decrease heat level
void decreaseHeat() {
  if (!stoveIsOn) {
    Serial.println("Cannot decrease heat - stove is OFF");
    return;
  }

  if (currentHeatLevel <= 1) {
    Serial.println("Already at minimum heat level (1)");
    return;
  }

  sendIRCommand(irCode_HeatDown);
  currentHeatLevel--;

  Serial.print("Heat level decreased to: ");
  Serial.println(currentHeatLevel);

  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
}

// IR Learning function
void checkForIRSignal() {
  if (!learningMode) return;

  if (irrecv.decode(&results)) {
    Serial.println("IR Signal Received!");
    Serial.print("Protocol: ");
    Serial.println(typeToString(results.decode_type));
    Serial.print("Code: 0x");
    Serial.println(uint64ToString(results.value, HEX));
    Serial.print("Bits: ");
    Serial.println(results.bits);

    // Store the code based on learning step
    if (results.value != 0xFFFFFFFFFFFFFFFF) { // Ignore repeat codes
      switch(learningStep) {
        case 0:
          irCode_PowerOn = results.value;
          irProtocol = results.decode_type;
          Serial.println("✓ Power ON learned! Now press POWER OFF on your remote...");
          learningStep = 1;
          break;
        case 1:
          irCode_PowerOff = results.value;
          Serial.println("✓ Power OFF learned! Now press HEAT UP on your remote...");
          learningStep = 2;
          break;
        case 2:
          irCode_HeatUp = results.value;
          Serial.println("✓ Heat Up learned! Now press HEAT DOWN on your remote...");
          learningStep = 3;
          break;
        case 3:
          irCode_HeatDown = results.value;
          Serial.println("✓ Heat Down learned! All codes captured!");
          Serial.println("\n=== IR CODES SAVED ===");
          Serial.print("Power ON: 0x");
          Serial.println(uint64ToString(irCode_PowerOn, HEX));
          Serial.print("Power OFF: 0x");
          Serial.println(uint64ToString(irCode_PowerOff, HEX));
          Serial.print("Heat Up: 0x");
          Serial.println(uint64ToString(irCode_HeatUp, HEX));
          Serial.print("Heat Down: 0x");
          Serial.println(uint64ToString(irCode_HeatDown, HEX));

          // Save codes to flash memory
          saveIRCodes();

          Serial.println("Learning mode complete!");
          learningMode = false;
          learningStep = 0;
          Blynk.virtualWrite(LEARN_MODE_VPIN, 0);
          break;
      }
    }

    irrecv.resume(); // Ready for next signal
  }
}

// PID-style temperature control using heat levels
void checkTemperatureControl() {
  if (!autoMode || !stoveIsOn) return;

  // Check if cooldown period has passed
  unsigned long currentTime = millis();
  if (currentTime - lastAdjustmentTime < adjustmentCooldown) {
    return; // Still in cooldown, don't adjust yet
  }

  float tempError = tempSetpoint - currentTemp;

  // If temperature is too low, increase heat
  if (tempError > tempHysteresis) {
    if (currentHeatLevel < 5) {
      Serial.print("AUTO: Temp too low (");
      Serial.print(currentTemp);
      Serial.print("°F < ");
      Serial.print(tempSetpoint);
      Serial.println("°F) - Increasing heat");
      increaseHeat();
      lastAdjustmentTime = currentTime;
    } else {
      Serial.println("AUTO: Temp low but already at max heat (5)");
    }
  }
  // If temperature is too high, decrease heat
  else if (tempError < -tempHysteresis) {
    if (currentHeatLevel > 1) {
      Serial.print("AUTO: Temp too high (");
      Serial.print(currentTemp);
      Serial.print("°F > ");
      Serial.print(tempSetpoint);
      Serial.println("°F) - Decreasing heat");
      decreaseHeat();
      lastAdjustmentTime = currentTime;
    } else {
      Serial.println("AUTO: Temp high but already at min heat (1)");
    }
  }
  // Temperature is within acceptable range
  else {
    Serial.print("AUTO: Temperature OK (");
    Serial.print(currentTemp);
    Serial.print("°F, target: ");
    Serial.print(tempSetpoint);
    Serial.print("°F, level: ");
    Serial.print(currentHeatLevel);
    Serial.println(")");
  }
}

// Read and send sensor data
void sendData() {
  // Read temperature and humidity from DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = (t * 9.0 / 5.0) + 32.0;

  // Check if readings failed
  if (isnan(h) || isnan(t)) {
    sensorFailCount++;
    Serial.print("Failed to read from DHT sensor! (Fail count: ");
    Serial.print(sensorFailCount);
    Serial.print("/");
    Serial.print(MAX_SENSOR_FAILS);
    Serial.println(")");

    // Disable auto mode if sensor fails too many times
    if (sensorFailCount >= MAX_SENSOR_FAILS && autoMode) {
      Serial.println("CRITICAL: Sensor failed multiple times - disabling auto mode for safety");
      autoMode = false;
      Blynk.virtualWrite(AUTO_MODE_VPIN, 0);
    }
    return;
  }

  // Reset fail counter on successful read
  sensorFailCount = 0;
  localTemp = f; // Store local temperature

  // Update current temperature based on active zone
  updateActiveTemperature();

  // Send data to Blynk app
  Blynk.virtualWrite(TEMP_VPIN, f);
  Blynk.virtualWrite(HUM_VPIN, h);

  // Print readings to serial monitor
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.print(" °C / ");
  Serial.print(f);
  Serial.println(" °F");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");

  if (stoveIsOn) {
    Serial.print("Stove: ON | Heat Level: ");
    Serial.print(currentHeatLevel);
    if (autoMode) {
      Serial.print(" | AUTO Mode | Target: ");
      Serial.print(tempSetpoint);
      Serial.print("°F");
    } else {
      Serial.print(" | MANUAL Mode");
    }
    Serial.println();
  } else {
    Serial.println("Stove: OFF");
  }

  // Check if automation should adjust heat level
  checkTemperatureControl();
}

// Blynk: Stove ON button
BLYNK_WRITE(STOVE_ON_VPIN) {
  if (param.asInt() == 1 && !learningMode) {
    turnStoveOn();
  }
}

// Blynk: Stove OFF button
BLYNK_WRITE(STOVE_OFF_VPIN) {
  if (param.asInt() == 1 && !learningMode) {
    turnStoveOff();
  }
}

// Blynk: Manual Heat Up button
BLYNK_WRITE(MANUAL_HEAT_UP_VPIN) {
  if (param.asInt() == 1 && !learningMode) {
    increaseHeat();
  }
}

// Blynk: Manual Heat Down button
BLYNK_WRITE(MANUAL_HEAT_DOWN_VPIN) {
  if (param.asInt() == 1 && !learningMode) {
    decreaseHeat();
  }
}

// Blynk: Temperature setpoint slider
BLYNK_WRITE(TEMP_SETPOINT_VPIN) {
  tempSetpoint = param.asFloat();
  Serial.print("Target temperature set to: ");
  Serial.print(tempSetpoint);
  Serial.println(" °F");
}

// Blynk: Auto mode switch
BLYNK_WRITE(AUTO_MODE_VPIN) {
  autoMode = param.asInt();
  Serial.print("Auto mode: ");
  Serial.println(autoMode ? "ENABLED" : "DISABLED");

  if (autoMode) {
    // Check if IR codes have been learned
    if (irCode_HeatUp == 0 || irCode_HeatDown == 0) {
      Serial.println("ERROR: Cannot enable auto mode - IR codes not learned!");
      Serial.println("Please use Learning Mode (V7) to capture IR codes first.");
      autoMode = false;
      Blynk.virtualWrite(AUTO_MODE_VPIN, 0);
      return;
    }

    if (stoveIsOn) {
      Serial.println("System will automatically adjust heat level based on temperature");
      lastAdjustmentTime = millis() - adjustmentCooldown; // Allow immediate first adjustment
    } else {
      Serial.println("Warning: Auto mode enabled but stove is OFF. Turn stove ON first!");
      autoMode = false;
      Blynk.virtualWrite(AUTO_MODE_VPIN, 0);
    }
  }
}

// Blynk: Adjustment cooldown slider
BLYNK_WRITE(COOLDOWN_VPIN) {
  int cooldownMinutes = param.asInt();

  // Validate cooldown range (5-30 minutes)
  if (cooldownMinutes < 5) {
    Serial.println("WARNING: Cooldown too short, setting to minimum (5 minutes)");
    cooldownMinutes = 5;
    Blynk.virtualWrite(COOLDOWN_VPIN, 5);
  } else if (cooldownMinutes > 30) {
    Serial.println("WARNING: Cooldown too long, setting to maximum (30 minutes)");
    cooldownMinutes = 30;
    Blynk.virtualWrite(COOLDOWN_VPIN, 30);
  }

  adjustmentCooldown = cooldownMinutes * 60000UL; // Convert minutes to milliseconds
  Serial.print("Adjustment cooldown set to: ");
  Serial.print(cooldownMinutes);
  Serial.println(" minutes");
}

// Blynk: Manual heat level sync
BLYNK_WRITE(HEAT_SYNC_VPIN) {
  int syncLevel = param.asInt();

  // Validate heat level is in range 0-5
  if (syncLevel < 0) syncLevel = 0;
  if (syncLevel > 5) syncLevel = 5;

  currentHeatLevel = syncLevel;
  Serial.print("Heat level manually synced to: ");
  Serial.println(currentHeatLevel);

  // Update display
  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);

  // If syncing to 0, assume stove is off
  if (currentHeatLevel == 0 && stoveIsOn) {
    stoveIsOn = false;
    Blynk.virtualWrite(STOVE_STATUS_VPIN, 0);
    Serial.println("Stove marked as OFF (heat level 0)");
  }
  // If syncing to 1-5, assume stove is on
  else if (currentHeatLevel > 0 && !stoveIsOn) {
    stoveIsOn = true;
    Blynk.virtualWrite(STOVE_STATUS_VPIN, 1);
    Serial.println("Stove marked as ON");
  }
}

// Blynk: Zone selector
BLYNK_WRITE(ZONE_SELECT_VPIN) {
  int newZone = param.asInt();

  // Validate zone selection (must be 0, 1, or 2)
  if (newZone < 0 || newZone > 2) {
    Serial.print("ERROR: Invalid zone selection: ");
    Serial.print(newZone);
    Serial.println(" - Using Zone 1 (Local)");
    newZone = 0;
    Blynk.virtualWrite(ZONE_SELECT_VPIN, 0); // Reset to valid value
  }

  activeZone = newZone;

  const char* zoneNames[] = {"Zone 1 (Stove Room)", "Zone 2 (Remote)", "Average of Both"};
  Serial.print("Active zone changed to: ");
  Serial.println(zoneNames[activeZone]);

  // Immediately update temperature based on new zone
  updateActiveTemperature();

  Serial.print("Using temperature: ");
  Serial.print(currentTemp);
  Serial.println("°F for automation");
}

// Blynk: IR Learning mode button
BLYNK_WRITE(LEARN_MODE_VPIN) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    learningMode = true;
    learningStep = 0;
    Serial.println("\n=== IR LEARNING MODE ===");
    Serial.println("Point your pellet stove remote at the IR receiver");
    Serial.println("Press the POWER ON button on your remote now...");
    irrecv.enableIRIn(); // Start the receiver
  }
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Arduino TCM - Smart Pellet Stove Controller ===");
  Serial.println("Control Mode: PID-style heat level adjustment (1-5)");

  // Initialize DHT sensor
  dht.begin();
  Serial.println("✓ DHT11 sensor initialized");

  // Initialize IR components
  irsend.begin();
  irrecv.enableIRIn();
  Serial.println("✓ IR transmitter initialized on GPIO15");
  Serial.println("✓ IR receiver initialized on GPIO14");

  // Load saved IR codes from flash memory
  loadIRCodes();

  // Set WiFi mode BEFORE Blynk initialization for ESP-NOW compatibility
  WiFi.mode(WIFI_AP_STA); // Enable both AP and Station mode for ESP-NOW

  // Connect to Blynk
  Serial.println("Connecting to WiFi and Blynk...");
  Blynk.begin(auth, ssid, pass);
  Serial.println("✓ Connected to Blynk");

  // Initialize ESP-NOW for multi-zone temperature
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(onDataReceive);
  Serial.println("✓ ESP-NOW initialized (ready to receive from remote sensors)");

  // Set timer to send data every 2 minutes (120,000 ms)
  timer.setInterval(120000L, sendData);

  // Set timer to check for IR signals every 100ms when in learning mode
  timer.setInterval(100L, checkForIRSignal);

  // Send initial status to Blynk
  Blynk.virtualWrite(STOVE_STATUS_VPIN, stoveIsOn ? 1 : 0);
  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
  Blynk.virtualWrite(TEMP_SETPOINT_VPIN, tempSetpoint);
  Blynk.virtualWrite(AUTO_MODE_VPIN, autoMode ? 1 : 0);
  Blynk.virtualWrite(COOLDOWN_VPIN, adjustmentCooldown / 60000); // Send in minutes
  Blynk.virtualWrite(ZONE_SELECT_VPIN, activeZone);
  Blynk.virtualWrite(REMOTE_TEMP_VPIN, remoteTemp);

  Serial.println("\n=== System Ready ===");
  Serial.println("Use Blynk app to:");
  Serial.println("- V3: Turn stove ON");
  Serial.println("- V4: Turn stove OFF");
  Serial.println("- V5: Set target temperature");
  Serial.println("- V6: Enable/disable auto mode");
  Serial.println("- V7: Enter IR learning mode");
  Serial.println("- V8: Manual heat UP");
  Serial.println("- V9: Manual heat DOWN");
  Serial.println("- V10: View current heat level (1-5)");
  Serial.println("- V11: View stove status");
  Serial.println("- V12: Adjust cooldown period (5-30 minutes)");
  Serial.println("- V13: Sync actual heat level (0-5)");
  Serial.println("- V14: Select zone (0=Zone1, 1=Zone2, 2=Average)");
  Serial.println("- V15: View Zone 2 temperature");
  Serial.println("\nMulti-Zone Control:");
  Serial.println("- Zone 1: Local DHT11 sensor (stove room)");
  Serial.println("- Zone 2: Remote ESP32 sensor (other room)");
  Serial.println("- Average: Uses average of both zones");
  Serial.println("\nAuto mode adjusts heat level based on selected zone temperature");
  Serial.print("Default cooldown: ");
  Serial.print(adjustmentCooldown / 60000);
  Serial.println(" minutes");
  Serial.println();
}

void loop() {
  Blynk.run();
  timer.run();
}
