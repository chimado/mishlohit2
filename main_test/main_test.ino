#include <Servo.h>

Servo steering;  // create servo object to control a servo

// variables
const int f = 0; // forwards
const int b = 1; // backwards
const int dservo = 110; // steering angle for going straight

// pins
const int motorf = 8;
const int motorb = 7;
const int pwm1 = 6;
const int steerpin = 9;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  steering.attach(steerpin);  // attaches the servo pin to the servo object, use [servo_name].write(angle{0-180}) to use servo
      
  pinMode(motorf, OUTPUT);
  pinMode(motorb, OUTPUT);

  pinMode(pwm1, OUTPUT);

  digitalWrite(motorb, LOW);
  digitalWrite(motorf, HIGH);
  stop();

  steering.write(dservo);
  }

void drive(int d, int p){ // use drive(direction{f or b}, power {0-255})
  analogWrite(pwm1, p);
    
  if (d == 0){
    digitalWrite(motorb, LOW);
    digitalWrite(motorf, HIGH);
  }

  else{
    digitalWrite(motorb, HIGH);
    digitalWrite(motorf, LOW);
  }
}

void stop(){
  analogWrite(pwm1, 0);
}

void loop() {
  // put your main code here, to run repeatedly:
  drive(f, 240);
  delay(500);
  steering.write(180);
  delay(1000);
  steering.write(0);
  delay(1000);
  steering.write(dservo);
  delay(300);
  drive(b, 255);
  delay(2200);
}
