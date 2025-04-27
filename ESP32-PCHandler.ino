#include <WiFi.h>
#include <NetworkClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ESPping.h>
#include "Env.h"

WebServer server(80);

const int pcPowerPin = 13;
const int ledPin = 2;

unsigned long prevTime = 0;
bool ledOn = false;

void handlePcSwitch() {
  digitalWrite(ledPin, 1);

  digitalWrite(pcPowerPin, 1);
  server.send(200, "text/plain", "hello from esp32!");
  delay(100);
  Serial.println("PC on");
  digitalWrite(pcPowerPin, 0);

  delay(1000 * 60);

  digitalWrite(ledPin, 0);
}

void getStatus() {
  bool ret = Ping.ping(ip);
  String statusText = "The PC status is: ";
  statusText += ret ? "On" : "Off";
  server.send(404, "text/plain", statusText);
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup(void) {
  pinMode(pcPowerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  digitalWrite(pcPowerPin, 0);
  digitalWrite(ledPin, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    ledOn = !ledOn;
    digitalWrite(ledPin, ledOn);
  }

  ledOn = true;
  digitalWrite(ledPin, 1);
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  initOTA();

  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/pcon", handlePcSwitch);
  server.on("/status", getStatus);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");

  prevTime = millis();
}


void loop(void) {
  server.handleClient();
  if(WiFi.status() != WL_CONNECTED)
  {
    ESP.restart();
  }

  // handle led blink
  unsigned long now = millis();
  unsigned long elapsedTime = now - prevTime;
  if(elapsedTime > 100 && ledOn)
  {
    digitalWrite(ledPin, 0);
    ledOn = false;
    prevTime = now;
  }
  else if(elapsedTime > 6000 && !ledOn)
  {
    digitalWrite(ledPin, 1);
    ledOn = true;
    prevTime = now;
  }

  delay(10);  //allow the cpu to switch to other tasks
}
