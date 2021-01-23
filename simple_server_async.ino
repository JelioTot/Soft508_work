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
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>


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
.buttonExplore {background-color: #f44336;}  /* Red */
.buttonControl {background-color: #555555;}  /* Black */ 

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
<form action="/explore">
  <button class="button buttonExplore">Explore Mode!</button>
</form>
<form action="/controller">
  <button class="button buttonControl">Use Controller</button>
</form>

<p>Distance: %PLACEHOLDER_DISTANCE%cm <p>

<meta http-equiv="refresh" content="2;/" />
</body>
</html>
)=====";

const char ControllerPAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<header>
<h1> You may now use the contoller to control the buggy </h1>
<h2> Press Number 1 on your contoller to exit controller mode </h2>

<p>Distance: %PLACEHOLDER_DISTANCE%cm <p>
<meta http-equiv="refresh" content="2;/controller" />
</header>
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

const uint16_t kRecvPin = 4;
IRrecv irrecv(kRecvPin);
decode_results results;
unsigned long key_value = 0;

volatile int interruptCounter;
int totalInterruptCounter;
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

///IR codes
const int one = 0xFFA25D;
const int two = 0xFF629D;
const int three = 0xFFE21D;
const int four = 0xFF22DD;
const int five = 0xFF02FD;
const int six = 0xFFC23D;
const int seven = 0xFFE01F;
const int eight = 0xFFA857;          
const int nine = 0xFF906F;
const int zero = 0xFF9867;
const int star = 0xFF6897;
const int hash = 0xFFB04F;
const int up = 0xFF18E7;
const int down = 0xFF4AB5;
const int left = 0xFF10EF;
const int right = 0xFF5AA5;
const int ok = 0xFF38C7;


void IRAM_ATTR isr(){
  if (irrecv.decode(&results)){
    Serial.print("inside");
        if (results.value == 0XFFFFFFFF) results.value = key_value;
        
  switch (results.value){
    case 0xFF18E7:   ///up arrow
    data = 2;
    vspiCommand();
    Serial.println("done");
  }
  if ((results.value)== one);//do nothing;
  if ((results.value)== two);//do nothing;
  if ((results.value)== three);//do nothing;
  if ((results.value)== four);//do nothing;
  if ((results.value)== five);//do nothing;
  if ((results.value)== six);//do nothing;
  if ((results.value)== seven);//do nothing;
  if ((results.value)== eight);//do nothing;
  if ((results.value)== nine);//do nothing;
  if ((results.value)== zero);//do nothing;
  if ((results.value)== hash);//do nothing;
  if ((results.value)== star);//do nothing;
  if ((results.value)== 0xFF38C7){
    data = 0;
    vspiCommand();
  }
  if ((results.value)== 0xFF18E7){
    data = 2;
    vspiCommand();
  }
  if ((results.value)== down){
    data = 4;
    vspiCommand();
  }
  if ((results.value)== left){
    data = 2;
    vspiCommand();
  }
  if ((results.value)== right){
    data = 3;
    vspiCommand();
  }
  if ((results.value)== down){
    data = 3;
    vspiCommand();
  }
  irrecv.resume();
  }
}

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  interruptCounter++;
  portEXIT_CRITICAL_ISR(&timerMux);
 
}

void setup() {
  
  Serial.begin(115200);
  irrecv.enableIRIn();  // Start the receiver
  while (!Serial)  // Wait for the serial connection to be establised.
    delay(50);
  Serial.println();
  Serial.print("IRrecvDemo is now running and waiting for IR message on Pin ");
  Serial.println(kRecvPin);
  //attachInterrupt(4, isr, RISING);       //not working atm
 
  timer = timerBegin(0, 160, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

  
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
      data = 1;
      vspiCommand();
    });

    server.on("/right", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 3;
      vspiCommand();
    });
    
    server.on("/backward", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 4;
      vspiCommand();
    });

    server.on("/stop", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
      data = 0;
      vspiCommand();
    });
    server.on("/explore", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, processor);
    });
    
     server.on("/controller", HTTP_GET, [] (AsyncWebServerRequest *request) {
      if (data == 5) request->send_P(200, "text/html", WEBPAGE, processor);
      else {
      request->send_P(200, "text/html", ControllerPAGE, processor);
      }
     });

    server.onNotFound(notFound);
    server.begin();
}

void loop() {
  if (interruptCounter > 0) {
 
    portENTER_CRITICAL(&timerMux);
    byte Mastersend,Mastereceive;
    digitalWrite(5, LOW);
    Mastereceive=SPI.transfer(Mastersend);
    distance= Mastereceive;
    digitalWrite(5, HIGH);
    interruptCounter--;
    portEXIT_CRITICAL(&timerMux);
 
    totalInterruptCounter++;
 
  }

   //Serial.println(Mastereceive);
   
   if (irrecv.decode(&results)) {
    // print() & println() can't handle printing long longs. (uint64_t)
    serialPrintUint64(results.value, HEX);
    Serial.println("");
    irrecv.resume();  // Receive the next value
    switch (results.value){
    case up:   ///up arrow
    data = 2;
    vspiCommand();
    break;
    case ok:   ///up arrow
    data = 0;
    vspiCommand();
    break;
    case hash:
    data = 5;
    vspiCommand();
    break;
    case left:   ///up arrow
    data = 1;
    vspiCommand();
    break;
    case right:   ///up arrow
    data = 3;
    vspiCommand();
    break;
    case down:   ///up arrow
    data = 4;
    vspiCommand();
    break;
    
  }
  }
  
  //Serial.println(data);
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
