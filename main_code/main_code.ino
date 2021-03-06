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
// these store the values for the steering
const int l = 0;
const int r = 180;
const int s = 90;

// these store the gps coordinates, lat - latitude lon - longitude t - target o - origin c - current p - previous
float tlat, tlon, olat, olon, clat, clon, plat, plon;

// IO pins
const int trunkPin = 3;
const int steeringPin = 9;
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
  trunkState("1");
  turn(s);
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

  else{
    stod();
  }
}

// initializes the navigation phase
void navinit(){
  float ang1, ang2; // two variables to calculate the starting angle
  float lat1, lon1, lat2, lon2; // variables to store locations temporarily

  trunkState("0");
  turn(s);
  delay(100);

  lat1 = clat;
  lon1 = clon;
  drive(1, crawl);
  calibrationCheck();
  ang1 = calcAngle(lat1, lon1);

  lat2 = clat;
  lon2 = clon;
  calibrationCheck();
  ang2 = calcAngle(lat2, lon2);

  angle = (ang1 + ang2) / 2; // calculates the angle of the current direction

  phase = 1;
}

// checks if there's an object in front of it during calibration
void calibrationCheck(){
  for (int i = 0; i < 100; i++){
    if(spaceForDriveStart() == false){
      sstop();
      Serial.println("error - not enough space in front to start drive");
      phase = 0;
      loop();
    }
  }
}

// is responsible for the navigation phase
void nav(){
  checkSurroundings();
  float angle = calcAngle(plat, plon); // calculates the current angle
  checkSurroundings();
  float anglec = calcAngle(tlat, tlon); // calculates the correct angle
  checkSurroundings();
  float angled = angle - anglec;

  if (atTarget() == true && isAfterDrive == true){
    phase = 0;
    sstop();
    loop();
  }

  else if (atTarget() == true){
    phase = 2;
    sstop();
    loop();
  }
  
  if (isObsticlePresent == false){
    if (isStuck() == true){
      Serial.println("it's stuck");
      turn(r);
      drive(0, slow);
      delay(200);
      turn(s);
      drive(0, crawl);
    }

    else if (abs(angled) > 90 && abs(angled) < 270){ // this means it needs to turn around because the angle difference is over 90 degrees
      turnAround();
      Serial.print("turn around because angled is ");
      Serial.print(angled);
      Serial.println("");
    }
  
    else if (abs(angled) < 5){ // this means it's on the right path
      turn(s);
      Serial.print("it's on the right path because angled is ");
      Serial.print(angled);
      Serial.println("");
    }
  
    else if ((angle < 90 && angled < 0) || (angle > 90 && angled > 0)){ // this means it needs to turn right
      turn(l);
      Serial.print("turn left because angled is ");
      Serial.print(angled);
      Serial.println("");
    }

    else if ((angle < 90 && angled > 0) || (angle > 90 && angled < 0)){ // this means it needs to turn left
      turn(r);
      Serial.print("turn right because angled is ");
      Serial.print(angled);
      Serial.println("");
    }
  }
  
  checkSurroundings();
  plat = clat;
  plon = clon;
}

// checks the surroundings for any obsticles and reacts accordingly
void checkSurroundings(){
  for (int i = 0; i < 200; i++){
    checkSides();
    checkFront();
  }
}

// makes the system turn around 180 degrees
void turnAround(){
  turn(r);
  drive(0, slow);
  delay(1000);
  turn(s);
  drive(1, crawl);
}

// checks if it's stuck
bool isStuck(){
  if (abs(plat - clat) < 0.000005 && abs(plon - clon) < 0.000005){
    return true;
  }

  return false;
}

// checks for objects on the sides, reacts accordingly
void checkSides(){
  if (getIR(lir) < 40){
    isObsticlePresent = true;
    turn(r);
  }

  else if (getIR(rir) < 40){
    isObsticlePresent = true;
    turn(l);
  }

  else{
    isObsticlePresent = false;
  }
}

// checks for objects in front of itself, reacts accordingly
void checkFront(){
  if (getIR(frir) < 40 || getIR(flir) < 40){
    sstop();
    passObject();
    isObsticlePresent = true;
   }

   else{
    isObsticlePresent = false;
   }
}

// passes an object that's in front
void passObject(){
  drive(0, slow);
  turn(l);
  delay(700);
  turn(r);
  drive(1, crawl);

  int pd = getIR(lir); // the difference between the previous getLIR and the current one
  delay(50);

  while(abs(pd - getIR(lir)) > 1 && getIR(flir) < 40){ // continues turning until it's perpendicular to the object
    if (getIR(frir) < 40 || getIR(lir) < 40){
    passObject();
    isObsticlePresent = true;
    }

    pd = getIR(lir);
    delay(50);
  }

  turn(s);
}

// checks if it's at the target location
bool atTarget(){
  getGPS();

  if (abs(clat - tlat) < 0.00007 && abs(clon - tlon) < 0.00007){
    Serial.println("destination reached");
    return true;
  }

  return false;
}

// turns the steering system to the selected direction
void turn(int directionn){ // l for left, r for right s for straight
  useServo(directionn, steeringPin);
  steeringDirection = directionn;
}

// changes the angle of a given servo to a given angle
void useServo(int angle, int servo){
  switch(angle){
    case 0: // turns left
      analogWrite(servo, 100);
      break;

    case 90: // turns straight
      analogWrite(servo, 180);
      break;

    case 180: // turns right
      analogWrite(servo, 254);
      break;
  }
}

// calculates the angle of a linear function created using the current location and of a target one (not the target one, although it can do that)
// it needs a target latitude and a target longitude
// it calculates the angle using tan(dlon / dlan) d is delta
float calcAngle(float trlat, float trlon){
  getGPS();
  float angle = tan((clon - trlon) / (clat - trlat));

  if (angle < 0){
    angle = angle + 360;
  }


  return angle;
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
  drive(0, fast);
  turn(s);
  delay(100);
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
        getGPS();
                
        if (isAfterDrive == true){
          tlat = olat;
          tlon = olon;
        }

        olat = clat;
        olon = clon;

        if (olat == 0.00 || olon == 0.00){
          Serial.println("error: no GPS signal, can't navigate");
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
  if (getIR(flir) < 40.00 || getIR(frir) < 40.00 || getIR(rir) < 40 || getIR(lir) < 40){ // checks if there's any object up to 40 cm in front or on the sides
    return false;
  }

  else{
    return true;
  }
}


// gets the sensor data from an IR proximity sensor using the manifacturer's library
// it also validates that the reading is accurate
int getIR(SharpIR sensor){
  int fdistance, cdistance, ldistance, adistance; // f is first , c is current l is last and a is average
  int dv = 15;
  fdistance = sensor.getDistance();

  for (int i = 0; i < 200; i++){
    cdistance = sensor.getDistance();
    adistance = (adistance + cdistance) / 2;

    if (cdistance > 27){
      return 40;
    }
    
    delay(1);
  }
  
  ldistance = sensor.getDistance();

  if (abs(fdistance - adistance) > dv || abs(ldistance - adistance) > dv){ // this compares the first and last readings to the average, when there's an invalid reading the sensor will output random data, this prevents that data from being read as accurate data
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

    switch(state){
      case 0:
        useServo(180, trunkPin);
        Serial.println("closing trunk");
        break;
        
      case 1:
        useServo(0, trunkPin);
        Serial.println("opening trunk");
        break;  
        
    }
    delay(1000);
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
