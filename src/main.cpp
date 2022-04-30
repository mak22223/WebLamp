/*
  Скетч к проекту "WebLamp"
  - Страница проекта (схемы, описания): https://alexgyver.ru/weblamp/
  - Исходники на GitHub: https://github.com/AlexGyver/WebLamp
  Проблемы с загрузкой? Читай гайд для новичков: https://alexgyver.ru/arduino-first/
  AlexGyver, AlexGyver Technologies, 2022

  1.0 - релиз
*/

#define LED_PIN D3    // пин ленты
#define BTN_PIN D7    // пин кнопки
#define PIR_PIN D8    // пин PIR (ИК-датчика)
#define LED_AMOUNT 18 // кол-во светодиодов
#define BTN_LEVEL 1   // 1 - кнопка подключает VCC, 0 - подключает GND
#define USE_PIR 1     // 1 - использовать PIR (ИК-датчик) на этой лампе
#define IGNORE_PIR 0  // 1 - игнорировать сигнал PIR (ИК датчика) с удалённой лампы


/*
  Запуск:
  Клик или >15 секунд при анимации подключения: запустить точку доступа
  Кнопка сохранить или клик: перезагрузить систему

  Анимация:
  - Мигает зелёным: подключение к роутеру
  - Мигает синим: запущена точка доступа WLamp <IP>

  Работа:
  1 клик: вкл/выкл
  2 клика: сменить цвет
  3 клика: подмигнуть
  Удержание: сменить яркость
*/

// ============= ВСЯКОЕ =============
#define MQTT_HEADER "GWL:"  // заголовок пакета данных

// ============= БИБЛЫ =============
#include <GyverPortal.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <EEManager.h>
#include <FastLED.h>
#define EB_STEP 100   // период step шага кнопки
#include <EncButton.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "Timer.h"

// ============= ДАННЫЕ =============

#define DEBUG

#ifdef DEBUG
#define DEBUG(x) Serial.print(x)
#define DEBUGLN(x) Serial.println(x)
#define DEBUG_START Serial.begin(115200);
#else
#define DEBUG(x)
#define DEBUGLN(x)
#define DEBUG_START
#endif

struct LampData {
  char ssid[32] = "";
  char pass[32] = "";
  char local[20] = "AG_lamp_1";
  char remote0[20] = "AG_lamp_2";
  char remote1[20] = "AG_lamp_3";
  char host[32] = "broker.mqttdashboard.com";
  int nightEnd = 8, nightStart = 20;
  char ntpUrl[32] = "ntp1.stratum2.ru";
  int ntpTimezone = 3;
  bool nightModeEn = true;
  uint16_t port = 1883;
  uint8_t ip[4] = {0, 0, 0, 0};

  bool power = 1;
  uint8_t bright = 50;
  uint8_t color = 0;
};

WiFiUDP ntpUdp;
NTPClient ntpTime(ntpUdp);
LampData data;
EncButton<EB_TICK, BTN_PIN> btn;
CRGB leds[LED_AMOUNT];
WiFiClient espClient;
PubSubClient mqtt(espClient);
GyverPortal portal;
EEManager memory(data);
bool pirFlag = 0;
bool winkFlag = 0;
uint8_t winkTimes = 0;
bool startFlag = 0;
const uint8_t hLen = strlen(MQTT_HEADER);

Timer onlineTmr(18000, false);  // 18 секунд таймаут онлайна
Timer pirTmr(60000, false);     // 1 минута таймаут пира
Timer hbTmr(8000);              // 8 секунд период отправки пакета

void webfaceBuilder();
void buttonTick();
bool checkPortal();
void mqttTick();
void connectMQTT();
bool checkPortal();
void localPortal(IPAddress ip);
void callback(char* topic, byte* payload, uint16_t len);
void sendPacket();
void heartbeat();
void wink();
void brightLoop(int from, int to, int step);
void loadAnimation(CRGB color);
int getFromIndex(char* str, int idx, char div = ',');

inline bool isNight() {
  return (ntpTime.getHours() < data.nightEnd || ntpTime.getHours() >= data.nightStart) && data.nightModeEn ? true : false;
}

//////////////////////////////////////////////////

void webfaceBuilder() {
  String s;
  BUILD_BEGIN(s);

  add.THEME(GP_DARK);
  add.AJAX_UPDATE("ledL,ledR,ledP,sw,br,col,nm", 2000);

  add.LABEL("STATUS");
  add.BLOCK_BEGIN();
  add.LABEL("Local:");
  add.LED_GREEN("ledL", mqtt.connected());
  /// TODO: добавить индикаторы для второго remote
  add.LABEL("Remote:");
  add.LED_GREEN("ledR", !onlineTmr.elapsed());
  add.BREAK();
  add.LABEL("Remote PIR:");
  add.LED_RED("ledP", (!pirTmr.elapsed() && !onlineTmr.elapsed()));
  add.BLOCK_END();

  add.LABEL("SETTINGS");
  add.BLOCK_BEGIN();
  add.LABEL("Power:");
  add.SWITCH("sw", data.power);
  add.BREAK();
  add.SLIDER("br", "Bright:", data.bright, 0, 255);
  add.SLIDER("col", "Color:", data.color, 0, 255);
  add.BLOCK_END();

  add.FORM_BEGIN("/save");

  add.LABEL("WIFI");
  add.BLOCK_BEGIN();
  add.TEXT("ssid", "SSID", data.ssid);
  add.BREAK();
  add.PASS("pass", "Password", data.pass);
  add.BLOCK_END();

  add.LABEL("MQTT");
  add.BLOCK_BEGIN();
  add.TEXT("local", "Local Name", data.local);
  add.BREAK();
  add.TEXT("remote0", "First Remote Name", data.remote0);
  add.BREAK();
  add.TEXT("remote1", "Second Remote Name", data.remote1);
  add.BREAK();
  add.TEXT("host", "Host", data.host);
  add.BREAK();
  add.NUMBER("port", "Port", data.port);
  add.BLOCK_END();

  add.LABEL("NTP");
  add.BLOCK_BEGIN();
  add.LABEL("Night mode enable");
  add.SWITCH("nm", data.nightModeEn);
  add.BREAK();
  add.LABEL("Night end");
  add.NUMBER("nightEnd", "Night end, h", data.nightEnd);
  add.BREAK();
  add.LABEL("Night start");
  add.NUMBER("nightStart", "Night start, h", data.nightStart);
  add.BREAK();
  add.LABEL("NTP Server URL");
  add.TEXT("ntpSrv", "NTP Server URL", data.ntpUrl);
  add.BREAK();
  add.LABEL("Timezone");
  add.SELECT("timezone", "UTC-12,UTC-11,UTC-10,UTC-9,UTC-8,UTC-7,UTC-6,\
UTC-5,UTC-4,UTC-3,UTC-2,UTC-1,UTC+0,UTC+1,UTC+2,UTC+3,UTC+4,\
UTC+5,UTC+6,UTC+7,UTC+8,UTC+9,UTC+10,UTC+11,UTC+12", data.ntpTimezone + 12);
  add.BLOCK_END();
  add.SUBMIT("Save");

  add.FORM_END();

  BUILD_END();
}

void buttonTick() {
  btn.tick();

  // клики
  switch (btn.hasClicks()) {
    case 1:   // вкл выкл
      data.power = !data.power;
      sendPacket();
      memory.update();
      break;
    case 2:   // сменить цвет
      data.color += 32;
      sendPacket();
      memory.update();
      break;
    case 3:   // подмигнуть
      winkFlag = 1;
      sendPacket();
      break;
  }

  // импульсное удержание
  static int8_t dir = 10;
  if (data.power) {
    if (btn.step()) {
      data.bright = constrain(data.bright + dir, 0, 255);
      if (data.bright == 255) {
        winkTimes = 1;
      }
    }
    if (btn.releaseStep()) {
      dir = -dir;
      memory.update();
    }
  }
}

bool checkPortal() {
  // клики
  if (portal.click()) {
    if (portal.click("br")) data.bright = portal.getInt("br");
    if (portal.click("sw")) {
      data.power = portal.getCheck("sw");
      sendPacket();
    }
    if (portal.click("col")) {
      data.color = portal.getInt("col");
      sendPacket();
    }
    if (portal.click("nm")) {
      data.nightModeEn = portal.getCheck("nm");
    }
    if (portal.click()) {
      memory.update();
    }
  }

  // обновления
  if (portal.update()) {
    if (portal.update("ledL")) portal.answer(mqtt.connected());
    if (portal.update("ledR")) portal.answer(!onlineTmr.elapsed());
    if (portal.update("ledP")) portal.answer((!pirTmr.elapsed() && !onlineTmr.elapsed()));
    if (portal.update("br")) portal.answer(data.bright);
    if (portal.update("sw")) portal.answer(data.power);
    if (portal.update("col")) portal.answer(data.color);
    if (portal.update("nm")) portal.answer(data.nightModeEn);
  }

  // формы
  if (portal.form()) {
    if (portal.form("/save")) {
      portal.copyStr("ssid", data.ssid);
      portal.copyStr("pass", data.pass);
      portal.copyStr("local", data.local);
      portal.copyStr("remote0", data.remote0);
      portal.copyStr("remote1", data.remote1);
      portal.copyStr("host", data.host);
      data.port = portal.getInt("port");

      int tempNightEnd = portal.getInt("nightEnd");
      int tempNightStart = portal.getInt("nightStart");
      if (tempNightEnd < tempNightStart) {
        data.nightEnd = tempNightEnd;
        data.nightStart = tempNightStart;
      }
      portal.copyStr("ntpSrv", data.ntpUrl);

      char timezone[8];
      portal.copyStr("timezone", timezone);
      for (int i = 3; i < 7; ++i) {
        timezone[i - 3] = timezone[i];
      }
      data.ntpTimezone = atoi(timezone);

      memory.updateNow();
      mqtt.disconnect();
      mqtt.setServer(data.host, data.port);
      connectMQTT();
      // true если submit, для выхода из цикла в AP
      return 1;
    }
  }
  return 0;
}

////////////////////////////////////////////////////

/*
  Протокол:
  GWL:0,ir  // пакет heartbeat, состояние ИК
  GWL:1,power,color,wink  // пакет данных
  GWL:2     // запрос настроек
*/

// опрашиваем mqtt
void mqttTick() {
  if (WiFi.status() != WL_CONNECTED) return;  // wifi не подключен
  if (!mqtt.connected()) connectMQTT();
  else {
    if (!startFlag) {
      startFlag = 1;
      char str[] = MQTT_HEADER "2";  // +2
      mqtt.publish(data.remote0, str); // запрос цвета у обеих ламп
      mqtt.publish(data.remote1, str); 
    }
  }
  mqtt.loop();
}

void connectMQTT() {
  // задаём случайный ID
  String id("WebLamp-");
  id += String(random(0xffffff), HEX) + String(random(0xffffff), HEX);
  
  // подписываемся на своё имя
  if (mqtt.connect(id.c_str())) {
    mqtt.subscribe(data.local);
  }
  /// TODO: убрать
  yield();
}

// тут нам прилетел пакет от удалённой лампы
void callback(char* topic, byte* payload, uint16_t len) {
  payload[len] = '\0';        // закрываем строку
  char* str = (char*)payload; // для удобства
  DEBUGLN(str);
  // не наш пакет, выходим
  if (strncmp(str, MQTT_HEADER, hLen)) return;

  str += hLen;   // смещаемся для удобства чтения

  switch (getFromIndex(str, 0)) {
    case 0:   // heartbeat
      if (!IGNORE_PIR && getFromIndex(str, 1)) {
        pirTmr.restart();
      }
      break;

    case 1:   // управление
      data.power = getFromIndex(str, 1);
      data.color = getFromIndex(str, 2);
      if (getFromIndex(str, 3)) {
        winkTimes = 3;
      } 
      break;

    case 2:   // запрос
      sendPacket();
      break;
  }

  onlineTmr.restart();  // перезапуск таймера онлайна
}

// отправляем пакет
void sendPacket() {
  // GWL:1,power,color,wink
  String s;
  s.reserve(10);
  s += MQTT_HEADER "1,";  // +1,
  s += data.power;
  s += ',';
  s += data.color;
  s += ',';
  s += winkFlag;
  winkFlag = 0;
  // отправляем
  mqtt.publish(data.remote0, s.c_str());
  mqtt.publish(data.remote1, s.c_str());
}

void heartbeat() {
  if (hbTmr.period()) {
    // GWL:0,pir
    char str[hLen + 4] = MQTT_HEADER "0,\0";  // ставим один нуль-символ и еще один компилятор сам поставит
    str[hLen + 2] = '0' + pirFlag;
    pirFlag = 0;
    mqtt.publish(data.remote0, str);
    mqtt.publish(data.remote1, str);
  }
}

///////////////////////////////////////////////

// выводим эффект на ленту
void animation() {
  static Timer tmr(30);
  static bool breath;   // здесь отвечает за погашение яркости для дыхания
  static uint8_t breathDivider = 30;
  static uint8_t count; // счётчик-пропуск периодов
  static Timer overlayTimer(60);
  static uint8_t overlayCnt = 0;
  static uint8_t overlayValues[] = { 7, 25, 53, 89, 128, 167, 203, 231, 249, 255 };
  static CRGB prevNCol(0, 0, 0);

  if (tmr.period()) {
    // переключаем локальную яркость для "дыхания"
    
    if (!onlineTmr.elapsed()) {
        breathDivider = 30;
        if (!pirTmr.elapsed()) {
          breathDivider = 10;
        }
        ++count;

        if (count % breathDivider == 0) {
          breath = !breath;
          count = 0;
        }
    } else {
      breath = true;
    }  
  }
  uint8_t curBr = data.power ? (breath ? 255 : 230) : 0;
  if (isNight()) {
    curBr = map(curBr, 0, 255, 0, 60);
  }
  
  // здесь делаем плавные переходы между цветами
  CRGB ncol = CHSV(data.color, 255, curBr);
  CRGB ccol = leds[0];

  static Timer fadeTimer(5);

  // отладка скорости переходов
  /*static bool flag = true;
  static uint32_t timer1 = millis();
  if (ccol == ncol) {
    if (flag) {
      DEBUGLN(String("Fade cycle took: ") + (millis() - timer1));
      flag = false;
    }
    timer1 = millis();
  } else {
    flag = true;
  }*/

  if (fadeTimer.period() && (ccol != ncol)) {
    if (prevNCol != ncol) {
      overlayCnt = 0;
      prevNCol = ncol;
    }
    ccol = blend(ccol, ncol, overlayValues[overlayCnt >> 2]);
    ++overlayCnt;
  }

  // анимация подмигивания
  static int16_t brightness = data.bright;
  static uint8_t step = 10;
  static bool dir = false;
  static Timer winkTimer(4);
  if (winkTimes != 0) {
    if (winkTimer.period()) {

      brightness = dir ? brightness + step : brightness - step;
      if (brightness > 255) {
        --winkTimes;
        /// TODO: при неполной яркости скорее всего будет резкий переход в конце подмигивания
        brightness = 255;
        dir = !dir;
      } else {
        if (brightness < 0) {
          brightness = 0;
          dir = !dir;
        }
      }
    }
  } else {
    brightness = data.bright;
  }

  // выводим на ленту
  fill_solid(leds, LED_AMOUNT, ccol);
  FastLED.setBrightness(brightness);
  FastLED.show();
}

// локальный запуск портала. При любом исходе заканчивается ресетом платы
void localPortal(IPAddress ip) {
  // создаём точку с именем WLamp и предыдущим успешным IP
  DEBUGLN(F("Create AP"));
  WiFi.mode(WIFI_AP);
  String s(F("WLamp "));
  s += ip.toString();
  WiFi.softAP(s);

  portal.start(WIFI_AP);    // запускаем портал
  while (portal.tick()) {   // портал работает
    loadAnimation(CRGB::Blue);      // мигаем синим
    btn.tick();
    // если нажали сохранить настройки или кликнули по кнопке
    // перезагружаем ESP
    if (checkPortal() || btn.click()) ESP.reset();
  }
}


// анимация работы локал портала
void loadAnimation(CRGB color) {
  static int8_t dir = 1;
  static uint8_t val = 0;
  static Timer tmr(20);
  if (tmr.period()) {
    val += dir;
    if (val >= 100 || val <= 0) dir = -dir;
    fill_solid(leds, LED_AMOUNT, color);
    FastLED.setBrightness(val);
    FastLED.show();
  }
  yield();
}

// цыганский парсер инта из указанного индекса в строке
int getFromIndex(char* str, int idx, char div) {
  int val = 0;
  uint16_t i = 0;
  int count = 0;
  bool sign = 0;
  while (str[i]) {
    if (idx == count) {
      if (str[i] == div) break;
      if (str[i] == '-') sign = -1;
      else {
        val *= 10L;
        val += str[i] - '0';
      }
    } else if (str[i] == div) count++;
    i++;
  }
  return sign ? -val : val;
}

///////////////////////////////////////////////

void setup() {
  delay(1000);
  DEBUG_START  // запускаем сериал для отладки  
  DEBUGLN("Starting...");
  portal.attachBuild(webfaceBuilder);  // подключаем интерфейс

  EEPROM.begin(sizeof(data) + 1); // +1 на ключ
  memory.begin(0, 'a');           // запускаем менеджер памяти

  // я хз, хранить IPAddress в памяти приводит к exception
  // так что вытаскиваем в IPAddress
  IPAddress ip = IPAddress(data.ip[0], data.ip[1], data.ip[2], data.ip[3]);

  // запускаем ленту
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, LED_AMOUNT).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50);
  FastLED.show();

  // настраиваем уровень кнопки
  btn.setButtonLevel(BTN_LEVEL);

  // таймер на 2 секунды перед подключением,
  // чтобы юзер успел кликнуть если надо
  Timer tmr(2000);
  while (!tmr.period()) {
    loadAnimation(CRGB::Green);    // анимация подключения
    btn.tick();
    if (btn.click()) localPortal(ip); // клик - запускаем портал
    // дальше код не пойдёт, уйдем в перезагрузку
  }

  // юзер не кликнул, пытаемся подключиться к точке
  DEBUGLN("Connecting...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(data.ssid, data.pass);
  
  tmr.setPeriod(15000);
  tmr.restart();

  while (WiFi.status() != WL_CONNECTED) {
    loadAnimation(CRGB::Green);      // анимация подключения
    btn.tick();
    // если клик по кнопке или вышел таймаут
    if (btn.click() || tmr.period()) {
      WiFi.disconnect();  // отключаемся
      localPortal(ip);    // открываем портал
      // дальше код не пойдёт, уйдем в перезагрузку
    }
  }
  FastLED.clear();
  FastLED.show();

  DEBUG(F("Connected! IP: "));
  DEBUGLN(WiFi.localIP());

  // переписываем удачный IP себе в память
  if (ip != WiFi.localIP()) {
    ip = WiFi.localIP();
    for (int i = 0; i < 4; i++) data.ip[i] = ip[i];
    memory.update();
  }

  // настраиваем и запускаем NTP-клиент
  ntpTime.setPoolServerName(data.ntpUrl);
  ntpTime.setTimeOffset(data.ntpTimezone * 3600);
  ntpTime.setUpdateInterval(12 * 60 * 60 * 1000); // обновлять время раз в 12 часов
  ntpTime.begin();

  // стартуем вебсокет
  mqtt.setServer(data.host, data.port);
  mqtt.setCallback(callback);
  randomSeed(micros());

  // стартуем портал
  portal.start();

  FastLED.setBrightness(data.bright);
}

void loop() {
  // uint32_t loopStart = millis();

  if (USE_PIR && digitalRead(PIR_PIN)) {
    pirFlag = 1;  // опрос ИК датчика
  }

  static uint32_t logTimer = 0;
  if (logTimer < millis()) {
    logTimer = millis() + 1000;
    // DEBUGLN(String("Current time: ") + String(ntpTime.getFormattedTime()));
    // DEBUGLN(String("Current timezone: ") + String(data.ntpTimezone));
    // DEBUGLN(String("Current night mode state: ") + String(data.nightModeEn));
    // DEBUGLN(String("Current night start: ") + String(data.nightStart));
    // DEBUGLN(String("Current night end: ") + String(data.nightEnd));
    // DEBUGLN(String("Current NTP URL: ") + String(data.ntpUrl));
    // DEBUGLN(String("Current PIR State: ") + String(pirFlag));
    // DEBUGLN(String("Current isNight(): ") + String(isNight()));
    // DEBUGLN(String("Current ntpTime.getHours(): ") + String(ntpTime.getHours()));    
    // DEBUGLN(String("Current ssid: ") + String(data.ssid));    
    // DEBUGLN(String("Current password: ") + String(data.pass));    
    // DEBUGLN(String("Saved ssid: ") + WiFi.SSID()); 
    // DEBUGLN(String("Current color: ") + data.color);
    // DEBUGLN(String("My comrades are: "));
    // DEBUGLN(String(data.remote0));
    // DEBUGLN(String(data.remote1));
  }

  ntpTime.update();
  heartbeat();    // отправляем пакет что мы онлайн
  memory.tick();  // проверяем обновление настроек
  animation();    // эффект ленты
  buttonTick();   // действия кнопки
  mqttTick();     // проверяем входящие
  portal.tick();  // пинаем портал
  if (checkPortal()) { // проверяем действия
    DEBUGLN(String("Portal reports about changes!"));
    ntpTime.setPoolServerName(data.ntpUrl);
    ntpTime.setTimeOffset(data.ntpTimezone * 3600);
  }  
  
  // DEBUGLN(String("Loop-cycle execution took: ") + String(millis() - loopStart) + String(" msec."));
}
