// libraries
#include <Servo.h>
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
Servo trunk; // creates servo object to control trunk
Servo steering; // creates servo object to control steering
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
String steeringDirection = "s"; // stores the current steering direction, values are "l", "r" and "s"
int angle; // is the angle from the target
bool isAfterDrive = false;

// constants
// these store the speed pwm values in text form
const int crawl = 90;
const int slow = 170;
const int medium = 210;
const int fast = 255;
// these store the values for the steering
const String l = "l";
const String r = "r";
const String s = "s";

// these store the gps coordinates, lat - latitude lon - longitude t - target o - origin c - current
float tlat, tlon, olat, olon, clat, clon;

// IO pins
const int trunkPin = 3;
const int steeringPin = 5;
const int motorb = 7;
const int motorf = 8;
const int mpwm = 6;

void setup() {
  Serial.begin(9600); // sets baud rate for arduino serial
  ss.begin(9600); // sets baud rate for gps serial
  
  // pin attachment
  pinMode(motorf, OUTPUT);
  pinMode(motorb, OUTPUT);
  pinMode(mpwm, OUTPUT);

  // set initial values
  trunkState("1");
  turn("s");
  sstop();
}

void loop() {
  // checks which phase is the code in, and activates functions accordingly
  /*
  if (phase == 0){
    stod();
  }

  else if (phase == 1){
    nav();
  }  

  else{
    stod();
  }*/
  //Serial.println(getIR(lir));
  Serial.println(lir.getDistance());
  delay(200);
}

void debugCheck(){//String debug){
  if (waitForInput() == "s"){
    sstop();
  }

  //Serial.println(debug);
}

// initializes the navigation phase
void navinit(){
  float ang1, ang2, ang3; // two variables to calculate the starting angle
  float lat1, lon1, lat2, lon2, lat3, lon3; // variables to store locations temporarily

  trunkState("0");
  turn("s");

  lat1 = clat;
  lon1 = clon;
  drive(1, slow);
  calibrationCheck();
  ang1 = calcAngle(lat1, lon1);

  lat2 = clat;
  lon2 = clon;
  calibrationCheck();
  ang2 = calcAngle(lat2, lon2);

  lat3 = clat;
  lon3 = clon;
  calibrationCheck();
  ang3 = calcAngle(lat3, lon3);

  angle = (ang1 + ang2 + ang3) / 3; // calculates the angle of the current direction

  phase = 1;
}

// checks if there's an object in front of it during calibration
void calibrationCheck(){
  for (int i = 0; i < 1001; i++){
    if(spaceForDriveStart() == false){
      sstop();
      Serial.println("error - not enough space in front to start drive");
      phase = 0;
      loop();
    }

    delay(1);
  }
}

// is responsible for the navigation phase
void nav(){
  float angled = angle - calcAngle(tlat, tlon); // calculates the angle difference between the current direction and the correct one
  //debugCheck();

  if (atTarget == true && isAfterDrive == true){
    phase = 0;
    sstop();
    loop();
  }

  else if (atTarget == true){
    phase = 2;
    sstop();
    loop();
  }

  if (angled > 0){ // this means it needs to turn right
    turn(r);
  }

  else if (angled < 0){ // this means it needs to turn left
    turn(l);
  }

  else{ // this means it's on the right path
    turn(s);
  }

  checkSides();
  checkFront();
}

// checks for objects on the sides, reacts accordingly
void checkSides(){
  if (getIR(rir) < 40.00 || getIR(lir) < 40.00){
    if (steeringDirection == "l" && getIR(lir) < 40.00){
      drive(0, medium);
      delay(500);
      drive(1, crawl);
      turn(r);
    }
   
    else if (steeringDirection == "r" && getIR(rir) < 40.00){
      drive(0, medium);
      delay(500);
      drive(1, crawl);
      turn(l);
    }

    else if (isSideClear(lir) == false){
      turn(r);
    }

    else if (isSideClear(rir) == false){
      turn(l);
    }
  }

  else{
    drive(1, slow);
  }
}

bool isSideClear(SharpIR sensor){
  int tempd = getIR(sensor);
  if (tempd < 40){
    delay(100);
    if (tempd - getIR(sensor) > 2){
      return false;
    }
  }

  else{
    return true;
  }
}

// checks for objects in front of itself, reacts accordingly
void checkFront(){
  if (getIR(frir) < 40.00 || getIR(lir) < 40.00){
    passObject();
   }

   else{
    drive(1, slow);
   }
}

// passes an object that's in front
void passObject(){
  drive(0, medium);
  turn(l);
  delay(500);
  turn(r);
  drive(1, crawl);
  delay(500);
  checkFront();

  float pd = getIR(lir); // the difference between the previous getLIR and the current one
  delay(100);

  while(abs(pd - getIR(lir)) > 0.1){ // continues turning until it's perpendicual to the object
    delay(100);
    checkFront();
   }

   if (getIR(lir) < 30.00){ // makes sure it won't crash into anything
    checkFront();
   }
  }

// checks if it's at the target location
bool atTarget(){
  getGPS();

  if (clat - tlat == 0.00 && clon - tlon == 0.00){
    return true;
  }

  return false;
}

// turns the steering system to the selected direction
void turn(String directionn){ // l for left, r for right s for straight
  steeringDirection = directionn;
  steering.attach(steeringPin);
  if (directionn == "l"){ // checks the input
    steering.write(0);
  }

  else if (directionn == "r"){
    steering.write(180);
  }

  else{
    steering.write(77);
  }
  delay(500);
  steering.detach();
}

// calculates the angle of a linear function created using the current location and of a target one (not the target one, although it can do that)
// it needs a target latitude and a target longitude
// it calculates the angle using tan(dlon / dlan) d is delta
float calcAngle(float trlat, float trlon){
  getGPS();

  return tan((clon - trlon) / (clat - trlat));
}

// tells the dc motors to which direction to spin and at what speed
// use drive(direction{1 or 0}, power {0-255})
void drive(int d, int p){ 
  analogWrite(mpwm, p); // set power
    
  if (d == 0){ // if d = o it goes backwards, and if it's 1 it goes forwards
    digitalWrite(motorf, LOW);
    digitalWrite(motorb, HIGH);
  }

  else{
    digitalWrite(motorf, HIGH);
    digitalWrite(motorb, LOW);
  }
}

// stop driving, set speed to 0
void sstop(){
  analogWrite(mpwm, 0);
  digitalWrite(motorf, LOW);
  digitalWrite(motorb, LOW);
}

// the start of drive phase function, it's responsible for phases 0 and 2
void stod(){
  if (authorized == true || (connectionAttempt() == true && isPasswordValid() == true)){ // checks if there's a device attempting to start a connection and if it has a valid password
    authorized = true;
    String lastMessage = waitForInput(); // lastMessage is the last valid message sent by the app

    if (lastMessage == "t"){ // checks which command the user gave, and does it
      trunkState(waitForInput());
    }

    else if (lastMessage == "g" && isAfterDrive == false){
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
                
        if (isAfterDrive == false){
          getGPS();
        }

        else{
          tlat = olat;
          tlon = olon;
        }

        olat = clat;
        olon = clon;

        if (olat == 0.00 || olon == 0.00){
          Serial.println("no GPS signal, can't navigate");
          loop();
        }
        Serial.println(olat, 6);
        Serial.println(olon, 6);

        if (spaceForDriveStart() == true){
          navinit();
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
      Serial.println("d - drive request");
      if (isAfterDrive == false){
        Serial.println("g - gps location incoming");
      }
    }

    else{
      Serial.println("invalid input");
    }
  }
}

// checks if there's enough space for the drive to start
bool spaceForDriveStart(){
  if (getIR(flir) < 40.00 || getIR(frir) < 40.00 || getIR(rir) < 20 || getIR(lir) < 20){ // checks if there's any object up to 40 cm in front and 20 cm on the sides
    return false;
  }

  else{
    return true;
  }
}


// gets the sensor data from an IR proximity sensor using the manifacturer's library
// it also validates that the reading is accurate
int getIR(SharpIR sensor){
  int fdistance, ldistance, adistance, pdistance, tdistance; // f is first , l is last, a is average, p is previous and t is test
  fdistance = sensor.getDistance();

  for (int i = 0; i > 50; i++){
    tdistance = sensor.getDistance();
    adistance = (adistance + tdistance) / 2;

    if (tdistance > 40 || abs(adistance - tdistance) > 25 || abs(tdistance - pdistance) > 10){
      return 40;
    }

    pdistance = tdistance;
  }
  
  ldistance = sensor.getDistance();

  if (abs(fdistance - adistance) > 25 || abs(ldistance - adistance) > 25 || ldistance > 40){ // this compares the first and last readings to the average, when there's an invalid reading the sensor will output random data, this prevents that data from being read as accurate data
    return 40;
  }

  else{
    return ldistance;
  }
}

// gets the GPS coordinates from the app, and sets tlat and tlon to the correct coordinates
void readGPS(){
  float temp = 00.000000; // stores the coordinates temporarily while they're being retrived
  String temps = ""; // stores the bluetooth message temporarily
  
  for (int z = 0; z < 2; z++){
    for (int i = 0; i < 8; i++){
      temps = waitForInput();

      if (temps != "."){
        temp = temp + float(temps.toInt()) * float(1.00 / pow(10.00, float(i - 1)));
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
    clat = flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat;
    clon = flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon;
  }
}

// sets the trunk's state, 0 for close 1 for open
void trunkState(String in){
  if (in == "0" || in == "1"){
    int state = in.toInt();
    trunk.attach(trunkPin);

    switch(state){
      case 0:
        trunk.write(180);
        Serial.println("closing trunk");
        break;
        
      case 1:
        trunk.write(0);
        Serial.println("opening trunk");
        break;  
        
    }
    delay(300);
    trunk.detach();
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
