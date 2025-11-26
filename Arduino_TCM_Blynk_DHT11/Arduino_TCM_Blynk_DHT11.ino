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
#include <RCSwitch.h>
#include <TFT_eSPI.h>       // ST7789 LCD library
#include <SPI.h>

// ST7789 LCD Configuration (135x240 in landscape mode = 240x135)
#define TFT_CS    5   // Chip select
#define TFT_DC    2   // Data/Command
#define TFT_RST   0   // Reset
#define TFT_MOSI  23  // SPI MOSI
#define TFT_SCLK  18  // SPI Clock
#define TFT_WIDTH  240
#define TFT_HEIGHT 135

// Define DHT11 pin and type
#define DHTPIN 4        // GPIO4
#define DHTTYPE DHT11   // DHT11 sensor

// Define IR pins
#define IR_RECV_PIN 14  // GPIO14 for IR receiver (VS1838B)
#define IR_SEND_PIN 15  // GPIO15 for IR LED transmitter

// Define RF pins
#define RF_TRANSMIT_PIN 16  // GPIO16 for 433MHz RF transmitter
#define RF_RECEIVE_PIN 17   // GPIO17 for 433MHz RF receiver (optional)

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

// Ceiling Fan Virtual Pins (V16-V28)
// Fan 1 Controls
#define FAN1_OFF_VPIN V16       // Fan 1 OFF button
#define FAN1_LOW_VPIN V17       // Fan 1 LOW speed
#define FAN1_MED_VPIN V18       // Fan 1 MEDIUM speed
#define FAN1_HIGH_VPIN V19      // Fan 1 HIGH speed
#define FAN1_LIGHT_VPIN V20     // Fan 1 Light toggle

// Fan 2 Controls
#define FAN2_OFF_VPIN V21       // Fan 2 OFF button
#define FAN2_LOW_VPIN V22       // Fan 2 LOW speed
#define FAN2_MED_VPIN V23       // Fan 2 MEDIUM speed
#define FAN2_HIGH_VPIN V24      // Fan 2 HIGH speed
#define FAN2_LIGHT_VPIN V25     // Fan 2 Light toggle

// Fan 3 Controls
#define FAN3_OFF_VPIN V26       // Fan 3 OFF button
#define FAN3_LOW_VPIN V27       // Fan 3 LOW speed
#define FAN3_MED_VPIN V28       // Fan 3 MEDIUM speed
#define FAN3_HIGH_VPIN V29      // Fan 3 HIGH speed
#define FAN3_LIGHT_VPIN V30     // Fan 3 Light toggle

// RF Learning Mode
#define RF_LEARN_MODE_VPIN V31  // RF Learning mode button

// Temperature Equalization Learning Virtual Pins
#define AUTO_EQUALIZE_VPIN V32    // Auto-equalization mode switch
#define TEMP_DIFF_THRESHOLD_VPIN V33  // Temperature difference threshold slider
#define LEARNING_STATUS_VPIN V34  // Learning status display
#define RESET_LEARNING_VPIN V35   // Reset learned data button

// Initialize components
DHT dht(DHTPIN, DHTTYPE);
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
RCSwitch rfSwitch = RCSwitch();
TFT_eSPI tft = TFT_eSPI(TFT_HEIGHT, TFT_WIDTH);  // 135x240 display
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

// RF code storage for ceiling fans (433MHz)
// Fan 1 codes
unsigned long fan1_Off = 0;
unsigned long fan1_Low = 0;
unsigned long fan1_Med = 0;
unsigned long fan1_High = 0;
unsigned long fan1_Light = 0;

// Fan 2 codes
unsigned long fan2_Off = 0;
unsigned long fan2_Low = 0;
unsigned long fan2_Med = 0;
unsigned long fan2_High = 0;
unsigned long fan2_Light = 0;

// Fan 3 codes
unsigned long fan3_Off = 0;
unsigned long fan3_Low = 0;
unsigned long fan3_Med = 0;
unsigned long fan3_High = 0;
unsigned long fan3_Light = 0;

// RF Learning mode
bool rfLearningMode = false;
int rfLearningStep = 0;  // 0-4: Fan1 codes, 5-9: Fan2 codes, 10-14: Fan3 codes
int rfBitLength = 24;    // Default bit length (will be detected)

// Temperature Equalization Learning System
#define MAX_LEARNING_SAMPLES 50  // Store up to 50 learning samples per fan/speed combination

// Fan state tracking
struct FanState {
  uint8_t speed;  // 0=Off, 1=Low, 2=Med, 3=High
  unsigned long lastChangeTime;
};

FanState fan1State = {0, 0};
FanState fan2State = {0, 0};
FanState fan3State = {0, 0};

// Learning data structure
struct TempLearningData {
  float zone1TempChange;  // Temperature change in Zone 1 (°F per minute)
  float zone2TempChange;  // Temperature change in Zone 2 (°F per minute)
  uint8_t sampleCount;    // Number of samples collected
  unsigned long lastUpdateTime;
};

// Learning model: [fanIndex][speedIndex] → temperature impact
// fanIndex: 0=Fan1, 1=Fan2, 2=Fan3
// speedIndex: 0=Off, 1=Low, 2=Med, 3=High
TempLearningData learningModel[3][4];

// Auto-equalization settings
bool autoEqualizationMode = false;
float tempDifferenceThreshold = 3.0;  // Start equalizing when zones differ by 3°F
unsigned long lastEqualizationAction = 0;
const unsigned long EQUALIZATION_COOLDOWN = 300000; // 5 minutes between actions

// Temperature tracking for learning
float lastZone1Temp = 0.0;
float lastZone2Temp = 0.0;
unsigned long lastTempSampleTime = 0;
const unsigned long TEMP_SAMPLE_INTERVAL = 60000; // Sample every 1 minute for learning

// LCD Display variables
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_UPDATE_INTERVAL = 2000; // Update display every 2 seconds
bool displayInitialized = false;

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

// Function to send RF command
void sendRFCommand(unsigned long code, int bitLength) {
  if (code == 0) {
    Serial.println("Error: RF code not set. Please learn codes first!");
    return;
  }

  Serial.print("Sending RF code: ");
  Serial.print(code);
  Serial.print(" (");
  Serial.print(bitLength);
  Serial.println(" bits)");

  rfSwitch.send(code, bitLength);
  delay(100); // Small delay after sending
}

// Save RF codes to EEPROM/Flash
void saveRFCodes() {
  preferences.begin("fans", false);  // false = read/write mode

  // Fan 1 codes
  preferences.putULong("fan1_off", fan1_Off);
  preferences.putULong("fan1_low", fan1_Low);
  preferences.putULong("fan1_med", fan1_Med);
  preferences.putULong("fan1_high", fan1_High);
  preferences.putULong("fan1_light", fan1_Light);

  // Fan 2 codes
  preferences.putULong("fan2_off", fan2_Off);
  preferences.putULong("fan2_low", fan2_Low);
  preferences.putULong("fan2_med", fan2_Med);
  preferences.putULong("fan2_high", fan2_High);
  preferences.putULong("fan2_light", fan2_Light);

  // Fan 3 codes
  preferences.putULong("fan3_off", fan3_Off);
  preferences.putULong("fan3_low", fan3_Low);
  preferences.putULong("fan3_med", fan3_Med);
  preferences.putULong("fan3_high", fan3_High);
  preferences.putULong("fan3_light", fan3_Light);

  // Save bit length
  preferences.putInt("rfBitLen", rfBitLength);

  preferences.end();
  Serial.println("RF codes saved to flash memory");
}

// Load RF codes from EEPROM/Flash
void loadRFCodes() {
  preferences.begin("fans", true);  // true = read-only mode

  // Fan 1 codes
  fan1_Off = preferences.getULong("fan1_off", 0);
  fan1_Low = preferences.getULong("fan1_low", 0);
  fan1_Med = preferences.getULong("fan1_med", 0);
  fan1_High = preferences.getULong("fan1_high", 0);
  fan1_Light = preferences.getULong("fan1_light", 0);

  // Fan 2 codes
  fan2_Off = preferences.getULong("fan2_off", 0);
  fan2_Low = preferences.getULong("fan2_low", 0);
  fan2_Med = preferences.getULong("fan2_med", 0);
  fan2_High = preferences.getULong("fan2_high", 0);
  fan2_Light = preferences.getULong("fan2_light", 0);

  // Fan 3 codes
  fan3_Off = preferences.getULong("fan3_off", 0);
  fan3_Low = preferences.getULong("fan3_low", 0);
  fan3_Med = preferences.getULong("fan3_med", 0);
  fan3_High = preferences.getULong("fan3_high", 0);
  fan3_Light = preferences.getULong("fan3_light", 0);

  // Load bit length
  rfBitLength = preferences.getInt("rfBitLen", 24);

  preferences.end();

  if (fan1_Off != 0) {
    Serial.println("✓ RF codes loaded from flash memory");
    Serial.print("  Bit length: ");
    Serial.println(rfBitLength);
    Serial.println("  Fan codes loaded for 3 fans");
  } else {
    Serial.println("No saved RF codes found - please use RF learning mode");
  }
}

// ===== TEMPERATURE EQUALIZATION LEARNING SYSTEM =====

// Initialize learning model
void initLearningModel() {
  for (int fan = 0; fan < 3; fan++) {
    for (int speed = 0; speed < 4; speed++) {
      learningModel[fan][speed].zone1TempChange = 0.0;
      learningModel[fan][speed].zone2TempChange = 0.0;
      learningModel[fan][speed].sampleCount = 0;
      learningModel[fan][speed].lastUpdateTime = 0;
    }
  }
}

// Update fan state when fan command is sent
void updateFanState(uint8_t fanIndex, uint8_t newSpeed) {
  FanState* fan;

  switch(fanIndex) {
    case 0: fan = &fan1State; break;
    case 1: fan = &fan2State; break;
    case 2: fan = &fan3State; break;
    default: return;
  }

  fan->speed = newSpeed;
  fan->lastChangeTime = millis();

  Serial.print("Fan ");
  Serial.print(fanIndex + 1);
  Serial.print(" state updated: Speed ");
  Serial.println(newSpeed);
}

// Collect temperature samples and learn correlations
void collectTemperatureSample() {
  unsigned long currentTime = millis();

  // Only sample every minute
  if (currentTime - lastTempSampleTime < TEMP_SAMPLE_INTERVAL) {
    return;
  }

  // Need both zones active for learning
  if (currentTime - lastRemoteTempTime > REMOTE_TIMEOUT) {
    return; // Remote sensor offline
  }

  // Calculate temperature changes since last sample
  float zone1Change = localTemp - lastZone1Temp;
  float zone2Change = remoteTemp - lastZone2Temp;

  // Calculate time elapsed in minutes
  float minutesElapsed = (currentTime - lastTempSampleTime) / 60000.0;

  if (minutesElapsed > 0 && lastTempSampleTime > 0) {
    // Temperature change rate (°F per minute)
    float zone1Rate = zone1Change / minutesElapsed;
    float zone2Rate = zone2Change / minutesElapsed;

    // Update learning model for each active fan
    for (int fan = 0; fan < 3; fan++) {
      FanState* fanState;
      switch(fan) {
        case 0: fanState = &fan1State; break;
        case 1: fanState = &fan2State; break;
        case 2: fanState = &fan3State; break;
      }

      // Only learn if fan has been in this state for at least 2 minutes
      if (currentTime - fanState->lastChangeTime >= 120000) {
        uint8_t speed = fanState->speed;
        TempLearningData* data = &learningModel[fan][speed];

        // Running average of temperature change rates
        if (data->sampleCount < MAX_LEARNING_SAMPLES) {
          // Add new sample
          data->zone1TempChange = ((data->zone1TempChange * data->sampleCount) + zone1Rate) / (data->sampleCount + 1);
          data->zone2TempChange = ((data->zone2TempChange * data->sampleCount) + zone2Rate) / (data->sampleCount + 1);
          data->sampleCount++;
          data->lastUpdateTime = currentTime;

          // Log learning progress
          if (data->sampleCount % 5 == 0) {
            Serial.print("Learning: Fan");
            Serial.print(fan + 1);
            Serial.print(" Speed");
            Serial.print(speed);
            Serial.print(" - Samples: ");
            Serial.print(data->sampleCount);
            Serial.print(" Z1: ");
            Serial.print(data->zone1TempChange, 3);
            Serial.print("°F/min Z2: ");
            Serial.print(data->zone2TempChange, 3);
            Serial.println("°F/min");
          }
        }
      }
    }
  }

  // Update last temperatures and time
  lastZone1Temp = localTemp;
  lastZone2Temp = remoteTemp;
  lastTempSampleTime = currentTime;
}

// Save learning data to EEPROM
void saveLearningData() {
  preferences.begin("templearn", false);

  for (int fan = 0; fan < 3; fan++) {
    for (int speed = 0; speed < 4; speed++) {
      String key = String(fan) + "_" + String(speed);
      preferences.putFloat((key + "_z1").c_str(), learningModel[fan][speed].zone1TempChange);
      preferences.putFloat((key + "_z2").c_str(), learningModel[fan][speed].zone2TempChange);
      preferences.putUChar((key + "_cnt").c_str(), learningModel[fan][speed].sampleCount);
    }
  }

  preferences.end();
  Serial.println("Learning data saved to flash memory");
}

// Load learning data from EEPROM
void loadLearningData() {
  preferences.begin("templearn", true);

  int totalSamples = 0;
  for (int fan = 0; fan < 3; fan++) {
    for (int speed = 0; speed < 4; speed++) {
      String key = String(fan) + "_" + String(speed);
      learningModel[fan][speed].zone1TempChange = preferences.getFloat((key + "_z1").c_str(), 0.0);
      learningModel[fan][speed].zone2TempChange = preferences.getFloat((key + "_z2").c_str(), 0.0);
      learningModel[fan][speed].sampleCount = preferences.getUChar((key + "_cnt").c_str(), 0);
      totalSamples += learningModel[fan][speed].sampleCount;
    }
  }

  preferences.end();

  if (totalSamples > 0) {
    Serial.println("✓ Temperature learning data loaded");
    Serial.print("  Total samples: ");
    Serial.println(totalSamples);
  } else {
    Serial.println("No learning data found - system will learn over time");
  }
}

// Auto-equalization: Use learned data to balance temperatures
void autoEqualizeTemperatures() {
  if (!autoEqualizationMode) return;

  unsigned long currentTime = millis();

  // Cooldown between actions
  if (currentTime - lastEqualizationAction < EQUALIZATION_COOLDOWN) {
    return;
  }

  // Need both zones active
  if (currentTime - lastRemoteTempTime > REMOTE_TIMEOUT) {
    return;
  }

  // Calculate temperature difference
  float tempDiff = localTemp - remoteTemp;

  // Only act if difference exceeds threshold
  if (abs(tempDiff) < tempDifferenceThreshold) {
    return;
  }

  Serial.print("Temperature difference detected: ");
  Serial.print(tempDiff);
  Serial.println("°F");

  // Determine which zone needs cooling/warming
  bool zone1NeedsCooling = (tempDiff > 0);  // Zone 1 is warmer
  bool zone2NeedsCooling = (tempDiff < 0);  // Zone 2 is warmer

  // Find best fan/speed combination using learned data
  int bestFan = -1;
  int bestSpeed = -1;
  float bestEffect = 0.0;

  for (int fan = 0; fan < 3; fan++) {
    for (int speed = 1; speed < 4; speed++) {  // Skip speed 0 (OFF)
      TempLearningData* data = &learningModel[fan][speed];

      // Need at least 5 samples to trust the data
      if (data->sampleCount < 5) continue;

      // Calculate expected effect on temperature difference
      // Positive effect = reduces temperature difference
      float effect = 0.0;

      if (zone1NeedsCooling) {
        // Want to cool Zone 1 or warm Zone 2
        effect = -data->zone1TempChange + data->zone2TempChange;
      } else {
        // Want to warm Zone 1 or cool Zone 2
        effect = data->zone1TempChange - data->zone2TempChange;
      }

      // Find fan with best (most positive) effect
      if (effect > bestEffect) {
        bestEffect = effect;
        bestFan = fan;
        bestSpeed = speed;
      }
    }
  }

  // Execute best fan action
  if (bestFan >= 0 && bestSpeed >= 0) {
    Serial.print("Auto-Equalization: Running Fan ");
    Serial.print(bestFan + 1);
    Serial.print(" at speed ");
    Serial.print(bestSpeed);
    Serial.print(" (expected effect: ");
    Serial.print(bestEffect, 3);
    Serial.println("°F/min)");

    // Send fan command
    unsigned long code = 0;
    switch(bestFan) {
      case 0:
        code = (bestSpeed == 1) ? fan1_Low : (bestSpeed == 2) ? fan1_Med : fan1_High;
        break;
      case 1:
        code = (bestSpeed == 1) ? fan2_Low : (bestSpeed == 2) ? fan2_Med : fan2_High;
        break;
      case 2:
        code = (bestSpeed == 1) ? fan3_Low : (bestSpeed == 2) ? fan3_Med : fan3_High;
        break;
    }

    if (code != 0) {
      sendRFCommand(code, rfBitLength);
      updateFanState(bestFan, bestSpeed);
      lastEqualizationAction = currentTime;
    }
  } else {
    Serial.println("Auto-Equalization: Not enough learned data yet");
  }
}

// ===== END TEMPERATURE EQUALIZATION LEARNING SYSTEM =====

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

// RF Learning function
void checkForRFSignal() {
  if (!rfLearningMode) return;

  if (rfSwitch.available()) {
    unsigned long code = rfSwitch.getReceivedValue();
    int bitLen = rfSwitch.getReceivedBitlength();

    if (code != 0) {
      Serial.println("RF Signal Received!");
      Serial.print("Code: ");
      Serial.print(code);
      Serial.print(" (");
      Serial.print(bitLen);
      Serial.println(" bits)");

      // Store bit length from first code
      if (rfLearningStep == 0) {
        rfBitLength = bitLen;
      }

      // Store the code based on learning step
      const char* stepNames[] = {
        "Fan 1 OFF", "Fan 1 LOW", "Fan 1 MED", "Fan 1 HIGH", "Fan 1 LIGHT",
        "Fan 2 OFF", "Fan 2 LOW", "Fan 2 MED", "Fan 2 HIGH", "Fan 2 LIGHT",
        "Fan 3 OFF", "Fan 3 LOW", "Fan 3 MED", "Fan 3 HIGH", "Fan 3 LIGHT"
      };

      switch(rfLearningStep) {
        // Fan 1 codes
        case 0: fan1_Off = code; break;
        case 1: fan1_Low = code; break;
        case 2: fan1_Med = code; break;
        case 3: fan1_High = code; break;
        case 4: fan1_Light = code; break;
        // Fan 2 codes
        case 5: fan2_Off = code; break;
        case 6: fan2_Low = code; break;
        case 7: fan2_Med = code; break;
        case 8: fan2_High = code; break;
        case 9: fan2_Light = code; break;
        // Fan 3 codes
        case 10: fan3_Off = code; break;
        case 11: fan3_Low = code; break;
        case 12: fan3_Med = code; break;
        case 13: fan3_High = code; break;
        case 14: fan3_Light = code; break;
      }

      Serial.print("✓ ");
      Serial.print(stepNames[rfLearningStep]);
      Serial.println(" learned!");

      rfLearningStep++;

      if (rfLearningStep < 15) {
        Serial.print("Now press ");
        Serial.print(stepNames[rfLearningStep]);
        Serial.println(" on your remote...");
      } else {
        Serial.println("\n=== ALL RF CODES LEARNED! ===");
        Serial.print("Bit length: ");
        Serial.println(rfBitLength);
        Serial.println("All 3 fan codes captured!");

        // Save codes to flash memory
        saveRFCodes();

        Serial.println("RF Learning mode complete!");
        rfLearningMode = false;
        rfLearningStep = 0;
        Blynk.virtualWrite(RF_LEARN_MODE_VPIN, 0);
      }
    }

    rfSwitch.resetAvailable();
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

// Blynk: RF Learning mode button
BLYNK_WRITE(RF_LEARN_MODE_VPIN) {
  int buttonState = param.asInt();
  if (buttonState == 1) {
    rfLearningMode = true;
    rfLearningStep = 0;
    Serial.println("\n=== RF LEARNING MODE ===");
    Serial.println("Point your ceiling fan remote #1 at the RF receiver");
    Serial.println("Press the OFF button on Fan 1 remote now...");
    rfSwitch.enableReceive(digitalPinToInterrupt(RF_RECEIVE_PIN));
  }
}

// Fan 1 Controls
BLYNK_WRITE(FAN1_OFF_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan1_Off, rfBitLength);
    updateFanState(0, 0);  // Fan 1, Speed 0 (OFF)
    Serial.println("Fan 1: OFF");
  }
}

BLYNK_WRITE(FAN1_LOW_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan1_Low, rfBitLength);
    updateFanState(0, 1);  // Fan 1, Speed 1 (LOW)
    Serial.println("Fan 1: LOW speed");
  }
}

BLYNK_WRITE(FAN1_MED_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan1_Med, rfBitLength);
    updateFanState(0, 2);  // Fan 1, Speed 2 (MED)
    Serial.println("Fan 1: MEDIUM speed");
  }
}

BLYNK_WRITE(FAN1_HIGH_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan1_High, rfBitLength);
    updateFanState(0, 3);  // Fan 1, Speed 3 (HIGH)
    Serial.println("Fan 1: HIGH speed");
  }
}

BLYNK_WRITE(FAN1_LIGHT_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan1_Light, rfBitLength);
    Serial.println("Fan 1: Light toggled");
  }
}

// Fan 2 Controls
BLYNK_WRITE(FAN2_OFF_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan2_Off, rfBitLength);
    updateFanState(1, 0);  // Fan 2, Speed 0 (OFF)
    Serial.println("Fan 2: OFF");
  }
}

BLYNK_WRITE(FAN2_LOW_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan2_Low, rfBitLength);
    updateFanState(1, 1);  // Fan 2, Speed 1 (LOW)
    Serial.println("Fan 2: LOW speed");
  }
}

BLYNK_WRITE(FAN2_MED_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan2_Med, rfBitLength);
    updateFanState(1, 2);  // Fan 2, Speed 2 (MED)
    Serial.println("Fan 2: MEDIUM speed");
  }
}

BLYNK_WRITE(FAN2_HIGH_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan2_High, rfBitLength);
    updateFanState(1, 3);  // Fan 2, Speed 3 (HIGH)
    Serial.println("Fan 2: HIGH speed");
  }
}

BLYNK_WRITE(FAN2_LIGHT_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan2_Light, rfBitLength);
    Serial.println("Fan 2: Light toggled");
  }
}

// Fan 3 Controls
BLYNK_WRITE(FAN3_OFF_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan3_Off, rfBitLength);
    updateFanState(2, 0);  // Fan 3, Speed 0 (OFF)
    Serial.println("Fan 3: OFF");
  }
}

BLYNK_WRITE(FAN3_LOW_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan3_Low, rfBitLength);
    updateFanState(2, 1);  // Fan 3, Speed 1 (LOW)
    Serial.println("Fan 3: LOW speed");
  }
}

BLYNK_WRITE(FAN3_MED_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan3_Med, rfBitLength);
    updateFanState(2, 2);  // Fan 3, Speed 2 (MED)
    Serial.println("Fan 3: MEDIUM speed");
  }
}

BLYNK_WRITE(FAN3_HIGH_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan3_High, rfBitLength);
    updateFanState(2, 3);  // Fan 3, Speed 3 (HIGH)
    Serial.println("Fan 3: HIGH speed");
  }
}

BLYNK_WRITE(FAN3_LIGHT_VPIN) {
  if (param.asInt() == 1 && !rfLearningMode) {
    sendRFCommand(fan3_Light, rfBitLength);
    Serial.println("Fan 3: Light toggled");
  }
}

// Temperature Equalization Learning Controls
BLYNK_WRITE(AUTO_EQUALIZE_VPIN) {
  autoEqualizationMode = param.asInt();
  Serial.print("Auto-Equalization Mode: ");
  Serial.println(autoEqualizationMode ? "ENABLED" : "DISABLED");

  if (autoEqualizationMode) {
    // Check if we have enough learned data
    int totalSamples = 0;
    for (int fan = 0; fan < 3; fan++) {
      for (int speed = 1; speed < 4; speed++) {
        totalSamples += learningModel[fan][speed].sampleCount;
      }
    }

    if (totalSamples < 10) {
      Serial.println("WARNING: Limited learning data. System will learn as it operates.");
    } else {
      Serial.print("Ready: ");
      Serial.print(totalSamples);
      Serial.println(" learning samples available");
    }
  }
}

BLYNK_WRITE(TEMP_DIFF_THRESHOLD_VPIN) {
  float newThreshold = param.asFloat();

  // Validate threshold (1-10°F)
  if (newThreshold < 1.0) {
    newThreshold = 1.0;
    Blynk.virtualWrite(TEMP_DIFF_THRESHOLD_VPIN, 1.0);
  } else if (newThreshold > 10.0) {
    newThreshold = 10.0;
    Blynk.virtualWrite(TEMP_DIFF_THRESHOLD_VPIN, 10.0);
  }

  tempDifferenceThreshold = newThreshold;
  Serial.print("Temperature difference threshold set to: ");
  Serial.print(tempDifferenceThreshold);
  Serial.println("°F");
}

BLYNK_WRITE(RESET_LEARNING_VPIN) {
  if (param.asInt() == 1) {
    Serial.println("Resetting all learning data...");
    initLearningModel();
    saveLearningData();
    Serial.println("Learning data reset complete!");
    Blynk.virtualWrite(LEARNING_STATUS_VPIN, "Learning data reset");
  }
}

// ==================== LCD DISPLAY FUNCTIONS ====================

void initDisplay() {
  tft.init();
  tft.setRotation(3);  // Landscape mode (240x135)
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Draw initial screen
  tft.setTextSize(1);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.println("  PELLET STOVE CONTROL");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.println("  Initializing...");

  displayInitialized = true;
  Serial.println("✓ ST7789 LCD initialized (240x135 landscape)");
}

void updateDisplay() {
  if (!displayInitialized) return;

  // Get current temperature for display
  float displayTemp = currentTemp;
  if (activeZone == 0) {
    displayTemp = localTemp;
  } else if (activeZone == 1) {
    displayTemp = remoteTemp;
  } else {
    displayTemp = (localTemp + remoteTemp) / 2.0;
  }

  // Calculate humidity
  float humidity = dht.readHumidity();
  if (isnan(humidity)) humidity = 0.0;

  // Get total learning samples across all fans/speeds
  int totalSamples = 0;
  for (int f = 0; f < 3; f++) {
    for (int s = 0; s < 4; s++) {
      totalSamples += learningModel[f][s].sampleCount;
    }
  }

  // Clear screen
  tft.fillScreen(TFT_BLACK);

  // ===== LINE 1: Header with status indicators (0-18px) =====
  tft.setTextSize(1);
  tft.setCursor(0, 2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print("STOVE CTRL");

  // WiFi status
  tft.setCursor(155, 2);
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("W");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("W");
  }

  // Blynk status
  tft.setCursor(170, 2);
  if (Blynk.connected()) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("B");
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("B");
  }

  // Auto mode indicator
  tft.setCursor(185, 2);
  if (autoMode) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print("AUTO");
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("MAN");
  }

  // ===== LINE 2: Zone 1 temp, humidity, target (20-38px) =====
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 20);
  tft.print("Z1:");
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.print(localTemp, 1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("F");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(75, 20);
  tft.print("H:");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print((int)humidity);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("%");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(135, 20);
  tft.print("Tgt:");
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.print(tempSetpoint, 0);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("F");

  // ===== LINE 3: Zone 2 temp and heat level (40-58px) =====
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 40);
  tft.print("Z2:");
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  tft.print(remoteTemp, 1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("F");

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(75, 40);
  tft.print("Heat:");

  // Draw heat level blocks
  tft.setCursor(115, 40);
  for (int i = 1; i <= 5; i++) {
    if (stoveIsOn && i <= currentHeatLevel) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.print((char)219); // Full block character
    } else {
      tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
      tft.print((char)176); // Light block character
    }
  }

  // Stove status
  tft.setCursor(165, 40);
  if (stoveIsOn) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("ON");
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("OFF");
  }

  // ===== LINE 4: Separator (60px) =====
  tft.setCursor(0, 60);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.println("------------------------");

  // ===== LINE 5: Fan statuses (70-88px) =====
  tft.setCursor(0, 70);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  // Fan 1
  tft.print("F1:");
  switch (fan1State.speed) {
    case 0: tft.setTextColor(TFT_DARKGREY, TFT_BLACK); tft.print("OFF"); break;
    case 1: tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("LOW"); break;
    case 2: tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print("MED"); break;
    case 3: tft.setTextColor(TFT_RED, TFT_BLACK); tft.print("HI "); break;
  }

  // Fan 2
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(65, 70);
  tft.print("F2:");
  switch (fan2State.speed) {
    case 0: tft.setTextColor(TFT_DARKGREY, TFT_BLACK); tft.print("OFF"); break;
    case 1: tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("LOW"); break;
    case 2: tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print("MED"); break;
    case 3: tft.setTextColor(TFT_RED, TFT_BLACK); tft.print("HI "); break;
  }

  // Fan 3
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(130, 70);
  tft.print("F3:");
  switch (fan3State.speed) {
    case 0: tft.setTextColor(TFT_DARKGREY, TFT_BLACK); tft.print("OFF"); break;
    case 1: tft.setTextColor(TFT_GREEN, TFT_BLACK); tft.print("LOW"); break;
    case 2: tft.setTextColor(TFT_YELLOW, TFT_BLACK); tft.print("MED"); break;
    case 3: tft.setTextColor(TFT_RED, TFT_BLACK); tft.print("HI "); break;
  }

  // ===== LINE 6: Learning status and auto-equalization (90-108px) =====
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 90);
  tft.print("Learn:");
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.print(totalSamples);

  // Auto-equalization status
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(80, 90);
  tft.print("AutoEQ:");
  if (autoEqualizationMode) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.print("ON");
  } else {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("OFF");
  }

  // ===== LINE 7: Temperature difference and zone indicator (110-128px) =====
  float tempDiff = abs(localTemp - remoteTemp);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(0, 110);
  tft.print("Diff:");
  if (tempDiff > tempDifferenceThreshold) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
  }
  tft.print(tempDiff, 1);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.print("F");

  // Active zone indicator
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setCursor(80, 110);
  tft.print("Zone:");
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  if (activeZone == 0) tft.print("1");
  else if (activeZone == 1) tft.print("2");
  else tft.print("AVG");
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Arduino TCM - Smart Home Climate Controller ===");
  Serial.println("Pellet Stove (IR) + 3 Ceiling Fans (RF)");
  Serial.println("Control Mode: PID-style heat level adjustment (1-5)");

  // Initialize DHT sensor
  dht.begin();
  Serial.println("✓ DHT11 sensor initialized");

  // Initialize LCD Display
  initDisplay();

  // Initialize IR components
  irsend.begin();
  irrecv.enableIRIn();
  Serial.println("✓ IR transmitter initialized on GPIO15");
  Serial.println("✓ IR receiver initialized on GPIO14");

  // Initialize RF components
  rfSwitch.enableTransmit(RF_TRANSMIT_PIN);
  rfSwitch.setRepeatTransmit(3);  // Send each code 3 times for reliability
  Serial.println("✓ RF transmitter initialized on GPIO16");
  Serial.println("  Note: Connect 433MHz RF transmitter to GPIO16");
  Serial.println("  Optional: Connect 433MHz RF receiver to GPIO17 for learning mode");

  // Load saved codes from flash memory
  loadIRCodes();
  loadRFCodes();

  // Initialize temperature equalization learning system
  initLearningModel();
  loadLearningData();
  Serial.println("✓ Temperature equalization learning system initialized");

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

  // Set timer to check for RF signals every 100ms when in RF learning mode
  timer.setInterval(100L, checkForRFSignal);

  // Set timer for temperature learning (every 1 minute)
  timer.setInterval(60000L, collectTemperatureSample);

  // Set timer for auto-equalization (every 5 minutes)
  timer.setInterval(300000L, autoEqualizeTemperatures);

  // Set timer for LCD display updates (every 2 seconds)
  timer.setInterval(2000L, updateDisplay);

  // Send initial status to Blynk
  Blynk.virtualWrite(STOVE_STATUS_VPIN, stoveIsOn ? 1 : 0);
  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
  Blynk.virtualWrite(TEMP_SETPOINT_VPIN, tempSetpoint);
  Blynk.virtualWrite(AUTO_MODE_VPIN, autoMode ? 1 : 0);
  Blynk.virtualWrite(COOLDOWN_VPIN, adjustmentCooldown / 60000); // Send in minutes
  Blynk.virtualWrite(ZONE_SELECT_VPIN, activeZone);
  Blynk.virtualWrite(REMOTE_TEMP_VPIN, remoteTemp);
  Blynk.virtualWrite(AUTO_EQUALIZE_VPIN, autoEqualizationMode ? 1 : 0);
  Blynk.virtualWrite(TEMP_DIFF_THRESHOLD_VPIN, tempDifferenceThreshold);
  Blynk.virtualWrite(LEARNING_STATUS_VPIN, "System ready");

  Serial.println("\n=== System Ready ===");
  Serial.println("\n** PELLET STOVE CONTROLS (IR) **");
  Serial.println("- V3: Turn stove ON");
  Serial.println("- V4: Turn stove OFF");
  Serial.println("- V5: Set target temperature");
  Serial.println("- V6: Enable/disable auto mode");
  Serial.println("- V7: IR learning mode (stove)");
  Serial.println("- V8: Manual heat UP");
  Serial.println("- V9: Manual heat DOWN");
  Serial.println("- V10: Current heat level (1-5)");
  Serial.println("- V11: Stove status");
  Serial.println("- V12: Cooldown period (5-30 min)");
  Serial.println("- V13: Sync heat level (0-5)");

  Serial.println("\n** TEMPERATURE SENSORS **");
  Serial.println("- V0: Local temperature");
  Serial.println("- V2: Local humidity");
  Serial.println("- V14: Zone selector (0=Local, 1=Remote, 2=Average)");
  Serial.println("- V15: Remote zone temperature");

  Serial.println("\n** CEILING FAN CONTROLS (RF 433MHz) **");
  Serial.println("- V16-V20: Fan 1 (Off/Low/Med/High/Light)");
  Serial.println("- V21-V25: Fan 2 (Off/Low/Med/High/Light)");
  Serial.println("- V26-V30: Fan 3 (Off/Low/Med/High/Light)");
  Serial.println("- V31: RF learning mode (fans)");

  Serial.println("\n** MULTI-ZONE INFO **");
  Serial.println("- Zone 1: Local DHT11 sensor (stove room)");
  Serial.println("- Zone 2: Remote ESP32 sensor (other room)");
  Serial.println("- Average: Average of both zones");

  Serial.println("\n** AUTO MODE **");
  Serial.println("Auto mode adjusts stove heat level based on selected zone temperature");
  Serial.print("Default cooldown: ");
  Serial.print(adjustmentCooldown / 60000);
  Serial.println(" minutes");

  Serial.println("\n** TEMPERATURE EQUALIZATION LEARNING **");
  Serial.println("- V32: Auto-Equalization Mode (learns which fan equalizes temps)");
  Serial.println("- V33: Temp difference threshold (1-10°F)");
  Serial.println("- V34: Learning status display");
  Serial.println("- V35: Reset all learning data");
  Serial.println("\nLearning System:");
  Serial.println("- Automatically learns how each fan affects each zone");
  Serial.println("- Uses learned data to balance temperatures");
  Serial.println("- Improves over time as it collects more samples");
  Serial.println("- Saves learned patterns to flash memory");
  Serial.println();

  // Initial display update
  updateDisplay();
}

void loop() {
  Blynk.run();
  timer.run();
}
