// Remote Temperature Sensor for Multi-Zone Pellet Stove Control
// This ESP32 sends temperature data to the main controller via ESP-NOW

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <DHT.h>

// DHT11 Configuration
#define DHTPIN 4        // GPIO4 (adjust if different)
#define DHTTYPE DHT11   // DHT11 sensor

// IMPORTANT: Replace with YOUR main controller's MAC address
// To find MAC address, upload this code to main ESP32 and check Serial Monitor
uint8_t mainControllerMAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};  // REPLACE THIS!

// WiFi credentials (needed for ESP-NOW to work properly)
const char* ssid = "Pahouse 2.4";       // Same as main controller
const char* password = "jennyjenny92";  // Same as main controller

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Data structure (must match main controller)
typedef struct {
  float temperature;
  float humidity;
  uint8_t sensorID;
} RemoteSensorData;

RemoteSensorData sensorData;

// Peer info for ESP-NOW
esp_now_peer_info_t peerInfo;

// Send interval (every 30 seconds)
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 30000;

// Callback when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== Remote Temperature Sensor ===");
  Serial.println("Zone 2 - ESP-NOW Transmitter");

  // Check if MAC address has been configured
  if (mainControllerMAC[0] == 0xFF && mainControllerMAC[1] == 0xFF &&
      mainControllerMAC[2] == 0xFF && mainControllerMAC[3] == 0xFF &&
      mainControllerMAC[4] == 0xFF && mainControllerMAC[5] == 0xFF) {
    Serial.println("\n*** ERROR: DEFAULT MAC ADDRESS DETECTED! ***");
    Serial.println("You MUST update 'mainControllerMAC' with your main controller's MAC address!");
    Serial.println("The system will continue but ESP-NOW will not work.");
    Serial.println("***********************************************\n");
  }

  // Initialize DHT sensor
  dht.begin();
  Serial.println("✓ DHT11 sensor initialized");

  // Connect to WiFi (required for ESP-NOW)
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  // Add timeout for WiFi connection (30 seconds)
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 60) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected");
  } else {
    Serial.println("\n⚠ WiFi connection timeout!");
    Serial.println("ESP-NOW will still work on same WiFi channel");
  }

  // Print MAC address (needed for main controller setup)
  Serial.print("This device's MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Copy this MAC to the main controller if needed!");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: ESP-NOW initialization failed!");
    Serial.println("System cannot function without ESP-NOW. Restarting in 10 seconds...");
    delay(10000);
    ESP.restart();
  }
  Serial.println("✓ ESP-NOW initialized");

  // Register send callback
  esp_now_register_send_cb(onDataSent);

  // Add main controller as peer
  memcpy(peerInfo.peer_addr, mainControllerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("ERROR: Failed to add peer!");
    Serial.println("Please verify the main controller MAC address is correct.");
    Serial.println("System will retry, but data may not be received...");
  } else {
    Serial.println("✓ Main controller added as peer");
  }

  // Sensor ID (useful if you have multiple remote sensors)
  sensorData.sensorID = 1;  // Change to 2, 3, etc. for additional sensors

  Serial.println("\n=== System Ready ===");
  Serial.println("Sending temperature data every 30 seconds...");
  Serial.println();
}

void loop() {
  unsigned long currentTime = millis();

  // Send data every 30 seconds
  if (currentTime - lastSendTime >= sendInterval) {
    lastSendTime = currentTime;

    // Read temperature and humidity
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = (t * 9.0 / 5.0) + 32.0;  // Convert to Fahrenheit

    // Check if readings are valid
    if (isnan(h) || isnan(t)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }

    // Prepare data packet
    sensorData.temperature = f;
    sensorData.humidity = h;

    // Send data via ESP-NOW
    esp_err_t result = esp_now_send(mainControllerMAC, (uint8_t *) &sensorData, sizeof(sensorData));

    // Print to serial
    Serial.print("Sending - Temp: ");
    Serial.print(f);
    Serial.print("°F (");
    Serial.print(t);
    Serial.print("°C), Humidity: ");
    Serial.print(h);
    Serial.print("% - ");

    if (result == ESP_OK) {
      Serial.println("Sent successfully");
    } else {
      Serial.println("Error sending data");
    }
  }

  delay(100);  // Small delay
}
