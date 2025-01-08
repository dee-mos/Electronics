#include <Arduino.h>


// MQTT names
const char* c_ntp_server = "ntp_server";
const char* c_gmt_zone = "gmt_zone";
const char* c_brightness = "brightness";
const char* c_font = "font";


struct Wifi 
{
  char ssid[32] = "";
  char pass[32] = "";
};
Wifi eeprom_wifi;

struct Clock 
{
  int gmt = 3;                     // часовой пояс, 3 для МСК
  char host[32] = "pool.ntp.org";  // NTP сервер
};
Clock eeprom_clock;

struct Other 
{
  float cor_tempH = 0;   // корректировка показаний датчика комнатной температуры
  float cor_tempS = 0;   // корректировка показаний датчика уличной температуры
  int cor_pres = 0;      // корректировка показаний датчика давления
  int cor_hum = 0;       // корректировка показаний датчика влажности
  bool auto_bright;      // автоматическая подстройка яркости от уровня внешнего освещения (1 - включить, 0 - выключить)
  int min_bright = 10;   // минимальная яркость (0 - 255)
  int max_bright = 200;  // максимальная яркость (0 - 255)
  int brg = 10;          // как часто проверять изменение по датчику освещенности в сек
  bool min_max = false;
  bool sens_bme = false;  // если модуль bme, то true, иначе false
  int interval = 5;
  int font = 4;           // 3 or 4
};
Other eeprom_other;

struct Monitoring 
{
  bool Monitoring = false;  // включаем мониторинг, иначе false
  int delay_narod = 300;    // как часто отправлять значения датчиков на мониторинг, минимум 5 минут, значение в секундах, плюс запас в 5 сек, на всякий случай, получается минимум 305
  bool nm_tempH = false;    // включить отправку показаний комнатной температуры
  bool nm_tempS = false;    // включить отправку показаний уличной температуры
  bool nm_pres = false;     // включить отправку показаний давления
  bool nm_hum = false;      // включить отправку показаний влажности
};
Monitoring eeprom_monitoring;

struct Color 
{
  uint32_t color_minus = 0x00ffff;
  uint32_t color_clock = 0x00ffff;
  uint32_t color_home = 0xffd300;
  uint32_t color_street = 0x6363d1;
  uint32_t color_press = 0xfc9607;
  uint32_t color_hum = 0xbd35d3;
  uint32_t color_text = 0xc5bbbb;
};
Color eeprom_colors;

struct MQTT 
{
  char host[32] = "192.168.1.100";  // MQTT brocker
  int port = 1883;
  char login[32] = "admin";
  char password[32] = "admin";
};
MQTT eeprom_mqtt;
