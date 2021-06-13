// libraries
#include <Servo.h>

/*
bluetooth communications dictionary:
c - connection request
p - password attempt
t - trunk request (0 afterwards for close or 1 for open)
g - gps location incoming
d - drive request
 */

// misc
Servo trunk; // creates servo object to control trunk
Servo steering; // creates servo object to control steering

//  variables
int phase = 0; // 0 is start of drive, 1 is navigation, 2 is end of drive
bool authorized = false; // indicates if the connected device is authorized

// IO pins
const int trunkPin = 3;
const int steeringPin = 5;

void setup() {
  Serial.begin(9600); // sets baud rate for arduino

  // pin attachment
  trunk.attach(trunkPin); // attaches the selected pin to the trunk servo object
  steering.attach(steeringPin); // attaches the selected pin to the steering servo object

  // set initial values
  trunk.write(0);
  steering.write(90);
}

void loop() {
  // checks which phase is the code in, and activates functions accordingly
  if (phase == 0){
    stod();
  }
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
      Serial.println("recieving gps coordinates");
    }

    else if (lastMessage == "d"){
      Serial.println("preparing to drive");
    }

    else{
      Serial.println("invalid input");
    }
    
    
  }
}

// sets the trunk's state, 0 for close 1 for open
void trunkState(String in){
  if (in == "0" || in == "1"){
    int state = in.toInt();

    switch(state){
      case 0:
        trunk.write(0);
        Serial.println("closing trunk");
        break;
        
      case 1:
        trunk.write(180);
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
