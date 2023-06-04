#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <MAX30100_PulseOximeter.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "OakOLED.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_ADDRESS);

#define SENSOR A0
#define BUZZER_PIN D3

#define THRESHOLD_HIGH 800 
#define THRESHOLD_LOW 200 

OakOLED oled;
PulseOximeter pox;

float BPM, SpO2;
float temperature;
uint32_t tsLastReport = 0;

const unsigned char bitmap[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x18, 0x00, 0x0f, 0xe0, 0x7f, 0x00, 0x3f, 0xf9, 0xff, 0xc0,
  0x7f, 0xf9, 0xff, 0xc0, 0x7f, 0xff, 0xff, 0xe0, 0x7f, 0xff, 0xff, 0xe0, 0xff, 0xff, 0xff, 0xf0,
  0xff, 0xf7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0xff, 0xe7, 0xff, 0xf0, 0x7f, 0xdb, 0xff, 0xe0,
  0x7f, 0x9b, 0xff, 0xe0, 0x00, 0x3b, 0xc0, 0x00, 0x3f, 0xf9, 0x9f, 0xc0, 0x3f, 0xfd, 0xbf, 0xc0,
  0x1f, 0xfd, 0xbf, 0x80, 0x0f, 0xfd, 0x7f, 0x00, 0x07, 0xfe, 0x7e, 0x00, 0x03, 0xfe, 0xfc, 0x00,
  0x01, 0xff, 0xf8, 0x00, 0x00, 0xff, 0xf0, 0x00, 0x00, 0x7f, 0xe0, 0x00, 0x00, 0x3f, 0xc0, 0x00,
  0x00, 0x0f, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define ONE_WIRE_BUS D4 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress temperatureSensor;

void onBeatDetected()
{
  Serial.println("Beat Detected!");
  oled.drawBitmap(60, 20, bitmap, 28, 28, 1);
  oled.display();
}

void setup()
{
  Serial.begin(115200);
  pinMode(SENSOR, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(D2, D1); 

  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  oled.begin();
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(1);
  oled.setCursor(0, 0);

  oled.println("Initializing pulse oximeter..");
  oled.display();

  pinMode(16, OUTPUT);

  Serial.print("Initializing Pulse Oximeter..");

  if (!pox.begin())
  {
    Serial.println("FAILED");
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("FAILED");
    oled.display();
    while (1);
  }
  else
  {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("SUCCESS");
    oled.display();
    Serial.println("SUCCESS");
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  
  sensors.begin();
  if (!sensors.getAddress(temperatureSensor, 0))
  {
    Serial.println("No temperature sensor found!");
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("Temp Sensor Not Found");
    oled.display();
    while (1);
  }
}

void loop()
{
  pox.update();

  BPM = pox.getHeartRate();
  SpO2 = pox.getSpO2();

  sensors.requestTemperatures();
  temperature = sensors.getTempC(temperatureSensor);

  if (millis() - tsLastReport > 1000)
  {
    Serial.print("Heart rate: ");
    Serial.print(BPM);
    Serial.print(" bpm / SpO2: ");
    Serial.print(SpO2);
    Serial.print(" % / Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 0);
    oled.println("Heart Rate: " + String(BPM) + " bpm");

    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 15);
    oled.println("SpO2: " + String(SpO2) + " %");

    oled.setTextSize(1);
    oled.setTextColor(1);
    oled.setCursor(0, 30);
    oled.println("Temperature: " + String(temperature) + " C");

    oled.display();
    delay(1000);

    if (temperature > 50.0 || temperature < 20.0 || SpO2 > 95.0)
    {
      digitalWrite(BUZZER_PIN, HIGH); 
    }
    else
    {
      digitalWrite(BUZZER_PIN, LOW);
    }

    tsLastReport = millis();
  }

  int sensorValue = analogRead(SENSOR); 

  Serial.print("Sensor Value: ");
  Serial.println(sensorValue);

  if (sensorValue > THRESHOLD_HIGH || sensorValue < THRESHOLD_LOW)
  {
    digitalWrite(BUZZER_PIN, HIGH);
  }
  else
  {
    digitalWrite(BUZZER_PIN, LOW); 
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.print("ECG Value: ");
  display.print(sensorValue);
  display.display();

  delay(1000);
}
