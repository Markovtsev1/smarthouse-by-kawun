#ifndef DATABASE_H
#define DATABASE_H

#include <Firebase_ESP_Client.h>

void getParamsFromDB();

void sendReport(float humidity, float temp);

void sendChangeName(String text);

String currentDataForJSON();

String currentTimeForJSON();

void setDevMode();

extern FirebaseData fbdo;
extern bool devMode;
extern String roomName;
extern byte hh;
extern byte mm;
extern byte ss;
#endif 