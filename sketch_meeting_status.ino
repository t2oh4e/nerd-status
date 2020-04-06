#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#include "wifidata.h"

#define MEETING_PIN D7
#define MEETING_DND_PIN D1
#define MEETING_OPEN_PIN D2
#define FOCUS_PIN D6

const char* ssid = MYSSID;
const char* password = MYPASSWORD;
const char* domain = "nerd-status";

ESP8266WebServer server(80);

void setup() {
  // Deactivate built in status leds. Might save some energy.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  pinMode(D0, OUTPUT);
  digitalWrite(D0, HIGH);

  pinMode(MEETING_PIN, OUTPUT);
  pinMode(MEETING_DND_PIN, OUTPUT);
  pinMode(MEETING_OPEN_PIN, OUTPUT);
  pinMode(FOCUS_PIN, OUTPUT);

  setupHttpServer();
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  MDNS.update();
}


void setupHttpServer() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin(domain)) {
    Serial.println("MDNS responder started");
  }

  defineRoutes();
  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void defineRoutes() {
  server.on("/", handleRoot);

  server.on("/meeting", []() {
    setPinStatusByServerArgs(MEETING_PIN, server.arg(0));
    handleRoot();
  });

  server.on("/meeting_dnd", []() {
    setPinStatusByServerArgs(MEETING_DND_PIN, server.arg(0));
    handleRoot();
  });
  
  server.on("/meeting_in", []() {
    setPinStatusByServerArgs(MEETING_OPEN_PIN, server.arg(0));
    handleRoot();
  });
  
  server.on("/headphones", []() {
    setPinStatusByServerArgs(FOCUS_PIN, server.arg(0));
    handleRoot();
  });

  server.on("/alloff", []() {
    setPinStatusByServerArgs(MEETING_PIN, "0");
    setPinStatusByServerArgs(MEETING_DND_PIN, "0");
    setPinStatusByServerArgs(MEETING_OPEN_PIN, "0");
    setPinStatusByServerArgs(FOCUS_PIN, "0");
    handleRoot();
  });
}

void setPinStatusByServerArgs(int pin, String statusArg) {
  if (statusArg == "1") {
      digitalWrite(pin, HIGH);
      Serial.println("activating led");
    } else {
      digitalWrite(pin, LOW);
      Serial.println("deactivating led");
    }
}

// Reads the index.html from SPIFFS and sends the content
void handleRoot() {
    SPIFFS.begin();
    String path = "/index.html";
    String dataType = "text/html";
    File dataFile = SPIFFS.open(path.c_str(), "r");
    String fileContent = replacePlaceholders(dataFile.readString());
    server.send(200, dataType, fileContent);
    dataFile.close();
}

// Replace placeholders from the html content to display LED status
String replacePlaceholders(String content) {
    int readPins[] = {MEETING_PIN, MEETING_DND_PIN, MEETING_OPEN_PIN, FOCUS_PIN};
    String search[] = {"[MEETING]", "[MEETING_DND]", "[MEETING_COME_IN]", "[FOCUS]"};
    
    int i = 0;
    for (int i = 0; i < 4; i++) {
      content.replace(search[i], digitalRead(readPins[i]) == HIGH ? "checked=\"checked\"" : "");
    }

    return content;
}

void handleNotFound() {
  server.send(404, "text/plain", "This is not the nerd you're looking for.");
}
