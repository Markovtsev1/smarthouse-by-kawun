#include <NTPClient.h>                  // NTP клиент 
#include <ESP8266WiFi.h>                // Предоставляет специальные процедуры Wi-Fi для ESP8266, которые мы вызываем для подключения к сети
#include <UniversalTelegramBot.h>       // Библиотека для работы с телеграмом
#include <DHT.h>                        // Библиотека для работы с датчиком температуры
#include <DNSServer.h>                  // Локальный DNS сервер для перенаправления всех запросов на страницу конфигурации
#include <WiFiManager.h>                // Библиотека для удобного подключения к WiFi
#include <ESP8266HTTPClient.h>          // HTTP клиент
#include <ArduinoJson.h>                // Библиотека для работы с JSON
#include "config.h"
#include <Firebase_ESP_Client.h>
#include "databaseFunctions.h"
#include "weatherClient.h" 
// Параметры сети WiFi
WiFiClientSecure secured_client;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;


X509List cert(TELEGRAM_CERTIFICATE_ROOT);

// Задержка для ответов бота
const unsigned long BOT_MTBS = 3000;

UniversalTelegramBot bot(BOT_TOKEN, secured_client);
unsigned long bot_lasttime;


// Пин для датчика температуры
#define DHTPIN 14
DHT dht(DHTPIN, DHT11);


//////////////////NTPClient///////////////

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "pool.ntp.org");

///////////////Переменные/////////////////

WiFiManager wifiManager;


byte hh, mm, ss;                        // Часы минуты секунды
String roomName = "Комната 1";
// Переменные для хранения точек отсчета
unsigned long timing, rndTiming, lastUpdWeather, lastUpdData;



FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;


bool devMode = false;

void setup() {
    Serial.begin(9600);
    dht.begin();
    configTime(0, 0, "pool.ntp.org");
    secured_client.setTrustAnchors(&cert);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);            // Подключаемся к WiFi сети

    while (WiFi.status() != WL_CONNECTED)            // Пока соеденение не установлено, выполняем задержку
        delay(300);
    if (devMode)
    bot.sendMessage(CHAT_ID, "Подключено успешно!", "");
    int code = weatherUpdate();                      //Обновление погоды
    if (code != 200) {
        bot.sendMessage(CHAT_ID, "Ошибка соединения с openweathermap.org!");
    }
    timeClient.begin();                              //Инициализация NTP клиента
    timeClient.setTimeOffset(timeOffset);            //Установка оффсета времени

    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    // Comment or pass false value when WiFi reconnection will control by your code or third party library e.g. WiFiManager
    Firebase.reconnectNetwork(true);

    // Since v4.4.x, BearSSL engine was used, the SSL buffer need to be set.
    // Large data transmission may require larger RX buffer, otherwise connection issue or data read time out can be occurred.
    fbdo.setBSSLBufferSize(4096 /* Rx buffer size in bytes from 512 - 16384 */,
                           1024 /* Tx buffer size in bytes from 512 - 16384 */);

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.setDoubleDigits(5);

    config.timeout.serverResponse = 10 * 1000;

    getParamsFromDB();
    sendReport(dht.readHumidity(), dht.readTemperature());
}

void loop() {

    timeClient.update();
    hh = timeClient.getHours();
    mm = timeClient.getMinutes();
    ss = timeClient.getSeconds();             //Обновление времени


    if (millis() - lastUpdWeather > 120000) {         //Обновление погоды раз в 2 минуты
        weatherUpdate();
        lastUpdWeather = millis();
        sendReport(dht.readHumidity(), dht.readTemperature());
    }

    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
}

void handleNewMessages(int numNewMessages) {
    for (int i = 0; i < numNewMessages; i++) {
        if (bot.messages[i].chat_id == CHAT_ID) {
            String text = bot.messages[i].text;
            if (text == "/room_state") {
                sendRoomState();
            }

            if (text == "/outside_state") {
                sendOutsideState();
            }

            if (text.startsWith("/change_name ")) {
                sendChangeName(text);
            }

            if (text == "/send_report") {
                sendReport(dht.readHumidity(), dht.readTemperature());
            }

            if (text == "/set_dev_mode"){
                setDevMode();
            }
        }
    }
}


String currentTime() {
    return (hh < 10 ? "0" + String(hh) : String(hh)) + ":" +
           (mm < 10 ? "0" + String(mm) : String(mm)) + ":" +
           (ss < 10 ? "0" + String(ss) : String(ss));
}

void sendRoomState() {
    int temp = int(dht.readTemperature());
    int humidity = int(dht.readHumidity());
    String roomReport = String("Отчет по комнате ") + roomName + " " + currentTime() + " " + currentData() + "\n" +
                        "Температура в комнате: " + String(temp) + "°C\n" +
                        "Влажность: " + String(humidity) + "%";

    bot.sendMessage(CHAT_ID, roomReport, "");
}


String currentData() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return "09.02.2006";
    }
    char buffer[11];  // Для формата DD-MM-YYYY
    strftime(buffer, sizeof(buffer), "%d.%m.%Y", &timeinfo);
    return String(buffer);
}

void sendOutsideState() {
    String weatherReport = "Отчет по улице " + currentTime() + " " + currentData() + "\n";
    weatherReport += "Температура: " + String(internetTemp) + "°C\n";
    weatherReport += "Минимальная температура: " + String(internetMinTemp) + "°C\n";
    weatherReport += "Максимальная температура: " + String(internetMaxTemp) + "°C\n";
    weatherReport += "Ветер: " + String(wind) + " м/с\n";
    weatherReport += "Описание: " + description + "\n";
    weatherReport += "Тип погоды: " + weather + "\n";
    weatherReport += "Влажность: " + String(humidity) + "%\n";
    weatherReport += "Облачность: " + String(clouds) + "%\n";
    weatherReport += "Местоположение: " + location + "\n";

    bot.sendMessage(CHAT_ID, weatherReport, "");
}