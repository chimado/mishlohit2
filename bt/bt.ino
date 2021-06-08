String incoming_string =  "";
char incoming_char = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  incoming_string = String (Serial.read());
  incoming_char = char (Serial.read());
    
  if (incoming_string != "-1"){
    Serial.println(incoming_char);
  }
}
