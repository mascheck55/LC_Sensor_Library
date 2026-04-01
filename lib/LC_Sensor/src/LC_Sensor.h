/*
  LC_Sensor - a library for Arduino UNO / Nano to handle A0 to A6 as LC Sensor
  uses D6 for compare, A7 is readen as AnalogInput with limited accuracy 0...255
  Arduino Nano 16MHz
  Timer2 and interrupt are used, D6 used,

  MIT licensed.
  Made by using:
  Arduino Sketch zum Betrieb induktiver Näherungssensoren
  Beschreibung siehe www.coole-basteleien.de/schranke.html
  (C) Copyright Autor: A. Messmer 2025
  
  zu Testzwecken stark bearbeitet von A.Mascheck 19.01.2026, 24.03.2026
  Nutzung von A7 als analogen Eingang zur Analog Digital Wandlung
  beispielsweise zur Einstellung von _trigger mittels Poti zur Laufzeit
*/

#ifndef LC_SENSOR_H
#define LC_SENSOR_H
#include "Arduino.h"
class LC_Sensor
{

public:
    LC_Sensor(void);
    bool end(void);
    bool begin(uint8_t nr_sensors, uint8_t trigger, uint8_t repeat);
    uint8_t read(uint8_t A_pin); //7 LC Sensors are connected to A0 to A6 they deliver a 30...0 counter
    bool activ(uint8_t A_pin);//gives true if sensor is active
    uint8_t pins(void); //gives the used LC_Sensor pins count starting from A0
};
#endif
