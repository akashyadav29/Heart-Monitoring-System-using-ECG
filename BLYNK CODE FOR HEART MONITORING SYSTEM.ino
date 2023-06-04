#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MAX30100_PulseOximeter.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BlynkSimpleEsp8266.h>

#define REPORTING_PERIOD_MS 1000

// Wi-Fi credentials
char ssid[] = "YourWiFiSSID";
char password[] = "YourWiFiPassword";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_ADDRESS);

#define SENSOR A0 // Set the A0 as SENSOR
#define BUZZER_PIN D3 // Set the D3 pin for the buzzer

#define THRESHOLD_HIGH 800 // Define the high threshold value for the ECG
#define THRESHOLD_LOW 200 // Define the low threshold value for the ECG

PulseOximeter pox;

float BPM, SpO2;
uint32_t tsLastReport = 0;

OneWire oneWire(D4); // Set the D4 pin for the DS18B20 temperature sensor
DallasTemperature sensors(&oneWire);
DeviceAddress tempDeviceAddress;

// Blynk authentication token
char auth[] = "YourAuthToken";

void onBeatDetected()
{
  Serial.println("Beat Detected!");
  // Add your code to handle beat detection
}

void setup()
{
  Serial.begin(115200);
  pinMode(SENSOR, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(D2, D1); // Initialize I2C communication with SDA on GPIO4/D2 and SCL on GPIO5/D1

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);

  Blynk.begin(auth, ssid, password);

  if (!pox.begin())
  {
    Serial.println("Failed to initialize Pulse Oximeter!");
    while (1);
  }

  pox.setOnBeatDetectedCallback(onBeatDetected);

  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, 9);
}

void loop()
{
  Blynk.run();

  pox.update();

  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();

  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);

  if (millis() - tsLastReport > REPORTING_PERIOD_MS)
  {
    Serial.print("Heart rate: ");
    Serial.print(BPM);
    Serial.print(" bpm / SpO2: ");
    Serial.print(SpO2);
    Serial.print(" % / Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Blynk.virtualWrite(V1, BPM); // Send heart rate value to Blynk virtual pin V1
    Blynk.virtualWrite(V2, SpO2); // Send SpO2 value to Blynk virtual pin V2
    Blynk.virtualWrite(V3, temperature); // Send temperature value to Blynk virtual pin V3

    // Check if temperature or SpO2 exceeds threshold values
    if (temperature > 30.0 || temperature < 20.0 || SpO2 > 95.0)
    {
      digitalWrite(BUZZER_PIN, HIGH); // Turn on the buzzer
      Blynk.virtualWrite(V4, HIGH); // Send buzzer status to Blynk virtual pin V4
    }
    else
    {
      digitalWrite(BUZZER_PIN, LOW); // Turn off the buzzer
      Blynk.virtualWrite(V4, LOW); // Send buzzer status to Blynk virtual pin V4
    }

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print("HR: ");
    display.print(BPM);
    display.setCursor(0, 16);
    display.print("SpO2: ");
    display.print(SpO2);
    display.setCursor(0, 32);
    display.print("Temp: ");
    display.print(temperature);
    display.display();

    tsLastReport = millis();
  }
}
