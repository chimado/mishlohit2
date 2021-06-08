char Incoming_value = 0;  

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  Incoming_value = Serial.read();
  Serial.println(Incoming_value);
  delay(200);

}
