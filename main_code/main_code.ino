// libraries
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SharpIR.h>

/*
bluetooth communications dictionary:
c - connection request
p - password attempt
t - trunk request (0 for closing it and 1 for opening it)
g - gps location incoming
d - drive request
h - help
 */

// misc
TinyGPS gps; // creates gps object
SoftwareSerial ss(11, 12); // sets gps Tx to 11 and Rx to 12

// ir sensor objects
SharpIR flir (SharpIR::GP2Y0A41SK0F, A1);
SharpIR frir (SharpIR::GP2Y0A41SK0F, A2);
SharpIR lir (SharpIR::GP2Y0A41SK0F, A0);
SharpIR rir (SharpIR::GP2Y0A41SK0F, A3);

//  variables
int phase = 0; // 0 is start of drive, 1 is navigation, 2 is end of drive
bool authorized = false; // indicates if the connected device is authorized
int steeringDirection = 77; // stores the current steering direction, values are "l", "r" and "s"
int angle; // is the angle from the target
bool isAfterDrive = false;
bool isObsticlePresent = false;

// constants
// these store the speed pwm values in text form
const int crawl = 140;
const int slow = 190;
const int medium = 210;
const int fast = 255;

// these store the gps coordinates, lat - latitude lon - longitude t - target o - origin c - current
float tlat, tlon, olat, olon, clat, clon;

// IO pins
const int motorb1 = 2;
const int motorb2 = 13;
const int motorf = 8;
const int mpwm1 = 5;
const int mpwm2 = 6;

void setup() {
  Serial.begin(9600); // sets baud rate for arduino serial
  ss.begin(9600); // sets baud rate for gps serial
  
  // pin attachment
  pinMode(motorf, OUTPUT);
  pinMode(motorb1, OUTPUT);
  pinMode(motorb2, OUTPUT);
  pinMode(mpwm1, OUTPUT);
  pinMode(mpwm2, OUTPUT);

  // set initial values
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

// initializes the navigation phase
void navinit(){
  getGPS();

  olon = clon;
  olat = clat;
  phase = 1;
}

// is responsible for the navigation phase
void nav(){
  drive(1, fast);
  delay(1000);
  drive(0, fast);
  delay(100);

  getGPS();
  tlon = clon;
  tlat = clat;

  tlat = tlat * 110947.2;
  olat = olan * 110947.2;
  tlon = tlon * 288200;
  olon = olon * 288200;
  Serial.print("the velocity was ");
  Serial.print(((1/sin(atan((tlat - olat) / (tlon - olon))))*(tlat - olat)) / 2);
  Serial.print("meters per second");
  Serial.println("");

  phase = 0;
}

// tells the dc motors to which direction to spin and at what speed
// use drive(direction{1 or 0}, power {0-255})
void drive(int d, int p){
  analogWrite(mpwm1, p); // set power
  analogWrite(mpwm2, p);
    
  if (d == 0){ // if d = o it goes backwards, and if it's 1 it goes forwards
    digitalWrite(motorf, LOW);
    digitalWrite(motorb1, HIGH);
    digitalWrite(motorb2, HIGH);
  }

  else{
    digitalWrite(motorf, HIGH);
    digitalWrite(motorb1, LOW);
    digitalWrite(motorb2, LOW);
  }
}

// stop driving, set speed to 0
void sstop(){
  analogWrite(mpwm1, 0);
  analogWrite(mpwm2, 0);
  digitalWrite(motorf, LOW);
  digitalWrite(motorb1, LOW);
  digitalWrite(motorb2, LOW);
}

// the start of drive phase function, it's responsible for phases 0 and 2
void stod(){
  if (authorized == true || (connectionAttempt() == true && isPasswordValid() == true)){ // checks if there's a device attempting to start a connection and if it has a valid password
    authorized = true;
    String lastMessage = waitForInput(); // lastMessage is the last valid message sent by the app

    if (lastMessage == "d"){ // it needs to save its current location in order to come back to it later
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
        getGPS();               

        olat = clat;
        olon = clon;

        if (olat == 0.00 || olon == 0.00){
          Serial.println("error: no GPS signal, can't navigate");
          loop();
        }
        Serial.println(olat, 6);
        Serial.println(olon, 6);

        navinit();
      }
    }

    else if (lastMessage == "h"){
      Serial.println("c - connection request");
      Serial.println(" p - password attempt");
      Serial.println("d - drive request");
    }

    else{
      Serial.println("invalid input");
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
    clat = flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat;
    clon = flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon;
  }
}

// checks if there's a connection attempt being made
// that happens when there's an input of "c"
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
