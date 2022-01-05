
#ifndef GETDATA_H
#define GETDATA_H
#include <stdint.h>
#include <string>
#include <Arduino.h>

//****** 天气数据
//我们要从此网页中提取的数据的类型
struct ActualWeather
{
  char status_code[64];  //错误代码
  char city[16];         //城市名称
  char weather_name[16]; //天气现象名称
  char weather_code[4];  //天气现象代码
  char temp[5];          //温度
  char last_update[25];  //最后更新时间
};


struct FutureWeather
{
  char status_code[64];       //错误代码

  char date0[14];             //今天日期
  char date0_text_day[20];    //白天天气现象名称
  char date0_code_day[4];     //白天天气现象代码
  char date0_text_night[16];  //晚上天气现象名称
  char date0_code_night[4];   //晚上天气现象代码
  char date0_high[5];         //最高温度
  char date0_low[5];          //最低温度
  char date0_humidity[5];     //相对湿度
  char date0_wind_scale[5];   //风力等级

  char date1[14];             //明天日期
  char date1_text_day[20];    //白天天气现象名称
  char date1_code_day[4];     //白天天气现象代码
  char date1_text_night[16];  //晚上天气现象名称
  char date1_code_night[4];   //晚上天气现象代码
  char date1_high[5];         //最高温度
  char date1_low[5];          //最低温度
  //char date1_humidity[5];     //相对湿度

  char date2[14];             //后天日期
  char date2_text_day[20];    //白天天气现象名称
  char date2_code_day[4];     //白天天气现象代码
  char date2_text_night[16];  //晚上天气现象名称
  char date2_code_night[4];   //晚上天气现象代码
  char date2_high[5];         //最高温度
  char date2_low[5];          //最低温度
  //char date2_humidity[5];     //相对湿度
};

struct LifeIndex //生活指数
{
  char status_code[64];  //错误代码
  char uvi[10];          //紫外线指数
}; 

struct Hitokoto  //一言API
{
  char status_code[64]; //错误代码
  char hitokoto[128];
  char from[32];
  char from_who[32];
  //https://v1.hitokoto.cn?encode=json&charset=utf-8&min_length=1&max_length=32
  //https://pa-1251215871.cos-website.ap-chengdu.myqcloud.com/
};


struct MyEEPROMStruct
{
  uint8_t auto_state;     //自动刷写eeprom状态 0-需要 1-不需要
  char  city[64];         //城市
  char  weatherKey[64];   //天气KEY
  uint8_t nightUpdata;    //夜间更新 1-更新 0-不更新
  char  inAWord[64];      //自定义一句话
  uint8_t screenType;     //屏幕类型
  uint8_t batDisplayType; //电量显示类型 0-电压 1-百分比
};


//////////////////////////////////////////////////////////////
////
/////////////////////////////////////////////////////////////

String callHttps(String url);
void ParseHitokoto(String content, struct Hitokoto* data);
bool ParseActualWeather(String content, struct ActualWeather* data);
bool ParseFutureWeather(String content, struct FutureWeather* data);
bool ParseLifeWeather(String content, struct LifeIndex* data);
void GetData();
//****** HTTP服务器配置 ******//

String week_calculate(int y, int m, int d);
String monthString(char mon);
int get_max_num(int arr[], int len);
void swap(int *t1, int *t2);







#endif
