#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Korneya";
const char* password = "acekorneya";

IPAddress staticIP(192, 168, 0, 100); // Replace with the desired fixed IP address
IPAddress gateway(192, 168, 0, 1); // Replace with your network gateway
IPAddress subnet(255, 255, 255, 0); // Replace with your network subnet mask

const int relayPin = 14; // the number of the relay pin
bool fanState = false;  // the current state of the fan

WebServer server(80);

void handleFanControl() {
  if (server.hasArg("state")) {
    String state = server.arg("state");
    if (state == "on") {
      digitalWrite(relayPin, HIGH); // Turn on the fan
      fanState = true;
      Serial.println("Fan turned on");
    } else if (state == "off") {
      digitalWrite(relayPin, LOW); // Turn off the fan
      fanState = false;
      Serial.println("Fan turned off");
    }
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize the relay pin as output
  pinMode(relayPin, OUTPUT);

  // Configure the static IP address
  if (!WiFi.config(staticIP, gateway, subnet)) {
    Serial.println("Failed to configure static IP");
    return;
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Set up the fan control endpoint
  server.on("/fanControl", handleFanControl);
  server.begin();
}

void loop() {
  server.handleClient();
}