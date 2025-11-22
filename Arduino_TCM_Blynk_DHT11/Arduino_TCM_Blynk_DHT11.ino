#define BLYNK_TEMPLATE_ID "TMPL24OkwKw_5"
#define BLYNK_TEMPLATE_NAME "test"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>

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

// Initialize components
DHT dht(DHTPIN, DHTTYPE);
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
BlynkTimer timer;

// Stove control variables
bool stoveIsOn = false;
bool autoMode = false;
int currentHeatLevel = 3;           // Current heat level (1-5), starts at medium
float tempSetpoint = 70.0;          // Default target temperature in °F
float currentTemp = 0.0;
float tempHysteresis = 2.0;         // Temperature buffer for adjustment trigger
unsigned long lastAdjustmentTime = 0;
unsigned long adjustmentCooldown = 300000; // 5 minutes between auto adjustments (300000 ms)

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
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  currentTemp = f; // Store current temperature for automation

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

  // Connect to Blynk
  Serial.println("Connecting to WiFi and Blynk...");
  Blynk.begin(auth, ssid, pass);
  Serial.println("✓ Connected to Blynk");

  // Set timer to send data every 2 minutes (120,000 ms)
  timer.setInterval(120000L, sendData);

  // Set timer to check for IR signals every 100ms when in learning mode
  timer.setInterval(100L, checkForIRSignal);

  // Send initial status to Blynk
  Blynk.virtualWrite(STOVE_STATUS_VPIN, stoveIsOn ? 1 : 0);
  Blynk.virtualWrite(HEAT_LEVEL_VPIN, currentHeatLevel);
  Blynk.virtualWrite(TEMP_SETPOINT_VPIN, tempSetpoint);
  Blynk.virtualWrite(AUTO_MODE_VPIN, autoMode ? 1 : 0);

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
  Serial.println("\nAuto mode adjusts heat level every 5 minutes based on temperature");
  Serial.println();
}

void loop() {
  Blynk.run();
  timer.run();
}
