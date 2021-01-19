//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include <SPI.h>

AsyncWebServer server(80);   // http port

const char buttons[] {"<!DOCTYPE html>\n<html>\n<head>\n<style>\n.button {\n  border: none;\n  color: white;\n  padding: 16px 32px;\n  text-align: center;\n  text-decoration: none;\n  display: inline-block;\n  font-size: 16px;\n  margin: 4px 2px;\n  transition-duration: 0.4s;\n  cursor: pointer;\n}\n\n.button1 {\n  background-color: white; \n  color: black; \n  border: 2px solid #4CAF50;\n}\n\n.button1:hover {\n  background-color: #4CAF50;\n  color: white;\n}\n\n.button2 {\n  background-color: white; \n  color: black; \n  border: 2px solid #008CBA;\n}\n\n.button2:hover {\n  background-color: #008CBA;\n  color: white;\n}\n.button3 {\n  background-color: white; \n  color: black; \n  border: 2px solid #008CBA;\n}\n\n.button3:hover {\n  background-color: #008CBA;\n  color: white;\n}\n.button4 {\n  background-color: white; \n  color: black; \n  border: 2px solid #008CBA;\n}\n\n.button4:hover {\n  background-color: #008CBA;\n  color: white;\n}\n\n</style>\n</head>\n<body>\n\n<h1>The button element - Styled with CSS</h1>\n\n<p>Use the :hover selector to change the style of the button when you move the mouse over it.</p>\n<p><strong>Tip:</strong> Use the transition-duration property to determine the speed of the \"hover\" effect:</p>\n\n<button class=\"button button1\">Green</button>\n<button class=\"button button2\">Blue</button>\n<button class=\"button button3\">Blue</button>\n<button class=\"button button4\">Blue</button>\n\n</body>\n</html>\n"};
const char* ssid     = "PLUSNET-PTJKQ3";
const char* password = "dfb6aaf46e";

const char* PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

static const int spiClk = 1000000; // 1 MHz

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;
SPIClass * hspi = NULL;

void setup() {
  //initialise two instances of the SPIClass attached to VSPI and HSPI respectively
  vspi = new SPIClass(VSPI);
  hspi = new SPIClass(HSPI);
  
  //clock miso mosi ss

  //initialise vspi with default pins
  //SCLK = 18, MISO = 19, MOSI = 23, SS = 5
  vspi->begin();
  //alternatively route through GPIO pins of your choice
  //hspi->begin(0, 2, 4, 33); //SCLK, MISO, MOSI, SS
  
  //initialise hspi with default pins
  //SCLK = 14, MISO = 12, MOSI = 13, SS = 15
  hspi->begin(); 
  //alternatively route through GPIO pins
  //hspi->begin(25, 26, 27, 32); //SCLK, MISO, MOSI, SS

  //set up slave select pins as outputs as the Arduino API
  //doesn't handle automatically pulling SS low
  pinMode(5, OUTPUT); //VSPI SS
  pinMode(15, OUTPUT); //HSPI SS

  Serial.begin(115200);
    pinMode(2, OUTPUT);      // set the LED pin mode

    delay(10);

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", buttons);
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message);
    });

    // Send a POST request to <IP>/post with a form field message set to <message>
    server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message);
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop() {
}

void vspiCommand() {
  byte data = 0b00001100;
  //use it as you would the regular arduino SPI API
  vspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(5, LOW); //pull SS slow to prep other end for transfer
  vspi->transfer(data);  
  digitalWrite(5, HIGH); //pull ss high to signify end of data transfer
  vspi->endTransaction();
  
}

void hspiCommand() {
  byte stuff = 0b11001100;
  
  hspi->beginTransaction(SPISettings(spiClk, MSBFIRST, SPI_MODE0));
  digitalWrite(15, LOW);
  hspi->transfer(stuff);
  digitalWrite(15, HIGH);
  hspi->endTransaction();
}
