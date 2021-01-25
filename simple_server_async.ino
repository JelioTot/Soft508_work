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


.buttonForward {background-color: #4CAF50;
position: absolute;
right: 875px;
width: 150px;
height: 50px;} /* Green */
.buttonLeft {background-color: #4CAF50;
position: absolute;
right: 960px;
width: 100px;
height: 50px;
top: 170px;} /* Green */
.buttonRight {background-color: #4CAF50;
position: absolute;
right: 840px;
width: 100px;
height: 50px;
top: 170px;} /* Green */
.buttonBackward {background-color: #4CAF50;
position: absolute;
right: 875px;
width: 150px;
height: 50px;
top: 230px} /* Green */
.buttonStop {background-color: #008CBA;
position: absolute;
right: 875px;
width: 150px;
height: 50px;
top: 290px} /* Blue */
.buttonExplore {background-color: #f44336;
position: absolute;
right: 865px;
width: 170px;
height: 50px;
top: 350px}  /* Red */
.buttonControl {background-color: #555555;
position: absolute;
right: 865px;
width: 170px;
height: 50px;
top: 410px}  /* Black */ 

</style>
</head>
<body>
<center>
<h1>Buggy Controller</h1>
<p>Distance: %PLACEHOLDER_DISTANCE%cm <p>

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
<form action="/explorePage">
  <button class="button buttonExplore">Explore Mode!</button>
</form>
<form action="/controller">
  <button class="button buttonControl">Use Controller</button>
</form>

<meta http-equiv="refresh" content="2;/" />
</body>
</html>
)=====";

const char ControllerPAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<body>
<centre>
<h1> You may now use the contoller to control the buggy </h1>
<h2> Press Hash on your contoller to exit controller mode </h2>
<p>NumPad Keys 1 to 6 control the speed of the buggy</p>
<p>1 - 20<span>&#37;</span></p>
<p>2 - 40<span>&#37;</span></p>
<p>3 - 60<span>&#37;</span></p>
<p>4 - 75<span>&#37;</span></p>
<p>5 - 90<span>&#37;</span></p>
<p>6 - 100<span>&#37;</span></p>
<p>Distance: %PLACEHOLDER_DISTANCE%cm <p>
<meta http-equiv="refresh" content="2;/controller" />
</body>
</html>

)=====";

const char ExplorePAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<style>
.button {
  border: none;
  color: black;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
}
</style>
<header>
<h1> Buggy is in Explore mode </h1>
<h2> click here to exit explore mode </h2>
<form action="/explore">
  <button class="button">here</button>
</form>

<p>Distance: %PLACEHOLDER_DISTANCE%cm <p>
<meta http-equiv="refresh" content="1;/explorePage" />
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
bool controller = false;

const char* pSpeed = "pSpeed";

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
    case up:   ///up arrow
    data = 2;
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
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
    });

    // Send a GET request to <IP>/get?message=<message>
    server.on("/forward", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
      data = 2;
      vspiCommand();
    });

    server.on("/left", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
      data = 1;
      vspiCommand();
    });

    server.on("/right", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
      data = 3;
      vspiCommand();
    });
    
    server.on("/backward", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
      data = 4;
      vspiCommand();
    });

    server.on("/stop", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
      data = 10;
      vspiCommand();
    });
    server.on("/explore", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
      data = 8;
      vspiCommand();
    });

    server.on("/explorePage", HTTP_GET, [] (AsyncWebServerRequest *request) {
      request->send_P(200, "text/html", ExplorePAGE, Ultrasonic);
      data = 6;
      vspiCommand();
    });
    
     server.on("/controller", HTTP_GET, [] (AsyncWebServerRequest *request) {
      if (data == 5){ 
        request->send_P(200, "text/html", WEBPAGE, Ultrasonic);
        data = 0;
        vspiCommand();
        controller = false;
      }
      else {
      request->send_P(200, "text/html", ControllerPAGE, Ultrasonic);
      data = 7;
      controller = true;
      vspiCommand();
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
   if (controller == true){
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
    case ok:   ///ok button
    data = 10;
    vspiCommand();
    break;
    case hash:  //hash button
    data = 5;
    vspiCommand();
    break;
    case left:   
    data = 1;
    vspiCommand();
    break;
    case right:   
    data = 3;
    vspiCommand();
    break;
    case down:   
    data = 4;
    vspiCommand();
    break;
    case one:   
    data = 11;
    vspiCommand();
    break;
    case two:   
    data = 12;
    vspiCommand();
    break;
    case three:   
    data = 13;
    vspiCommand();
    break;
    case four:   
    data = 14;
    vspiCommand();
    break;
    case five:   
    data = 15;
    vspiCommand();
    break;
    case six:   
    data = 16;
    vspiCommand();
    break;
    
  }
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



String Ultrasonic(const String& dist)
{

  if (dist == "PLACEHOLDER_DISTANCE") {
    return String(distance); 
  }
  return String();
}
