
//  variables
int phase = 0; // 0 is start of drive, 1 is navigation, 2 is end of drive
String incoming_string =  "";  // stores the last input sent by the bluetooth serial in decimal form (ascii), if there's no input its value is -1

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // checks which phase is the code in, and activates functions accordingly
  if (phase == 0){
    incoming_string = String(Serial.read()); // reads the serial input from the bluetooth antenna
    stod();
  }
}

// the start of drive phase function, it's responsible for phase 1
void stod(){
  if (conattempt() == true){ //&& ispasswardvalid() == true){ // checks if there's a device attempting to start a connection and if it has a valid password
    
  }
}

// checks if there's a connection attempt being made
// that happens when there's a constant input of "c"
// if there is a connection attempt it accepts it and returns true, otherwise it returns false
bool conattempt(){
  if (btread() == "c"){
    Serial.println("cna"); // this is the string that tells the app the connection has been accepted
    return true
  }
  
  else{
    return false
  }
}

// this makes sure the password is valid, the password is a 4 character message (they're in seperate rows)
// the way it knows if there's a password attempt is by the char "p" being sent
// it sends a confirmation message for each password character, correct or not
/*
bool ispasswordvalid(){
  if (btread() == "p"){
    
  }
  
}
*/
// takes the decimal input of the incoming_string variable and converts it into a char if it's valid, otherwise it returns x
char btread(){
  if (incoming_string != "-1"){
    char message = incoming_string.toInt();
    return message;
  }
  
  else{
    return "x";
  }
}
