String incoming_value =  "";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  incoming_value = String (Serial.read());
  
  if (incoming_value != "-1"){
    Serial.println(rd());
  }
}

String  rd(){ // string array, it needs to be dynamic and increase its size with each loop, maybe create two arrays and each loop copy one to the other and remake the one copied from but with an additional cell
  String* msg1 = new String[1]{ "0" };
  String* msg2 = new String[1]{ "0" };
  String ans = "";
  
  while (incoming_value != "-1"){ // inserts the last value of incoming_value to the array, then checks if the next value is -1, if not the loop runs again with the value gathered at its end
    msg1[0] = msg2[0];
    msg2[0] = incoming_value;
    incoming_value = String (Serial.read());
    Serial.println(incoming_value);
  }
  // here you need to either convert it all from binary to normal letters, do it with ascii conversion, also the array of strings needs to be made into a single string for the function to output
  Serial.println("1: " + msg1[0]);
  Serial.println("2: "+ msg2[0]);

  ans = "ans: " + msg1[0] + msg2[0];
  return ans;
}
