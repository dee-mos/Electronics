#include <Arduino.h>
#include <GyverHub.h>
#include <GyverOS.h>
#include <FileData.h>
#include <LittleFS.h>

#include <GyverMAX7219.h>
#include "Ntp.h"
#include "Date.h"

#include "constants.h"

#define DEVICE_GROUP   "DimusDevices"
#define DEVICE_NAME    "KitchenClock"
#define DEVICE_VERSION "dee-mos/Electronics/" DEVICE_NAME "@1.00"

#define AP_SSID "Rakabas"
#define AP_PASS "abcdef2908"

#define ssidAP DEVICE_NAME
#define passAP "12345678"  // не менее 8 символов

GyverOS<3> OS;	// указать макс. количество задач

GyverHub hub(DEVICE_GROUP, DEVICE_NAME);  // префикс, имя, иконка

MAX7219<4, 1, D1, D2, D0> display; // подключение к любым пинам (софт SPI)
// W и H - количество МАТРИЦ по горизонтали и вертикали
// CS, DATA, CLK - номера пинов


FileData wifi_(&LittleFS, "/wifi.dat", 'A', &eeprom_wifi, sizeof(eeprom_wifi));
FileData clock_(&LittleFS, "/clock.dat", 'A', &eeprom_clock, sizeof(eeprom_clock));
FileData other_(&LittleFS, "/other.dat", 'A', &eeprom_other, sizeof(eeprom_other));
FileData narod_(&LittleFS, "/narod.dat", 'A', &eeprom_monitoring, sizeof(eeprom_monitoring));
FileData color_(&LittleFS, "/color.dat", 'A', &eeprom_colors, sizeof(eeprom_colors));
FileData mqtt_(&LittleFS, "/mqtt.dat", 'A', &eeprom_mqtt, sizeof(eeprom_mqtt));


#include "GyverNTP.h"
GyverNTP ntp(eeprom_clock.gmt);

#define DEF_NTP_SERVER  "pool.ntp.org"
#define DEF_NTP_TZ      3
#define DEF_NTP_INTERVAL  (3600 * 4)

const uint8_t charMap4[] = {
    0x3e, 0x41, 0x41, 0x3e,
    0x00, 0x42, 0x7f, 0x40,
    0x62, 0x51, 0x49, 0x46,
    0x22, 0x49, 0x49, 0x36,
    0x0f, 0x08, 0x08, 0x7f,
    0x2f, 0x49, 0x49, 0x31, 
    0x3e, 0x49, 0x49, 0x32,
    0x01, 0x79, 0x05, 0x03,
    0x36, 0x49, 0x49, 0x36,
    0x26, 0x49, 0x49, 0x3e
  };

void draw_digit(uint8_t x, uint8_t dig)
{
  display.setCursor(x, 0);
  for (uint8_t col = 0; col < 4; col++) // 4 столбика буквы 
  {  
    uint8_t bits = charMap4[dig*4 + col];
    display.drawByte(bits);
  }
}


void wifi_connected() 
{
  WiFi.mode(WIFI_STA);
  byte tries = 10;

  //strcpy(w.ssid, AP_SSID);
  //strcpy(w.pass, AP_PASS);

  WiFi.begin(eeprom_wifi.ssid, eeprom_wifi.pass);
  while (--tries && WiFi.status() != WL_CONNECTED) 
  {
    Serial.print F(".");
    delay(1000);
  }
  if (WiFi.status() != WL_CONNECTED) {      // Если не удалось подключиться запускаем в режиме AP
    IPAddress apIP(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    ///display.printStr(0,0,"W-F AP");
    Serial.println F("");
    Serial.print F("WiFi up ACCESS POINT: ");
    Serial.println(ssidAP);
    Serial.print F("Start Settings IP: ");
    Serial.println(apIP);
    WiFi.disconnect();                      // Отключаем WIFI
    WiFi.mode(WIFI_AP);                     // Меняем режим на режим точки доступа
    WiFi.softAPConfig(apIP, apIP, subnet);  // Задаем настройки сети
    WiFi.softAP(ssidAP, passAP);            // Включаем WIFI в режиме точки доступа с именем и паролем
  } else {
    ///display.printStr(0,0,"WiFi");
    Serial.println F("");
    Serial.println F("WiFi запущен");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());

    delay(5000);
    ntp.setGMT(eeprom_clock.gmt);
    ntp.setHost(eeprom_clock.host);
    ntp.begin();
    ntp.updateNow();
  }
}


//============================================================================================================================================

void build_wifi(gh::Builder& b)
{
  bool flag = 0;
  b.Title(F("Настройки WiFi"));
  b.Input(eeprom_wifi.ssid).label("Network SSID").maxLen(sizeof(eeprom_wifi.ssid)-1).attach(&flag);
  b.Pass(eeprom_wifi.pass).label("Password").maxLen(sizeof(eeprom_wifi.pass)-1).attach(&flag);
  if(flag)
  {
    wifi_.update();
    wifi_connected();
  }
}

void build_mqtt(gh::Builder& b)
{
  bool flag = 0;  
  b.Title(F("Настройки MQTT"));
  b.Input(eeprom_mqtt.host).label("Host").maxLen(sizeof(eeprom_mqtt.host)-1).attach(&flag);
  b.Input(&eeprom_mqtt.port).label("Port").attach(&flag);
  b.Input(eeprom_mqtt.login).label("User").maxLen(sizeof(eeprom_mqtt.login)-1).attach(&flag);
  b.Pass(eeprom_mqtt.password).label("Password").maxLen(sizeof(eeprom_mqtt.password)-1).attach(&flag);
  if(flag)
  {
    mqtt_.update();
    hub.mqtt.config(eeprom_mqtt.host, eeprom_mqtt.port);
  }
}

void update_clock_labels()
{
  //hub.update(F("ntp_time")).valueStr(ntp.timeString());
  //hub.update(F("ntp_date")).valueStr(ntp.dateString());
}

void build_clock(gh::Builder& b)
{
  bool flag = 0;
  b.Title(F("Часы"));
  b.Label_(F("ntp_time")).valueStr("-").label("Время");
  b.Label_(F("ntp_date")).valueStr("-").label("Дата");;
  //update_clock_labels();

  if(b.Icon_(F("")).click())
  {
    ntp.updateNow();
    update_clock_labels();
    hub.update(F("ntp_date")).valueStr("new");
    //b.refresh();
  }

  if (b.beginRow()) 
  {
      flag |= b.Input(eeprom_clock.host).label(F("Сервер NTP")).maxLen(sizeof(eeprom_clock.host)-1).size(3).click();
      flag |= b.Input(&eeprom_clock.gmt).label(F("GMT зона")).size(1).click();
      if(flag)
      {
        
        clock_.update();
        ntp.updateNow();
        //b.refresh();
      }
      b.endRow();
  }
}


#define MAX_TEXT 20
#define DYN_MAX 6
bool sws[DYN_MAX];
int16_t slds[DYN_MAX];
String inputs[DYN_MAX];
uint8_t spin_am = 2;
uint8_t tab = 0;


void build_colors(gh::Builder& b)
{
    if (b.beginRow()) {
        bool ref = 0;

        // делаем вкладки, перезагрузка по клику
        ref |= b.Tabs(&tab).text(F("Sliders;Switches;Inputs")).size(4).click();

        // спиннер с количеством, перезагрузка по клику
        //ref |= b.Spinner(&spin_am).label(F("Amount")).range(0, DYN_MAX, 1).click();

        // перезагрузим
        if (ref) b.refresh();
        b.endRow();
    }

    for (int i = 0; i < spin_am; i++) {
        b.beginRow();
        switch (tab) {
            case 0:
                b.Slider(&slds[i]).label(String("Slider #") + i);
                break;
            case 1:
                b.Switch(&sws[i]).label(String("Switch #") + i);
                break;
            case 2:
                b.Input(&inputs[i]).label(String("Input #") + i);
                break;
        }
        b.endRow();
    }
}

void build(gh::Builder& b) 
{
  b.Menu(F("Wi-Fi;MQTT;Clock;Colors"));

  switch (b.menu()) {
        case 0:
            build_wifi(b);
            break;
        case 1:
            build_mqtt(b);
            break;
        case 2:
            build_clock(b);
            break;
        case 3:
            build_colors(b);
            break;
    }
}

// =================================================================================================================================

void task_clock() 
{
  uint32_t t;    
  static uint8_t s_h{255}, s_m{255}, s_s{255};
  uint16_t y;
  uint8_t h, m, s, w, d, mo, upd;
  char h_str[3], m_str[3];

  upd = 0;

  if ((t = ntpTime())) 
  {
    parseEpoch(t, &h, &m, &s, &w, &d, &mo, &y);

    if( (m != s_m) || (h != s_h) )
    {
      itoa(h, h_str, 10);
      itoa(m, m_str, 10);
      s_h = h;
      s_m = m;
      upd |= 1;
    }

    uint8_t sec_frame_x = display.width() - 10, sec_frame_y = 1;
    if (s == 0) 
      display.rect(sec_frame_x, sec_frame_y, sec_frame_x+10-1, sec_frame_y+6-1, GFX_CLEAR);

    for(uint8_t r = 0; r < (s / 10); r++)
      display.lineH(sec_frame_y + r, sec_frame_x, sec_frame_x+10-1, GFX_FILL);

    display.lineH(sec_frame_y + s / 10, sec_frame_x, sec_frame_x+ (s % 10), GFX_FILL);

      
    if( (s & 1) != s_s )
    {
      upd |= 2;
      s_s = s & 1; 
    }
  }

  if(upd)
  {
    //update_clock_labels();

    display.begin();
    display.setBright(slds[0]);
    display.clear();

    //if(upd & 1)
    { 
      draw_digit(0,  h / 10);
      draw_digit(5,  h % 10);
      draw_digit(12, m / 10);
      draw_digit(17, m % 10);
    }
    //if(upd & 2)
    { 
      display.dot(10, 2, s_s);
      display.dot(10, 5, s_s);
    }

    display.update();
  }

}

void task_ntp() 
{
  //display.printStr(0,0,"NTP");
  ntpUpdate(DEF_NTP_SERVER, 3);
}

void task_led()
{
  static int led = 1;
  digitalWrite(LED_BUILTIN, led);
  // led = 1 - led;
}

void setup() 
{
  hub.setVersion(DEVICE_VERSION);

  Serial.begin(115200);

  if(!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  /*FDstat_t stat1 = */wifi_.read();
  /*FDstat_t stat2 = */clock_.read();
  /*FDstat_t stat3 = */other_.read();
  /*FDstat_t stat4 = */narod_.read();
  /*FDstat_t stat5 = */color_.read();
  /*FDstat_t stat6 = */mqtt_.read();

  /*
  display.init();
  display.clear();
  */

  /*
  display.begin(1);
  display.scroll("Hello", 50);
  delay(3000);
  display.noScroll();
  //display.clear();
  */

  pinMode(LED_BUILTIN, OUTPUT);

  wifi_connected();

  hub.mqtt.config(eeprom_mqtt.host, eeprom_mqtt.port);

  hub.onBuild(build);                     // подключаем билдер
  hub.begin();                            // запускаем систему

  // подключаем задачи (порядковый номер, имя функции, период в мс)
  OS.attach(0, task_clock, 200);
  OS.attach(1, task_ntp, 1000*60*60*5);
  OS.attach(2, task_led, 1000);

  OS.exec(1);
}


void loop() 
{
  OS.tick();	// вызывать как можно чаще, задачи выполняются здесь
  hub.tick();

  wifi_.tick();
  clock_.tick();
  other_.tick();
  narod_.tick();
  color_.tick();
  mqtt_.tick();  

  ntp.tick();
}



