// libraries
#include <Servo.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>

/*
bluetooth communications dictionary:
c - connection request
p - password attempt
t - trunk request (0  for closing it and 1 for opening it)
g - gps location incoming
d - drive request
h - help
 */

// misc
Servo trunk; // creates servo object to control trunk
Servo steering; // creates servo object to control steering
TinyGPS gps; // creates gps object
SoftwareSerial ss(10, 9); // sets gps Tx to 9 and Rx to 10

//  variables
int phase = 0; // 0 is start of drive, 1 is navigation, 2 is end of drive
bool authorized = false; // indicates if the connected device is authorized

// these store the gps coordinates, lat - latitude lon - longitude t - target o - origin
float tlat, tlon, olat, olon;

// IO pins
const int trunkPin = 3;
const int steeringPin = 5;
const int flir = 0;
const int frir = 1;
const int lir = 2;
const int rir = 3;
const int motorb = 7;
const int motorf = 8;
const int mpwm = 6;

void setup() {
  Serial.begin(9600); // sets baud rate for arduino serial
  ss.begin(9600); // sets baud rate for gps serial
  
  // pin attachment
  trunk.attach(trunkPin); // attaches the selected pin to the trunk servo object
  steering.attach(steeringPin); // attaches the selected pin to the steering servo object
  pinMode(motorf, OUTPUT);
  pinMode(motorb, OUTPUT);
  pinMode(mpwm, OUTPUT);

  // set initial values
  trunk.write(0);
  steering.write(90);
  digitalWrite(motorf, LOW);
  digitalWrite(motorb, HIGH);
  sstop();
}

void loop() {
  // checks which phase is the code in, and activates functions accordingly
  if (phase == 0){
    stod();
  }

  else if (phase == 1){
    nav();
  }
}

void nav(){
   
}

void drive(int d, int p){ // use drive(direction{f or b}, power {0-255})
  analogWrite(mpwm, p);
    
  if (d == 0){
    digitalWrite(motorf, LOW);
    digitalWrite(motorb, HIGH);
  }

  else{
    digitalWrite(motorf, HIGH);
    digitalWrite(motorb, LOW);
  }
}

void sstop(){
  analogWrite(mpwm, 0);
}

// the start of drive phase function, it's responsible for phase 1
void stod(){
  if ((connectionAttempt() == true && isPasswordValid() == true) || authorized == true){ // checks if there's a device attempting to start a connection and if it has a valid password
    authorized = true;
    String lastMessage = waitForInput(); // lastMessage is the last valid message sent by the app

    if (lastMessage == "t"){ // checks which command the user gave, and does it
      trunkState(waitForInput());
    }

    else if (lastMessage == "g"){
      Serial.println("recieving gps coordinates, send the latitude before the longitude"); // gps has 6 numbers after the decimal place
      readGPS();
    }

    else if (lastMessage == "d"){ // it needs to save its current location in order to come back to it later
      Serial.println("are you sure you want to start driving? type yes if you're sure or anything else if you're not");
      Serial.println("warning: there is no way to reverse this action, type yes only if you're absolutely sure you want the drive to start");

      bool confirm = true;

      for (int i = 0; i < 3; i++){
        switch(i){
          case 0:
            if (waitForInput() != "y"){
              confirm = false;
            }
            break;

            case 1:
            if (waitForInput() != "e"){
              confirm = false;
            }
            break;

            case 2:
            if (waitForInput() != "s"){
              confirm = false;
            }
            break;
        }
      }
      
      if (confirm = true){
        Serial.println("preparing to drive");
        trunkState("0");
        getGPS();

        if (spaceForDriveStart() == true){
          phase = 1;
        }

        else{
          Serial.println("error - not enough space in front to start drive");
        }
      }
    }

    else if (lastMessage == "h"){
      Serial.println("c - connection request");
      Serial.println(" p - password attempt");
      Serial.println(" t - trunk request (0 for closing it and 1 for opening it)");
      Serial.println("g - gps location incoming");
      Serial.println("d - drive request");
    }

    else{
      Serial.println("invalid input");
    }
  }
}

bool spaceForDriveStart(){
  if (getFLIR() < 30.00 && getFRIR < 30.00){ // checks if there's any object up to 30 cm in front
    return false;
  }

  else{
    return true;
  }
}

// all get IR functions operate on the same principles
// it multiplies the analog input of the selected IR sensor (front left, front right, left and right) by several constants
// 
float getFLIR(){
  float distance =  11.58 * pow(analogRead(flir)*0.0048828125, -1.10);

  return distance;
}

float getFRIR(){
  float distance =  11.58 * pow(analogRead(frir)*0.0048828125, -1.10);

  return distance;
}

float getLIR(){
  float distance =  11.58 * pow(analogRead(lir)*0.0048828125, -1.10);

  return distance;
}

float getRIR(){
  float distance =  11.58 * pow(analogRead(rir)*0.0048828125, -1.10);

  return distance;
}

// gets the GPS coordniates from the app, and sets tlat and tlon to the correct coordinates
void readGPS(){
  float temp = 0; // stores the coordinates temporarily while they're being retrived
  String temps = ""; // stores the bluetooth message temporarily
  
  for (int z = 0; z < 2; z++){
    for (int i = 0; i < 6; i++){
      temps = waitForInput();

      if (temps != "."){
        temp = temp + float(temps.toInt()) * pow(10.00, -float(i));
      }

      else{
        i = i - 1;
      }
    }

    if (z == 0){
      tlat = temp;
      temp = 0;
    }
    
    else{
      tlon = temp;
    }
  }
}

// gets the GPS coordinates from the GPS module
void getGPS(){
  bool newData = false;
  unsigned long chars;
  unsigned short sentences, failed;

  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      char c = ss.read();
      if (gps.encode(c)) // Did a new valid sentence come in?
        newData = true;
    }
  }

  if (newData)
  {
    float flat, flon;
    unsigned long age;
    gps.f_get_position(&flat, &flon, &age);

    // sets the global variables to the current location
    olat = flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6;
    olon = flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6;
  }
}

// sets the trunk's state, 0 for close 1 for open
void trunkState(String in){
  if (in == "0" || in == "1"){
    int state = in.toInt();

    switch(state){
      case 0:
        trunk.write(180);
        Serial.println("closing trunk");
        break;
        
      case 1:
        trunk.write(0);
        Serial.println("opening  trunk");
        break;  
        
    }
  }
}

// checks if there's a connection attempt being made
// that happens when there's a constant input of "c"
// if there is a connection attempt it accepts it and returns true, otherwise it returns false
bool connectionAttempt(){
  if (btread() == "c"){
    Serial.println("connection accepted"); // this is the string that tells the app the connection has been accepted
    return true;
  }
  
  else{
    return false;
  }
}

// this makes sure the password is valid, the password is a 4 character message (they're in seperate rows)
// the way it knows if there's a password attempt is by the char "p" being sent
// password is ardu
bool isPasswordValid(){
  Serial.println("send the letter p to start password entry");
  if (waitForInput() == "p"){
    Serial.println("enter password"); // tells the app to enter the password

    for (int i = 1; i < 5; i++){ // checks all four inputs for the code, returns false if there's an incorrect input
      switch(i){
        case 1:
          if (waitForInput() != "a"){
            return false;
           }
           break;
          
        case 2:
           if (waitForInput() != "r"){
            return false;
           }
           break;

          case 3:
             if (waitForInput() != "d"){
              return false;
           }
           break;

          case 4:
             if (waitForInput() != "u"){ 
              return false;
           }
           break;
      }
    }
    Serial.println("the password is valid"); // tells the app the password is valid
    return true;
  }

  else{
    return false;
  }
}

// waits for a bluetooth input
String waitForInput(){
  String ans = btread(); // stores the last value sent
  while (ans == "x"){ans = btread();}
  return ans;
}

// takes the decimal input of the incoming_string variable and converts it into a char if it's valid, otherwise it returns x
String btread(){
  String incoming_string =  "-1";  // stores the last input sent by the bluetooth serial in decimal form (ascii), if there's no input its value is -1
  incoming_string = String(Serial.read()); // reads the serial input from the bluetooth antenna
  if (incoming_string != "-1"){
    char message = incoming_string.toInt();
    return String(message);
  }
  
  else{
    return "x";
  }
}
