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
#define TEMP_VPIN V0        // Temperature (°F)
#define HUM_VPIN V2         // Humidity (%)
#define STOVE_POWER_VPIN V3 // Stove Power Button
#define STOVE_STATUS_VPIN V4 // Stove Status (On/Off)
#define TEMP_SETPOINT_VPIN V5 // Target temperature setpoint
#define AUTO_MODE_VPIN V6    // Auto mode switch (0=Manual, 1=Auto)
#define LEARN_MODE_VPIN V7   // IR Learning mode button

// Initialize components
DHT dht(DHTPIN, DHTTYPE);
IRsend irsend(IR_SEND_PIN);
IRrecv irrecv(IR_RECV_PIN);
decode_results results;
BlynkTimer timer;

// Stove control variables
bool stoveIsOn = false;
bool autoMode = false;
float tempSetpoint = 70.0;  // Default target temperature in °F
float currentTemp = 0.0;
float tempHysteresis = 2.0; // Temperature buffer to prevent rapid on/off cycling

// IR code storage (you'll capture these from your stove remote)
uint64_t irCode_Power = 0;      // Store your stove's power button code
uint64_t irCode_HeatUp = 0;     // Store heat increase code
uint64_t irCode_HeatDown = 0;   // Store heat decrease code
decode_type_t irProtocol = UNKNOWN; // Will be detected when learning

// Learning mode
bool learningMode = false;
int learningStep = 0;  // 0=Power, 1=HeatUp, 2=HeatDown

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

// Toggle stove power
void toggleStovePower() {
  sendIRCommand(irCode_Power);
  stoveIsOn = !stoveIsOn;

  Serial.print("Stove turned ");
  Serial.println(stoveIsOn ? "ON" : "OFF");

  Blynk.virtualWrite(STOVE_STATUS_VPIN, stoveIsOn ? 1 : 0);
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
          irCode_Power = results.value;
          irProtocol = results.decode_type;
          Serial.println("✓ Power button learned! Now press HEAT UP on your remote...");
          learningStep = 1;
          break;
        case 1:
          irCode_HeatUp = results.value;
          Serial.println("✓ Heat Up learned! Now press HEAT DOWN on your remote...");
          learningStep = 2;
          break;
        case 2:
          irCode_HeatDown = results.value;
          Serial.println("✓ Heat Down learned! All codes captured!");
          Serial.println("\n=== IR CODES SAVED ===");
          Serial.print("Power: 0x");
          Serial.println(uint64ToString(irCode_Power, HEX));
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

// Temperature-based automation
void checkTemperatureAutomation() {
  if (!autoMode) return;

  // Turn stove ON if temperature drops below setpoint minus hysteresis
  if (currentTemp < (tempSetpoint - tempHysteresis) && !stoveIsOn) {
    Serial.println("AUTO: Temperature too low, turning stove ON");
    toggleStovePower();
  }
  // Turn stove OFF if temperature exceeds setpoint plus hysteresis
  else if (currentTemp > (tempSetpoint + tempHysteresis) && stoveIsOn) {
    Serial.println("AUTO: Temperature reached, turning stove OFF");
    toggleStovePower();
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

  if (autoMode) {
    Serial.print("Target: ");
    Serial.print(tempSetpoint);
    Serial.print(" °F | Stove: ");
    Serial.println(stoveIsOn ? "ON" : "OFF");
  }

  // Check if automation should trigger
  checkTemperatureAutomation();
}

// Blynk: Manual power button control
BLYNK_WRITE(STOVE_POWER_VPIN) {
  int buttonState = param.asInt();
  if (buttonState == 1 && !learningMode) {
    toggleStovePower();
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
    Serial.println("Stove will automatically control based on temperature");
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
    Serial.println("Press the POWER button on your remote now...");
    irrecv.enableIRIn(); // Start the receiver
  }
}

void setup() {
  // Start serial communication
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Arduino TCM - Pellet Stove Controller ===");

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
  Blynk.virtualWrite(TEMP_SETPOINT_VPIN, tempSetpoint);
  Blynk.virtualWrite(AUTO_MODE_VPIN, autoMode ? 1 : 0);

  Serial.println("\n=== System Ready ===");
  Serial.println("Use Blynk app to:");
  Serial.println("- V3: Manual stove power control");
  Serial.println("- V5: Set target temperature");
  Serial.println("- V6: Enable/disable auto mode");
  Serial.println("- V7: Enter IR learning mode");
  Serial.println();
}

void loop() {
  Blynk.run();
  timer.run();
}
