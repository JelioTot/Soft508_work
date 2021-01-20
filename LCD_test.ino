#include <HID.h>

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include<SPI.h> 

#define trigPin 19
#define echoPin 18
#define LEDpin 13


LiquidCrystal_I2C lcd(0x27,16,2);  //set LCD addressto 0x27 for a 16 chars and 2 ine display
int n=1;
int delaylegnth = 30;
int tooClose = 0;
int Speed = 125;
int exploreMode = 0;

volatile boolean received;
volatile byte Slavereceived,Slavesend;

void setup() {
  // put your setup code here, to run once:
lcd.init();
lcd.backlight();
Serial.begin(115200);

pinMode(trigPin, OUTPUT);
pinMode(echoPin, INPUT);

//establish motor direction toggle pins
  pinMode(12, OUTPUT); //CH A -- HIGH = forwards and LOW = backwards???
  pinMode(13, OUTPUT); //CH B -- HIGH = forwards and LOW = backwards???
  
  //establish motor brake pins
  pinMode(9, OUTPUT); //brake (disable) CH A
  pinMode(8, OUTPUT); //brake (disable) CH B

  digitalWrite(9, HIGH); 
  digitalWrite(8, HIGH);

            
  pinMode(LEDpin,OUTPUT); 

  pinMode(MISO,OUTPUT);
  SPCR |= _BV(SPE); //turn on SPI in slave move using register
  received = false;
  SPI.attachInterrupt(); //turn On SPI communication
  


}

ISR (SPI_STC_vect)
{
  Slavereceived = SPDR;                  
  received = true;                       
}

void loop() {
  if (n>=16)
  n=1;
  // put your main code here, to run repeatedly:
  lcd.setCursor(5, 0);
  lcd.print("Hello");
  lcd.setCursor(n, 1);
  lcd.print("SOFT");
  delay(500);
  lcd.clear();
  
  Ultrasconic();
  if (exploreMode == 1){
  if (tooClose == 1){
    
  Stop();
  Spin();
  
  }
  else if (tooClose ==0){
    
  Forwards();
  }
  
  }
  Serial.println(Slavereceived);
if(received){
   if (Slavereceived==1)
   {
   digitalWrite(LEDpin,HIGH); //Sets pin 13 as HIGH LED ON
   Serial.println("Slave LED ON");
   Serial.println(Slavereceived);
     }
  else if (Slavereceived==0)
      {
  digitalWrite(LEDpin,LOW);     //Sets pin 13 as LOW LED OFF
  Serial.println("Slave LED OFF");
  Serial.println(Slavereceived);
      }
  else if (Slavereceived==2)
  Forwards();

  else if (Slavereceived==3)
  BackWards();

  else
  Stop();
}
received = false;
  
n=n+1;
}



void Ultrasconic(){

  // Looking at the HC SR04 datasheet
  // Formula: uS / 58 = centimeters
  // "We suggest to use over 60ms measurement cycle, in order to prevent trigger signal to the echo signal" who knows...
  
  // long is 64 bit size number
  long duration, distance;

  // send pulses
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);  
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2) / 29.1;

  // Print the distance to the terminal
  if (distance >= 200 || distance <= 0){
    Serial.println("Out of range");
    tooClose = 0;
  }
  else {
    Serial.print(distance);
    Serial.println(" cm");
    tooClose = 0;
      if (distance < 10) {
        tooClose = 1;
        Serial.println("STOP BUGGY");
        
      }
  }
  
  delay(500);
  
}

void Forwards(){
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW);  //ENABLE CH B

  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, Speed);   //Moves CH A
  digitalWrite(13, LOW);   //Sets direction of CH B
  analogWrite(11, Speed);   //Moves CH B
  
  delay(delaylegnth);
}

void BackWards(){
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW);  //ENABLE CH B

  digitalWrite(12, LOW);   //Sets direction of CH A
  analogWrite(3, Speed);   //Moves CH A
  digitalWrite(13, HIGH);   //Sets direction of CH B
  analogWrite(11, Speed);   //Moves CH B
  
  delay(delaylegnth);
}

void Spin(){
  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, 125);   //Moves CH A
  digitalWrite(13, HIGH);   //Sets direction of CH B
  analogWrite(11, 125);   //Moves CH B
  
  delay(delaylegnth);
  
}

void Stop(){
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW);  //ENABLE CH B

  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, 0);   //Moves CH A
  digitalWrite(13, LOW);   //Sets direction of CH B
  analogWrite(11, 0);   //Moves CH B
  
  delay(delaylegnth);
}


String READ(){
  String text;
  
  // Check for input (polling)
  
  if (Serial.available() > 0) {

    // Save incoming data
    text = Serial.readString();

    // Print data to terminal
    Serial.print("You typed: ");
    Serial.println(text); 
    
  }
  
  return text;
}
