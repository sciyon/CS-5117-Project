#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"
#include <Arduino_JSON.h>
#include <DHT.h>
#include "arduinoFFT.h"

#define DHT_SENSOR_PIN  21 // ESP32 pin GPIO21 connected to DHT22 sensor
#define DHT_SENSOR_TYPE DHT22
#define AO_PIN 36  // ESP32's pin GPIO36 connected to AO pin of the MQ2 sensor

DHT dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

const char* ssid = "WIFI_SSID";
const char* password = "WIFI_PASSWORD";

JSONVar readings;
unsigned long lastTime = 0;
unsigned long timerDelay = 200;

const uint16_t samples = 64;        // Number of samples (MUST be a power of 2)
const double samplingFrequency = 1000; // Hz (adjust based on actual sampling rate)

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String getSensorReadings(){
  readings["temperature"] = String(dht_sensor.readTemperature());
  readings["humidity"] =  String(dht_sensor.readHumidity());
  readings["gas"] = String(readSensor());
  
  String jsonString = JSON.stringify(readings);
  return jsonString;
}

int readSensor() {
  unsigned int sensorValue = analogRead(AO_PIN);  // Read the analog value from sensor
  unsigned int outputValue = map(sensorValue, 0, 1023, 0, 255); // map the 10-bit data to 8-bit data
  return outputValue;             // Return analog moisture value
}

// Initialize LittleFS
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}

// Initialize WiFi
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void notifyClients(String sensorReadings) {
  ws.textAll(sensorReadings);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    //data[len] = 0;
    //String message = (char*)data;
    // Check if the message is "getReadings"
    //if (strcmp((char*)data, "getReadings") == 0) {
      //if it is, send current sensor readings
      String sensorReadings = getSensorReadings();
      Serial.print(sensorReadings);
      notifyClients(sensorReadings);
    //}
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  initWiFi();
  initLittleFS();
  initWebSocket();
  dht_sensor.begin(); // initialize the DHT sensor
  analogSetAttenuation(ADC_11db); // Adjust ADC input range

  // Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  // Start server
  server.begin();
}

void calculateFFT(double *data, const char *label) {
  double vReal[samples];
  double vImag[samples] = {0};

  // Copy data to the FFT input arrays
  memcpy(vReal, data, sizeof(vReal));

  // Declare and instantiate the FFT object
  ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, samples, samplingFrequency);

  // Perform FFT
  FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  FFT.compute(FFTDirection::Forward);
  FFT.complexToMagnitude();

  // Send FFT results over WebSocket
  JSONVar fftResults;
  JSONVar magnitudes;
  JSONVar frequencies;

  for (uint16_t i = 0; i < (samples >> 1); i++) { // Only half the FFT results are useful
    double frequency = (i * samplingFrequency) / samples;
    frequencies[i] = frequency;
    magnitudes[i] = vReal[i];
  }

  fftResults["label"] = label;
  fftResults["frequencies"] = frequencies;
  fftResults["magnitudes"] = magnitudes;

  ws.textAll(JSON.stringify(fftResults));
}

void loop() {
  static double fftDataGas[samples] = {0};
  static double fftDataTemp[samples] = {0};
  static double fftDataHumi[samples] = {0};
  static int sampleIndex = 0;

  if (sampleIndex < samples) {
    fftDataGas[sampleIndex] = readSensor();                // Gas data
    fftDataTemp[sampleIndex] = dht_sensor.readTemperature(); // Temperature data
    fftDataHumi[sampleIndex] = dht_sensor.readHumidity();    // Humidity data
    sampleIndex++;
  }

  if (sampleIndex >= samples) {
    // Perform FFT for each dataset
    calculateFFT(fftDataGas, "gas");
    calculateFFT(fftDataTemp, "temperature");
    calculateFFT(fftDataHumi, "humidity");
    sampleIndex = 0; // Reset sample index for the next interval
  }

  if ((millis() - lastTime) > timerDelay) {
    String sensorReadings = getSensorReadings();
    Serial.println(sensorReadings);
    notifyClients(sensorReadings);
    lastTime = millis();
  }
  ws.cleanupClients();
}
