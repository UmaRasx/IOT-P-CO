#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Replace with your Wi-Fi credentials
const char* ssid = "vivoLaTT1";
const char* password = "alkhikam";

//const char* ssid = "SNK";
//const char* password = "Samin2237";

// Replace with your ThingSpeak API key
const char* apiKey = "JNG4L3C9Y984B7JH";

// Replace with your ThingSpeak channel ID
const unsigned long channelID = 2189706;

// Define the pin for the MQ-7 gas sensor
const int mq7Pin = 34;

// ThingSpeak server settings
const char* server = "api.thingspeak.com";
const int httpPort = 80;

// Define calibration constants
const float RO_CLEAN_AIR_FACTOR = 9.83;  // RO value in clean air
const float CALIBRATION_SAMPLE_TIMES = 3;      // Number of samples to take during calibration
const float CALIBRATION_SAMPLE_INTERVAL = 15000; // Interval between calibration samples in milliseconds

// Create an instance of the WiFiClient class
WiFiClient client;

// Define the pin connections and create an instance of the OLED display object
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

void connectToWiFi();
void sendDataToThingSpeak(float value);

void setup() {
  // Start serial communication
  Serial.begin(9600);

  // Connect to Wi-Fi network
  connectToWiFi();

  // Initialize I2C bus for the OLED display
  Wire.begin(21, 22); // SDA pin = GPIO 21, SCL pin = GPIO 22

  // Initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
}

void loop() {
  // Read gas sensor value
  int rawValue = analogRead(mq7Pin);
  float gasValue = rawValue / 1023.0 * 3.3;  // Convert raw input to decimal (assuming a 10-bit ADC and 3.3V reference voltage)

  // Print the gas value to the serial monitor
  Serial.print("Gas value: ");
  Serial.println(gasValue, 2);  // Print gas value with 2 decimal places

  // Print the gas value on the OLED display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("Gas value: ");
  display.println(gasValue, 2);
  display.display();

  // Delay for 1 second to allow the sensor to stabilize
  delay(1000);

  // Send data to ThingSpeak
  sendDataToThingSpeak(gasValue);

  // Wait for 20 seconds
  delay(20000);
}

void connectToWiFi() {
  // Connect to Wi-Fi network
  Serial.print("Connecting to Wi-Fi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected to Wi-Fi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void sendDataToThingSpeak(float value) {
  // Calibrate the gas sensor value
  float ro = 0;
  for (int i = 0; i < CALIBRATION_SAMPLE_TIMES; i++) {
    // Read gas sensor value
    int sensorValue = analogRead(mq7Pin);

    // Calculate Ro (Resistance in clean air)
    ro += (float)sensorValue / RO_CLEAN_AIR_FACTOR;

    delay(CALIBRATION_SAMPLE_INTERVAL);
  }
  ro = ro / CALIBRATION_SAMPLE_TIMES;

  // Calculate Rs/Ro (Ratio of sensor resistance to Ro)
  float ratio = value / ro;

  // Construct the ThingSpeak update URL
  String url = "/update?api_key=";
  url += apiKey;
  url += "&field1=";
  url += String(value, 2);  // Limit gas value to 2 decimal places
  url += "&field2=";
  url += String(ratio, 2);  // Limit ratio to 2 decimal places

  Serial.print("Sending data to ThingSpeak...");

  if (client.connect(server, httpPort)) {
    client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                 "Host: " + server + "\r\n" +
                 "Connection: close\r\n\r\n");
    client.stop();
    Serial.println("Data sent to ThingSpeak!");
  } else {
    Serial.println("Connection to ThingSpeak failed!");
  }
}
