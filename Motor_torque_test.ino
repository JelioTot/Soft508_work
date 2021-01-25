
int Speed = 0;
int delaylegnth = 1000;
int start = 1;

void setup() {
  // put your setup code here, to run once:
pinMode(12, OUTPUT); //CH A -- HIGH = forwards and LOW = backwards???
  pinMode(13, OUTPUT); //CH B -- HIGH = forwards and LOW = backwards???
  
  //establish motor brake pins
  pinMode(9, OUTPUT); //brake (disable) CH A
  pinMode(8, OUTPUT); //brake (disable) CH B

  digitalWrite(9, HIGH); 
  digitalWrite(8, HIGH);
  Serial.begin(115200);
}

void loop() {

 if (start == 1){
  if (Speed<255){
  Serial.println(Speed);
  //Forwards(Speed);
  Left(Speed);
  Speed = Speed+10;
  }
  else{
  start = 0;
  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, 0);   //Moves CH A
  digitalWrite(13, LOW);   //Sets direction of CH B
  analogWrite(11, 0);   //Moves CH B
  }
 }
  
}



void Forwards(int Speed){
  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW);  //ENABLE CH B

  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, Speed);   //Moves CH A
  digitalWrite(13, LOW);   //Sets direction of CH B
  analogWrite(11, Speed);   //Moves CH B
  
  delay(delaylegnth); 
}

void Left(int Speed){

  digitalWrite(9, LOW);  //ENABLE CH A
  digitalWrite(8, LOW);  //ENABLE CH B
  
  digitalWrite(12, HIGH);   //Sets direction of CH A
  analogWrite(3, Speed);   //Moves CH A
  digitalWrite(13, HIGH);   //Sets direction of CH B
  analogWrite(11, Speed/2);   //Moves CH B
  
  delay(delaylegnth);
}
