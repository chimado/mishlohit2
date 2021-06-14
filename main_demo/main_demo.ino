// libraries
#include <Servo.h>

// misc
Servo trunk; // creates servo object to control trunk
Servo steering; // creates servo object to control steering

//  variables
bool authorized = false; // indicates if the connected device is authorized
String steeringDirection = "s"; // stores the current steering direction, values are "l", "r" and "s"
int velocity = 0;
String drivingDirection = "f";
bool driving = false;

// constants
// these store the speed pwm values in text form
const int slow = 170;
const int medium = 210;
const int fast = 255;
// these store the values for the steering
const String l = "l";
const String r = "r";
const String s = "s";

// IO pins
const int trunkPin = 3;
const int steeringPin = 5;
const int motorb = 7;
const int motorf = 8;
const int mpwm = 6;

void setup() {
  Serial.begin(9600); // sets baud rate for arduino serial

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
  if (driving == true){
    cdrive();
  }

  else{
    stod();
  }
}

// turns the steering system to the selected direction
void turn(String directionn){ // l for left, r for right s for straight
  steeringDirection = directionn;
  if (directionn == "l"){ // checks the input
    steering.write(180);
  }

  else if (directionn == "r"){
    steering.write(0);
  }

  else{
    steering.write(90);
  }
}

// tells the dc motors to which direction to spin and at what speed
// use drive(direction{f or b}, power {0-255})
void drive(String d, int p){ 
  analogWrite(mpwm, p); // set power
    
  if (d == "b"){ // if d = b it goes backwards, and if it's f it goes forwards
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
}

void stod(){
  if ((connectionAttempt() == true && isPasswordValid() == true) || authorized == true){ // checks if there's a device attempting to start a connection and if it has a valid password
    authorized = true;
    String lastMessage = waitForInput(); // lastMessage is the last valid message sent by the app

    if (lastMessage == "t"){ // checks which command the user gave, and does it
      trunkState(waitForInput());
    }

    else if (lastMessage == "d"){ // it needs to save its current location in order to come back to it later
      Serial.println("are you sure you want to start driving? type yes if you're sure or anything else if you're not");
      Serial.println("warning: there is no way to reverse this action, type yes only if you're absolutely sure you want the drive to start");
      
      if (readLine("yes") == true){
        Serial.println("preparing to drive");
        trunkState("0");

        drivingHelp();
        driving = true;
      }
    }

    else if (lastMessage == "h"){
      Serial.println("c - connection request");
      Serial.println("p - password attempt");
      Serial.println("t - trunk request (0 for closing it and 1 for opening it)");
      Serial.println("d - drive request");
    }

    else{
      Serial.println("invalid input");
    }
  }
}

// is responsible for handling user input and moving accordingly
void cdrive(){
  String lastMessage = waitForInput(); // lastMessage is the last valid message sent by the app

  if (lastMessage == "h"){
    drivingHelp();
  }

  else if (lastMessage == "a" && readLine("bort") == true){
    sstop();
    driving = false;
  }

  else if (lastMessage == "f" || lastMessage == "b"){
    drivingDirection = lastMessage;
  }

  else if (lastMessage == "l" || lastMessage == "r" || lastMessage == "s"){
    steeringDirection = lastMessage;
  }

  else if (lastMessage == "v"){
    lastMessage = waitForInput();
    
    if (lastMessage == "f" && readLine("ast") == true){
      velocity = fast;
    }

    else if (lastMessage == "s" && readLine("low") == true){
      velocity = slow;
    }

    else if (lastMessage == "m" && readLine("edium") == true){
      velocity = medium;
    }

    else{
      Serial.println("invalid input");
    }
  }

  else{
    Serial.println("invalid input");
  }

  drive(drivingDirection, velocity);
}

//  sends the driving instrucitons to the operator
void drivingHelp(){
  Serial.println("send abort to stop (ends driving mode)");
  Serial.println("send f to go forwards and b to go backwards");
  Serial.println("send l to go left, r to go right, and s to go straight");
  Serial.println("send v and afterwards fast, slow or medium to change speed");
}

// checks if a certain line of user input matches a required value (up to 8 letters)
bool readLine(String val){
  int len = val.length();
  
  char val_array[len + 1];
  strcpy(val_array, val.c_str());
  
  for (int i = 0; i < len + 1; i++){
    switch(i){
      case 0:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;
        
      case 1:
        if (waitForInput() != String(val_array[i])){
          return false;
         }
         break;

      case 2:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;

      case 3:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;

      case 4:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;
        
      case 5:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;

      case 6:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;

      case 7:
        if (waitForInput() != String(val_array[i])){
          return false;
        }
        break;
     }
   }

   return true;
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

    if (readLine("ardu") == true){
      Serial.println("the password is valid"); // tells the app the password is valid
      return true;
    }

    else{
      return false;
    }
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
