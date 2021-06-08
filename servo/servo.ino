#include <Servo.h>

Servo myservo;  // create servo object to control a servo

int pos = 0;

void setup() {
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
}

void loop() {
  for (pos = 0; pos <=180; pos += 45){
    myservo.write(pos);                  // sets the servo position according to the scaled value
    delay(500);                           // waits for the servo to get there
  }
}
