#ifndef CONFIG_h
#define CONFIG_h

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Конфигурационный файл для настройки проекта
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define ATARI_CMD_DELAY   5         // Задержка перед приёмом кода команды

// Порт для подключения Atari SIO к Arduino DUE (Serial1)
// Atari SIO 3 (data in)  <-> DUE 18 (TX1)
// Atari SIO 5 (data out) <-> DUE 19 (RX1)
// Atari SIO 7 (command)  <-> DUE 22
#define ATARI_SIO         Serial1
#define ATARI_SIO_EVENT   serialEvent1 
#define ATARI_CMD_PIN     22

// Номер пина для подключения SD-Card
#define SD_PIN_SELECT     4         // Shield

// Флаг разрешающий запись отладочной информации в консоль
#define DEBUG

// Имя устройства для вывода отладочной информации
#define LOG               Serial

// Настройка пинов сдвоенного семисегментного индикатора
#define LED_BASE_1        33        // Управляющий PIN (земля) для отображения 1й цифры
#define LED_BASE_2        35        // Управляющий PIN (земля) для отображения 2й цифры

#define LED_A             37        // PIN к которому подключен сегмент A
#define LED_B             39        // PIN к которому подключен сегмент B
#define LED_C             41        // PIN к которому подключен сегмент C
#define LED_D             43        // PIN к которому подключен сегмент D
#define LED_E             45        // PIN к которому подключен сегмент E
#define LED_F             47        // PIN к которому подключен сегмент F
#define LED_G             49        // PIN к которому подключен сегмент G
#define LED_P             51        // PIN к которому подключен сегмент точка

#define LED_GND           53        // PIN земли. На него подаётся LOW уровень

// Настройка пинов кнопки «Play» магнитофона
#define BTN_GND           23        // PIN земли. На него подаётся LOW уровень
#define PLAY_BTN_PIN      25        // PIN кнопки «Play»

// Задержка в милисекундах для исключения «дребезга» контактов
#define DEBOUNCE_DELAY    20

#endif
