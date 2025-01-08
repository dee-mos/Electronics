#include <Arduino.h>
#include <GyverHub.h>
#include <GyverOS.h>
#include "GyverNTP.h"
#include <FileData.h>
#include <LittleFS.h>
#include <GyverMAX7219.h>
#include <EncButton.h>

#include "clock_fonts.h"

#include "constants.h"

#define GITHUB_AUTHOR  "dee-mos" 
#define DEVICE_GROUP   "DimusDevices"
#define DEVICE_NAME    "KitchenClock"
#define DEVICE_VERSION GITHUB_AUTHOR "/Electronics/" DEVICE_NAME "@1.01"

#define AP_SSID "Rakabas"
#define AP_PASS "abcdef2908"

#define ssidAP DEVICE_NAME
#define passAP "12345678"  // не менее 8 символов

//#define USE_BUTTONS 
#undef USE_BUTTONS 
#ifdef USE_BUTTONS
#define BTN_FIRST  D7
#define BTN_SECOND D8
EncButton  button_1(BTN_FIRST);
EncButton  button_2(BTN_SECOND);
#endif

//#define DEBUG_DIGIT(d) { display.begin(); display.setBright(brightness); display.clear(); draw_digit(0, d); display.update(); }
#define DEBUG_DIGIT(d)

// отключаем встроенную реализацию MQTT (для esp)
#define GH_NO_MQTT
// MQTT
#include <PubSubClient.h>
WiFiClient espClient;
PubSubClient client(espClient);




GyverOS<3> OS;	// указать макс. количество задач

GyverHub hub(DEVICE_GROUP, DEVICE_NAME);  // префикс, имя, иконка

// pins: CS, DAT, CLK
//MAX7219<4, 1, D1, D2, D0> display; // подключение к любым пинам (софт SPI)
MAX7219<4, 1, D5, D6, D7> display; // подключение к любым пинам (софт SPI)


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

  Serial.print F("WiFi in eeprom: ");
  Serial.print(eeprom_wifi.ssid);
  Serial.print(eeprom_wifi.pass);

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
    Serial.print F("Set GMT");
    ntp.setGMT(eeprom_clock.gmt);
    Serial.println(eeprom_clock.gmt);
    Serial.print F("Set clock host");
    ntp.setHost(eeprom_clock.host);
    Serial.println(eeprom_clock.host);
    ntp.begin();
    ntp.updateNow();
    Serial.println F("NTP initialized");
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
      case 3: current_font = f57; break;
      default: current_font = f47; break;
    }
}

void build_clock(gh::Builder& b)
{
  bool flag = 0;
  b.Title(F("Часы"));

  if(b.Slider_(c_font,&eeprom_other.font).label(String("Font")).range(0, 3, 1).click())
  {
    setup_font(eeprom_other.font);
  }

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
    b.Label_(F("ntp_time")).label("Время");
    b.Label_(F("ntp_date")).label("Дата");;
    //update_clock_labels();
    b.endRow();
  }

  if(b.Icon_(F("")).click())
  {
    ntp.updateNow();
    update_clock_labels();
    hub.update(F("ntp_date")).value("new");
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
    DEBUG_DIGIT(6) 

    h = ntp.hour();
    m = ntp.minute();
    s = ntp.second();

    DEBUG_DIGIT(7) 

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

    DEBUG_DIGIT(8) 
      
    if( (s & 1) != s_s )
    {
      upd |= 2;
      s_s = s & 1; 
    }
  }

  DEBUG_DIGIT(9) 

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
  ntp.setGMT(eeprom_clock.gmt);
  ntp.setHost(eeprom_clock.host);
}

void task_led()
{
  static int led = 1;
  digitalWrite(LED_BUILTIN, led);
  led = 1 - led;
}

void task_weather()
{
}



class HubMQTT : public gh::Bridge {
   public:
    HubMQTT(void* hub) : gh::Bridge(hub, gh::Connection::MQTT, GyverHub::parseHook), mqtt(espClient) {}

    void begin() {
        mqtt.setServer("test.mosquitto.org", 1883);
        mqtt.setCallback([this](char* topic, uint8_t* data, uint16_t len) {
            uint16_t tlen = strlen(topic);
            char topic_buf[tlen + 1];
            memcpy(topic_buf, topic, tlen);
            topic_buf[tlen] = 0;

            char data_buf[len + 1];
            memcpy(data_buf, data, len);
            data_buf[len] = 0;

            parse(sutil::AnyText(topic_buf, tlen), sutil::AnyText(data_buf, len));
        });
    }
    void end() {
        mqtt.disconnect();
    }
    void tick() {
        if (!mqtt.connected()) reconnect();
        mqtt.loop();
    }
    void send(gh::BridgeData& data) {
        if (!mqtt.connected()) return;
        mqtt.beginPublish(data.topic.c_str(), data.text.length(), 0);
        mqtt.print(data.text);
        mqtt.endPublish();
    }

   private:
    WiFiClient espClient;
    PubSubClient mqtt;

    void reconnect() {
        while (!mqtt.connected()) {
            String clientId = "hub-";
            clientId += String(random(0xffff), HEX);
            if (mqtt.connect(clientId.c_str())) {
                Serial.println("MQTT connected");
                mqtt.subscribe(hub.topicDiscover().c_str());
                mqtt.subscribe(hub.topicHub().c_str());
            } else {
                Serial.println("MQTT connection loop");
                delay(1000);
                break;
            }
        }
    }
};

//HubMQTT mqtt(&hub);


#ifdef USE_BUTTONS
#include "button.inc"
#endif


void setup() 
{
  hub.setVersion(DEVICE_VERSION);

  Serial.begin(115200);

  if(!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  /*FDstat_t stat1 =*/ wifi_.read();
  /*FDstat_t stat2 =*/ clock_.read();
  /*FDstat_t stat3 =*/ other_.read();
  /*FDstat_t stat4 =*/ narod_.read();
  /*FDstat_t stat5 =*/ color_.read();
  /*FDstat_t stat6 =*/ mqtt_.read();

  setup_font(eeprom_other.font);

  //display.init();
  display.begin();
  display.clear();

  DEBUG_DIGIT(0)

  /*
  display.begin(1);
  display.scroll("Hello", 50);
  delay(3000);
  display.noScroll();
  //display.clear();
  */

  pinMode(LED_BUILTIN, OUTPUT);

#ifdef USE_BUTTONS
  button_1.attach(button_1_cb);
  button_2.attach(button_2_cb);
#endif

  DEBUG_DIGIT(1)

  wifi_connected();

  DEBUG_DIGIT(2)

  hub.onBuild(build);                     // подключаем билдер
  //hub.addBridge(&mqtt);
  hub.begin();                            // запускаем систему

  DEBUG_DIGIT(3)

  // подключаем задачи (порядковый номер, имя функции, период в мс)
  OS.attach(0, task_clock, 200);
  OS.attach(1, task_ntp, 1000*60*60*5);
  OS.attach(2, task_led, 1000);
  OS.attach(3, task_weather, 1000*60*10); // 10 min

  DEBUG_DIGIT(4)

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

#ifdef USE_BUTTONS
  button_1.tick();
  button_2.tick();
#endif
}

    
   
