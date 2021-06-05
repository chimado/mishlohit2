const int motor1pin1 = 2;
const int motor1pin2 = 3;
const int motor2pin1 = 4;
const int motor2pin2 = 5;
const int pwm1 = 9;

void setup() {
  // put your setup code here, to run once:
   Serial.begin(9600);
    
  pinMode(motor1pin1, OUTPUT);
  pinMode(motor1pin2, OUTPUT);
  pinMode(motor2pin1, OUTPUT);
  pinMode(motor2pin2, OUTPUT);

  pinMode(pwm1, OUTPUT);

  digitalWrite(motor1pin1, LOW);
  digitalWrite(motor2pin1, LOW);
  digitalWrite(motor1pin2, HIGH);
  digitalWrite(motor2pin2, HIGH);
}

void drive(int d, int p){
  analogWrite(pwm1, p);
    
  if (d == 0){
    digitalWrite(motor1pin1, LOW);
    digitalWrite(motor2pin1, LOW);
    digitalWrite(motor1pin2, HIGH);
    digitalWrite(motor2pin2, HIGH);
  }

  else{
    digitalWrite(motor1pin1, HIGH);
    digitalWrite(motor2pin1, HIGH);
    digitalWrite(motor1pin2, LOW);
    digitalWrite(motor2pin2, LOW);
  }
}

void stop(){
  analogWrite(pwm1, 0);
}

void loop() {
  // put your main code here, to run repeatedly:   
  drive(1, 100);
  delay(1000);
  stop();
  drive(0, 200);
  delay(1000);
  stop();
}
