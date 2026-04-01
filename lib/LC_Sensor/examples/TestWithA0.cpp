#include <Arduino.h>
#include "LC_Sensor.h"
#define Sensors 1
#define FirstSensor 0 //A0
#define Threshold 36 // sensitivity against noise - higher collects more noise
#define Repeats 5 // masking single events - five in a row are expected (400Hz)

LC_Sensor mysensor = LC_Sensor();  


void setup() {
// put your setup code here, to run once:
  mysensor.begin(Sensors,Threshold,Repeats);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, mysensor.activ(FirstSensor));
  delay(20);
}

