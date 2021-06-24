//import the library in the sketch
#include <SharpIR.h>

//Create a new instance of the library
//Call the sensor "sensor"
//The model of the sensor is "GP2YA41SK0F"
//The sensor output pin is attached to the pin A0
SharpIR sensor( SharpIR::GP2Y0A41SK0F, A2 );

void setup()
{
  Serial.begin( 9600 ); //Enable the serial comunication
}

void loop()
{
  int distance = sensor.getDistance(); //Calculate the distance in centimeters and store the value in a variable

  Serial.println( distance ); //Print the value to the serial monitor

  delay(100);
}
