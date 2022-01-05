#include <Arduino.h>
#define ESP32
#include <string.h>
using namespace std;
#include <WiFi.h> //https://github.com/esp8266/Arduino
#include "NTPClient.h"
#include <DNSServer.h>
#include <WiFiMulti.h>
#include <WiFiClient.h>
//#include "WiFiManager.h" //https://github.com/tzapu/WiFiManager
#include <SPI.h>
#include <Time.h>
#include <Timezone.h>
#include "esp_bt.h"
#include "esp_wifi.h"
#if defined(ESP32)
#include "SPIFFS.h"
#endif

#include <FS.h>
#define FileClass fs::File

//#include <U8g2lib.h>
#include <WebServer.h> 
#include <ESPmDNS.h>
//#include "epd2in9.h"
//#include "EPD_2in9.h"
#include <stdlib.h>
#include <Button2.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP32AnalogRead.h>
#include <OneWire.h>
#include "Wire.h"
#include "SHT31.h"
#include <DallasTemperature.h>
#include "NTP.h"
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include "getData.h"
#include "bitmapData.h"
#include "config.h"
#include "nongli.h"
#include "SmartConfigManager.h"
#include "u8g2_simhei_16_gb2312.h"
#include "u8g2_AlibabaRegular_16_gb2312.h"
#include "u8g2_AlibabaLight_16_gb2312.h"
#include <ESP_WiFiManager.h>              //https://github.com/khoih-prog/ESP_WiFiManager
#include <ESP_WiFiManager-Impl.h>         //https://github.com/khoih-prog/ESP_WiFiManager
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#define ENABLE_GxEPD2_GFX 1

//################  VERSION  ##########################
String version = "3.1.0";     // Version of this program
//################ VARIABLES ###########################


//Epd epd;
// !!!!!!!!!!! /Arduino/libraries/U8g2/src/clib/u8g2.h 去掉 #define U8G2_16BIT 注释，让2.9寸墨水屏显示区域变大成整屏
//U8G2_IL3820_V2_296X128_1_4W_SW_SPI u8g2(U8G2_R0, /* clock=*/14, /* data=*/13, /* cs=*/15, /* dc=*/0, /* reset=*/16); // ePaper Display, lesser flickering and faster speed, enable 16 bit mode for this display!
//GxEPD2_BW<GxEPD2_213_B73, GxEPD2_213_B73::HEIGHT> display(GxEPD2_213_B73(/*CS=D8*/ 15, /*DC=D3*/ 0, /*RST=D4*/ 16, /*BUSY=D2*/ 17));
GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(GxEPD2_154_D67(/*CS=D8*/ 15, /*DC=D3*/ 0, /*RST=D4*/ 16, /*BUSY=D2*/ 17));

U8G2_FOR_ADAFRUIT_GFX u8g2;  // Select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
// 北京时间时区
#define STD_TIMEZONE_OFFSET +8
TimeChangeRule mySTD = {"", First, Sun, Jan, 0, STD_TIMEZONE_OFFSET * 60};
Timezone myTZ(mySTD, mySTD);
time_t previousMinute = 0;
int lastHour = 0;
ESP32AnalogRead adc;

WebServer esp8266_server(80);
enum alignment {LEFT, RIGHT, CENTER};
String todo = " 稀里糊涂 上了贼船";




////////////////////////////////////////////

#define BUTTON_PIN 0
Button2 button = Button2(BUTTON_PIN);

//屏幕局部更新次数
int updateTimes = 0;
//5分钟全局刷新屏幕一次
int clearDisMin = 5;

String cityCode = "101010900";
//北京丰台区 101010900
//北京房山区 101011200

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 12

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

// arrays to hold device address
DeviceAddress insideThermometer;

long SleepDuration = 1; // Sleep time in minutes, aligned to the nearest minute boundary, so if 30 will always update at 00 or 30 past the hour
int  WakeupTime    = 7;  // Don't wakeup until after 07:00 to save battery power
int  SleepTime     = 23; // Sleep after (23+1) 00:00 to save battery power

static const uint16_t input_buffer_pixels = 400; // may affect performance

//static const uint16_t max_row_width = 800;      // for up to 7.5" display 800x480,4.2 display 400X300
static const uint16_t max_row_width = 400;      // for up to 7.5" display 800x480,4.2 display 400X300
static const uint16_t max_palette_pixels = 256; // for depth <= 8

uint8_t input_buffer[3 * input_buffer_pixels];        // up to depth 24
uint8_t output_row_mono_buffer[max_row_width / 8];    // buffer for at least one row of b/w bits
uint8_t output_row_color_buffer[max_row_width / 8];   // buffer for at least one row of color bits
uint8_t mono_palette_buffer[max_palette_pixels / 8];  // palette buffer for depth <= 8 b/w
uint8_t color_palette_buffer[max_palette_pixels / 8]; // palette buffer for depth <= 8 c/w
uint16_t rgb_palette_buffer[max_palette_pixels];      // palette buffer for depth <= 8 for buffered graphics, needed for 7-color display

boolean LargeIcon = true, SmallIcon = false;
#define Large  11           // For icon drawing, needs to be odd number for best effect
#define Small  5            // For icon drawing, needs to be odd number for best effect
String  time_str, date_str; // strings to hold time and received weather data
int     wifi_signal, CurrentHour = 0, CurrentMin = 0, CurrentSec = 0;
long    StartTime = 0;
String ds18b20Temp = "0.0";

extern RTC_DATA_ATTR ActualWeather actual;  //创建结构体变量 目前的
extern RTC_DATA_ATTR FutureWeather future; //创建结构体变量 未来
extern RTC_DATA_ATTR LifeIndex life_index; //创建结构体变量 未来
extern RTC_DATA_ATTR Hitokoto yiyan; //创建结构体变量 一言
extern MyEEPROMStruct eeprom;
ChinaCalendar chinacalendar;

RTC_DATA_ATTR time_t RTCLocalTime = 0; //将变量存放于RTC Memory
RTC_DATA_ATTR uint16_t RTCSleepFlag = 0;// 0-power reset,1-59:60s timer wakeup, 180-3hours 重新获取wifi时间和信息,定义=getWifiInteval-1 是特定时间获取wifi数据
RTC_DATA_ATTR int oldHour = 0;
uint16_t getWifiInteval = 360; //分钟，180分钟=3小时更新一次，减少电池消耗
int updateHours[] = {0,6,9,11,13,15,18,21};//定时更新
/*
extern String weatherApiKey = "St5brIYqbP5B1Hfmr";
extern String weatherCity = "苏州";
*/
//函数声明
void handleCLEAR();
void clearDis();
void initNTP();
void webInit();
void myMDNSinit();
void returnRoot();
void wifiStatus(String myStatus, bool needClear);
void displayWetherDays(bool needClear);
void displayDaysInfo(bool needClear);
//void configModeCallback(WiFiManager *myWiFiManager);
void updateDisplay(String todo);
String changeWeek(int weekSum);
String getParam(String name);
void handler(Button2 &btn);
void getCityCode();
void getCityWeater();
void weaterData(String *cityDZ, String *dataSK, String *dataFC);
void printTemperature(DeviceAddress deviceAddress);
void printAddress(DeviceAddress deviceAddress);
void getTemp();
uint32_t read32(fs::File &f);
uint16_t read16(fs::File &f);
void print_wakeup_reason();
void drawBitmapFromSpiffs_Buffered(const char *filename, int16_t x, int16_t y, bool with_color, bool partial_update, bool overwrite);
void ShowWiFiSmartConfig();
void DrawBattery(int x, int y) ;
void DrawMultiLineString(string content, uint16_t x, uint16_t y, uint16_t contentAreaWidthWithMargin, uint16_t lineHeight);
void drawString(int x, int y, String text, alignment align);
void BeginSleep() ;
void display_tbpd(uint16_t circle_x);
void getSHT30Value();
void controlRTCAllWakeup(int currentHour);


//3天情况的位置
#define day_x0 54
#define day_x1 day_x0+34
#define day_x2 day_x1+43
#define day_x3 day_x2+51
#define day_y0 100
#define day_y1 day_y0+22
#define day_y2 day_y1+22
//小提示图标位置
#define cg_y0 14         //更新时间图标
#define cg_y1 cg_y0+19   //位置图标
#define cg_y2 cg_y1+19   //天气状态图标

#define cg_x0  1         // 左边小图标X位置
#define cg_x1  cg_x0+15  // 左边小图标后的文字的X位置

#define cg_x2  295-12    // 右边小图标X位置
//实况温度位置
uint16_t temp_x;
#define temp_y0 50


void setup()
{

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  // put your setup code here, to run once:
  //RTCSleepFlag = 0;
  Serial.begin(115200);
  print_wakeup_reason();

  if(RTCSleepFlag != 0)
  {
    //only update dispaly
    //display.init(); //for older Waveshare HAT's
    display.init(0, false, 2, false);
    Serial.println("InitialiseDisplay over");
    SPI.end();
    SPI.begin(14, 12, 13, 15);  
    u8g2.begin(display);
    u8g2.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
    u8g2.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
    updateDisplay(todo);
    //u8g2.setFontDirection(0);             // left to right (this is default)
  //u8g2.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  //u8g2.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
  
    return;
  }
  // Initialise SPIFFS
  if (!SPIFFS.begin())
  {
    Serial.println("SPIFFS initialisation failed!");
    while (1)
      yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS Initialisation done.");
  
   display.init(); //for older Waveshare HAT's
  Serial.println("InitialiseDisplay over");
  SPI.end();
  SPI.begin(14, 12, 13, 15);
  
  u8g2.begin(display);
  //u8g2.enableUTF8Print();
  //u8g2.begin(display); // connect u8g2 procedures to Adafruit GFX
  //u8g2.setFontMode(1);                  // use u8g2 transparent mode (this is default)
  u8g2.setFontDirection(0);             // left to right (this is default)
  u8g2.setForegroundColor(GxEPD_BLACK); // apply Adafruit GFX color
  u8g2.setBackgroundColor(GxEPD_WHITE); // apply Adafruit GFX color
  u8g2.setFont(u8g2_font_helvB10_tf);   // select u8g2 font from here: https://github.com/olikraus/u8g2/wiki/fntlistall
  display.fillScreen(GxEPD_WHITE);
  display.setFullWindow();
  //display.setPartialWindow(0, 0, display.width(), display.height());
  
  wifiStatus("WiFi连接中...", false);

#if 1
  SmartConfigManager scm;
  scm.initWiFi(ShowWiFiSmartConfig);
  if(!WiFi.isConnected())
  {
    display.fillScreen(GxEPD_WHITE);
    display.setFullWindow();
    delay(1000);
    wifiStatus("WiFi连接失败！休眠，请手动重启唤醒", true);
    delay(5000);
    display.powerOff();
    long SleepTimer = 100*60*60;//休眠100小时
    esp_sleep_enable_timer_wakeup((SleepTimer) * 1000000LL); // Added +20 seconnds to cover ESP32 RTC timer source inaccuracies
    Serial.println(" wifi not connectStarting deep-sleep period...");
    esp_deep_sleep_start();      // Sleep for e.g. 30 minutes
    return;
  }
#else
  WiFi.mode(WIFI_STA); //设置工作模式
  //delay(10);
  //WiFi.begin("BMT-WIFI","Bowing9180");  //要连接的WiFi名称和密码
  //if you get here you have connected to the WiFi
  WiFi.begin();
  Serial.println("connected...");
  uint8_t wifi_count = 0;
  uint8_t smartConnectFlag = 0;
  uint8_t retrynum = 7;

while (!WiFi.isConnected())
  {
    wifi_count++;
    ////display_jdthdy();    //显示进度条和电压
    delay(2000);
    if(wifi_count > retrynum)//20s首次检查网络
    {
      //wifiStatus("WiFi连接失败！！！", false);
      if(smartConnectFlag == 1)
      {
        wifiStatus("WiFi连接失败！休眠，请手动重启唤醒", false);
        delay(5000);
        
        display.powerOff();
        long SleepTimer = 100*60*60;
        esp_sleep_enable_timer_wakeup((SleepTimer) * 1000000LL); // Added +20 seconnds to cover ESP32 RTC timer source inaccuracies
        Serial.println("Starting deep-sleep period...");
        esp_deep_sleep_start();      // Sleep for e.g. 30 minutes
        return;
      }
      WiFi.disconnect();
      wifiStatus("请连接Epaper_AutoConnectAP进行网络配置", false);
      Serial.print(F("\nStarting AutoConnect_ESP32_minimal on "));
      ESP_WiFiManager ESP_wifiManager("Epaper_AutoConnectAP");
      ESP_wifiManager.autoConnect("Epaper_AutoConnectAP");
      
      delay(3000);
      wifi_count = 0;
      smartConnectFlag = 1;
      retrynum = 60;//2分钟网络配置时间
    }
    Serial.println("正在连接WIFI");
  }
#endif
  
  wifiStatus("WiFi连接成功！！！", false);
  
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  initNTP();

  //webInit();
  //myMDNSinit();
  GetData();
  //clearDis();
  //button.setClickHandler(handler);
  //button.setLongClickHandler(handler);
  //button.setDoubleClickHandler(handler);
  //button.setTripleClickHandler(handler);

  // // locate devices on the bus
  // Serial.print("Locating devices...");
  // sensors.begin();
  // Serial.print("Found ");
  // Serial.print(sensors.getDeviceCount(), DEC);
  // Serial.println(" devices.");

  // // report parasite power requirements
  // Serial.print("Parasite power is: ");
  // if (sensors.isParasitePowerMode())
  //   Serial.println("ON");
  // else
  //   Serial.println("OFF");
  // if (!sensors.getAddress(insideThermometer, 0))
  //   Serial.println("Unable to find address for Device 0");
  // // show the addresses we found on the bus
  // Serial.print("Device 0 Address: ");
  // printAddress(insideThermometer);
  // Serial.println();

  // // set the resolution to 9 bit (Each Dallas/Maxim device is capable of several different resolutions)
  // sensors.setResolution(insideThermometer, 9);

  // Serial.print("Device 0 Resolution: ");
  // Serial.print(sensors.getResolution(insideThermometer), DEC);
  // Serial.println();
}

void loop()
{
  // delay(5000);
  // getCityWeater();

  // getTemp();

  //button.loop();
  //MDNS.update();
  //MDNS.begin();
  
  //esp8266_server.handleClient(); // 处理http服务器访问

  //  Update the display only if time has changed
  if (timeStatus() != timeNotSet)
  {
    if (minute() != previousMinute)
    {
      previousMinute = minute();
      // Update the display
      Serial.println("-----------------");
      Serial.println("updateDisplay start...");
      updateDisplay(todo);
      Serial.println("updateDisplay end...");
    }
  }
  delay(1);
}

/*void configModeCallback(WiFiManager *myWiFiManager)
{
  wifiStatus("请检查WiFi连接后重启", false);
}
*/
void updateDisplay(String todo)
{

  TimeChangeRule *tcr; // Pointer to the time change rule
  time_t localTime;
  if(RTCSleepFlag == 0)
  {
    // Read the current UTC time from the NTP provider
    time_t utc = now();
  
    // Convert to local time taking DST into consideration
    localTime = myTZ.toLocal(utc, &tcr);
    RTCLocalTime = localTime;
    oldHour = hour(localTime);
  }
  else
  {
    RTCLocalTime = RTCLocalTime+60;
    localTime = RTCLocalTime;
  }
  
  //Serial.println("updateDisplay end...");
  // Map time to pixel pos78itions
  int weekdays = weekday(localTime);
  int days = day(localTime);
  int months = month(localTime);
  int years = year(localTime);
  int seconds = second(localTime);
  int minutes = minute(localTime);
  int hours = hour(localTime); //12 hour format use : hourFormat12(localTime)  isPM()/isAM()
  int thisHour = hours;

  controlRTCAllWakeup(hours);


  /////process time///////
  String h = "";
  String m = "";
  String s = "";
  if (hours < 10)
  {
    h = "0" + String(hours);
  }
  else
  {
    h = String(hours);
  }
  if (minutes < 10)
  {
    m = "0" + String(minutes);
  }
  else
  {
    m = String(minutes);
  }
  if (seconds < 10)
  {
    s = "0" + String(seconds);
  }
  else
  {
    s = String(seconds);
  }
  /////process date///////
  String mm = "";
  String dd = "";
  String w = dayStr(weekdays);

  if (months < 10)
  {
    mm = "0" + String(months);
  }
  else
  {
    mm = String(months);
  }

  if (days < 10)
  {
    dd = "0" + String(days);
  }
  else
  {
    dd = String(days);
  }
  String myTime = h + ":" + m;
  String myDate = String(years)+ "年"+ mm + "月";
  String myWeek = w.substring(0, 3);

  Serial.println("myTime: " + myTime);
  Serial.println("myDate: " + myDate);
  Serial.println("myWeek: " + myWeek);

  //////process display////////
  //display.powerOff();
  //Serial.println("u8g2.sleepOff().... ");
  display.setRotation(0);
  if((RTCSleepFlag == 0) || (RTCSleepFlag%10 == 0))
  {
    //clearDis();
    display.setFullWindow();//每隔一段时间进行全刷
    Serial.println("all display clear !!!");
  }
  else
  {
    display.setPartialWindow(50, 0, 270, 100);//局部刷新，减少耗电
  }
  
  
  display.firstPage();
  do
  {
    //u8g2_font_logisoso92_tn
    u8g2.setFont(u8g2_font_logisoso92_tn);
    char __myTime[sizeof(myTime)];
    myTime.toCharArray(__myTime, sizeof(__myTime));
    u8g2.drawStr(50, 95, __myTime);

    // u8g2.drawFrame(0, 96, 294, 30);
    // u8g2.drawLine(141, 96, 141, 126);
    // u8g2.drawLine(64, 96, 64, 126);
    // u8g2.drawLine(98, 96, 98, 126);

    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    //u8g2.setCursor(13, 120-2);//年月坐标
    //u8g2.print(myDate);

    //u8g2.setCursor(67+55, 117);
    //u8g2.print("周" + changeWeek(weekdays));

    //u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    //u8g2.setCursor(142+50, 117);
    //u8g2.print(todo);
    if((RTCSleepFlag == 0) || (RTCSleepFlag%10 == 0))
    {
      u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
      u8g2.setCursor(400-5-u8g2.getUTF8Width(yiyan.from_who), 293);
      u8g2.print(yiyan.from_who);
      
      u8g2.setFont(u8g2_AlibabaRegular_16_gb2312);
      //u8g2.setCursor(5, 255);
      //u8g2.print(yiyan.hitokoto);
      display.drawRoundRect(0,228,400,300-228,5,GxEPD_BLACK);
      DrawMultiLineString(yiyan.hitokoto,5,255,380,25);
      
      getSHT30Value();
      DrawBattery(0,15);
      displayWetherDays(false);
      displayDaysInfo(false);
    }
    
  } 
  while (display.nextPage());
  //display.display(1);
  //display.displayWindow(0,0,400,300);

    // //每隔1小时，刷新一次屏幕
  // if (lastHour != thisHour)
  // {
  //   clearDis();
  //   lastHour = thisHour;
  // }
  //每隔X分钟全局刷新屏幕
  /* updateTimes++;
  if (updateTimes == clearDisMin)
  {
    clearDis();
    Serial.println("all display clear !!!");
    updateTimes = 0;
  }*/

  //display.powerOff();
  //display.hibernate();
  //Serial.println("u8g2.sleepOff().... ");
  Serial.println("u8g2.sleepOn().... ");
  BeginSleep();
}

///显示左侧的日历，农历等

//3天情况的位置
#define day_x0 130 //今天
#define day_x1 day_x0+29 //日期
#define day_x2 day_x1+41 //星期
#define day_x3 day_x2+38
#define day_y0 183
#define day_y1 day_y0+18
#define day_y2 day_y1+18

void displayDaysInfo(bool needClear)
{
      //****** 显示未来天气 ******
    //提取月日字符串 格式02-02
    String day0, day1, day2;
    for (uint8_t i = 5; i < 10; i++)
    {
      day0 += future.date0[i];
      day1 += future.date1[i];
      day2 += future.date2[i];
    }
    //显示3天的日期 月日
    u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
    u8g2.setCursor(day_x0, day_y0);
    u8g2.print("今天");
    u8g2.setCursor(day_x0, day_y1);
    u8g2.print("明天");
    u8g2.setCursor(day_x0, day_y2);
    u8g2.print("后天");

    u8g2.setCursor(day_x1, day_y0);
    u8g2.print(day0);
    u8g2.setCursor(day_x1, day_y1);
    u8g2.print(day1);
    u8g2.setCursor(day_x1, day_y2);
    u8g2.print(day2);
    //显示星期几
    //提取年月日并转换成int
    String nian0, nian1, nian2, yue0, yue1, yue2, ri0, ri1, ri2;
    String chinaDate,chinaJieqi;
    uint16_t nian0_i, nian1_i, nian2_i;
    uint8_t yue0_i, yue1_i, yue2_i, ri0_i, ri1_i, ri2_i;
    for (uint8_t i = 0; i <= 9; i++)
    {
      if (i <= 3)
      {
        nian0 += future.date0[i];
        nian1 += future.date1[i];
        nian2 += future.date2[i];
      }
      else if (i == 5 || i == 6)
      {
        yue0 += future.date0[i];
        yue1 += future.date1[i];
        yue2 += future.date2[i];
      }
      else if (i == 8 || i == 9)
      {
        ri0 += future.date0[i];
        ri1 += future.date1[i];
        ri2 += future.date2[i];
      }
    }
    nian0_i = atoi(nian0.c_str()); yue0_i = atoi(yue0.c_str()); ri0_i = atoi(ri0.c_str());
    nian1_i = atoi(nian1.c_str()); yue1_i = atoi(yue1.c_str()); ri1_i = atoi(ri1.c_str());
    nian2_i = atoi(nian2.c_str()); yue2_i = atoi(yue2.c_str()); ri2_i = atoi(ri2.c_str());

    display.drawRoundRect(0,100,106,126,5,GxEPD_BLACK);
    display.drawRoundRect(108,100,400-108,126,5,GxEPD_BLACK);
    //u8g2.setForegroundColor(GxEPD_WHITE);
    u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
    u8g2.setCursor(5, 120);
    u8g2.print(future.date0);
    u8g2.setCursor(5+u8g2.getUTF8Width(future.date0)+2, 120);//星期坐标
    u8g2.print(week_calculate(nian0_i, yue0_i, ri0_i));
  // 计算出农历日期，并显示
    chinacalendar.GetChinaCalendarStr(nian0_i, yue0_i, ri0_i,&chinaDate);
    u8g2.setCursor(5, 220-2);//农历坐标
    u8g2.print(chinaDate);
    //u8g2.setForegroundColor(GxEPD_BLACK);
    u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
    chinacalendar.GetJieQiStr(nian0_i, yue0_i, ri0_i,&chinaJieqi);
    u8g2.setCursor((106-u8g2.getUTF8Width(chinaJieqi.c_str()))/2, 200-2);
    u8g2.print(chinaJieqi);
    
    u8g2.setFont(u8g2_font_fub49_tn);
    u8g2.setCursor(14, 180-2);//日期大字坐标
    u8g2.print(ri0.c_str());

    u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
    u8g2.setCursor(day_x2, day_y0);
    u8g2.print(week_calculate(nian0_i, yue0_i, ri0_i));
    u8g2.setCursor(day_x2, day_y1);
    u8g2.print(week_calculate(nian1_i, yue1_i, ri1_i));
    u8g2.setCursor(day_x2, day_y2);
    u8g2.print(week_calculate(nian2_i, yue2_i, ri2_i));

    //显示白天和晚上的天气现象
    uint16_t x_zl = 5; //高低温X位置增量
    String text_day0, text_night0, dn0_s;
    String text_day1, text_night1, dn1_s;
    String text_day2, text_night2, dn2_s;
    const char* dn0; const char* dn1; const char* dn2;

    u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
    if (strcmp(future.date0_text_day, future.date0_text_night) != 0) //今天
    {
      text_day0 = future.date0_text_day;
      text_night0 = future.date0_text_night;
      dn0_s = text_day0 + "转" + text_night0;
      dn0 = dn0_s.c_str();
      u8g2.setCursor(day_x3, day_y0);
      u8g2.print(dn0);
    }
    else
    {
      dn0 = future.date0_text_night;
      u8g2.setCursor(day_x3, day_y0);
      u8g2.print(dn0);
    }

    if (strcmp(future.date1_text_day, future.date1_text_night) != 0) //明天
    {
      text_day1 = future.date1_text_day;
      text_night1 = future.date1_text_night;
      dn1_s = text_day1 + "转" + text_night1;
      dn1 = dn1_s.c_str();
      u8g2.setCursor(day_x3, day_y1);
      u8g2.print(dn1);
    }
    else
    {
      dn1 = future.date1_text_night;
      u8g2.setCursor(day_x3, day_y1);
      u8g2.print(dn1);
    }

    if (strcmp(future.date2_text_day, future.date2_text_night) != 0) //后天
    {
      text_day2 = future.date2_text_day;
      text_night2 = future.date2_text_night;
      dn2_s = text_day2 + "转" + text_night2;
      dn2 = dn2_s.c_str();
      u8g2.setCursor(day_x3, day_y2);
      u8g2.print(dn2);
    }
    else
    {
      dn2 = future.date2_text_night;
      u8g2.setCursor(day_x3, day_y2);
      u8g2.print(dn2);
    }

    //显示高低温
    String  high0, high1, high2, low0, low1, low2, hl0_s, hl1_s, hl2_s;
    high0 = future.date0_high; high1 = future.date1_high; high2 = future.date2_high;
    low0 = future.date0_low; low1 = future.date1_low; low2 = future.date2_low;
    hl0_s = high0 +"°"+ "/" + low0+"°";
    hl1_s = high1 +"°"+ "/" + low1+"°";
    hl2_s = high2 +"°"+ "/" + low2+"°";
    const char* hl0 = hl0_s.c_str();
    const char* hl1 = hl1_s.c_str();
    const char* hl2 = hl2_s.c_str();
    u8g2.setCursor(day_x3 + u8g2.getUTF8Width("多云转小雨") + x_zl, day_y0);
    u8g2.print(hl0);
    u8g2.setCursor(day_x3 + u8g2.getUTF8Width("多云转小雨") + x_zl, day_y1);
    u8g2.print(hl1);
    u8g2.setCursor(day_x3 + u8g2.getUTF8Width("多云转小雨") + x_zl, day_y2);
    u8g2.print(hl2);

    
}

///显示右侧的天气情况，温湿度
void displayWetherDays(bool needClear)
{
    //圈圈位置
    uint16_t circle_x;
     //****** 显示实况温度和天气图标 ******
    //提取最后更新时间的 仅提取 小时:分钟
    String minutes_hours;
    for (uint8_t i = 11; i < 16; i++) minutes_hours += actual.last_update[i];

    const char* minutes_hours_c = minutes_hours.c_str();                         //String转换char
    uint16_t minutes_hours_length = u8g2.getUTF8Width(minutes_hours_c);     //获取最后更新时间的长度
    uint16_t city_length = u8g2.getUTF8Width(actual.city);                  //获取城市的字符长度
    uint16_t weather_name_length = u8g2.getUTF8Width(actual.weather_name);  //获取天气现象的字符长度
    uint16_t uvi_x = 399 - (strlen(life_index.uvi) * 26 / 6 + 3);                //UVI字符的位置

    //计算这三个数值谁最大
    int num[3] = {minutes_hours_length, city_length, weather_name_length};
    int len = sizeof(num) / sizeof(*num);
    int sy_length = uvi_x + (get_max_num(num, len) + cg_x1); //剩余长度

    u8g2.setFont(u8g2_font_fub30_tn); //实况温度用字体

    int temp_x_length = u8g2.getUTF8Width(actual.temp) + 20 + 58;
    temp_x = (sy_length / 2) - (temp_x_length / 2);
    u8g2.setCursor(temp_x+18, 142);  //显示实况温度
    u8g2.print(actual.temp);
    u8g2.setFont(u8g2_font_wqy16_t_gb2312a);
    u8g2.setCursor(temp_x+temp_x_length-58, 125);//摄氏度符号
    u8g2.print("°C");
    //画圆圈
    //circle_x = temp_x+30 + u8g2.getUTF8Width(actual.temp) + 8; //计算圈圈的位置
    //display.drawCircle(circle_x, 140 - 34, 3, 0);
    //display.drawCircle(circle_x, 140 - 34, 4, 0);
    //显示天气图标
    display_tbpd(115);


    //****** 显示小图标和详情信息 ******
    u8g2.setFont(u8g2_font_wqy14_t_gb2312a);
    //最后更新时间
    //display.drawInvertedBitmap(0, 20, Bitmap_gengxing, 13, 13, GxEPD_BLACK); //画最后更新时间小图标
    u8g2.setCursor(360, 220);//更新时间的坐标
    u8g2.print(minutes_hours);
    //城市名
    u8g2.setFont(u8g2_font_wqy16_t_gb2312a);
    //display.drawInvertedBitmap(395, 20, Bitmap_weizhi, 13, 13, GxEPD_BLACK); //画位置小图标
    u8g2.setCursor(400-5-u8g2.getUTF8Width(actual.city), 15);//城市名坐标
    u8g2.print(actual.city);
    //实况天气状态
    u8g2.setFont(u8g2_AlibabaRegular_16_gb2312);
    //display.drawInvertedBitmap(cg_x0, cg_y2 - 12, Bitmap_zhuangtai, 13, 13, GxEPD_BLACK); //画天气状态小图标
    u8g2.setCursor(220, 139);
    u8g2.print(actual.weather_name);
    
    u8g2.setFont(u8g2_font_wqy14_t_gb2312a);
    //紫外线指数
    //u8g2Fonts.setFont(u8g2_font_u8glib_4_tf);
    u8g2.setCursor(220+20+15+15, 140);
    u8g2.print("紫外线");
    //u8g2Fonts.setFont(u8g2_mfxuanren_16_gb2312);
    u8g2.setCursor(220+20+15+u8g2.getUTF8Width("紫外线")/2, 122);
    u8g2.print(life_index.uvi);
    //Serial.print("life_index.uvi长度："); Serial.println(strlen(life_index.uvi));
    //湿度
    //display.drawInvertedBitmap(cg_x2, cg_y1 - 12, Bitmap_humidity, 13, 13, GxEPD_BLACK);
    u8g2.setCursor(220+20+15+u8g2.getUTF8Width("紫外线")+30, 140);
    u8g2.print("湿度");
    u8g2.setCursor(220+20+15+u8g2.getUTF8Width("紫外线")+20+u8g2.getUTF8Width("湿度")/2, 122);
    u8g2.print(String(future.date0_humidity)+"%" );
    //u8g2.setCursor(temp_x+30+u8g2.getUTF8Width("湿度:88")+5, 215);
    //u8g2.print("%");
#if 1
//风力等级
    //display.drawInvertedBitmap(cg_x2, cg_y2 - 12, Bitmap_fx, 13, 13, GxEPD_BLACK);
    //u8g2.setCursor(cg_x2 - (strlen(future.date0_wind_scale) * 6 + 3), cg_y2);
    u8g2.setCursor(340+10+15, 140);
    u8g2.print("风力");
    u8g2.setCursor(340+15+15, 122);
    u8g2.print(String(future.date0_wind_scale)+"级");
#endif
}

void wifiStatus(String myStatus, bool needClear)
{
  display.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
    u8g2.setCursor(80, 70);
    u8g2.print(myStatus);
  } while (display.nextPage());

  if (needClear)
  {
    clearDis();
  }
}
String changeWeek(int weekSum)
{

  if (1 == weekSum)
  {
    return "日";
  }
  if (7 == weekSum)
  {
    return "六";
  }
  if (6 == weekSum)
  {
    return "五";
  }
  if (5 == weekSum)
  {
    return "四";
  }
  if (4 == weekSum)
  {
    return "三";
  }
  if (3 == weekSum)
  {
    return "二";
  }
  if (2 == weekSum)
  {
    return "一";
  }
}

void handleRoot()
{ //处理网站根目录“/”的访问请求
  String content = "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'></head><body><center><form action='TODO 'method='POST'><br>提醒事项：<input type='text'name='todo'value=''><br><br><br><input type='submit'value='更新提醒事项'></form><form action='CLEAR 'method='POST'><br><br><input type='submit'value='清屏'></form></center></body></html>";
  esp8266_server.send(200, "text/html", content); // NodeMCU将调用此函数。访问成功返回200状态码，返回信息类型text/html
}

// 设置处理404情况的函数'handleNotFound'
void handleNotFound()
{                                                           // 当浏览器请求的网络资源无法在服务器找到时，返回404状态码，和提升信息类型和内容
  esp8266_server.send(404, "text/plain", "404: Not found"); // NodeMCU将调用此函数。
}

void handleTODO()
{
  todo = getParam("todo");
  Serial.println("todo:" + todo);
  handleCLEAR();
}
void handleCLEAR()
{
  clearDis();
  returnRoot();
  updateDisplay(todo);
}
String getParam(String name)
{
  //read parameter from server, for customhmtl input
  String value;
  if (esp8266_server.hasArg(name))
  {
    value = esp8266_server.arg(name);
  }
  return value;
}

void webInit()
{

  //--------"启动网络服务功能"程序部分开始--------
  esp8266_server.begin();
  esp8266_server.on("/", HTTP_GET, handleRoot);        //访问网站的根目录 处理GET请求 执行handleRoot函数
  esp8266_server.on("/TODO", HTTP_POST, handleTODO);   //设置处理墨水屏提醒文字请求函数  处理POST请求
  esp8266_server.on("/CLEAR", HTTP_POST, handleCLEAR); //设置处理墨水屏清理请求函数  处理POST请求
  esp8266_server.onNotFound(handleNotFound);           //当请求服务器找不到时，执行handleNotFound函数
  //--------"启动网络服务功能"程序部分结束--------
  Serial.println("HTTP esp8266_server started"); //  告知用户ESP8266网络服务功能已经启动
}

void returnRoot()
{
  esp8266_server.sendHeader("Location", "/"); // 跳转回页面根目录
  esp8266_server.send(303);                   // 发送Http相应代码303 跳转
}
void clearDis()
{
  // epd.Init(lut_full_update);
  // EPD_2IN9_FULL			0
  // EPD_2IN9_PART			1
  //   EPD_2IN9_Init(EPD_2IN9_FULL);
  // EPD_2IN9_Init(EPD_2IN9_PART);
  // EPD_2IN9_Clear();

  //epd.ClearFrameMemory(0xFF); // bit set = white, bit reset = black
  //epd.DisplayFrame();
  //epd.ClearFrameMemory(0xFF); // bit set = white, bit reset = black
  //epd.DisplayFrame();
  display.setFullWindow();
  //display.fillScreen(GxEPD_WHITE);
  display.display();
  delay(500);

  // epd.ClearFrameMemory(0x00); // bit set = white, bit reset = black
  // epd.DisplayFrame();
  // epd.ClearFrameMemory(0x00); // bit set = white, bit reset = black
  // epd.DisplayFrame();

  // delay(500);

  // u8g2.begin();
  // u8g2.enableUTF8Print();
}
void myMDNSinit()
{
  // Set up mDNS responder:
  // - first argument is the domain name, in this example
  //   the fully-qualified domain name is "EPaperThing.local"
  // - second argument is the IP address to advertise
  //   we send our IP address on the WiFi network
  mdns_init();

  if (!MDNS.begin("EPaperThing"))
  {
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);
}
void handler(Button2 &btn)
{
  switch (btn.getClickType())
  {
  case SINGLE_CLICK:
    Serial.print("single ");
    clearDis();
    break;
  case DOUBLE_CLICK:
    Serial.print("double ");
    break;
  case TRIPLE_CLICK:
    Serial.print("triple ");
    break;
  case LONG_CLICK:
    Serial.print("long");
    break;
  }
  Serial.print("click");
  Serial.print(" (");
  Serial.print(btn.getNumberOfClicks());
  Serial.println(")");
}
// 发送HTTP请求并且将服务器响应通过串口输出
void getCityCode()
{
  String URL = "http://wgeo.weather.com.cn/ip/?_=" + String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;

  //配置请求地址。此处也可以不使用端口号和PATH而单纯的
  httpClient.begin(URL);

  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.print("Send GET request to URL: ");
  Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {
    String str = httpClient.getString();

    int aa = str.indexOf("id=");
    if (aa > -1)
    {
      //cityCode = str.substring(aa+4,aa+4+9).toInt();
      cityCode = str.substring(aa + 4, aa + 4 + 9);
      Serial.println(cityCode);
      getCityWeater();
    }
    else
    {
      Serial.println("获取城市代码失败");
    }
  }
  else
  {
    Serial.println("请求城市代码错误：");
    Serial.println(httpCode);
  }

  //关闭ESP8266与服务器连接
  httpClient.end();
}

// 获取城市天气
void getCityWeater()
{
  String URL = "http://d1.weather.com.cn/weather_index/" + cityCode + ".html?_=" + String(now());
  //创建 HTTPClient 对象
  HTTPClient httpClient;

  httpClient.begin(URL);

  //设置请求头中的User-Agent
  httpClient.setUserAgent("Mozilla/5.0 (iPhone; CPU iPhone OS 11_0 like Mac OS X) AppleWebKit/604.1.38 (KHTML, like Gecko) Version/11.0 Mobile/15A372 Safari/604.1");
  httpClient.addHeader("Referer", "http://www.weather.com.cn/");

  //启动连接并发送HTTP请求
  int httpCode = httpClient.GET();
  Serial.println("正在获取天气数据");
  Serial.println(URL);

  //如果服务器响应OK则从服务器获取响应体信息并通过串口输出
  if (httpCode == HTTP_CODE_OK)
  {

    String str = httpClient.getString();
    int indexStart = str.indexOf("weatherinfo\":");
    int indexEnd = str.indexOf("};var alarmDZ");

    String jsonCityDZ = str.substring(indexStart + 13, indexEnd);
    Serial.println(jsonCityDZ);

    indexStart = str.indexOf("dataSK =");
    indexEnd = str.indexOf(";var dataZS");
    String jsonDataSK = str.substring(indexStart + 8, indexEnd);
    Serial.println(jsonDataSK);

    indexStart = str.indexOf("\"f\":[");
    indexEnd = str.indexOf(",{\"fa");
    String jsonFC = str.substring(indexStart + 5, indexEnd);
    Serial.println(jsonFC);

    weaterData(&jsonCityDZ, &jsonDataSK, &jsonFC);
    Serial.println("获取成功");
  }
  else
  {
    Serial.println("请求城市天气错误：");
    Serial.print(httpCode);
  }

  //关闭ESP8266与服务器连接
  httpClient.end();
}
void weaterData(String *cityDZ, String *dataSK, String *dataFC)
{

  DynamicJsonDocument doc(512);
  deserializeJson(doc, *dataSK);
  JsonObject sk = doc.as<JsonObject>();

  String temp = sk["temp"].as<String>();
  Serial.println("温度：" + temp);

  //PM2.5空气指数
  String aqiTxt = "优";
  int pm25V = sk["aqi"];
  if (pm25V > 200)
  {
    aqiTxt = "重度";
  }
  else if (pm25V > 150)
  {
    aqiTxt = "中度";
  }
  else if (pm25V > 100)
  {
    aqiTxt = "轻度";
  }
  else if (pm25V > 50)
  {
    aqiTxt = "良";
  }
  Serial.println("PM2.5空气指数：" + aqiTxt);

  //实时天气
  String realWeather = sk["weather"].as<String>();
  Serial.println("实时天气：" + realWeather);

  deserializeJson(doc, *dataFC);
  JsonObject fc = doc.as<JsonObject>();
  String lowTemp = fc["fd"].as<String>();
  Serial.println("最低温度：" + lowTemp);
  String highTemp = fc["fc"].as<String>();
  Serial.println("最高温度：" + highTemp);
}
// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  // method 1 - slower
  //Serial.print("Temp C: ");
  //Serial.print(sensors.getTempC(deviceAddress));
  //Serial.print(" Temp F: ");
  //Serial.print(sensors.getTempF(deviceAddress)); // Makes a second call to getTempC and then converts to Fahrenheit

  // method 2 - faster
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC == DEVICE_DISCONNECTED_C)
  {
    Serial.println("Error: Could not read temperature data");
    return;
  }
  Serial.print("Temp C: ");
  ds18b20Temp = String(tempC);
  Serial.print(ds18b20Temp);
}
// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16)
      Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
void getTemp()
{
  // request to all devices on the bus
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");

  // It responds almost immediately. Let's print out the data
  printTemperature(insideThermometer); // Use a simple function to print out the data
}

void BeginSleep() 
{
  display.powerOff();
  //long SleepTimer = (SleepDuration * 60 - ((CurrentMin % SleepDuration) * 60 + CurrentSec)); //Some ESP32 are too fast to maintain accurate time
  long SleepTimer = SleepDuration *60;
  esp_sleep_enable_timer_wakeup((SleepTimer-2) * 1000000LL); // Added +20 seconnds to cover ESP32 RTC timer source inaccuracies
#ifdef BUILTIN_LED
  pinMode(BUILTIN_LED, INPUT); // If it's On, turn it off and some boards use GPIO-5 for SPI-SS, which remains low after screen use
  digitalWrite(BUILTIN_LED, HIGH);
#endif
  Serial.println("Entering " + String(SleepTimer) + "-secs of sleep time");
  Serial.println("Awake for : " + String((millis() - StartTime) / 1000.0, 3) + "-secs");
  Serial.println("Starting deep-sleep period...");
  esp_deep_sleep_start();      // Sleep for e.g. 30 minutes
}

void drawString(int x, int y, String text, alignment align) 
{
  int16_t  x1, y1; //the bounds of x,y and w and h of the variable 'text' in pixels.
  uint16_t w, h;
  display.setTextWrap(false);
  display.getTextBounds(text, x, y, &x1, &y1, &w, &h);
  if (align == RIGHT)  x = x - w;
  if (align == CENTER) x = x - w / 2;
  u8g2.setCursor(x, y + h);
  u8g2.print(text);
}
//using namespace std;
void DrawMultiLineString(string content, uint16_t x, uint16_t y, uint16_t contentAreaWidthWithMargin, uint16_t lineHeight)
{
  string ch;
  vector<string> contentArray;
  if(u8g2.getUTF8Width(content.c_str()) < contentAreaWidthWithMargin)
  {
    uint16_t content_len = u8g2.getUTF8Width(content.c_str());
    x = (400 - content_len)/2;
    y = y +7;
  }
  for (size_t i = 0, len = 0; i != content.length(); i += len)
  {
    unsigned char byte = (unsigned)content[i];
    if (byte >= 0xFC)
      len = 6;
    else if (byte >= 0xF8)
      len = 5;
    else if (byte >= 0xF0)
      len = 4;
    else if (byte >= 0xE0)
      len = 3;
    else if (byte >= 0xC0)
      len = 2;
    else
      len = 1;
    ch = content.substr(i, len);
    contentArray.push_back(ch);
  }

  string outputContent;
  for (size_t j = 0; j < contentArray.size(); j++)
  {
    outputContent += contentArray[j];
    int16_t wTemp = u8g2.getUTF8Width(outputContent.c_str());
    if (wTemp >= contentAreaWidthWithMargin || j == (contentArray.size() - 1))
    {
      u8g2.drawUTF8(x, y, outputContent.c_str());
      y += lineHeight;
      outputContent = "";
    }
  }
}

void getSHT30Value()
{
  SHT31 sht;
  
  //*tempV = 0;
  //*humV = 0;
  
  Wire.begin();
  sht.begin(SHT31_ADDRESS,IIC_SDA,IIC_SCL);
  Wire.setClock(100000);

  uint16_t stat = sht.readStatus();
  if(stat == 0xFFFF)
  {
    Serial.println("No HT Sensor");
    return;
  }
  Serial.print(stat, HEX);
  //Serial.println();
  sht.read();         // default = true/fast       slow = false
  //sht.getTemperature();
  //sht.getHumidity();

  u8g2.setFont(u8g2_font_wqy14_t_gb2312b);
  u8g2.setCursor(350, 50);
  //u8g2.print("室温: ");
  //u8g2.setCursor(340+u8g2.getUTF8Width("室温: "), 190);
  u8g2.print(String(sht.getTemperature(),1) + "°C");

  u8g2.setCursor(350, 80);
  //u8g2.print("湿度: ");
  //u8g2.setCursor(340+u8g2.getUTF8Width("湿度: "), 210);
  u8g2.print(String(sht.getHumidity(),1) + "%");
  
}
void DrawBattery(int x, int y) 
{
  uint8_t percentage = 100;
  
  pinMode(bat_switch_pin, OUTPUT);
  digitalWrite(bat_switch_pin, 1);
  
  //float voltage = analogRead(bat_vcc_pin) / 4096.0 * 7.46;
  /*
  float vcc_cache = 0.0;
  for (uint8_t i = 0; i < 20; i++)
  {
    delay(1);
    vcc_cache += analogRead(bat_vcc_pin) * 0.0009765625 * 5.537;
  }
  float voltage = vcc_cache / 20;
  */
  adc.attach(bat_vcc_pin);
  float voltage = adc.readMiliVolts()*5.7/1000;
  Serial.println("VoltageanalogRead = " + String(voltage));
  digitalWrite(bat_switch_pin, 0); //关闭电池测量
  pinMode(bat_switch_pin, INPUT);  //改为输入状态避免漏电
  if (voltage > 1 ) { // Only display if there is a valid reading
    //Serial.println("Voltage = " + String(voltage));
    percentage = 2836.9625 * pow(voltage, 4) - 43987.4889 * pow(voltage, 3) + 255233.8134 * pow(voltage, 2) - 656689.7123 * voltage + 632041.7303;
    if (voltage >= 4.20) percentage = 100;
    if (voltage <= 3.50) percentage = 0;
    display.drawRect(x + 15-10, y - 12, 19, 10, GxEPD_BLACK);
    display.fillRect(x + 34-10, y - 10, 2, 5, GxEPD_BLACK);
    display.fillRect(x + 17-10, y - 10, 15 * percentage / 100.0, 6, GxEPD_BLACK);
    //drawString(x + 65, y - 11, String(percentage) + "%", RIGHT);
    //drawString(x + 24+2, y - 12,  String(voltage, 2) + "v", CENTER);
    u8g2.setFont(u8g2_font_wqy12_t_gb2312b);
    u8g2.setCursor(x + 4, y+10);
    u8g2.print(String(voltage, 2) + "v");
  }
}

const uint16_t SMARTCONFIG_QR_CODE_WIDTH = 120;
const uint16_t SMARTCONFIG_QR_CODE_HEIGHT = 120;

void ShowWiFiSmartConfig()
{
  display.clearScreen(GxEPD_WHITE);

  const uint16_t x = (400 - SMARTCONFIG_QR_CODE_WIDTH) / 2;
  const uint16_t y = (300 - SMARTCONFIG_QR_CODE_HEIGHT) / 2;

  u8g2.setFont(u8g2_font_wqy16_t_gb2312b);
  uint16_t tipsY = y + SMARTCONFIG_QR_CODE_HEIGHT + 40;

  display.firstPage();
  do
  {
    DrawMultiLineString("请用微信扫描上方的二维码配置网络。", 5, tipsY, 290, 30);
  } while (display.nextPage());
  drawBitmapFromSpiffs_Buffered("smartconfig.bmp", x, y, false, true, false);
}


void drawBitmapFromSpiffs_Buffered(const char *filename, int16_t x, int16_t y, bool with_color, bool partial_update, bool overwrite)
{
  fs::File file;
  bool valid = false; // valid format to be handled
  bool flip = true;   // bitmap is stored bottom-to-top
  bool has_multicolors = display.epd2.panel == GxEPD2::ACeP565;
  uint32_t startTime = millis();
  if ((x >= 400) || (y >= 300))
    return;
  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');
#if defined(ESP32)
  file = SPIFFS.open(String("/") + filename, "r");
#else
  file = SPIFFS.open(filename, "r");
#endif
  if (!file)
  {
    Serial.print("File not found");
    return;
  }
  // Parse BMP header
  if (read16(file) == 0x4D42) // BMP signature
  {
    uint32_t fileSize = read32(file);
    uint32_t creatorBytes = read32(file);
    uint32_t imageOffset = read32(file); // Start of image data
    uint32_t headerSize = read32(file);
    uint32_t width = read32(file);
    uint32_t height = read32(file);
    uint16_t planes = read16(file);
    uint16_t depth = read16(file); // bits per pixel
    uint32_t format = read32(file);
    if ((planes == 1) && ((format == 0) || (format == 3))) // uncompressed is handled, 565 also
    {
      Serial.print("File size: ");
      Serial.println(fileSize);
      Serial.print("Image Offset: ");
      Serial.println(imageOffset);
      Serial.print("Header size: ");
      Serial.println(headerSize);
      Serial.print("Bit Depth: ");
      Serial.println(depth);
      Serial.print("Image size: ");
      Serial.print(width);
      Serial.print('x');
      Serial.println(height);
      // BMP rows are padded (if needed) to 4-byte boundary
      uint32_t rowSize = (width * depth / 8 + 3) & ~3;
      if (depth < 8)
        rowSize = ((width * depth + 8 - depth) / 8 + 3) & ~3;
      if (height < 0)
      {
        height = -height;
        flip = false;
      }
      uint16_t w = width;
      uint16_t h = height;
      if ((x + w - 1) >= 400)
        w = 400 - x;
      if ((y + h - 1) >= 300)
        h = 300 - y;
      //if (w <= max_row_width) // handle with direct drawing
      {
        valid = true;
        uint8_t bitmask = 0xFF;
        uint8_t bitshift = 8 - depth;
        uint16_t red, green, blue;
        bool whitish, colored;
        if (depth == 1)
          with_color = false;
        if (depth <= 8)
        {
          if (depth < 8)
            bitmask >>= depth;
          //file.seek(54); //palette is always @ 54
          file.seek(imageOffset - (4 << depth)); // 54 for regular, diff for colorsimportant
          for (uint16_t pn = 0; pn < (1 << depth); pn++)
          {
            blue = file.read();
            green = file.read();
            red = file.read();
            file.read();
            whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
            colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
            if (0 == pn % 8)
              mono_palette_buffer[pn / 8] = 0;
            mono_palette_buffer[pn / 8] |= whitish << pn % 8;
            if (0 == pn % 8)
              color_palette_buffer[pn / 8] = 0;
            color_palette_buffer[pn / 8] |= colored << pn % 8;
            rgb_palette_buffer[pn] = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
          }
        }
        if (partial_update)
          display.setPartialWindow(x, y, w, h);
        else
          display.setFullWindow();
        display.firstPage();
        do
        {
          if (!overwrite)
            display.fillScreen(GxEPD_WHITE);
          uint32_t rowPosition = flip ? imageOffset + (height - h) * rowSize : imageOffset;
          for (uint16_t row = 0; row < h; row++, rowPosition += rowSize) // for each line
          {
            uint32_t in_remain = rowSize;
            uint32_t in_idx = 0;
            uint32_t in_bytes = 0;
            uint8_t in_byte = 0; // for depth <= 8
            uint8_t in_bits = 0; // for depth <= 8
            uint16_t color = GxEPD_WHITE;
            file.seek(rowPosition);
            for (uint16_t col = 0; col < w; col++) // for each pixel
            {
              // Time to read more pixel data?
              if (in_idx >= in_bytes) // ok, exact match for 24bit also (size IS multiple of 3)
              {
                in_bytes = file.read(input_buffer, in_remain > sizeof(input_buffer) ? sizeof(input_buffer) : in_remain);
                in_remain -= in_bytes;
                in_idx = 0;
              }
              switch (depth)
              {
              case 24:
                blue = input_buffer[in_idx++];
                green = input_buffer[in_idx++];
                red = input_buffer[in_idx++];
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
                color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                break;
              case 16:
              {
                uint8_t lsb = input_buffer[in_idx++];
                uint8_t msb = input_buffer[in_idx++];
                if (format == 0) // 555
                {
                  blue = (lsb & 0x1F) << 3;
                  green = ((msb & 0x03) << 6) | ((lsb & 0xE0) >> 2);
                  red = (msb & 0x7C) << 1;
                  color = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
                }
                else // 565
                {
                  blue = (lsb & 0x1F) << 3;
                  green = ((msb & 0x07) << 5) | ((lsb & 0xE0) >> 3);
                  red = (msb & 0xF8);
                  color = (msb << 8) | lsb;
                }
                whitish = with_color ? ((red > 0x80) && (green > 0x80) && (blue > 0x80)) : ((red + green + blue) > 3 * 0x80); // whitish
                colored = (red > 0xF0) || ((green > 0xF0) && (blue > 0xF0));                                                  // reddish or yellowish?
              }
              break;
              case 1:
              case 4:
              case 8:
              {
                if (0 == in_bits)
                {
                  in_byte = input_buffer[in_idx++];
                  in_bits = 8;
                }
                uint16_t pn = (in_byte >> bitshift) & bitmask;
                whitish = mono_palette_buffer[pn / 8] & (0x1 << pn % 8);
                colored = color_palette_buffer[pn / 8] & (0x1 << pn % 8);
                in_byte <<= depth;
                in_bits -= depth;
                color = rgb_palette_buffer[pn];
              }
              break;
              }
              if (with_color && has_multicolors)
              {
                // keep color
              }
              else if (whitish)
              {
                color = GxEPD_WHITE;
              }
              else if (colored && with_color)
              {
                color = GxEPD_COLORED;
              }
              else
              {
                color = GxEPD_BLACK;
              }
              uint16_t yrow = y + (flip ? h - row - 1 : row);
              display.drawPixel(x + col, yrow, color);
            } // end pixel
          }   // end line
        } while (display.nextPage());
        Serial.print("loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      }
    }
  }
  file.close();
  if (!valid)
  {
    Serial.println("bitmap format not handled.");
  }
}

void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("Wakeup caused by external signal using RTC_IO");
    break;
  case ESP_SLEEP_WAKEUP_EXT1:
    Serial.println("Wakeup caused by external signal using RTC_CNTL");
    break;
  case ESP_SLEEP_WAKEUP_TIMER:    
    if(RTCSleepFlag < getWifiInteval)
    {
      RTCSleepFlag++;
    }
    else
    {
      RTCSleepFlag = 0;
    }
    //RTCSleepFlag++;
    Serial.println("Wakeup caused by timer,RTCSleepFlag="+String(RTCSleepFlag));
    break;
  case ESP_SLEEP_WAKEUP_TOUCHPAD:
    Serial.println("Wakeup caused by touchpad");
    break;
  case ESP_SLEEP_WAKEUP_ULP:
    Serial.println("Wakeup caused by ULP program");
    break;
  default:
    RTCSleepFlag = 0;
    Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
    break;
  }
}

uint16_t read16(fs::File &f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f)
{
  // BMP data is stored little-endian, same as Arduino.
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void display_tbpd(uint16_t circle_x) //天气图标显示
{
  uint16_t x = circle_x +5; //计算图标的位置
  uint16_t y = 120-16;
  String code;
  code = actual.weather_code;
  if (code == "0")      display.drawInvertedBitmap(x, y + 5, Bitmap_qt, 45, 45, GxEPD_BLACK);
  else if (code == "1") display.drawInvertedBitmap(x, y, Bitmap_qt_ws, 45, 45, GxEPD_BLACK);
  else if (code == "2") display.drawInvertedBitmap(x, y + 5, Bitmap_qt, 45, 45, GxEPD_BLACK);
  else if (code == "3") display.drawInvertedBitmap(x, y, Bitmap_qt_ws, 45, 45, GxEPD_BLACK);
  else if (code == "4") display.drawInvertedBitmap(x, y, Bitmap_dy, 45, 45, GxEPD_BLACK);
  else if (code == "5") display.drawInvertedBitmap(x, y, Bitmap_dy, 45, 45, GxEPD_BLACK);
  else if (code == "6") display.drawInvertedBitmap(x, y, Bitmap_dy_ws, 45, 45, GxEPD_BLACK);
  else if (code == "7") display.drawInvertedBitmap(x, y, Bitmap_dy, 45, 45, GxEPD_BLACK);
  else if (code == "8") display.drawInvertedBitmap(x, y, Bitmap_dy_ws, 45, 45, GxEPD_BLACK);
  else if (code == "9") display.drawInvertedBitmap(x, y + 3, Bitmap_yt, 45, 45, GxEPD_BLACK);
  else if (code == "10") display.drawInvertedBitmap(x, y, Bitmap_zheny, 45, 45, GxEPD_BLACK);
  else if (code == "11") display.drawInvertedBitmap(x, y, Bitmap_lzy, 45, 45, GxEPD_BLACK);
  else if (code == "12") display.drawInvertedBitmap(x, y, Bitmap_lzybbb, 45, 45, GxEPD_BLACK);
  else if (code == "13") display.drawInvertedBitmap(x, y, Bitmap_xy, 45, 45, GxEPD_BLACK);
  else if (code == "14") display.drawInvertedBitmap(x, y, Bitmap_zhongy, 45, 45, GxEPD_BLACK);
  else if (code == "15") display.drawInvertedBitmap(x, y, Bitmap_dayu, 45, 45, GxEPD_BLACK);
  else if (code == "16") display.drawInvertedBitmap(x, y, Bitmap_by, 45, 45, GxEPD_BLACK);
  else if (code == "17") display.drawInvertedBitmap(x, y, Bitmap_dby, 45, 45, GxEPD_BLACK);
  else if (code == "18") display.drawInvertedBitmap(x, y, Bitmap_tdby, 45, 45, GxEPD_BLACK);
  else if (code == "19") display.drawInvertedBitmap(x, y, Bitmap_dongy, 45, 45, GxEPD_BLACK);
  else if (code == "20") display.drawInvertedBitmap(x, y, Bitmap_yjx, 45, 45, GxEPD_BLACK);
  else if (code == "21") display.drawInvertedBitmap(x, y, Bitmap_zhenx, 45, 45, GxEPD_BLACK);
  else if (code == "22") display.drawInvertedBitmap(x, y, Bitmap_xx, 45, 45, GxEPD_BLACK);
  else if (code == "23") display.drawInvertedBitmap(x, y, Bitmap_zhongy, 45, 45, GxEPD_BLACK);
  else if (code == "24") display.drawInvertedBitmap(x, y, Bitmap_dx, 45, 45, GxEPD_BLACK);
  else if (code == "25") display.drawInvertedBitmap(x, y, Bitmap_bx, 45, 45, GxEPD_BLACK);
  else if (code == "26") display.drawInvertedBitmap(x, y, Bitmap_fc, 45, 45, GxEPD_BLACK);
  else if (code == "27") display.drawInvertedBitmap(x, y, Bitmap_ys, 45, 45, GxEPD_BLACK);
  else if (code == "28") display.drawInvertedBitmap(x, y, Bitmap_scb, 45, 45, GxEPD_BLACK);
  else if (code == "29") display.drawInvertedBitmap(x, y, Bitmap_scb, 45, 45, GxEPD_BLACK);
  else if (code == "30") display.drawInvertedBitmap(x, y + 5, Bitmap_w, 45, 45, GxEPD_BLACK);
  else if (code == "31") display.drawInvertedBitmap(x, y, Bitmap_m, 45, 45, GxEPD_BLACK);
  else if (code == "32") display.drawInvertedBitmap(x, y, Bitmap_f, 45, 45, GxEPD_BLACK);
  else if (code == "33") display.drawInvertedBitmap(x, y, Bitmap_f, 45, 45, GxEPD_BLACK);
  else if (code == "34") display.drawInvertedBitmap(x, y, Bitmap_jf, 45, 45, GxEPD_BLACK);
  else if (code == "35") display.drawInvertedBitmap(x, y, Bitmap_rdfb, 45, 45, GxEPD_BLACK);
  //else if (code == 37) display.drawInvertedBitmap(x,y, Bitmap_dy, 45, 45, GxEPD_BLACK);
  //else if (code == 38) display.drawInvertedBitmap(x,y, Bitmap_dy, 45, 45, GxEPD_BLACK);
  else if (code == "99") display.drawInvertedBitmap(x, y, Bitmap_wz, 45, 45, GxEPD_BLACK);
}

void controlRTCAllWakeup(int currentHour) //当前小时
{
  if(oldHour == currentHour)
  {
      return;
  }
  else
  {
    oldHour = currentHour;
    for(int index =0; index < sizeof(updateHours)/sizeof(int); index++)
    {
      if(currentHour == updateHours[index])
      {
        RTCSleepFlag = getWifiInteval;// if current time is update time, set RTCSleepFlag = getWifiInteval = 360;
      }
    }
    
  }
  
}

  
