# Настройка проекта


## Метеостанция

Перед запуском проекта необходимо создать файл `config.h` в папке `modules/meteo` на основе шаблона `configExample.h` и указать свои значения SSID и пароля, а также остальные приватные данные.

### Создание файла `config.h`

1. Скопируйте файл `configExample.h` и переименуйте  его в `config.h`.
2. Откройте файл `config.h` для редактирования.
3. Заполните следующие параметры:
#### Параметры сети WiFi
```c
#define WIFI_SSID "ваш SSID"
#define WIFI_PASSWORD "ваш пароль сети"
```

- **WIFI_SSID**: SSID вашей WiFi сети. Это имя сети, к которой будет подключаться ваша метеостанция.
- **WIFI_PASSWORD**: Пароль вашей WiFi сети.

#### Параметры бота Telegram

```c
#define BOT_TOKEN "ваш токен бота"
#define CHAT_ID "ваш chat_id"
```

- **BOT_TOKEN**: Токен вашего Telegram бота, который вы можете получить у [BotFather](<https://t.me/BotFather>).
- **CHAT_ID**: Идентификатор чата, куда будут отправляться уведомления. Вы можете получить его, добавив бота в чат и воспользовавшись соответствующими методами Telegram API для получения chat_id.

#### API ключ для openweathermap.org

```c
#define String appid "ваш API для openweathermap.org"
```

- **appid**: API ключ для доступа к данным погоды с [openweathermap.org](<https://openweathermap.org/>). Зарегистрируйтесь на сайте и получите свой ключ в разделе API.

#### Параметры для Firebase

```c
#define DATABASE_API "ваш API для firebase"
#define DATABASE_URL "ваш URL для firebase"
#define USER_EMAIL "ваш логин"
#define USER_PASSWORD "ваш пароль"
```

- **DATABASE_API**: API ключ для доступа к базе данных Firebase. Получите его в консоли Firebase в разделе "Настройки проекта".
- **DATABASE_URL**: URL вашей базы данных Firebase. Его можно найти в консоли Firebase.
- **USER_EMAIL**: Email пользователя для входа в Firebase.
- **USER_PASSWORD**: Пароль пользователя для входа в Firebase.


#### Параметры путей в базе данных

```c
#define NAME_PATH "meteo/params/name"
#define MODE_PATH "meteo/params/devMode"
#define REPORTS_PATH "meteo/reports/room1"
```

- **NAME_PATH**: Путь в базе данных Firebase для хранения имени устройства.
- **MODE_PATH**: Путь в базе данных Firebase для хранения режима работы устройства.
- **REPORTS_PATH**: Путь в базе данных Firebase для хранения пути хранения отчетов по комнате.