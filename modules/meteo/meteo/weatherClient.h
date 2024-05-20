#ifndef WEATHER_CLIENT_H
#define WEATHER_CLIENT_H

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>

// Прототипы функций
int weatherUpdate();

// Внешние переменные
extern int internetTemp, internetMinTemp, internetMaxTemp, weatherID;
extern byte humidity, clouds;
extern String location, weather, description;
extern float wind;
extern long timeOffset;
extern byte httpErrCount;

#endif