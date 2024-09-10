#include <Arduino.h>
#include <GyverHub.h>
#include <GyverOS.h>
#include "GyverNTP.h"
#include <FileData.h>
#include <LittleFS.h>
#include <GyverMAX7219.h>
#include <EncButton.h>

#include "constants.h"

#define GITHUB_AUTHOR  "dee-mos" 
#define DEVICE_GROUP   "DimusDevices"
#define DEVICE_NAME    "KitchenClock"
#define DEVICE_VERSION GITHUB_AUTHOR "/Electronics/" DEVICE_NAME "@1.01"

#define AP_SSID "Rakabas"
#define AP_PASS "abcdef2908"

#define ssidAP DEVICE_NAME
#define passAP "12345678"  // не менее 8 символов

#define BTN_FIRST  D7
#define BTN_SECOND D8
EncButton  button_1(BTN_FIRST);
EncButton  button_2(BTN_SECOND);

// отключаем встроенную реализацию MQTT (для esp)
#define GH_NO_MQTT
// MQTT
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);




GyverOS<3> OS;	// указать макс. количество задач

GyverHub hub(DEVICE_GROUP, DEVICE_NAME);  // префикс, имя, иконка

MAX7219<4, 1, D1, D2, D0> display; // подключение к любым пинам (софт SPI)
// W и H - количество МАТРИЦ по горизонтали и вертикали
// CS, DATA, CLK - номера пинов
uint8_t brightness = 0; // 0..3

FileData wifi_ (&LittleFS, "/wifi.dat",  'A', &eeprom_wifi, sizeof(eeprom_wifi));
FileData clock_(&LittleFS, "/clock.dat", 'A', &eeprom_clock, sizeof(eeprom_clock));
FileData other_(&LittleFS, "/other.dat", 'A', &eeprom_other, sizeof(eeprom_other));
FileData narod_(&LittleFS, "/narod.dat", 'A', &eeprom_monitoring, sizeof(eeprom_monitoring));
FileData color_(&LittleFS, "/color.dat", 'A', &eeprom_colors, sizeof(eeprom_colors));
FileData mqtt_ (&LittleFS, "/mqtt.dat",  'A', &eeprom_mqtt, sizeof(eeprom_mqtt));

GyverNTP ntp(eeprom_clock.gmt);

#define DEF_NTP_SERVER  "pool.ntp.org"
#define DEF_NTP_TZ      3
#define DEF_NTP_INTERVAL  (3600 * 4)

struct TMonoDigitalFont
{
public:
    TMonoDigitalFont(uint8_t width, uint8_t height, uint8_t* fontptr): _width{width}, _height{height}, _fontptr{fontptr} {}
    uint8_t digit(uint8_t digit, uint8_t col) { return *(_fontptr + digit*_width + col); }
    uint8_t width() { return _width; } 
private:
    uint8_t _width;
    uint8_t _height;
    uint8_t* _fontptr;
};

// http://arduino.on.kg/matrix-font
// fill uooer left corner, then rotate once

uint8_t charMap35[] = {// 3x5
0x1F,	0x11,	0x1F,
0x12,	0x1F,	0x10,
0x19,	0x15,	0x17,
0x15,	0x15,	0x1F,
0x07,	0x4,	0x1F,
0x17,	0x15,	0x1D,
0x1F,	0x15,	0x1D,
0x01,	0x1D,	0x03,
0x1F,	0x15,	0x1F,
0x17,	0x15,	0x0F
};

uint8_t charMap37[] = {
	0x3E,	0x41,	0x3E,
	0x42,	0x7F,	0x40,
  0x71,	0x49,	0x47,
  0x49,	0x49,	0x7F,
  0x0F,	0x08,	0x7F,
  0x47,	0x45,	0x39,  
  0x7F,	0x49,	0x79,
	0x71,	0x09,	0x07,  
  0x7F,	0x49,	0x7F,  
  0x4F,	0x49,	0x7F
};  

uint8_t charMap47[] = {
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

TMonoDigitalFont f35(3,5,charMap35);
TMonoDigitalFont f37(3,7,charMap37);
TMonoDigitalFont f47(4,7,charMap47);
TMonoDigitalFont current_font = f47;



void draw_digit(uint8_t x, uint8_t dig)
{
  display.setCursor(x, 0);
  for (uint8_t col = 0; col < current_font.width(); col++)
  {  
    uint8_t bits = current_font.digit(dig,col);
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
    Serial.println F("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    delay(5000);
    ntp.setGMT(eeprom_clock.gmt);
    ntp.setHost(eeprom_clock.host);
    ntp.begin();
    ntp.updateNow();
  }
}


//============================================================================================================================================

void build_networks(gh::Builder& b)
{
  bool flag = 0;
  b.Title(F("Настройки WiFi"));
  flag |= b.Input(eeprom_wifi.ssid).label("Network SSID").maxLen(sizeof(eeprom_wifi.ssid)-1).click();
  flag |= b.Pass(eeprom_wifi.pass).label("Password").maxLen(sizeof(eeprom_wifi.pass)-1).click();
  if(flag)
  {
    wifi_.update();
    wifi_connected();
  }
  b.Title(F("Настройки MQTT"));
  flag |= b.Input(eeprom_mqtt.host).label("Host").maxLen(sizeof(eeprom_mqtt.host)-1).click();
  flag |= b.Input(&eeprom_mqtt.port).label("Port").click();
  flag |= b.Input(eeprom_mqtt.login).label("User").maxLen(sizeof(eeprom_mqtt.login)-1).click();
  flag |= b.Pass(eeprom_mqtt.password).label("Password").maxLen(sizeof(eeprom_mqtt.password)-1).click();
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

void setup_font(uint8_t fontsize)
{
    switch(eeprom_other.font)
    {
      case 0: current_font = f35; break;
      case 1: current_font = f37; break;
      case 2: current_font = f47; break;
      default: current_font = f47; break;
    }
}

void build_clock(gh::Builder& b)
{
  bool flag = 0;
  b.Title(F("Часы"));

/*
  if(b.Slider(&eeprom_other.font).label(String("Font")).range(0, 2, 1).click())
  {
    setup_font(eeprom_other.font);
  }
*/  

  if (b.beginRow()) 
  {
    if(b.Slider_(c_brightness,&brightness).label(String("Яркость")).color(gh::Colors::Yellow).range(0, 2, 1).click())
    {
      //setup_font(eeprom_other.font);
    }
    b.endRow();
  }

/*
  if (b.beginRow()) 
  {
    static uint8_t tab;
    if(b.Tabs(&tab).text("Small;Middle;Large").color(gh::Colors::Red).noLabel(true).noTab(true).square(false).click())
    {
      setup_font(tab);
    }
    b.endRow();
  }
*/ 

/*
  if (b.beginRow()) 
  {
    static gh::Flags flags;
    b.Flags(&flags).text("Small;Middle;Large").color(gh::Colors::Blue).disabled(false).noLabel(false).noTab(true).square(true);
    //setup_font(eeprom_other.font);
    b.endRow();
  }
*/

  if (b.beginRow()) 
  {
    b.Label_(F("ntp_time")).valueStr("-").label("Время");
    b.Label_(F("ntp_date")).valueStr("-").label("Дата");;
    //update_clock_labels();
    b.endRow();
  }

  if(b.Icon_(F("")).click())
  {
    ntp.updateNow();
    update_clock_labels();
    hub.update(F("ntp_date")).valueStr("new");
  }

  if (b.beginRow()) 
  {
      flag |= b.Input_(c_ntp_server,eeprom_clock.host).label(F("Сервер NTP")).maxLen(sizeof(eeprom_clock.host)-1).size(3).click();
      flag |= b.Input_(c_gmt_zone,&eeprom_clock.gmt).label(F("GMT зона")).size(1).click();
      if(flag)
      {
        clock_.update();
        ntp.setGMT(eeprom_clock.gmt);
        ntp.setHost(eeprom_clock.host);
        ntp.updateNow();
      }
      b.endRow();
  }
}



uint8_t spin_am = 2;
uint8_t tab = 0;

void build(gh::Builder& b) 
{
  b.Menu(F("Networks;Clock"));

  switch (b.menu()) {
        case 0:
            build_networks(b);
            break;
        case 1:
            build_clock(b);
            break;
    }
}

// =================================================================================================================================

void task_clock() 
{
  static uint8_t s_h{255}, s_m{255}, s_s{255};
  uint8_t h, m, s, upd;
  char h_str[3], m_str[3];

  upd = 0;

  if(ntp.synced()) 
  {
    h = ntp.hour();
    m = ntp.minute();
    s = ntp.second();

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
    display.setBright(brightness);
    display.clear();

    uint8_t start_x = 0, x_inc = current_font.width() + 1, dot_x = 0;

    //if(upd & 1)
    { 
      draw_digit(start_x, h / 10); start_x += x_inc;
      draw_digit(start_x, h % 10); start_x += x_inc; dot_x = start_x; start_x += 2;
      draw_digit(start_x, m / 10); start_x += x_inc;
      draw_digit(start_x, m % 10); start_x += x_inc;
    }
    //if(upd & 2)
    { 
      display.dot(dot_x, 2, s_s);
      display.dot(dot_x, 5, s_s);
    }

    display.update();
  }

}

void task_ntp() 
{
  //display.printStr(0,0,"NTP");
  ntp.setGMT(eeprom_clock.gmt);
  ntp.setHost(eeprom_clock.host);
}

void task_led()
{
  static int led = 1;
  digitalWrite(LED_BUILTIN, led);
  // led = 1 - led;
}

// MQTT
// обработчик ответа
void manual(gh::Manual m) 
{
    switch (m.connection) 
    {
        case gh::Connection::MQTT:
            client.beginPublish(m.topic, m.text.len, 0);
            if (m.text.pgm) 
            {
                uint8_t buf[m.text.len];
                memcpy_P(buf, m.text.str, m.text.len);
                client.write(buf, m.text.len);
            } 
            else 
            {
                client.write((uint8_t*)m.text.str, m.text.len);
            }
            client.endPublish();
            break;

        default:
            break;
    }
}

// MQTT
void callback(char* topic, byte* data, unsigned int len) 
{
    char data_buf[len + 1];
    memcpy(data_buf, data, len);
    data_buf[len] = 0;

    uint16_t tlen = strlen(topic);
    char topic_buf[tlen + 1];
    memcpy(topic_buf, topic, tlen);
    topic_buf[tlen] = 0;

    Serial.println(topic_buf);
    hub.parse(topic_buf, data_buf, gh::Connection::MQTT);
}

// MQTT
void reconnect() 
{
  static uint8_t tries = 10;

  if(tries > 0)
    Serial.println("MQTT reconnecting...");
  else
    return;  

  while (--tries && !client.connected()) 
  {
      Serial.print('.');
      String clientId = "hub-";
      clientId += String(random(0xffff), HEX);
      if (client.connect(clientId.c_str(), eeprom_mqtt.login, eeprom_mqtt.password)) 
      {
          Serial.println("MQTT connected");
          Serial.println(hub.topicDiscover());
          Serial.println(hub.topicHub());
          client.subscribe(hub.topicDiscover().c_str());
          client.subscribe(hub.topicHub().c_str());
          tries = 10;
      } 
      else 
      {
          delay(1000);
      }
  }
}


void button_1_cb() 
{
  EncButton b = button_1;

    Serial.print("callback: ");
    switch (b.action()) {
        case EB_PRESS:
            Serial.println("press");
            client.publish(hub.topicHub().c_str(),"button_1");
            break;
        case EB_HOLD:
            Serial.println("hold");
            break;
        case EB_STEP:
            Serial.println("step");
            break;
        case EB_RELEASE:
            Serial.print("release. steps: ");
            Serial.print(b.getSteps());
            Serial.print(", press for: ");
            Serial.print(b.pressFor());
            Serial.print(", hold for: ");
            Serial.print(b.holdFor());
            Serial.print(", step for: ");
            Serial.println(b.stepFor());
            break;
        case EB_CLICK:
            Serial.println("click");
            break;
        case EB_CLICKS:
            Serial.print("clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_TURN:
            Serial.print("turn ");
            Serial.print(b.dir());
            Serial.print(" ");
            Serial.print(b.fast());
            Serial.print(" ");
            Serial.println(b.pressing());
            break;
        case EB_REL_HOLD:
            Serial.println("release hold");
            break;
        case EB_REL_HOLD_C:
            Serial.print("release hold clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_REL_STEP:
            Serial.println("release step");
            break;
        case EB_REL_STEP_C:
            Serial.print("release step clicks ");
            Serial.println(b.getClicks());
            break;
        default:
            Serial.println();
    }
}

void button_2_cb() 
{
  EncButton b = button_2;

    Serial.print("callback: ");
    switch (b.action()) {
        case EB_PRESS:
            Serial.println("press");
            client.publish(hub.topicHub().c_str(),"button_2");
            break;
        case EB_HOLD:
            Serial.println("hold");
            break;
        case EB_STEP:
            Serial.println("step");
            break;
        case EB_RELEASE:
            Serial.print("release. steps: ");
            Serial.print(b.getSteps());
            Serial.print(", press for: ");
            Serial.print(b.pressFor());
            Serial.print(", hold for: ");
            Serial.print(b.holdFor());
            Serial.print(", step for: ");
            Serial.println(b.stepFor());
            break;
        case EB_CLICK:
            Serial.println("click");
            break;
        case EB_CLICKS:
            Serial.print("clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_TURN:
            Serial.print("turn ");
            Serial.print(b.dir());
            Serial.print(" ");
            Serial.print(b.fast());
            Serial.print(" ");
            Serial.println(b.pressing());
            break;
        case EB_REL_HOLD:
            Serial.println("release hold");
            break;
        case EB_REL_HOLD_C:
            Serial.print("release hold clicks ");
            Serial.println(b.getClicks());
            break;
        case EB_REL_STEP:
            Serial.println("release step");
            break;
        case EB_REL_STEP_C:
            Serial.print("release step clicks ");
            Serial.println(b.getClicks());
            break;
        default:
            Serial.println();
    }
}



void setup() 
{
  uint8_t dot_x = 0;

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

  setup_font(eeprom_other.font);

  //display.init();
  display.begin();
  display.clear();

  /*
  display.begin(1);
  display.scroll("Hello", 50);
  delay(3000);
  display.noScroll();
  //display.clear();
  */

  pinMode(LED_BUILTIN, OUTPUT);

  button_1.attach(button_1_cb);
  button_2.attach(button_2_cb);

  display.dot(dot_x++, 0);

  wifi_connected();

  display.dot(dot_x++, 0);

  // MQTT
  //hub.mqtt.config(eeprom_mqtt.host, eeprom_mqtt.port);
  client.setServer(eeprom_mqtt.host, eeprom_mqtt.port);
  client.setCallback(callback);
  client.publish("mqtt_init","1");

  display.dot(dot_x++, 0);

  hub.onBuild(build);                     // подключаем билдер
  hub.onManual(manual);
  hub.begin();                            // запускаем систему

  display.dot(dot_x++, 0);

  // подключаем задачи (порядковый номер, имя функции, период в мс)
  OS.attach(0, task_clock, 200);
  OS.attach(1, task_ntp, 1000*60*60*5);
  OS.attach(2, task_led, 1000);

  display.dot(dot_x++, 0);

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

  if (!client.connected()) reconnect();
  if (client.connected()) client.loop();

  button_1.tick();
  button_2.tick();
}



