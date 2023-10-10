#include <Arduino.h>
#include <GyverHub.h>
#include <GyverOS.h>
#include <FileData.h>
#include <LittleFS.h>

#include <GyverMAX7219.h>
//#include "MAX7219.h"
#include "Ntp.h"
#include "Date.h"

#include "constants.h"

#define DEVICE_VERSION "dee-mos/Electronics/Clock7219@1.06"

#define AP_SSID "Rakabas"
#define AP_PASS "abcdef2908"

#define ssidAP "MatrixClock"
#define passAP "12345678"  // не менее 8 символов

GyverOS<3> OS;	// указать макс. количество задач

GyverHub hub("DimusDevices", "Clock 7219");  // префикс, имя, иконка

MAX7219<4, 1, D1, D2, D0> display; // подключение к любым пинам (софт SPI)
// W и H - количество МАТРИЦ по горизонтали и вертикали
// CS, DATA, CLK - номера пинов


FileData wifi_(&LittleFS, "/wifi.dat", 'A', &w, sizeof(w));
FileData clock_(&LittleFS, "/clock.dat", 'A', &c, sizeof(c));
FileData other_(&LittleFS, "/other.dat", 'A', &o, sizeof(o));
FileData narod_(&LittleFS, "/narod.dat", 'A', &m, sizeof(m));
FileData color_(&LittleFS, "/color.dat", 'A', &col, sizeof(col));


#include "GyverNTP.h"
GyverNTP ntp(c.gmt);

#define DEF_NTP_SERVER  "pool.ntp.org"
#define DEF_NTP_TZ      3
#define DEF_NTP_INTERVAL  (3600 * 4)


#define MAX_TEXT 20
#define DYN_MAX 5

bool sws[DYN_MAX];
int16_t slds[DYN_MAX];
char inputs[DYN_MAX][20];
uint8_t spin_am = 2;
uint8_t tab = 0;


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



void build() 
{
  hub.BeginWidgets();
  hub.WidgetSize(100);
  if (hub.Tabs(&tab, F("Часы,Общее"), GH_NO_LABEL)) {
    hub.refresh();  // обновить страницу
  }

  switch (tab) {
    case 0:
      {
        hub.Title(F("Настройки WiFi"));
        hub.WidgetSize(100);
        bool flag_w = 0;
        flag_w |= hub.Input(&w.ssid, GH_CSTR, F("SSID"), 32);
        flag_w |= hub.Pass(&w.pass, GH_CSTR, F("PASS"), 32);
        if (flag_w) {
          wifi_.update();
        }
        
        bool flag_c = 0;
        bool flag_sync = 0;
        hub.Title(F("Настройки времени"));
        hub.WidgetSize(45);
        hub.Label_(F("n1"), String(ntp.timeString()), GH_NO_LABEL, GH_DEFAULT, 20);
        hub.Label(String(ntp.dateString()), GH_NO_LABEL, GH_DEFAULT, 20);
        hub.WidgetSize(10);
        flag_sync |= hub.ButtonIcon(0, F(""), GH_DEFAULT, 20);
        if (flag_sync) {
          ntp.updateNow();
        }
        hub.WidgetSize(70);
        flag_c |= hub.Input(&c.host, GH_CSTR, F("Сервер NTP"), 32);
        hub.WidgetSize(30);
        flag_c |= hub.Input(&c.gmt, GH_UINT16, F("GMT зона"), 3);
        if (flag_c) {
          clock_.update();
        }
      }
      break;

    case 1:
      {
        hub.WidgetSize(100);
        bool a = hub.Slider(&slds[0], GH_INT16, "Яркость", 0, 15);
      }
      break;
  }
  hub.EndWidgets();
}



void build2() {
    hub.BeginWidgets();

    hub.WidgetSize(80);
    hub.Tabs(&tab, F("Settings"));

    // спиннер с настройкой количества. По клику обновляем страницу
    /*
    hub.WidgetSize(20);
    if (hub.Spinner(&spin_am, GH_UINT8, F("Amount"), 0, DYN_MAX, 1)) {
        hub.refresh();
    }
    */

    switch (tab) {
        case 0:
            hub.WidgetSize(100);
            
            bool a = hub.Slider(&slds[0], GH_INT16, "Brightness", 0, 15);
            //if (a) Serial.println(String("Set slider: #") + i + ", value: " + slds[i]);
            
            break;
        /*
        case 1:
            hub.WidgetSize(25);
            for (int i = 0; i < spin_am; i++) {
                bool a = hub.Switch(&sws[i], String("Switch #") + i);
                if (a) Serial.println(String("Set switch: #") + i + ", value: " + sws[i]);
            }
            break;
        case 2:
            hub.WidgetSize(50);
            for (int i = 0; i < spin_am; i++) {
                bool a = hub.Input(&inputs[i], GH_CSTR, String("Input #") + i);
                if (a) Serial.println(String("Set input: #") + i + ", value: " + inputs[i]);
            }
            break;
        case 3:
            hub.WidgetSize(25);
            for (int i = 0; i < spin_am; i++) {
                // имена компонентов тоже можно генерировать, если это нужно
                bool a = (hub.Button_(String("btn/") + i, 0, String("Button #") + i) == 1);
                if (a) Serial.println(String("Pressed button: btn/") + i);
            }
            break;
            */    
    }
}


void task_clock() 
{
  uint32_t t;    
  static uint8_t s_h{255}, s_m{255}, s_s{255};
  uint16_t y;
  uint8_t h, m, s, w, d, mo, upd;
  char str[12], h_str[3], m_str[3];

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
  static int led = 0;
  digitalWrite(LED_BUILTIN, led);
  // led = 1 - led;
}

void display_brightness()
{
    char str[12];
    /*
    display.beginUpdate();
    display.clear();
    display.setBrightness(slds[0]);    
    sprintf_P(str, PSTR("%d"), slds[0]);
    display.printStr(0, 0, str);
    display.endUpdate();
    */ 
}


void wifi_connected() 
{
  WiFi.mode(WIFI_STA);
  byte tries = 30;

  //strcpy(w.ssid, AP_SSID);
  //strcpy(w.pass, AP_PASS);

  WiFi.begin(w.ssid, w.pass);
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
    ntp.setGMT(c.gmt);
    ntp.setHost(c.host);
    ntp.begin();
    ntp.updateNow();
  }
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

  FDstat_t stat1 = wifi_.read();
  FDstat_t stat2 = clock_.read();
  FDstat_t stat3 = other_.read();
  FDstat_t stat4 = narod_.read();
  FDstat_t stat5 = color_.read();

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

  hub.onBuild(build);                     // подключаем билдер
  hub.begin();                            // запускаем систему

  //hub.setupMQTT("test.mosquitto.org", 1883);

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
  ntp.tick();
}



