#include "databaseFunctions.h"
#include <UniversalTelegramBot.h> // Для отправки сообщений через Telegram бота
#include "config.h"

// Определите переменные, которые используются в функциях
extern FirebaseData fbdo;
extern UniversalTelegramBot bot;
extern String roomName;
extern bool devMode;
extern byte hh;
extern byte mm;
extern byte ss;

// Функция для получения параметров из базы данных
void getParamsFromDB() {
  // Получение пути к имени комнаты
  String roomPath = NAME_PATH;
  String modePath = MODE_PATH;


  // Получение названия комнаты из базы данных Firebase
  if (Firebase.RTDB.getString(&fbdo, roomPath.c_str())) {
    // Если получение успешно, сохраняем название комнаты в переменную roomName
    roomName = fbdo.stringData();
    if (devMode) 
      bot.sendMessage(CHAT_ID, "Название комнаты успешно получено", "");
  } else {
    bot.sendMessage(CHAT_ID, "Не удалось получить название комнаты", "");
    roomName = ""; // Если не удалось получить название комнаты, возвращаем пустую строку
  }

  if (Firebase.RTDB.getString(&fbdo, modePath.c_str())) {
    // Если получение успешно, сохраняем название комнаты в переменную roomName
    devMode = bool(fbdo.stringData());
    bot.sendMessage(CHAT_ID, "Режим отладки установлен", "");
  } else {
    bot.sendMessage(CHAT_ID, "Не удалось получить режим отладки", "");
    devMode = false; // Если не удалось получить режим, возвращаем false
  }
}

// Функция для отправки отчета
void sendReport(float humidity,float temp) {

  // Создание JSON файла
  String jsonData = "{\"temperature\":\"" + String(temp) + "°C\", \"humidity\":\"" + String(humidity) + "%\"}";
  FirebaseJson json;
  json.setJsonData(jsonData);

  String path = REPORTS_PATH;
  path += "/" + currentDataForJSON() + " " + currentTimeForJSON();

  if (!Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json) && devMode) {
    bot.sendMessage(CHAT_ID, fbdo.errorReason().c_str(), "");
  } else if (devMode)
  {
    bot.sendMessage(CHAT_ID, "Отчет успешно отправлен!", "");
  }
}

// Функция для изменения имени комнаты
void sendChangeName(String text) {
  if (text.length() > 12) {
    roomName = text.substring(12);
    String path = NAME_PATH;
    if (Firebase.RTDB.setString(&fbdo, path.c_str(), roomName.c_str())) {
      // Отправляем сообщение в Telegram о успешном изменении названия комнаты
      String message = "Название комнаты успешно изменено на " + roomName;
      bot.sendMessage(CHAT_ID, message, "");
    } else {
      // Отправляем сообщение в Telegram о неудачной попытке изменения названия комнаты
      bot.sendMessage(CHAT_ID, "Не удалось изменить название комнаты в базе данных", "");
    }
  } else {
    bot.sendMessage(CHAT_ID, "Имя комнаты не может быть пустым!", "");
  }
}

// Вспомогательные функции
String currentDataForJSON() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "09-02-2006";
  }
  char buffer[11];  // Для формата DD-MM-YYYY
  strftime(buffer, sizeof(buffer), "%Y-%m-%d", &timeinfo);
  return String(buffer);
}

String currentTimeForJSON() {
  return (hh < 10 ? "0" + String(hh) : String(hh)) + ":" +
         (mm < 10 ? "0" + String(mm) : String(mm))  + ":" +
         (ss < 10 ? "0" + String(ss) : String(ss));
}

void setDevMode()
{
  String path = MODE_PATH;
  if (Firebase.RTDB.setBool(&fbdo, path.c_str(), !devMode)) {
      // Отправляем сообщение в Telegram о успешном изменении режима отладки
      String message = "Режим отладки изменен";
      bot.sendMessage(CHAT_ID, message, "");
      devMode = !devMode;
    } else {
      // Отправляем сообщение в Telegram о неудачной попытке изменения режима отладки
      bot.sendMessage(CHAT_ID, "Не удалось изменить режим отладки", "");
    }
  } 