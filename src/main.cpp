#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SerialCommands.h>
#include "asprintf.h"

#include "config.h"

int ledPin = LED_BUILTIN;
int voltage_analog = A0;

WiFiServer server(80);
WiFiClient wifiClient;
HTTPClient client;

const int relayPin = D1;
const long interval = 2000;

char serial_command_buffer_[32];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

void buzz() {
    Serial.println("buzzing");
    digitalWrite(ledPin, LOW);
    digitalWrite(relayPin, HIGH);
    delay(400);
    digitalWrite(ledPin, HIGH);
    digitalWrite(relayPin, LOW);
}

//This is the default handler, and gets called when no other command matches.
void cmd_unrecognized(SerialCommands* sender, const char* cmd)
{
  sender->GetSerial()->print("Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}

void cmd_ping(SerialCommands* sender)
{
  sender->GetSerial()->println("pong");
}

void cmd_buzz(SerialCommands* sender)
{
  buzz();
}

//Note: Commands are case sensitive
SerialCommand cmd_ping_("ping", cmd_ping);
SerialCommand cmd_buzz_("buzz", cmd_buzz);

void setup() {
  Serial.begin(9600);

  serial_commands_.SetDefaultHandler(cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_ping_);
  serial_commands_.AddCommand(&cmd_buzz_);

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
  if (client.begin(wifiClient, url)) {
    int status = client.POST("{}");
    Serial.println(status);
    client.end();
  }
  free(url);

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.print("Buzzer URL: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void serverLoop() {
  // Check if a client has connected

  WiFiClient client = server.accept();
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
    buzz();
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

int throttleCountdown = 0;

void ringerLoop() {
  // only post every 2 seconds
  if (throttleCountdown > 0) {
    throttleCountdown--;
  } else {
    int voltage = analogRead(voltage_analog);
    if (voltage >= 1200) { // 12
      throttleCountdown = 2000;
      Serial.println("");
      Serial.println("ring");
      if (client.begin(wifiClient, ringer_url)) {
        client.POST("{}");
        client.end();
      }
    }
  }
}

void loop() {
  delay(1);

  serverLoop();
  ringerLoop();
  serial_commands_.ReadSerial();
}