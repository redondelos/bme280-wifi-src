#include <Arduino.h>

#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>

// BME280 sensor
#define SEALEVELPRESSURE_HPA (1013.25) // hPa
Adafruit_BME280 bme;

OneWire ourWire(4);

DallasTemperature sensors(&ourWire);

// OLED SCREEN 1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define SCREEN_I2C_ADDRESS 0x3C
#define OLED_RESET_PIN -1

// OLED SCREEN 2
#define SCREEN2_WIDTH 128
#define SCREEN2_HEIGHT 32
#define SCREEN2_I2C_ADDRESS 0x3C
TwoWire I2C_1 = TwoWire(1); // second I2C bus

Adafruit_SSD1306 screen(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);
Adafruit_SSD1306 screen2(SCREEN2_WIDTH, SCREEN2_HEIGHT, &I2C_1, OLED_RESET_PIN);

// Declare functions to print stuff on screens
void printTitles();
void printData(float temperature, float humidity, float atmPressure);

// Wireless Connection and Webpage
// Create a web server on port 80
WebServer server(80);

const char *ssid = "NETLIFE-PUENTE";
const char *password = "LosElenes1996";
void handleRoot();
void handleCSS();
void handleNormalize();
void handleJS();
void handleSensor();
void handleLedOn();
void handleLedOff();
void handleLedData();
void setLed(bool state);
void printWifi();

void addCorsHeaders();

// LED
const int ledPin = 23;
bool outputState;

// Symbols
const unsigned char tempIcon[] PROGMEM = {
    0xff, 0x3f, 0xf0, 0xfe, 0xdf, 0xf0, 0xfe, 0xd1, 0xf0, 0xfe, 0x5f, 0xf0, 0xfe, 0x5f, 0xf0, 0xfe,
    0x51, 0xf0, 0xfe, 0x57, 0xf0, 0xfe, 0x5f, 0xf0, 0xfe, 0x51, 0xf0, 0xfe, 0x5f, 0xf0, 0xfd, 0x2f,
    0xf0, 0xff, 0x8f, 0xf0, 0xfb, 0x1f, 0xf0, 0xfe, 0x0f, 0xf0, 0xfd, 0x2f, 0xf0, 0xfe, 0x1f, 0xf0};

const unsigned char pressureIcon[] PROGMEM = {
    0xfe, 0x07, 0xf0, 0xf8, 0xf1, 0xf0, 0xf3, 0x0c, 0xf0, 0xe5, 0xfa, 0x70, 0xeb, 0x7d, 0x70, 0xcb,
    0x3d, 0x30, 0xd7, 0x1e, 0xb0, 0xd7, 0x6e, 0xb0, 0xd7, 0x6e, 0xb0, 0xd7, 0x9e, 0xb0, 0xcb, 0xfd,
    0x30, 0xef, 0xff, 0x70, 0xe7, 0xfe, 0x70, 0xf3, 0xfc, 0xf0, 0xf8, 0xf1, 0xf0, 0xfe, 0x07, 0xf0};

const unsigned char humidityIcon[] PROGMEM = {
    0xff, 0x9f, 0xf0, 0xff, 0x9f, 0xf0, 0xff, 0x0f, 0xf0, 0xfe, 0x07, 0xf0, 0xfc, 0x03, 0xf0, 0xfc,
    0x03, 0xf0, 0xf9, 0xc1, 0xf0, 0xf9, 0x49, 0xf0, 0xf1, 0xd0, 0xf0, 0xf0, 0x20, 0xf0, 0xf0, 0x40,
    0xf0, 0xf0, 0xb8, 0xf0, 0xf9, 0x29, 0xf0, 0xf8, 0x39, 0xf0, 0xfc, 0x03, 0xf0, 0xff, 0x0f, 0xf0};

const unsigned char wifi[] PROGMEM = {
    0xfe, 0x07, 0xf0, 0xf0, 0x00, 0xf0, 0xe0, 0xf0, 0x70, 0xc7, 0xfe, 0x30, 0x8f, 0xff, 0x10, 0x1c,
    0x03, 0x80, 0xb8, 0x01, 0xd0, 0xf1, 0xf8, 0xf0, 0xe3, 0xfc, 0x70, 0xf7, 0x0e, 0xf0, 0xfc, 0x03,
    0xf0, 0xfc, 0x63, 0xf0, 0xfe, 0xf7, 0xf0, 0xff, 0x9f, 0xf0, 0xff, 0x9f, 0xf0, 0xff, 0x9f, 0xf0};

const unsigned char nowifi[] PROGMEM = {
    0xff, 0xff, 0xf0, 0xff, 0xff, 0xf0, 0xff, 0xfb, 0xf0, 0xfc, 0x03, 0xf0, 0xf9, 0xf5, 0xf0, 0xf7,
    0xee, 0xf0, 0xfe, 0x07, 0xf0, 0xfc, 0xd3, 0xf0, 0xff, 0xbf, 0xf0, 0xff, 0x0f, 0xf0, 0xfe, 0x7f,
    0xf0, 0xfe, 0xff, 0xf0, 0xfc, 0x9f, 0xf0, 0xfd, 0xff, 0xf0, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xf0};

// Count seconds since program started
const unsigned long INTERVAL = 1000; // ms
unsigned long lastRun = 0;

void setup()
{
  Serial.begin(9600);

  // BME 280 sensor
  // ---------------------------------------------------------------
  Wire.begin(21, 22); // SDA, SCL
  if (!bme.begin(0x76))
  {
    Serial.println("BME280 not found. Check wiring/address.");
    while (1)
      ;
  }
  Serial.println("BME280 ready.");
  // ---------------------------------------------------------------

  // DS18B20 Sensor
  // ---------------------------------------------------------------
  sensors.begin();
  // ---------------------------------------------------------------

  //                          WiFi Configuration
  // ===============================================================

  // Connect to wifi
  // ---------------------------------------------------------------
  Serial.println("Connecting to Wi-Fi...");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wifi Connected!");

  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  // LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  // ---------------------------------------------------------------

  //                          Webpage Configuration
  // ===============================================================
  // Start LittleFS
  // ---------------------------------------------------------------
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS failed!");
  }

  // Tell the server what to do when someone visits "/"
  server.on("/", handleRoot);

  // Tell the server what to do when someone visits "/styles.css"
  // and /normalize.css
  server.on("/styles.css", handleCSS);
  server.on("/normalize.css", handleNormalize);

  // Tell the server what to do when someone visits "/script.js"
  server.on("/script.js", handleJS);

  // Tell the server what to do when someone visits "/temperature"
  server.on("/bme280", handleSensor);

  // Tell the server what to do when someone visits "/led/on"
  server.on("/led/on", handleLedOn);

  // Tell the server what to do when someone visits "/led/on"
  server.on("/led/off", handleLedOff);

  // Tell the server what to do when someone visits "/led/on"
  server.on("/led/status", handleLedData);

  // Start the web server
  server.begin();

  Serial.println("Web server started!");

  // Initialize screens
  // ---------------------------------------------------------------
  Wire.begin(21, 22, 100000);  // bus 0: SDA=21, SCL=22
  I2C_1.begin(26, 27, 100000); // bus 1: SDA=26, SCL=27 (example)

  if (!screen.begin(SSD1306_SWITCHCAPVCC, SCREEN_I2C_ADDRESS))
  {
    Serial.println("screen init failed");
    while (true)
    {
    }
  }

  if (!screen2.begin(SSD1306_SWITCHCAPVCC, SCREEN2_I2C_ADDRESS))
  {
    Serial.println("screen2 init failed");
    while (true)
    {
    }
  }

  screen.clearDisplay();
  screen2.clearDisplay();

  screen.setTextSize(2);
  screen2.setTextSize(1);

  screen.setTextColor(WHITE);
  screen2.setTextColor(WHITE);

  screen.setCursor(0, 0);
  screen2.setCursor(0, 16);

  screen.print("BME280");
  screen2.print("INITIALIZING");

  screen.display();
  screen2.display();
  // ---------------------------------------------------------------

  delay(5000);
}

void loop()
{
  // Listen for incoming browser requests
  server.handleClient();

  if (millis() - lastRun >= INTERVAL)
  {
    lastRun = millis();

    // BME280 Sensor
    float roomTemp = bme.readTemperature();
    float humidity = bme.readHumidity();
    // float atmPress = bme.readPressure() / 100.0F; // BME280 originally gives Pa. Divide by 100 to get hPa.
    float atmPress = bme.readPressure() / 133.322; // Divide by 133.322 to get mmHg

    // DS18B20 Sensor
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);

    screen.clearDisplay();
    screen2.clearDisplay();

    printTitles();
    printData(roomTemp, humidity, atmPress);

    printWifi();

    screen.display();
    screen2.display();
  }
}

void printTitles()
{
  screen.setTextSize(1);
  screen.setTextColor(WHITE);
  screen.drawBitmap(108, 0, tempIcon, 20, 16, WHITE);
  screen.drawLine(0, 16, 128, 16, WHITE);
  screen.drawBitmap(108, 17, pressureIcon, 20, 16, WHITE);

  screen2.setTextSize(1);
  screen2.setTextColor(WHITE);
  screen2.drawLine(0, 16, 128, 16, WHITE);
  screen2.drawBitmap(108, 0, humidityIcon, 20, 16, WHITE);
}

void printWifi()
{
  screen2.setCursor(0, 20);
  if (WiFi.status() == WL_CONNECTED)
  {
    screen2.print("WiFi connected");
    screen2.drawBitmap(108, 17, wifi, 20, 16, WHITE);
  }
  else
  {
    screen2.print("WiFi disconnected");
    screen2.drawBitmap(108, 17, nowifi, 20, 16, WHITE);
  }
}

void printData(float temperature, float humidity, float atmPressure)
{
  screen.setTextSize(2);
  screen.setCursor(0, 0);
  screen.print(temperature);
  screen.setTextSize(1);
  screen.write(247);
  screen.print("C");
  screen.setCursor(72, 7);
  screen.print("BME280");

  screen.setTextSize(2);
  screen.setCursor(0, 18);
  screen.print(atmPressure);
  screen.setTextSize(1);
  screen.print("mmHg");

  // Serial.print(temperature);
  // Serial.print(",");
  // Serial.print(humidity);
  // Serial.print(",");
  // Serial.println(atmPressure);

  screen2.setTextSize(2);
  screen2.setCursor(0, 0);
  screen2.print(humidity);
  screen2.setTextSize(1);
  screen2.print("%");
}

void handleRoot()
{
  Serial.println("Someone requested the homepage.");

  File file = LittleFS.open("/index.html", "r");

  if (!file)
  {
    Serial.println("Could not open index.html");

    server.send(500, "text/plain", "Could not open index.html");

    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleCSS()
{
  Serial.println("Someone requested styles.css");

  File file = LittleFS.open("/styles.css", "r");

  if (!file)
  {
    Serial.println("Could not open styles.css");

    server.send(500, "text/plain", "Could not open styles.css");

    return;
  }

  server.streamFile(file, "text/css");
  file.close();
}

void handleNormalize()
{
  Serial.println("Someone requested normalize.css");

  File fileNormalize = LittleFS.open("/normalize.css", "r");

  if (!fileNormalize)
  {
    Serial.println("Could not open normalize.css");

    return;
  }

  server.streamFile(fileNormalize, "text/css");

  fileNormalize.close();
}

void handleJS()
{
  Serial.println("Someone requested script.js");

  File file = LittleFS.open("/script.js", "r");

  if (!file)
  {
    Serial.println("Could not open script.js");

    server.send(500, "text/plain", "Could not open script.js");

    return;
  }

  server.streamFile(file, "application/javascript");

  file.close();
}

void handleSensor()
{
  Serial.println("Someone requested BME280 data!");

  float roomTemp = bme.readTemperature();
  float humidity = bme.readHumidity();
  // float atmPress = bme.readPressure() / 100.0F; // BME280 originally gives Pa. Divide by 100 to get hPa.
  float atmPress = bme.readPressure() / 133.322; // Divide by 133.322 to get mmHg

  Serial.print("Temperature: ");
  Serial.print(roomTemp);
  Serial.println(" ºC");

  Serial.print("Pressure: ");
  Serial.print(atmPress);
  Serial.println(" mmHg");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Send value in .json format
  String json = "{";
  json += "\"temperature\":";
  json += roomTemp;
  json += ",";
  json += "\"pressure\":";
  json += atmPress;
  json += ",";
  json += "\"humidity\":";
  json += humidity;
  json += "}";

  addCorsHeaders();

  // server.send(200, "text/plain", String(temperature));
  server.send(200, "application/json", json);
}

void handleLedOn()
{
  Serial.println("Someone requested /led/on");
  outputState = true;
  setLed(outputState);

  addCorsHeaders();
  server.send(200, "text/plain", "LED is ON");
}

void handleLedOff()
{
  Serial.println("Someone requested /led/off");
  outputState = false;
  setLed(outputState);

  addCorsHeaders();
  server.send(200, "text/plain", "LED is OFF");
}

void setLed(bool state)
{
  if (outputState)
  {
    digitalWrite(ledPin, HIGH);
  }
  else
  {
    digitalWrite(ledPin, LOW);
  }
}

void handleLedData()
{
  // Send value in .json format
  String json = "{";
  json += "\"LED\":";
  json += outputState ? "true" : "false";
  json += "}";

  addCorsHeaders();

  server.send(200, "application/json", json);
}

void addCorsHeaders()
{
  server.sendHeader("Access-Control-Allow-Origin", "*");
}