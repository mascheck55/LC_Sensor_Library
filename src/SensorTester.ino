#include <Arduino.h>
#include "LC_Sensor.h"
#define Sensors 7
#define FirstSensor 0 //A0
#define Threshold 36
#define Repeats 5

LC_Sensor mysensor = LC_Sensor();  


void setup() {
// put your setup code here, to run once:
  mysensor.begin(Sensors,Threshold,Repeats);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_BUILTIN, mysensor.activ(6));
  delay(20);
}

