#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "asprintf.h"

#include "config.h"

int ledPin = LED_BUILTIN;
WiFiServer server(80);
WiFiClient wifiClient;
HTTPClient sender;

const int relayPin = D1;
const long interval = 2000;

void setup() {
  Serial.begin(9600);
  delay(10);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  char *url = NULL;
  asprintf(&url, ping_url_format, WiFi.localIP().toString().c_str(), hostname);
  Serial.println(url);
  if (sender.begin(wifiClient, url)) {
    sender.end();
  }
  free(url);

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Use this URL : ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  // Check if a client has connected

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  // Read the first line of the request
  String request = client.readStringUntil('\r');
  Serial.println(request);
  client.flush();

  // Match the request

  if (request.indexOf(secret) == -1) {
    Serial.println("invalid request");
  } else {
    Serial.println("buzzing");
    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, HIGH);
    delay(400);
    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, LOW);
  }

  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one

  client.println("<!DOCTYPE html><body>Hello!</body></html>");
  client.flush();

  delay(1);
  Serial.println("Client disconnected");
  Serial.println("");

}