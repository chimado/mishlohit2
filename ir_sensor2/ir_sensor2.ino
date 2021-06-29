int IRpin = 0;  
float ch = 60;/*
int counter = 0;
float pvalue = 0;*/ 
                                  
void setup() {
  Serial.begin(9600);                             
}

void loop() { // 15 11.58
  float volts = analogRead(IRpin)*0.0048828125;   
  float distance = 11.58 * pow(volts, -1.10);
  Serial.println(distance);
  /*
  counter = counter + 1;
  if (abs(distance * ch - 10) < abs(distance * pvalue - 10)){
    pvalue = (pvalue + ch) / 2;
  }

  else {
    ch = (pvalue + ch) / 2;
  }

  if (counter == 10){
    Serial.println(ch);
    counter = 0;
  }
  */
  delay(100);                                     
}
