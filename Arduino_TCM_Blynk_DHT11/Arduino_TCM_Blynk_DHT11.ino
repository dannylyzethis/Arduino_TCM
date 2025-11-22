#define BLYNK_TEMPLATE_ID "TMPL24OkwKw_5"
#define BLYNK_TEMPLATE_NAME "test"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>

// Define DHT11 pin and type
#define DHTPIN 4        // GPIO4 (adjust if your DHT11 is on a different pin)
#define DHTTYPE DHT11   // DHT11 sensor

// Blynk Auth Token (from your Blynk project)
char auth[] = "39B95_5S2gbtY8hGCdPOEz4XOiyVqlpz";  // Replace with your Blynk Auth Token

// WiFi credentials
char ssid[] = "Pahouse 2.4";   // Replace with your WiFi SSID
char pass[] = "jennyjenny92";   // Replace with your WiFi password

// Virtual pins for Blynk
#define TEMP_VPIN V0    // Virtual pin for temperature
#define HUM_VPIN V2     // Virtual pin for humidity

// Initialize DHT sensor and Blynk timer
DHT dht(DHTPIN, DHTTYPE);
BlynkTimer timer;

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

  // Send data to Blynk app
  Blynk.virtualWrite(TEMP_VPIN, f);  // Send temperature
  Blynk.virtualWrite(HUM_VPIN, h);   // Send humidity

  // Print readings to serial monitor
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C");
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.println(" %");
}

void setup() {
  // Start serial communication
  Serial.begin(115200);

  // Small delay to allow sensor to stabilize
  delay(1000);

  // Initialize DHT sensor
  dht.begin();

  // Connect to Blynk
  Blynk.begin(auth, ssid, pass);

  // Set timer to send data every 5 minutes (300,000 ms)
  timer.setInterval(300000L, sendData);
}

void loop() {
  // Keep Blynk and timer running
  Blynk.run();

  timer.run();
}
