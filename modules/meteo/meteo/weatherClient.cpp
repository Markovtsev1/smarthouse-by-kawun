#include "weatherClient.h"
#include <ESP8266WiFi.h>
#include "config.h"

HTTPClient http;
DynamicJsonDocument doc(1500);

int internetTemp, internetMinTemp, internetMaxTemp, weatherID;
byte humidity, clouds;
String location, weather, description;
float wind;
long timeOffset;
byte httpErrCount = 0;
const String lat = "53.90";             // Географические координаты
const String lon = "27.567";            // Минск

int weatherUpdate() {
    if (WiFi.status() == WL_CONNECTED) {
        String httpStr = String("http://api.openweathermap.org/data/2.5/weather") + 
                         String("?lat=") + String(lat) +
                         String("&lon=") + String(lon) + 
                         String("&appid=") + String(appid) +
                         String("&units=metric&lang=en");
        WiFiClient client;
        http.begin(client, httpStr);

        int httpCode = http.GET();
        String json = http.getString();
        http.end();

        if (httpCode != 200) {
            httpErrCount++;
            return httpCode;
        }

        DeserializationError error = deserializeJson(doc, json);

        if (error) {
            return httpCode;
        }

        internetTemp = doc["main"]["temp"];
        internetMinTemp = doc["main"]["temp_min"];
        internetMaxTemp = doc["main"]["temp_max"];
        wind = doc["wind"]["speed"];
        description = doc["weather"][0]["description"].as<String>();
        weather = doc["weather"][0]["main"].as<String>();
        humidity = doc["main"]["humidity"];
        clouds = doc["clouds"]["all"];
        location = doc["name"].as<String>();
        timeOffset = doc["timezone"];
        weatherID = doc["weather"][0]["id"];

        httpErrCount = 0;

        return httpCode;
    } else {
        return -1;
    }
}