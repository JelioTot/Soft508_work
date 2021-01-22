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
#include <PS4Controller.h>


AsyncWebServer server(80);   // http port

const char* ssid     = "PLUSNET-PTJKQ3";
const char* password = "dfb6aaf46e";
byte data;

const char WEBPAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<style>
.button {
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
}

.buttonForward {background-color: #4CAF50;} /* Green */
.buttonLeft {background-color: #4CAF50;} /* Green */
.buttonRight {background-color: #4CAF50;} /* Green */
.buttonBackward {background-color: #4CAF50;} /* Green */
.buttonStop {background-color: #008CBA;} /* Blue */
</style>
</head>
<body>

<h1>Buggy Controller</h1>

<form action="/forward">
  <button class="button buttonForward">Forward!</button>
</form>
<form action="/left">
  <button class="button buttonLeft">Left!</button>
</form>
<form action="/right">
  <button class="button buttonRight">Right!</button>
</form>
<form action="/backward">
  <button class="button buttonBackward">Backward!</button>
</form>
<form action="/stop">
  <button class="button buttonStop">Stop!</button>
</form>

<p>Distance: %PLACEHOLDER_DISTANCE%m <p>

<meta http-equiv="refresh" content="2;/" />
</body>
</html>
)=====";


void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

static const int spiClk = 1000000; // 1 MHz

//uninitalised pointers to SPI objects
SPIClass * vspi = NULL;
SPIClass * hspi = NULL;

int distance = 0;

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

  SPI.begin();                            //Begins the SPI commnuication
  SPI.setClockDivider(SPI_CLOCK_DIV16); 
   digitalWrite(SS,HIGH);                  // Setting SlaveSelect as HIGH (So master doesnt connnect with slave)

  Serial.begin(115200);
  
    pinMode(2, OUTPUT);      // set the LED pin mode

    delay(10);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.printf("WiFi Failed!\n");
        return;
    }

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send_P(200, "text/html", WEBPAGE, processor);
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/forward", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 2;
      vspiCommand();
    });

    server.on("/left", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 3;
      vspiCommand();
    });

    server.on("/right", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 3;
      vspiCommand();
    });
    
    server.on("/backward", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 3;
      vspiCommand();
    });

    server.on("/stop", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 4;
      vspiCommand();
    });

    server.onNotFound(notFound);
    server.begin();
    vspiCommand();
}

void loop() {
  byte Mastersend,Mastereceive;
   digitalWrite(5, LOW);
   Mastereceive=SPI.transfer(Mastersend);
   distance= Mastereceive;
   digitalWrite(5, HIGH);
   Serial.println(Mastereceive);
   
   delay(1000);
}


void vspiCommand() {
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

String processor(const String& dist)
{

  if (dist == "PLACEHOLDER_DISTANCE") {
    return String(distance); 
  }

  else if (dist == "PLACEHOLDER_ACTION") {
    return String(random(0, 50)); // This needs actual action
  }

  return String();
}
