#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include "bsec.h"
#include <esp_now.h>
#include <SD_MMC.h>
#include <FS.h>


#define I2C_SDA 21
#define I2C_SCL 22

const float LOCATION_ALTITUDE = -12; // Replace with your location's altitude in meters
const int ledPin = 27;  // the number of the led pin
const float targetTemp = 78.50;  // the desired temperature in Fahrenheit
const float tempTolerance = 0.75;  // the temperature tolerance in Fahrenheit
bool fanState = false;  // the current state of the fan
bool manualFanControl = false;  // flag for manual fan control
unsigned long belowTargetTempTime = 0;
const unsigned long belowTargetTempDuration = 1200000;  // 20 minutes in milliseconds
float referencePressure = 1013.25; // Initial reference pressure

const char* ssid = "Korneya";
const char* password = "acekorneya";
const char* authUsername = "acekorneya";
const char* authPassword = "kny";
WebServer server(80);

Bsec iaqSensor;
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint8_t broadcastAddress[] = {0xEC, 0x64, 0xC9, 0x85, 0x83, 0x88};

float tempValue;
float humValue;
float pressureValue;
float iaqValue;
float dewPointValue;
float altitudeValue;
float seaLevelPressure;

// ESP-NOW peer info
esp_now_peer_info_t peerInfo;

// Callback function called when data is sent
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

bool authenticate() {
  if (!server.authenticate(authUsername, authPassword)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  if (!authenticate()) {
    return;
  }

  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<title>Korneya HQ</title>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>";
  html += "<script src='https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom'></script>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; }";
  html += ".sensor-reading { display: inline-block; margin: 20px; text-align: center; }";
  html += ".sensor-reading img { width: 50px; height: 50px; cursor: pointer; }";
  html += ".slider-container { width: 200px; margin: 0 auto; }";
  html += "#graphContainer { width: 600px; height: 400px; margin: 0 auto; }";
  html += ".chart-buttons { margin-bottom: 10px; }";
  html += ".chart-buttons button { margin-right: 10px; }";
  html += ".fan-status {";
  html += "display: inline-block;";
  html += "padding: 10px 20px;";
  html += "border-radius: 20px;";
  html += "font-weight: bold;";
  html += "color: white;";
  html += "}";
  html += ".fan-status-on {";
  html += "background-color: #4CAF50;";
  html += "}";
  html += ".fan-status-off {";
  html += "background-color: #F44336;";
  html += "}";
  html += "</style>";
  html += "</head><body>";
  html += "<h1>Korneya HQ</h1>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn1.iconfinder.com/data/icons/farming-agriculture-3/42/temperature-512.png' alt='Temperature'>";
  html += "<p>Temperature: <span id='temperature'>--</span> &deg;F</p>";
  html += "</div>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn3.iconfinder.com/data/icons/weather-972/48/humadity-512.png' alt='Humidity'>";
  html += "<p>Humidity: <span id='humidity'>--</span> %</p>";
  html += "</div>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn0.iconfinder.com/data/icons/30px-weather/30/07_weather-barometer-atmosphere-pressure-512.png' alt='Pressure'>";
  html += "<p>Pressure: <span id='pressure'>--</span> mb</p>";
  html += "</div>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn0.iconfinder.com/data/icons/air-pollution-1-4/504/air-quality-index-pollution-monitoring-512.png' alt='Air Quality Index'>";
  html += "<p>Air Quality Index: <span id='iaq'>--</span></p>";
  html += "</div>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn0.iconfinder.com/data/icons/weather-540/48/05-dew-dew_point-thermometer-drop-weather-512.png' alt='Dew Point'>";
  html += "<p>Dew Point: <span id='dewPoint'>--</span> &deg;F</p>";
  html += "</div>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn2.iconfinder.com/data/icons/geography-28/32/altitude_height_distance_vertical_mountain_sea_level_geography_geographic-512.png' alt='Altitude'>";
  html += "<p>Altitude: <span id='altitude'>--</span> m</p>";
  html += "</div>";
  html += "<div class='sensor-reading'>";
  html += "<img src='https://cdn4.iconfinder.com/data/icons/modern-technologies-1/32/technology_cooling_fan_cooler-512.png' alt='Fan Status'>";
  html += "<p>Fan Status: <span id='fanStatus' class='fan-status'>--</span></p>";
  html += "</div>";
  html += "<br><h2>LED Control</h2>";
  html += "<div class='slider-container'>";
  html += "<input type='range' min='0' max='1' step='1' id='ledSlider' onchange='toggleLED(this.value)' value='" + String(digitalRead(ledPin)) + "'>";
  html += "</div>";
  html += "<br><h2>Fan Control</h2>";
  html += "<button onclick=\"controlFan('on')\">Turn Fan On</button>";
  html += "<button onclick=\"controlFan('off')\">Turn Fan Off</button>";
  html += "<button onclick=\"controlFan('auto')\">Set Fan to Auto</button>";
  html += "<h2>Data Download</h2>";
  html += "<p><a href='/download'>Download Sensor Data (CSV)</a></p>";
  html += "<div class='chart-buttons'>";
  html += "<button onclick=\"showGraph('temperature')\">Temperature</button>";
  html += "<button onclick=\"showGraph('humidity')\">Humidity</button>";
  html += "<button onclick=\"showGraph('pressure')\">Pressure</button>";
  html += "<button onclick=\"showGraph('iaq')\">IAQ</button>";
  html += "<button onclick=\"showGraph('dewPoint')\">Dew Point</button>";
  html += "<button onclick=\"showGraph('altitude')\">Altitude</button>";
  html += "<button onclick=\"showGraph('all')\">All Sensors</button>";
  html += "</div>";
  html += "<div id='graphContainer' style='display: none;'>";
  html += "<canvas id='dataGraph'></canvas>";
  html += "</div>";
  html += "<script>";
  html += "let temperatureElement = document.getElementById('temperature');";
  html += "let humidityElement = document.getElementById('humidity');";
  html += "let pressureElement = document.getElementById('pressure');";
  html += "let iaqElement = document.getElementById('iaq');";
  html += "let dewPointElement = document.getElementById('dewPoint');";
  html += "let altitudeElement = document.getElementById('altitude');";
  html += "let fanStatusElement = document.getElementById('fanStatus');";
  html += "let chartData = {";
  html += "temperature: { data: [], labels: [], color: 'rgba(75, 192, 192, 1)' },";
  html += "humidity: { data: [], labels: [], color: 'rgba(54, 162, 235, 1)' },";
  html += "pressure: { data: [], labels: [], color: 'rgba(255, 99, 132, 1)' },";
  html += "iaq: { data: [], labels: [], color: 'rgba(75, 192, 75, 1)' },";
  html += "dewPoint: { data: [], labels: [], color: 'rgba(153, 102, 255, 1)' },";
  html += "altitude: { data: [], labels: [], color: 'rgba(255, 159, 64, 1)' },";
  html += "};";
  html += "let chartInstance = null;";
  html += "function initializeGraph(type) {";
  html += "const ctx = document.getElementById('dataGraph').getContext('2d');";
  html += "const datasets = [];";
  html += "if (type === 'all') {";
  html += "Object.keys(chartData).forEach(sensor => {";
  html += "datasets.push({";
  html += "label: sensor.charAt(0).toUpperCase() + sensor.slice(1),";
  html += "data: chartData[sensor].data,";
  html += "backgroundColor: chartData[sensor].color.replace('1)', '0.2)'),";
  html += "borderColor: chartData[sensor].color,";
  html += "borderWidth: 1,";
  html += "yAxisID: sensor";
  html += "});";
  html += "});";
  html += "} else {";
  html += "datasets.push({";
  html += "label: type.charAt(0).toUpperCase() + type.slice(1),";
  html += "data: chartData[type].data,";
  html += "backgroundColor: chartData[type].color.replace('1)', '0.2)'),";
  html += "borderColor: chartData[type].color,";
  html += "borderWidth: 1,";
  html += "yAxisID: type";
  html += "});";
  html += "}";
  html += "const options = {";
  html += "scales: {";
  html += "y: {";
  html += "beginAtZero: true";
  html += "}";
  html += "},";
  html += "plugins: {";
  html += "zoom: {";
  html += "zoom: {";
  html += "wheel: {";
  html += "enabled: true,";
  html += "},";
  html += "pinch: {";
  html += "enabled: true";
  html += "},";
  html += "mode: 'xy',";
  html += "},";
  html += "pan: {";
  html += "enabled: true,";
  html += "mode: 'xy',";
  html += "}";
  html += "}";
  html += "}";
  html += "};";
  html += "if (type === 'all') {";
  html += "options.scales = {";
  html += "temperature: {";
  html += "type: 'linear',";
  html += "display: true,";
  html += "position: 'left',";
  html += "},";
  html += "humidity: {";
  html += "type: 'linear',";
  html += "display: true,";
  html += "position: 'right',";
  html += "},";
  html += "pressure: {";
  html += "type: 'linear',";
  html += "display: true,";
  html += "position: 'left',";
  html += "},";
  html += "iaq: {";
  html += "type: 'linear',";
  html += "display: true,";
  html += "position: 'right',";
  html += "},";
  html += "dewPoint: {";
  html += "type: 'linear',";
  html += "display: true,";
  html += "position: 'right',";
  html += "},";
  html += "altitude: {";
  html += "type: 'linear',";
  html += "display: true,";
  html += "position: 'right',";
  html += "}";
  html += "};";
  html += "}";
  html += "chartInstance = new Chart(ctx, {";
  html += "type: 'line',";
  html += "data: {";
  html += "labels: chartData[Object.keys(chartData)[0]].labels,";
  html += "datasets: datasets";
  html += "},";
  html += "options: options";
  html += "});";
  html += "}";
  html += "function updateSensorData(data) {";
  html += "temperatureElement.textContent = data.temperature.toFixed(2);";
  html += "humidityElement.textContent = data.humidity.toFixed(2);";
  html += "pressureElement.textContent = data.pressure.toFixed(2);";
  html += "iaqElement.textContent = data.iaq;";
  html += "dewPointElement.textContent = data.dewPoint.toFixed(2);";
  html += "altitudeElement.textContent = data.altitude.toFixed(2);";
  html += "fanStatusElement.textContent = data.fanStatus ? 'On' : 'Off';";
  html += "fanStatusElement.className = data.fanStatus ? 'fan-status fan-status-on' : 'fan-status fan-status-off';";
  html += "Object.keys(chartData).forEach(type => {";
  html += "chartData[type].data.push(data[type].toFixed(2));";
  html += "chartData[type].labels.push(new Date().toLocaleTimeString());";
  html += "});";
  html += "if (chartInstance) {";
  html += "const type = chartInstance.data.datasets[0].yAxisID;";
  html += "if (type === 'all') {";
  html += "Object.keys(chartData).forEach((sensor, index) => {";
  html += "chartInstance.data.datasets[index].data = chartData[sensor].data;";
  html += "});";
  html += "chartInstance.data.labels = chartData[Object.keys(chartData)[0]].labels;";
  html += "} else {";
  html += "chartInstance.data.labels = chartData[type].labels;";
  html += "chartInstance.data.datasets[0].data = chartData[type].data;";
  html += "}";
  html += "chartInstance.update();";
  html += "}";
  html += "}";
  html += "function toggleLED(value) {";
  html += "fetch(`/led?state=${value === '1' ? 'on' : 'off'}`)";
  html += ".then(response => response.text())";
  html += ".then(data => {";
  html += "console.log(data);";
  html += "document.getElementById('ledSlider').value = value;";
  html += "});";
  html += "}";
  html += "function controlFan(state) {";
  html += "fetch(`/fan?state=${state}`)";
  html += ".then(response => response.text())";
  html += ".then(data => {";
  html += "console.log(data);";
  html += "});";
  html += "}";
  html += "function showGraph(type) {";
  html += "const graphContainer = document.getElementById('graphContainer');";
  html += "graphContainer.style.display = 'block';";
  html += "if (chartInstance) {";
  html += "chartInstance.destroy();";
  html += "}";
  html += "initializeGraph(type);";
  html += "}";
  html += "function fetchSensorDataFromCSV() {";
  html += "fetch('/sensor_data.csv')";
  html += ".then(response => response.text())";
  html += ".then(data => {";
  html += "const lines = data.trim().split('\\n');";
  html += "lines.forEach(line => {";
  html += "const [temperature, humidity, pressure, iaq, dewPoint, altitude, fanStatus] = line.split(',');";
  html += "chartData.temperature.data.push(parseFloat(temperature));";
  html += "chartData.humidity.data.push(parseFloat(humidity));";
  html += "chartData.pressure.data.push(parseFloat(pressure));";
  html += "chartData.iaq.data.push(parseFloat(iaq));";
  html += "chartData.dewPoint.data.push(parseFloat(dewPoint));";
  html += "chartData.altitude.data.push(parseFloat(altitude));";
  html += "const timestamp = new Date().toLocaleTimeString();";
  html += "Object.keys(chartData).forEach(type => {";
  html += "chartData[type].labels.push(timestamp);";
  html += "});";
  html += "});";
  html += "if (chartInstance) {";
  html += "const type = chartInstance.data.datasets[0].yAxisID;";
  html += "if (type === 'all') {";
  html += "Object.keys(chartData).forEach((sensor, index) => {";
  html += "chartInstance.data.datasets[index].data = chartData[sensor].data;";
  html += "});";
  html += "chartInstance.data.labels = chartData[Object.keys(chartData)[0]].labels;";
  html += "} else {";
  html += "chartInstance.data.labels = chartData[type].labels;";
  html += "chartInstance.data.datasets[0].data = chartData[type].data;";
  html += "}";
  html += "chartInstance.update();";
  html += "}";
  html += "})";
  html += ".catch(error => {";
  html += "console.error('Error fetching sensor data from CSV:', error);";
  html += "});";
  html += "}";
  html += "setInterval(() => {";
  html += "fetch('/data')";
  html += ".then(response => response.json())";
  html += ".then(data => {";
  html += "updateSensorData(data);";
  html += "})";
  html += ".catch(error => {";
  html += "console.error('Error fetching sensor data:', error);";
  html += "});";
  html += "}, 5000);";
  html += "fetchSensorDataFromCSV();";
  html += "</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleLedControl() {
  if (!authenticate()) {
    return;
  }

  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED turned on");
    } else if (state == "off") {
      digitalWrite(ledPin, LOW);
      Serial.println("LED turned off");
    }
  }
  server.send(200, "text/plain", String(digitalRead(ledPin)));
}

void handleFanControl() {
  if (!authenticate()) {
    return;
  }

  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      fanState = true;
      manualFanControl = true;
      Serial.println("Fan turned on manually");
      esp_now_send(peerInfo.peer_addr, (uint8_t *)"on", 2); // Send "on" command to second ESP32
    } else if (state == "off") {
      fanState = false;
      manualFanControl = true;
      Serial.println("Fan turned off manually");
      esp_now_send(peerInfo.peer_addr, (uint8_t *)"off", 3); // Send "off" command to second ESP32
    } else if (state == "auto") {
      manualFanControl = false;
      Serial.println("Fan control set to automatic");
    }
  }
  server.send(200, "text/plain", "Fan state changed");
}

void controlFan() {
  if (!manualFanControl) {
    if (tempValue > targetTemp + tempTolerance) {
      if (!fanState) {
        fanState = true;
        belowTargetTempTime = 0;  // Reset the timer
        Serial.println("Fan turned on automatically");
        esp_now_send(peerInfo.peer_addr, (uint8_t *)"on", 2); // Send "on" command to second ESP32
      }
    } else if (tempValue < targetTemp - tempTolerance) {
      if (fanState) {
        fanState = false;
        belowTargetTempTime = 0;  // Reset the timer
        Serial.println("Fan turned off automatically");
        esp_now_send(peerInfo.peer_addr, (uint8_t *)"off", 3); // Send "off" command to second ESP32
      }
    } else {
      if (tempValue < targetTemp) {
        if (belowTargetTempTime == 0) {
          belowTargetTempTime = millis();  // Start the timer when temperature goes below target
        }
        if (fanState && millis() - belowTargetTempTime >= belowTargetTempDuration) {
          fanState = false;
          belowTargetTempTime = 0;  // Reset the timer
          Serial.println("Fan turned off automatically after being below target temperature for 20 minutes");
          esp_now_send(peerInfo.peer_addr, (uint8_t *)"off", 3); // Send "off" command to second ESP32
        }
      } else {
        belowTargetTempTime = 0;  // Reset the timer if temperature goes above target
      }
    }
  }
}

void handleSensorData() {
  if (!authenticate()) {
    return;
  }

  String json = "{";
  json += "\"temperature\":" + String(tempValue, 2) + ",";
  json += "\"humidity\":" + String(humValue, 2) + ",";
  json += "\"pressure\":" + String(pressureValue, 2) + ",";
  json += "\"iaq\":" + String(iaqValue, 2) + ","; 
  json += "\"dewPoint\":" + String(dewPointValue, 2) + ",";
  json += "\"altitude\":" + String(altitudeValue, 2) + ",";
  json += "\"fanStatus\":" + String(fanState);
  json += "}";
  server.send(200, "application/json", json);
}

void updateLcdDisplay() {
  static unsigned long lastDisplayUpdate = 0;
  static int displayMode = 0;

  // Update the display every 3 seconds
  if (millis() - lastDisplayUpdate >= 3000) {
    lastDisplayUpdate = millis();
    displayMode = (displayMode + 1) % 3; // Cycle through 0, 1, and 2

    lcd.clear();

    // Display temperature and humidity on the first line
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(tempValue, 1);
    lcd.print("F ");
    lcd.print("H:");
    lcd.print(humValue, 1);
    lcd.print("%");

    // Display information on the second line based on the display mode
    lcd.setCursor(0, 1);
    switch (displayMode) {
      case 0:
        // Display dew point and fan status
        lcd.print("DP:");
        lcd.print(dewPointValue, 1);
        lcd.print("F ");
        
        if (manualFanControl) {
          lcd.print(fanState ? "Fan:ON" : "Fan:OFF");
        } else {
          lcd.print("Fan:AUTO");
        }
        break;
      case 1:
        // Display sea level pressure
        lcd.print("SLP:");
        lcd.print(seaLevelPressure);
        lcd.print("mb ");
        break;
      case 2:
        // Display IAQ and IAQ accuracy
        lcd.print("IAQ:");
        lcd.print(iaqValue);
        lcd.print(" Acc:");
        lcd.print(iaqSensor.iaqAccuracy);
        break;
    }
  }
}

void saveSensorDataToSD() {
  File file = SD_MMC.open("/sensor_data.csv", FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Write the sensor data to the file
  file.print(tempValue);
  file.print(",");
  file.print(humValue);
  file.print(",");
  file.print(pressureValue);
  file.print(",");
  file.print(iaqValue);
  file.print(",");
  file.print(dewPointValue);
  file.print(",");
  file.print(altitudeValue);
  file.print(",");
  file.println(fanState ? "On" : "Off");
  delay(100);
  file.close();
}

void handleDownload() {
  if (!authenticate()) {
    return;
  }

  File file = SD_MMC.open("/sensor_data.csv");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }

  server.sendHeader("Content-Disposition", "attachment; filename=sensor_data.csv");
  server.streamFile(file, "text/csv");
  file.close();
}
void setup() {
  Serial.begin(115200);

  // Wait for serial connection to be established
  while (!Serial) {
    delay(10);
  }

  // Check the flash memory size
  uint32_t flashSize = ESP.getFlashChipSize();
  Serial.print("Flash memory size: ");
  Serial.print(flashSize);
  Serial.println(" bytes");
  
  WiFi.mode(WIFI_AP_STA);
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize the BSEC library
  Wire.begin(I2C_SDA, I2C_SCL);
  iaqSensor.begin(BME68X_I2C_ADDR_LOW, Wire);
  checkIaqSensorStatus();

  bsec_virtual_sensor_t sensorList[12] = {
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS
  };

  iaqSensor.updateSubscription(sensorList, 12, BSEC_SAMPLE_RATE_LP);
 
 // Initialize SD card
  if (!SD_MMC.begin()) {
    Serial.println("SD card initialization failed!");
    return;
  }
  delay(100); // Add a small delay after SD card initialization
  Serial.println("SD card initialized.");

  // Set up the web server routes
  server.on("/", handleRoot);
  server.on("/led", handleLedControl);
  server.on("/fan", handleFanControl);
  server.on("/data", handleSensorData);
  server.on("/download", handleDownload);
  server.begin();
  
  // Initialize the LED pin as output
  pinMode(ledPin, OUTPUT);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register the send callback
  esp_now_register_send_cb(onDataSent);

  // Register the peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = WiFi.channel();
  peerInfo.encrypt = false;

  // Add the peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void checkIaqSensorStatus() {
  if (iaqSensor.bsecStatus != BSEC_OK) {
    if (iaqSensor.bsecStatus < BSEC_OK) {
      Serial.println("BSEC error code : " + String(iaqSensor.bsecStatus));
    } else {
      Serial.println("BSEC warning code : " + String(iaqSensor.bsecStatus));
    }
  }

  if (iaqSensor.bme68xStatus != BME68X_OK) {
    if (iaqSensor.bme68xStatus < BME68X_OK) {
      Serial.println("BME680 error code : " + String(iaqSensor.bme68xStatus));
    } else {
      Serial.println("BME680 warning code : " + String(iaqSensor.bme68xStatus));
    }
  }
}

void loop() {
  server.handleClient();

  // Update sensor data periodically (e.g., every 3 seconds)
  static unsigned long lastUpdateTime = 0;
  if (millis() - lastUpdateTime >= 3000) {
    lastUpdateTime = millis();
    if (iaqSensor.run()) {
      float temperatureC = iaqSensor.temperature;  // Get temperature in Celsius
      tempValue = temperatureC * 1.8 + 32;  // Convert to Fahrenheit for other uses
      humValue = iaqSensor.humidity;
      pressureValue = iaqSensor.pressure / 100.0;
      iaqValue = iaqSensor.staticIaq;

      float a = 17.27;
      float b = 237.7;
      float alpha = ((a * temperatureC) / (b + temperatureC)) + log(humValue / 100.0);
      float dewPointCelsius = (b * alpha) / (a - alpha);  // Dew point in Celsius
      dewPointValue = dewPointCelsius * 1.8 + 32;  // Now dewPointValue holds the Fahrenheit value
      
      // Calculate sea level pressure
      seaLevelPressure = pressureValue / pow(1 - (LOCATION_ALTITUDE/44330), 5.255);

      // Update the reference pressure periodically (e.g., every hour)
      static unsigned long lastCalibrationTime = 0;
      if (millis() - lastCalibrationTime >= 300000) { // 5 Minutes in milliseconds
        lastCalibrationTime = millis();
        referencePressure = seaLevelPressure;
      }

      // Calculate altitude using the reference pressure
      altitudeValue = 44330.77 * (1 - pow(pressureValue / referencePressure, 1/5.255));
      
      Serial.print("Raw Temperature: ");
      Serial.print(iaqSensor.rawTemperature * 1.8 + 32);
      Serial.println(" *F");

      Serial.print("Pressure: ");
      Serial.print(iaqSensor.pressure / 100.0);  // Convert pressure to hPa
      Serial.println(" hPa");
      
      Serial.print("Sea Level Pressure: ");
      Serial.print(seaLevelPressure);
      Serial.println(" hPa");

      Serial.print("Altitude: ");
      Serial.print(altitudeValue);
      Serial.println(" m");

      Serial.print("Stabilization Status: ");
      Serial.println(iaqSensor.stabStatus ? "Finished" : "Ongoing");

      Serial.print("Run-in Status: ");
      Serial.println(iaqSensor.runInStatus ? "Finished" : "Ongoing");

      Serial.print("Raw Humidity: ");
      Serial.print(iaqSensor.rawHumidity);
      Serial.println(" %");

      Serial.print("Gas Resistance: ");
      Serial.print(iaqSensor.gasResistance / 1000.0);  // Convert gas resistance to kOhms
      Serial.println(" kOhms");

      Serial.print("IAQ: ");
      Serial.println(iaqSensor.iaq);

      Serial.print("IAQ Accuracy: ");
      Serial.println(iaqSensor.iaqAccuracy);

      Serial.print("Temperature: ");
      Serial.print(iaqSensor.temperature * 1.8 + 32);
      Serial.println(" *F");

      Serial.print("Humidity: ");
      Serial.print(iaqSensor.humidity);
      Serial.println(" %");

      Serial.print("Static IAQ: ");
      Serial.println(iaqSensor.staticIaq);

      Serial.print("CO2 Equivalent: ");
      Serial.print(iaqSensor.co2Equivalent);
      Serial.println(" ppm");

      Serial.print("Breath VOC Equivalent: ");
      Serial.print(iaqSensor.breathVocEquivalent);
      Serial.println(" ppm");

      Serial.print("Dew Point: ");
      Serial.print(dewPointValue);
      Serial.println(" F");

      Serial.println();

      // Control the fan based on the current temperature
      controlFan();

      // Save the sensor data to the SD card
      saveSensorDataToSD();

      // Update the LCD display
      updateLcdDisplay();
    } else {
      checkIaqSensorStatus();
    }
  }
} 
