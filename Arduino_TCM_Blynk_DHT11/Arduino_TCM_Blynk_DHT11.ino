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
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // ST7789 LCD library
#include <SPI.h>

// ST7789 LCD Configuration (135x240 in landscape mode = 240x135)
#define LCD_MOSI  23  // SPI MOSI - ESP32 D23
#define LCD_SCLK  18  // SPI Clock - ESP32 D18
#define LCD_CS    15  // Chip select - ESP32 D15
#define LCD_DC    2   // Data/Command - ESP32 D2
#define LCD_RST   4   // Reset - ESP32 D4
#define LCD_BLK   32  // Backlight - ESP32 D32
#define TFT_WIDTH  240
#define TFT_HEIGHT 135

// Define DHT11 pin and type
#define DHTPIN 13       // GPIO13 (moved from GPIO4 due to LCD conflict)
#define DHTTYPE DHT11   // DHT11 sensor

// Define IR pins
#define IR_RECV_PIN 14  // GPIO14 for IR receiver (VS1838B)
#define IR_SEND_PIN 5   // GPIO5 for IR LED transmitter (moved from GPIO15 due to LCD conflict)

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
Adafruit_ST7789 lcd = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);  // 135x240 display
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

// Multi-zone temperature control (4 zones total)
#define TOTAL_ZONES 4
float zoneTemp[TOTAL_ZONES] = {0.0, 0.0, 0.0, 0.0};  // Temps for all 4 zones
float zoneHumidity[TOTAL_ZONES] = {0.0, 0.0, 0.0, 0.0};  // Humidity for all 4 zones
unsigned long lastZoneTempTime[TOTAL_ZONES] = {0, 0, 0, 0};  // Last update time per zone
const unsigned long REMOTE_TIMEOUT = 300000; // 5 minutes - consider remote offline if no update
int thermostatZone = 0;  // Zone 0 controls the pellet stove (main thermostat)
int activeZone = 0;      // Active zone for Blynk display (0-3, for backward compatibility)

// Zone assignments:
// Zone 0: Main/Stove area (local DHT11 on main controller)
// Zone 1: Fan 1 area (remote sensor #1)
// Zone 2: Fan 2 area (remote sensor #2)
// Zone 3: Fan 3 area (remote sensor #3)

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
const unsigned long DISPLAY_UPDATE_INTERVAL = 4000; // Update display every 4 seconds (page rotation)
bool displayInitialized = false;
int currentDisplayPage = 0;  // 0-4 for 5 different pages
const int TOTAL_DISPLAY_PAGES = 5;

// Blynk rate limiting - reduce message usage
unsigned long lastBlynkUpdate[TOTAL_ZONES] = {0, 0, 0, 0};  // Last Blynk update time per zone
const unsigned long BLYNK_UPDATE_INTERVAL = 300000;  // Update Blynk every 5 minutes (300,000 ms)
// This reduces Blynk messages from ~10,000/day to ~600/day

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

  // Need zone 1 active for learning (check if remote sensor is online)
  if (currentTime - lastZoneTempTime[1] > REMOTE_TIMEOUT) {
    return; // Zone 1 sensor offline
  }

  // Calculate temperature changes since last sample
  float zone1Change = zoneTemp[0] - lastZone1Temp;  // Zone 0 (main)
  float zone2Change = zoneTemp[1] - lastZone2Temp;  // Zone 1 (fan 1 area)

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
            Serial.print(" Z0: ");
            Serial.print(data->zone1TempChange, 3);
            Serial.print("°F/min Z1: ");
            Serial.print(data->zone2TempChange, 3);
            Serial.println("°F/min");
          }
        }
      }
    }
  }

  // Update last temperatures and time
  lastZone1Temp = zoneTemp[0];
  lastZone2Temp = zoneTemp[1];
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

  // Need zone 1 active (check if remote sensor is online)
  if (currentTime - lastZoneTempTime[1] > REMOTE_TIMEOUT) {
    return;
  }

  // Calculate temperature difference between zones 0 and 1
  float tempDiff = zoneTemp[0] - zoneTemp[1];

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
void onDataReceive(const uint8_t *mac, const uint8_t *incomingDataPtr, int len) {
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

  // Get zone ID from sensor (1-3 for remote sensors)
  uint8_t zoneID = incomingData.sensorID;

  // Validate zone ID (remote sensors are zones 1-3)
  if (zoneID < 1 || zoneID >= TOTAL_ZONES) {
    Serial.print("ERROR: Invalid zone ID received: ");
    Serial.println(zoneID);
    return;
  }

  // Store temperature and humidity for this zone
  zoneTemp[zoneID] = incomingData.temperature;
  zoneHumidity[zoneID] = incomingData.humidity;
  lastZoneTempTime[zoneID] = millis();

  Serial.print("ESP-NOW: Received from Zone ");
  Serial.print(zoneID);
  Serial.print(" - Temp: ");
  Serial.print(zoneTemp[zoneID]);
  Serial.print("°F, Humidity: ");
  Serial.print(zoneHumidity[zoneID]);
  Serial.println("%");

  // Update Blynk with zone temperatures (RATE LIMITED to every 5 minutes)
  unsigned long currentTime = millis();
  if (currentTime - lastBlynkUpdate[zoneID] >= BLYNK_UPDATE_INTERVAL) {
    Blynk.virtualWrite(REMOTE_TEMP_VPIN, zoneTemp[zoneID]);  // For backward compatibility
    lastBlynkUpdate[zoneID] = currentTime;
    Serial.print("  → Blynk updated for Zone ");
    Serial.println(zoneID);
  }

  // Update current temp for thermostat control
  currentTemp = zoneTemp[thermostatZone];
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

// Adjust to specific heat level
void adjustHeatLevel(int targetLevel) {
  if (!stoveIsOn) {
    Serial.println("Cannot adjust heat - stove is OFF");
    return;
  }

  if (targetLevel < 1 || targetLevel > 5) {
    Serial.println("Invalid heat level (must be 1-5)");
    return;
  }

  Serial.print("Adjusting heat from ");
  Serial.print(currentHeatLevel);
  Serial.print(" to ");
  Serial.println(targetLevel);

  // Adjust heat level step by step
  while (currentHeatLevel < targetLevel) {
    increaseHeat();
    delay(500);  // Small delay between adjustments
  }

  while (currentHeatLevel > targetLevel) {
    decreaseHeat();
    delay(500);  // Small delay between adjustments
  }

  Serial.print("✓ Heat level adjusted to ");
  Serial.println(currentHeatLevel);
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
  Serial.println("\n========== DHT11 SENSOR READ ==========");

  // Read temperature and humidity from DHT11
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float f = (t * 9.0 / 5.0) + 32.0;

  // Show raw readings first
  Serial.print("RAW Humidity: ");
  Serial.print(h);
  Serial.println(" %");
  Serial.print("RAW Temp (C): ");
  Serial.print(t);
  Serial.println(" °C");
  Serial.print("Converted (F): ");
  Serial.print(f);
  Serial.println(" °F");

  // Check if readings failed
  if (isnan(h) || isnan(t)) {
    sensorFailCount++;
    Serial.println("❌ SENSOR READ FAILED!");
    Serial.print("⚠️  Fail count: ");
    Serial.print(sensorFailCount);
    Serial.print("/");
    Serial.println(MAX_SENSOR_FAILS);

    // Check sensor connection
    Serial.println("\n🔧 TROUBLESHOOTING:");
    Serial.println("   1. Check DHT11 is connected to GPIO13");
    Serial.println("   2. Check VCC → 3.3V");
    Serial.println("   3. Check GND → GND");
    Serial.println("   4. Check DATA → GPIO13");
    Serial.println("   5. Wait 2 seconds between readings");

    // Disable auto mode if sensor fails too many times
    if (sensorFailCount >= MAX_SENSOR_FAILS && autoMode) {
      Serial.println("\n❌ CRITICAL: Sensor failed multiple times!");
      Serial.println("   AUTO MODE DISABLED FOR SAFETY");
      autoMode = false;
      Blynk.virtualWrite(AUTO_MODE_VPIN, 0);
    }
    Serial.println("=======================================\n");
    return;
  }

  // Reset fail counter on successful read
  sensorFailCount = 0;
  Serial.println("✓ Sensor read SUCCESS!");

  // Store in Zone 0 (main/stove area)
  zoneTemp[0] = f;
  zoneHumidity[0] = h;
  lastZoneTempTime[0] = millis();

  // Update current temperature for thermostat control (Zone 0)
  currentTemp = zoneTemp[thermostatZone];

  // Send data to Blynk app (called every 5 minutes via timer to reduce message usage)
  Blynk.virtualWrite(TEMP_VPIN, f);
  Blynk.virtualWrite(HUM_VPIN, h);
  lastBlynkUpdate[0] = millis();  // Track update time
  Serial.println("  → Blynk updated for Zone 0");

  // Print all zone temperatures
  Serial.println("\n--- ALL ZONES STATUS ---");
  for (int i = 0; i < TOTAL_ZONES; i++) {
    Serial.print("Zone ");
    Serial.print(i);
    Serial.print(": ");

    // Check if zone data is recent
    unsigned long timeSinceUpdate = millis() - lastZoneTempTime[i];
    if (timeSinceUpdate < REMOTE_TIMEOUT || i == 0) {
      Serial.print(zoneTemp[i], 1);
      Serial.print("°F, ");
      Serial.print(zoneHumidity[i], 0);
      Serial.print("%");

      if (i == 0) {
        Serial.print(" (LOCAL)");
      } else {
        Serial.print(" (");
        Serial.print(timeSinceUpdate / 1000);
        Serial.print("s ago)");
      }

      if (i == thermostatZone) {
        Serial.print(" ← THERMOSTAT");
      }
      Serial.println();
    } else {
      Serial.println("OFFLINE (no data)");
    }
  }

  // Stove status
  Serial.println("\n--- STOVE STATUS ---");
  if (stoveIsOn) {
    Serial.print("Stove: ON | Heat Level: ");
    Serial.print(currentHeatLevel);
    Serial.print("/5");
    if (autoMode) {
      Serial.print(" | AUTO Mode | Target: ");
      Serial.print(tempSetpoint);
      Serial.print("°F | Current: ");
      Serial.print(currentTemp, 1);
      Serial.print("°F");
    } else {
      Serial.print(" | MANUAL Mode");
    }
    Serial.println();
  } else {
    Serial.println("Stove: OFF");
  }

  Serial.println("=======================================\n");

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

  // Validate zone selection (must be 0-3)
  if (newZone < 0 || newZone >= TOTAL_ZONES) {
    Serial.print("ERROR: Invalid zone selection: ");
    Serial.print(newZone);
    Serial.println(" - Using Zone 0 (Main)");
    newZone = 0;
    Blynk.virtualWrite(ZONE_SELECT_VPIN, 0); // Reset to valid value
  }

  activeZone = newZone;

  const char* zoneNames[] = {"Zone 0 (Main/Stove)", "Zone 1 (Fan 1)", "Zone 2 (Fan 2)", "Zone 3 (Fan 3)"};
  Serial.print("Active zone changed to: ");
  Serial.println(zoneNames[activeZone]);

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
  // Initialize backlight
  pinMode(LCD_BLK, OUTPUT);
  digitalWrite(LCD_BLK, HIGH);  // Turn on backlight

  // Initialize display
  lcd.init(135, 240);           // Init with width & height
  lcd.setRotation(3);           // Landscape mode (240x135)
  lcd.fillScreen(ST77XX_BLACK);
  lcd.setTextColor(ST77XX_WHITE);

  // Draw initial screen
  lcd.setTextSize(1);
  lcd.setCursor(0, 0);
  lcd.setTextColor(ST77XX_CYAN);
  lcd.println("  PELLET STOVE CONTROL");
  lcd.setTextColor(ST77XX_WHITE);
  lcd.println("  Initializing...");

  displayInitialized = true;
  Serial.println("✓ ST7789 LCD initialized (240x135 landscape)");
}

void updateDisplay() {
  if (!displayInitialized) return;

  // Get total learning samples
  int totalSamples = 0;
  for (int f = 0; f < 3; f++) {
    for (int s = 0; s < 4; s++) {
      totalSamples += learningModel[f][s].sampleCount;
    }
  }

  // Clear screen
  lcd.fillScreen(ST77XX_BLACK);

  // Rotate through pages
  currentDisplayPage = (currentDisplayPage + 1) % TOTAL_DISPLAY_PAGES;

  // ===== PAGE 0: ZONES 0 & 1 TEMPERATURES (BIG) =====
  if (currentDisplayPage == 0) {
    lcd.setTextSize(2);
    lcd.setCursor(0, 5);
    lcd.setTextColor(ST77XX_CYAN);
    lcd.println("ZONES 0 & 1");

    // Zone 0 (Main/Stove)
    lcd.setTextSize(4);
    lcd.setCursor(0, 35);
    lcd.setTextColor(ST77XX_ORANGE);
    lcd.print("Z0:");
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print(zoneTemp[0], 1);
    lcd.setTextSize(2);
    lcd.print("F");

    // Zone 1 (Fan 1 area)
    lcd.setTextSize(4);
    lcd.setCursor(0, 85);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.print("Z1:");
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print(zoneTemp[1], 1);
    lcd.setTextSize(2);
    lcd.print("F");
  }

  // ===== PAGE 1: ZONES 2 & 3 TEMPERATURES (BIG) =====
  else if (currentDisplayPage == 1) {
    lcd.setTextSize(2);
    lcd.setCursor(0, 5);
    lcd.setTextColor(ST77XX_CYAN);
    lcd.println("ZONES 2 & 3");

    // Zone 2 (Fan 2 area)
    lcd.setTextSize(4);
    lcd.setCursor(0, 35);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.print("Z2:");
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print(zoneTemp[2], 1);
    lcd.setTextSize(2);
    lcd.print("F");

    // Zone 3 (Fan 3 area)
    lcd.setTextSize(4);
    lcd.setCursor(0, 85);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.print("Z3:");
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print(zoneTemp[3], 1);
    lcd.setTextSize(2);
    lcd.print("F");
  }

  // ===== PAGE 2: STOVE CONTROL =====
  else if (currentDisplayPage == 2) {
    lcd.setTextSize(2);
    lcd.setCursor(0, 5);
    lcd.setTextColor(ST77XX_CYAN);
    lcd.println("STOVE CONTROL");

    // Target temp
    lcd.setTextSize(3);
    lcd.setCursor(0, 35);
    lcd.setTextColor(ST77XX_GREEN);
    lcd.print("Target:");
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print(tempSetpoint, 0);
    lcd.setTextSize(2);
    lcd.print("F");

    // Heat level with BIG blocks
    lcd.setTextSize(2);
    lcd.setCursor(0, 75);
    lcd.setTextColor(ST77XX_YELLOW);
    lcd.print("Heat: ");
    lcd.setTextSize(3);
    for (int i = 1; i <= 5; i++) {
      if (stoveIsOn && i <= currentHeatLevel) {
        lcd.setTextColor(ST77XX_RED);
        lcd.print("#");
      } else {
        lcd.setTextColor(0x4208);
        lcd.print("-");
      }
    }

    // Stove status
    lcd.setTextSize(3);
    lcd.setCursor(0, 110);
    if (stoveIsOn) {
      lcd.setTextColor(ST77XX_RED);
      lcd.print("STOVE ON");
    } else {
      lcd.setTextColor(0x7BEF);
      lcd.print("STOVE OFF");
    }
  }

  // ===== PAGE 3: FAN STATUS =====
  else if (currentDisplayPage == 3) {
    lcd.setTextSize(2);
    lcd.setCursor(0, 5);
    lcd.setTextColor(ST77XX_CYAN);
    lcd.println("CEILING FANS");

    // Fan 1
    lcd.setTextSize(3);
    lcd.setCursor(0, 35);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print("F1: ");
    switch (fan1State.speed) {
      case 0: lcd.setTextColor(0x7BEF); lcd.print("OFF"); break;
      case 1: lcd.setTextColor(ST77XX_GREEN); lcd.print("LOW"); break;
      case 2: lcd.setTextColor(ST77XX_YELLOW); lcd.print("MED"); break;
      case 3: lcd.setTextColor(ST77XX_RED); lcd.print("HIGH"); break;
    }

    // Fan 2
    lcd.setCursor(0, 70);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print("F2: ");
    switch (fan2State.speed) {
      case 0: lcd.setTextColor(0x7BEF); lcd.print("OFF"); break;
      case 1: lcd.setTextColor(ST77XX_GREEN); lcd.print("LOW"); break;
      case 2: lcd.setTextColor(ST77XX_YELLOW); lcd.print("MED"); break;
      case 3: lcd.setTextColor(ST77XX_RED); lcd.print("HIGH"); break;
    }

    // Fan 3
    lcd.setCursor(0, 105);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print("F3: ");
    switch (fan3State.speed) {
      case 0: lcd.setTextColor(0x7BEF); lcd.print("OFF"); break;
      case 1: lcd.setTextColor(ST77XX_GREEN); lcd.print("LOW"); break;
      case 2: lcd.setTextColor(ST77XX_YELLOW); lcd.print("MED"); break;
      case 3: lcd.setTextColor(ST77XX_RED); lcd.print("HIGH"); break;
    }
  }

  // ===== PAGE 4: SYSTEM STATUS =====
  else if (currentDisplayPage == 4) {
    lcd.setTextSize(2);
    lcd.setCursor(0, 5);
    lcd.setTextColor(ST77XX_CYAN);
    lcd.println("SYSTEM STATUS");

    // WiFi
    lcd.setTextSize(3);
    lcd.setCursor(0, 35);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print("WiFi: ");
    if (WiFi.status() == WL_CONNECTED) {
      lcd.setTextColor(ST77XX_GREEN);
      lcd.print("OK");
    } else {
      lcd.setTextColor(ST77XX_RED);
      lcd.print("NO");
    }

    // Auto mode
    lcd.setCursor(0, 70);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print("Mode: ");
    if (autoMode) {
      lcd.setTextColor(ST77XX_YELLOW);
      lcd.print("AUTO");
    } else {
      lcd.setTextColor(0x7BEF);
      lcd.print("MAN");
    }

    // Learning samples
    lcd.setTextSize(2);
    lcd.setCursor(0, 105);
    lcd.setTextColor(ST77XX_WHITE);
    lcd.print("Learning: ");
    lcd.setTextColor(ST77XX_CYAN);
    lcd.print(totalSamples);
    lcd.print(" samples");
  }
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
  Serial.println("✓ IR transmitter initialized on GPIO5");
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

  // Set timer to send data every 5 minutes (300,000 ms) - reduces Blynk message usage
  timer.setInterval(300000L, sendData);

  // Set timer to check for IR signals every 100ms when in learning mode
  timer.setInterval(100L, checkForIRSignal);

  // Set timer to check for RF signals every 100ms when in RF learning mode
  timer.setInterval(100L, checkForRFSignal);

  // Set timer for temperature learning (every 1 minute)
  timer.setInterval(60000L, collectTemperatureSample);

  // Set timer for auto-equalization (every 5 minutes)
  timer.setInterval(300000L, autoEqualizeTemperatures);

  // Set timer for LCD display updates (every 4 seconds - page rotation)
  timer.setInterval(4000L, updateDisplay);

  // Send initial status to Blynk
  Blynk.virtualWrite(STOVE_STATUS_VPIN, stoveIsOn ? 1 : 0);
  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
  Blynk.virtualWrite(TEMP_SETPOINT_VPIN, tempSetpoint);
  Blynk.virtualWrite(AUTO_MODE_VPIN, autoMode ? 1 : 0);
  Blynk.virtualWrite(COOLDOWN_VPIN, adjustmentCooldown / 60000); // Send in minutes
  Blynk.virtualWrite(ZONE_SELECT_VPIN, activeZone);
  Blynk.virtualWrite(REMOTE_TEMP_VPIN, zoneTemp[1]);  // Zone 1 temp
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

  Serial.println("\n========================================");
  Serial.println("  SERIAL COMMAND INTERFACE READY");
  Serial.println("  Type 'HELP' for available commands");
  Serial.println("========================================\n");
}

// ==================== SERIAL COMMAND HANDLER ====================

void processSerialCommand() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  cmd.toUpperCase();

  Serial.print("\n> ");
  Serial.println(cmd);

  // HELP command
  if (cmd == "HELP" || cmd == "?") {
    Serial.println("\n=== AVAILABLE COMMANDS ===");
    Serial.println("\nSENSOR COMMANDS:");
    Serial.println("  TEMP          - Read temperature now");
    Serial.println("  ZONES         - Show all zone status");
    Serial.println("\nSTOVE COMMANDS:");
    Serial.println("  ON            - Turn stove ON");
    Serial.println("  OFF           - Turn stove OFF");
    Serial.println("  HEAT <1-5>    - Set heat level (1-5)");
    Serial.println("  AUTO          - Enable auto mode");
    Serial.println("  MANUAL        - Disable auto mode");
    Serial.println("  TARGET <60-80>- Set target temp (°F)");
    Serial.println("\nFAN COMMANDS:");
    Serial.println("  FAN1 OFF/LOW/MED/HIGH");
    Serial.println("  FAN2 OFF/LOW/MED/HIGH");
    Serial.println("  FAN3 OFF/LOW/MED/HIGH");
    Serial.println("  FANS OFF      - Turn all fans off");
    Serial.println("\nLEARNING COMMANDS:");
    Serial.println("  LEARN IR      - Learn stove remote IR codes");
    Serial.println("  LEARN RF      - Learn fan remote RF codes");
    Serial.println("\nSYSTEM COMMANDS:");
    Serial.println("  STATUS        - Show full system status");
    Serial.println("  WIFI          - Show WiFi status");
    Serial.println("  RESET         - Reset learning data");
    Serial.println("==========================\n");
    return;
  }

  // TEMP command
  if (cmd == "TEMP") {
    sendData();  // Force immediate temp read
    return;
  }

  // ZONES command
  if (cmd == "ZONES") {
    Serial.println("\n--- ALL ZONES ---");
    for (int i = 0; i < TOTAL_ZONES; i++) {
      Serial.print("Zone ");
      Serial.print(i);
      Serial.print(": ");
      unsigned long age = (millis() - lastZoneTempTime[i]) / 1000;
      if (age < 300 || i == 0) {
        Serial.print(zoneTemp[i], 1);
        Serial.print("°F, ");
        Serial.print(zoneHumidity[i], 0);
        Serial.print("% (");
        Serial.print(age);
        Serial.println("s ago)");
      } else {
        Serial.println("OFFLINE");
      }
    }
    Serial.println();
    return;
  }

  // STATUS command
  if (cmd == "STATUS") {
    Serial.println("\n=== SYSTEM STATUS ===");
    Serial.print("Stove: ");
    Serial.println(stoveIsOn ? "ON" : "OFF");
    Serial.print("Heat Level: ");
    Serial.print(currentHeatLevel);
    Serial.println("/5");
    Serial.print("Mode: ");
    Serial.println(autoMode ? "AUTO" : "MANUAL");
    Serial.print("Target: ");
    Serial.print(tempSetpoint);
    Serial.println("°F");
    Serial.print("Current: ");
    Serial.print(currentTemp);
    Serial.println("°F");
    Serial.print("WiFi: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    Serial.print("Fan 1: ");
    Serial.println(fan1State.speed == 0 ? "OFF" : fan1State.speed == 1 ? "LOW" : fan1State.speed == 2 ? "MED" : "HIGH");
    Serial.print("Fan 2: ");
    Serial.println(fan2State.speed == 0 ? "OFF" : fan2State.speed == 1 ? "LOW" : fan2State.speed == 2 ? "MED" : "HIGH");
    Serial.print("Fan 3: ");
    Serial.println(fan3State.speed == 0 ? "OFF" : fan3State.speed == 1 ? "LOW" : fan3State.speed == 2 ? "MED" : "HIGH");
    Serial.println("=====================\n");
    return;
  }

  // ON command
  if (cmd == "ON") {
    Serial.println("✓ Turning stove ON");
    turnStoveOn();
    return;
  }

  // OFF command
  if (cmd == "OFF") {
    Serial.println("✓ Turning stove OFF");
    turnStoveOff();
    return;
  }

  // AUTO command
  if (cmd == "AUTO") {
    autoMode = true;
    Serial.println("✓ AUTO mode enabled");
    return;
  }

  // MANUAL command
  if (cmd == "MANUAL") {
    autoMode = false;
    Serial.println("✓ MANUAL mode enabled");
    return;
  }

  // FANS OFF command
  if (cmd == "FANS OFF") {
    Serial.println("✓ Turning all fans OFF");
    if (fan1_Off) sendRFCommand(fan1_Off, rfBitLength);
    if (fan2_Off) sendRFCommand(fan2_Off, rfBitLength);
    if (fan3_Off) sendRFCommand(fan3_Off, rfBitLength);
    updateFanState(0, 0);
    updateFanState(1, 0);
    updateFanState(2, 0);
    return;
  }

  // HEAT <level> command
  if (cmd.startsWith("HEAT ")) {
    int level = cmd.substring(5).toInt();
    if (level >= 1 && level <= 5) {
      currentHeatLevel = level;
      Serial.print("✓ Heat level set to ");
      Serial.println(level);
      if (stoveIsOn) {
        adjustHeatLevel(level);
      }
    } else {
      Serial.println("❌ Invalid heat level (use 1-5)");
    }
    return;
  }

  // TARGET <temp> command
  if (cmd.startsWith("TARGET ")) {
    int temp = cmd.substring(7).toInt();
    if (temp >= 60 && temp <= 80) {
      tempSetpoint = temp;
      Serial.print("✓ Target temperature set to ");
      Serial.print(temp);
      Serial.println("°F");
    } else {
      Serial.println("❌ Invalid temp (use 60-80°F)");
    }
    return;
  }

  // FAN commands
  if (cmd.startsWith("FAN1 ")) {
    String speed = cmd.substring(5);
    Serial.print("✓ Fan 1 → ");
    Serial.println(speed);
    if (speed == "OFF") { sendRFCommand(fan1_Off, rfBitLength); updateFanState(0, 0); }
    else if (speed == "LOW") { sendRFCommand(fan1_Low, rfBitLength); updateFanState(0, 1); }
    else if (speed == "MED") { sendRFCommand(fan1_Med, rfBitLength); updateFanState(0, 2); }
    else if (speed == "HIGH") { sendRFCommand(fan1_High, rfBitLength); updateFanState(0, 3); }
    else Serial.println("❌ Invalid speed (OFF/LOW/MED/HIGH)");
    return;
  }

  if (cmd.startsWith("FAN2 ")) {
    String speed = cmd.substring(5);
    Serial.print("✓ Fan 2 → ");
    Serial.println(speed);
    if (speed == "OFF") { sendRFCommand(fan2_Off, rfBitLength); updateFanState(1, 0); }
    else if (speed == "LOW") { sendRFCommand(fan2_Low, rfBitLength); updateFanState(1, 1); }
    else if (speed == "MED") { sendRFCommand(fan2_Med, rfBitLength); updateFanState(1, 2); }
    else if (speed == "HIGH") { sendRFCommand(fan2_High, rfBitLength); updateFanState(1, 3); }
    else Serial.println("❌ Invalid speed (OFF/LOW/MED/HIGH)");
    return;
  }

  if (cmd.startsWith("FAN3 ")) {
    String speed = cmd.substring(5);
    Serial.print("✓ Fan 3 → ");
    Serial.println(speed);
    if (speed == "OFF") { sendRFCommand(fan3_Off, rfBitLength); updateFanState(2, 0); }
    else if (speed == "LOW") { sendRFCommand(fan3_Low, rfBitLength); updateFanState(2, 1); }
    else if (speed == "MED") { sendRFCommand(fan3_Med, rfBitLength); updateFanState(2, 2); }
    else if (speed == "HIGH") { sendRFCommand(fan3_High, rfBitLength); updateFanState(2, 3); }
    else Serial.println("❌ Invalid speed (OFF/LOW/MED/HIGH)");
    return;
  }

  // WIFI command
  if (cmd == "WIFI") {
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.print(WiFi.RSSI());
      Serial.println(" dBm");
    }
    return;
  }

  // RESET command
  if (cmd == "RESET") {
    Serial.println("⚠️  Resetting all learning data...");
    initLearningModel();
    saveLearningData();
    Serial.println("✓ Learning data reset complete!");
    return;
  }

  // LEARN IR command
  if (cmd == "LEARN IR" || cmd == "LEARNIR") {
    learningMode = true;
    learningStep = 0;
    Serial.println("\n========================================");
    Serial.println("       IR LEARNING MODE ACTIVATED");
    Serial.println("========================================");
    Serial.println("\nPoint your pellet stove remote at the");
    Serial.println("IR receiver (GPIO14)");
    Serial.println("\nYou will capture 4 codes:");
    Serial.println("  1. Power ON");
    Serial.println("  2. Power OFF");
    Serial.println("  3. Heat UP");
    Serial.println("  4. Heat DOWN");
    Serial.println("\nPress POWER ON button now...");
    Serial.println("========================================\n");
    irrecv.enableIRIn(); // Start the receiver
    return;
  }

  // LEARN RF command
  if (cmd == "LEARN RF" || cmd == "LEARNRF") {
    rfLearningMode = true;
    rfLearningStep = 0;
    Serial.println("\n========================================");
    Serial.println("       RF LEARNING MODE ACTIVATED");
    Serial.println("========================================");
    Serial.println("\nPoint your ceiling fan remotes at the");
    Serial.println("RF receiver (GPIO17)");
    Serial.println("\nYou will capture 15 codes:");
    Serial.println("  Fan 1: OFF, LOW, MED, HIGH, LIGHT (5)");
    Serial.println("  Fan 2: OFF, LOW, MED, HIGH, LIGHT (5)");
    Serial.println("  Fan 3: OFF, LOW, MED, HIGH, LIGHT (5)");
    Serial.println("\nPress Fan 1 OFF button now...");
    Serial.println("========================================\n");
    rfSwitch.enableReceive(digitalPinToInterrupt(RF_RECEIVE_PIN));
    return;
  }

  // Unknown command
  Serial.println("❌ Unknown command. Type HELP for available commands.");
}

void loop() {
  Blynk.run();
  timer.run();
  processSerialCommand();  // Check for serial commands
}
