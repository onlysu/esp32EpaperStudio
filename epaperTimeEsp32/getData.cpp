
#include "getData.h"
#include "config.h"
#include "toxicsoul.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>

RTC_DATA_ATTR ActualWeather actual;  //创建结构体变量 目前的
RTC_DATA_ATTR FutureWeather future; //创建结构体变量 未来
RTC_DATA_ATTR LifeIndex life_index; //创建结构体变量 未来
RTC_DATA_ATTR Hitokoto yiyan; //创建结构体变量 一言
MyEEPROMStruct eeprom;
String weatherApiKey = "St5brIYqbP5B1Hfmr";
String weatherCity = "苏州";


//https请求
String callHttps(String url)
{
  String payload;
  Serial.print("requesting URL: ");
  Serial.println(url);
  //std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
  //client->setFingerprint(fingerprint); //检验指纹，未成功
  //client->setInsecure(); //不检验
  HTTPClient https;
  if (https.begin(url))
  {
    int httpsCode = https.GET();
    if (httpsCode > 0)  //判断有无返回值
    {
      if (httpsCode == 200 || httpsCode == 304 || httpsCode == 403 || httpsCode == 404 || httpsCode ==   500) //判断请求是正确
      {
        payload = https.getString();
        Serial.println(payload);
        Serial.println(" ");
        //ParseHitokoto(payload, &yiyan);
        //Serial.print("句子："); Serial.println(yiyan.hitokoto); Serial.println(" ");
        return payload;
      }
      else
      {
        Serial.print("请求错误："); Serial.println(httpsCode); Serial.println(" ");
        char* httpsCode_c = new char[8];
        itoa(httpsCode, httpsCode_c, 10); //int转char*
        payload = "{\"status_code\":\"" + String("请求错误:") + String(httpsCode_c) + "\"}";
        return payload;
      }
    }
    else
    {
      Serial.println(" "); Serial.print("GET请求错误："); Serial.println(httpsCode);
      Serial.printf("[HTTPS] GET... 失败, 错误: %s\n", https.errorToString(httpsCode).c_str());
      payload = "{\"status_code\":\"" + String(https.errorToString(httpsCode).c_str()) + "\"}";
      //Serial.println(payload);
      return payload;
    }
  }
  else
  {
    Serial.printf("[HTTPS] 无法连接服务器\n");
    payload = "{\"status_code\":\"" + String("无法连接服务器") + "\"}";
    return payload;
  }
  https.end();
}

//****** 获取一言数据
void ParseHitokoto(String content, struct Hitokoto* data)
{
  DynamicJsonDocument json(1536); //分配内存,动态
  DeserializationError error = deserializeJson(json, content); //解析json
  //serializeJson(json, Serial);//构造序列化json,将内容从串口输出
  memset(data,sizeof(data),0);
  if (error)   //检查API是否有返回错误信息，有返回则进入休眠
  {
    Serial.print("一言加载json配置失败:");
    Serial.println(error.c_str());
    Serial.println(" ");
    String z = "一言json配置失败:" + String(error.c_str()) + " " + content;
    //display_partialLine(7, z);
    //esp_sleep(0);
  }
  if (json["status_code"].isNull() == 0) //检查为空
  {
    strcpy(data->status_code, json["status_code"]);
    String z = "一言异常:" + String(data->status_code);
    //display_partialLine(7, z);
    Serial.print("一言异常:"); Serial.println(data->status_code);
    strcpy(data->hitokoto, ToxicSoul[random(ToxicSoulCount)]);
    strcpy(data->from, "毒鸡汤");
    strcpy(data->from_who, "Onlyang");
    
  }
  else
  {
    if (json["hitokoto"].isNull() == 0) strcpy(data->hitokoto, json["hitokoto"]);
    else strcpy(data->hitokoto, ToxicSoul[random(ToxicSoulCount)]);

    if (json["from"].isNull() == 0) strcpy(data->from, json["from"]);
    else strcpy(data->from, "毒鸡汤");

    if (json["from_who"].isNull() == 0) strcpy(data->from_who, json["from_who"]);
    else strcpy(data->from_who, "Onlyang");
  }
  // 复制我们感兴趣的字符串 ,先检查是否为空，空会导致系统无限重启
  // 这不是强制复制，你可以使用指针，因为他们是指向"内容"缓冲区内
  // 所以你需要确保 当你读取字符串时它仍在内存中
  // isNull()检查是否为空 空返回1 非空0
}

//****** 天气数据获取
//使用Json解析天气数据，天气实况
bool ParseActualWeather(String content, struct ActualWeather* data)
{
  DynamicJsonDocument json(1536); //分配内存,动态
  DeserializationError error = deserializeJson(json, content); //解析json
  //serializeJson(json, Serial);//构造序列化json,将内容从串口输出
  if (error)
  {
    Serial.print("实况天气加载json配置失败:");
    Serial.println(error.c_str());
    Serial.println(" ");
    //String z = "实况天气json配置失败:" + String(error.c_str()) + " " + content;
    //display_partialLine(7, z);
    //esp_sleep(0);
    return false;
  }

  //检查API是否有返回错误信息，有返回则进入休眠
  if (json["status_code"].isNull() == 0) //检查到不为空
  {
    strcpy(data->status_code, json["status_code"]);
    String z;
    if (String(actual.status_code) == "AP010001") z = "API 请求参数错误" ;
    else if (String(actual.status_code) == "AP010002") z = "没有权限访问这个 API 接口" ;
    else if (String(actual.status_code) == "AP010003") z = "API 密钥 key 错误" ;
    else if (String(actual.status_code) == "AP010004") z = "签名错误" ;
    else if (String(actual.status_code) == "AP010005") z = "你请求的 API 不存在" ;
    else if (String(actual.status_code) == "AP010006") z = "没有权限访问这个地点: " + String(eeprom.city);
    else if (String(actual.status_code) == "AP010007") z = "JSONP 请求需要使用签名验证方式" ;
    else if (String(actual.status_code) == "AP010008") z = "没有绑定域名" ;
    else if (String(actual.status_code) == "AP010009") z = "API 请求的 user-agent 与你设置的不一致" ;
    else if (String(actual.status_code) == "AP010010") z = "没有这个地点" + String(eeprom.city);
    else if (String(actual.status_code) == "AP010011") z = "无法查找到指定 IP 地址对应的城市" ;
    else if (String(actual.status_code) == "AP010012") z = "你的服务已经过期" ;
    else if (String(actual.status_code) == "AP010013") z = "访问量余额不足" ;
    else if (String(actual.status_code) == "AP010014") z = "访问频率超过限制" ;
    else if (String(actual.status_code) == "AP010015") z = "暂不支持该城市的车辆限行信息" ;
    else if (String(actual.status_code) == "AP010016") z = "暂不支持该城市的潮汐数据" ;
    else if (String(actual.status_code) == "AP010017") z = "请求的坐标超出支持的范围" ;
    else if (String(actual.status_code) == "AP100001") z = "心知系统内部错误：数据缺失" ;
    else if (String(actual.status_code) == "AP100002") z = "心知系统内部错误：数据错误" ;
    else if (String(actual.status_code) == "AP100003") z = "心知系统内部错误：服务内部错误" ;
    else if (String(actual.status_code) == "AP100004") z = "心知系统内部错误：网关错误" ;
    else z = "实况天气异常:" + String(actual.status_code);
    //display_partialLine(7, z);
    Serial.print("实况天气异常:"); Serial.println(actual.status_code);
    //esp_sleep(60);
  }

  // 复制我们感兴趣的字符串 ,先检查是否为空，空会导致系统无限重启
  // isNull()检查是否为空 空返回1 非空0
  if (json["results"][0]["location"]["name"].isNull() == 0)
    strcpy(data->city, json["results"][0]["location"]["name"]);
  if (json["results"][0]["now"]["text"].isNull() == 0)
    strcpy(data->weather_name, json["results"][0]["now"]["text"]);
  if (json["results"][0]["now"]["code"].isNull() == 0)
    strcpy(data->weather_code, json["results"][0]["now"]["code"]);
  if (json["results"][0]["now"]["temperature"].isNull() == 0)
    strcpy(data->temp, json["results"][0]["now"]["temperature"]);
  if (json["results"][0]["last_update"].isNull() == 0)
    strcpy(data->last_update, json["results"][0]["last_update"]);
  // 这不是强制复制，你可以使用指针，因为他们是指向"内容"缓冲区内，所以你需要确保
  // 当你读取字符串时它仍在内存中
  return true;
}

//使用Json解析天气数据，今天和未来2天
bool ParseFutureWeather(String content, struct FutureWeather* data)
{
  DynamicJsonDocument json(2560); //分配内存,动态
  DeserializationError error = deserializeJson(json, content); //解析json
  // serializeJson(json, Serial);//构造序列化json,将内容从串口输出
  if (error)
  {
    Serial.print("未来天气json配置失败:");
    Serial.println(error.c_str());
    Serial.println(" ");
    //String z = "未来天气加载json配置失败:" + String(error.c_str()) + " " + content;
    //display_partialLine(7, z);
    //esp_sleep(0);
    return false;
  }

  //检查API是否有返回错误信息，有返回则进入休眠
  if (json["status_code"].isNull() == 0) //检查到不为空
  {
    strcpy(data->status_code, json["status_code"]);
    String z;
    if (String(future.status_code) == "AP010001") z = "API 请求参数错误" ;
    else if (String(future.status_code) == "AP010002") z = "没有权限访问这个 API 接口" ;
    else if (String(future.status_code) == "AP010003") z = "API 密钥 key 错误" ;
    else if (String(future.status_code) == "AP010004") z = "签名错误" ;
    else if (String(future.status_code) == "AP010005") z = "你请求的 API 不存在" ;
    else if (String(future.status_code) == "AP010006") z = "没有权限访问这个地点: " + String(eeprom.city);
    else if (String(future.status_code) == "AP010007") z = "JSONP 请求需要使用签名验证方式" ;
    else if (String(future.status_code) == "AP010008") z = "没有绑定域名" ;
    else if (String(future.status_code) == "AP010009") z = "API 请求的 user-agent 与你设置的不一致" ;
    else if (String(future.status_code) == "AP010010") z = "没有这个地点" + String(eeprom.city);
    else if (String(future.status_code) == "AP010011") z = "无法查找到指定 IP 地址对应的城市" ;
    else if (String(future.status_code) == "AP010012") z = "你的服务已经过期" ;
    else if (String(future.status_code) == "AP010013") z = "访问量余额不足" ;
    else if (String(future.status_code) == "AP010014") z = "访问频率超过限制" ;
    else if (String(future.status_code) == "AP010015") z = "暂不支持该城市的车辆限行信息" ;
    else if (String(future.status_code) == "AP010016") z = "暂不支持该城市的潮汐数据" ;
    else if (String(future.status_code) == "AP010017") z = "请求的坐标超出支持的范围" ;
    else if (String(future.status_code) == "AP100001") z = "心知系统内部错误：数据缺失" ;
    else if (String(future.status_code) == "AP100002") z = "心知系统内部错误：数据错误" ;
    else if (String(future.status_code) == "AP100003") z = "心知系统内部错误：服务内部错误" ;
    else if (String(future.status_code) == "AP100004") z = "心知系统内部错误：网关错误" ;
    else z = "未来天气异常:" + String(future.status_code);
    //display_partialLine(7, z);
    Serial.print("未来天气异常:"); Serial.println(future.status_code);
    //esp_sleep(60);
  }

  // 复制我们感兴趣的字符串，先检查是否为空，空会复制失败导致系统无限重启
  if (json["results"][0]["daily"][0]["date"].isNull() == 0)        //日期
    strcpy(data->date0, json["results"][0]["daily"][0]["date"]);
  if (json["results"][0]["daily"][1]["date"].isNull() == 0)
    strcpy(data->date1, json["results"][0]["daily"][1]["date"]);
  if (json["results"][0]["daily"][2]["date"].isNull() == 0)
    strcpy(data->date2, json["results"][0]["daily"][2]["date"]);

  if (json["results"][0]["daily"][0]["text_day"].isNull() == 0)    //白天天气现象
    strcpy(data->date0_text_day, json["results"][0]["daily"][0]["text_day"]);
  if (json["results"][0]["daily"][1]["text_day"].isNull() == 0)
    strcpy(data->date1_text_day, json["results"][0]["daily"][1]["text_day"]);
  if (json["results"][0]["daily"][2]["text_day"].isNull() == 0)
    strcpy(data->date2_text_day, json["results"][0]["daily"][2]["text_day"]);

  if (json["results"][0]["daily"][0]["text_night"].isNull() == 0)    //晚间天气现象
    strcpy(data->date0_text_night, json["results"][0]["daily"][0]["text_night"]);
  if (json["results"][0]["daily"][1]["text_night"].isNull() == 0)
    strcpy(data->date1_text_night, json["results"][0]["daily"][1]["text_night"]);
  if (json["results"][0]["daily"][2]["text_night"].isNull() == 0)
    strcpy(data->date2_text_night, json["results"][0]["daily"][2]["text_night"]);

  if (json["results"][0]["daily"][0]["high"].isNull() == 0)
    strcpy(data->date0_high, json["results"][0]["daily"][0]["high"]);  //最高温度
  if (json["results"][0]["daily"][1]["high"].isNull() == 0)
    strcpy(data->date1_high, json["results"][0]["daily"][1]["high"]);
  if (json["results"][0]["daily"][2]["high"].isNull() == 0)
    strcpy(data->date2_high, json["results"][0]["daily"][2]["high"]);

  if (json["results"][0]["daily"][0]["low"].isNull() == 0)             //最低温度
    strcpy(data->date0_low, json["results"][0]["daily"][0]["low"]);
  if (json["results"][0]["daily"][1]["low"].isNull() == 0)
    strcpy(data->date1_low, json["results"][0]["daily"][1]["low"]);
  if (json["results"][0]["daily"][2]["low"].isNull() == 0)
    strcpy(data->date2_low, json["results"][0]["daily"][2]["low"]);

  if (json["results"][0]["daily"][0]["humidity"].isNull() == 0)                //湿度
    strcpy(data->date0_humidity, json["results"][0]["daily"][0]["humidity"]);

  if (json["results"][0]["daily"][0]["wind_scale"].isNull() == 0)        //风力等级
    strcpy(data->date0_wind_scale, json["results"][0]["daily"][0]["wind_scale"]);
  // 这不是强制复制，你可以使用指针，因为他们是指向"内容"缓冲区内，所以你需要确保
  // 当你读取字符串时它仍在内存中
  return true;
}


//使用Json解析天气数据，天气实况
bool ParseLifeWeather(String content, struct LifeIndex* data)
{
  DynamicJsonDocument json(1536); //分配内存,动态
  DeserializationError error = deserializeJson(json, content); //解析json
  //serializeJson(json, Serial);//构造序列化json,将内容从串口输出
  if (error)
  {
    Serial.print("天气指数加载json配置失败:");
    Serial.println(error.c_str());
    Serial.println(" ");
    //String z = "天气指数json配置失败:" + String(error.c_str()) + " " + content;
    //display_partialLine(7, z);
    //esp_sleep(0);
    return false;
  }

  //检查API是否有返回错误信息，有返回则进入休眠
  if (json["status_code"].isNull() == 0) //检查到不为空
  {
    strcpy(data->status_code, json["status_code"]);
    String z;
    if (String(actual.status_code) == "AP010001") z = "API 请求参数错误" ;
    else if (String(actual.status_code) == "AP010002") z = "没有权限访问这个 API 接口" ;
    else if (String(actual.status_code) == "AP010003") z = "API 密钥 key 错误" ;
    else if (String(actual.status_code) == "AP010004") z = "签名错误" ;
    else if (String(actual.status_code) == "AP010005") z = "你请求的 API 不存在" ;
    else if (String(actual.status_code) == "AP010006") z = "没有权限访问这个地点: " + String(eeprom.city);
    else if (String(actual.status_code) == "AP010007") z = "JSONP 请求需要使用签名验证方式" ;
    else if (String(actual.status_code) == "AP010008") z = "没有绑定域名" ;
    else if (String(actual.status_code) == "AP010009") z = "API 请求的 user-agent 与你设置的不一致" ;
    else if (String(actual.status_code) == "AP010010") z = "没有这个地点" + String(eeprom.city);
    else if (String(actual.status_code) == "AP010011") z = "无法查找到指定 IP 地址对应的城市" ;
    else if (String(actual.status_code) == "AP010012") z = "你的服务已经过期" ;
    else if (String(actual.status_code) == "AP010013") z = "访问量余额不足" ;
    else if (String(actual.status_code) == "AP010014") z = "访问频率超过限制" ;
    else if (String(actual.status_code) == "AP010015") z = "暂不支持该城市的车辆限行信息" ;
    else if (String(actual.status_code) == "AP010016") z = "暂不支持该城市的潮汐数据" ;
    else if (String(actual.status_code) == "AP010017") z = "请求的坐标超出支持的范围" ;
    else if (String(actual.status_code) == "AP100001") z = "心知系统内部错误：数据缺失" ;
    else if (String(actual.status_code) == "AP100002") z = "心知系统内部错误：数据错误" ;
    else if (String(actual.status_code) == "AP100003") z = "心知系统内部错误：服务内部错误" ;
    else if (String(actual.status_code) == "AP100004") z = "心知系统内部错误：网关错误" ;
    else z = "天气指数异常:" + String(actual.status_code);
    //display_partialLine(7, z);
    Serial.print("天气指数异常:"); Serial.println(actual.status_code);
    //esp_sleep(60);
  }

  // 复制我们感兴趣的字符串 ,先检查是否为空，空会导致系统无限重启
  // isNull()检查是否为空 空返回1 非空0
  if (json["results"][0]["suggestion"]["uv"]["brief"].isNull() == 0)          //紫外线指数
    strcpy(data->uvi, json["results"][0]["suggestion"]["uv"]["brief"]);

  //if (json["results"][0]["now"]["text"].isNull() == 0)
  //strcpy(data->weather_name, json["results"][0]["now"]["text"]);

  // 这不是强制复制，你可以使用指针，因为他们是指向"内容"缓冲区内，所以你需要确保
  // 当你读取字符串时它仍在内存中
  return true;
}

void GetData()
{
  String language = "zh-Hans"; // 请求语言
  String url_yiyan = "https://v1.hitokoto.cn/?encode=json&min_length=1&max_length=21";//一言获取地址
  String url_ActualWeather;   //实况天气地址
  String url_FutureWeather;   //未来天气地址

  //"http://api.seniverse.com/v3/weather/now.json?key=S6pG_Q54kjfnBAi6i&location=深圳&language=zh-Hans&unit=c"
  //拼装实况天气API地址
  url_ActualWeather = "https://api.seniverse.com/v3/weather/now.json";
  url_ActualWeather += "?key=" + weatherApiKey;
  url_ActualWeather += "&location=" + weatherCity;
  url_ActualWeather += "&language=" + language;
  url_ActualWeather += "&unit=c";

  //https://api.seniverse.com/v3/weather/daily.json?key=S6pG_Q54kjfnBAi6i&location=深圳&language=zh-Hans&unit=c&start=0&days=3
  //拼装实况未来API地址
  url_FutureWeather = "https://api.seniverse.com/v3/weather/daily.json";
  url_FutureWeather += "?key=" + weatherApiKey;
  url_FutureWeather += "&location=" + weatherCity;
  url_FutureWeather += "&language=" + language;
  url_FutureWeather += "&unit=c";
  url_FutureWeather += "&start=0";
  url_FutureWeather += "&days=3";

  //https://api.seniverse.com/v3/life/suggestion.json?key=S6pG_Q54kjfnBAi6i&location=shanghai&language=zh-Hans
  //拼装生活指数
  String url_LifeIndex = "https://api.seniverse.com/v3/life/suggestion.json";
  url_LifeIndex += "?key=" + weatherApiKey;
  url_LifeIndex += "&location=" + weatherCity;
  
  //请求数据并Json处理
  //display_partialLine(7, "获取生活指数");
  ParseLifeWeather(callHttps(url_LifeIndex), &life_index); //获取生活指数
  
  //display_bitmap_bottom(Bitmap_wlq1, "获取实况天气数据中");
  ParseActualWeather(callHttps(url_ActualWeather), &actual);

  //display_bitmap_bottom(Bitmap_wlq2, "获取未来天气数据中");
  ParseFutureWeather(callHttps(url_FutureWeather), &future);

  //display_bitmap_bottom(Bitmap_wlq3, "获取一言数据中");
  ParseHitokoto(callHttps(url_yiyan), &yiyan);

  //display_bitmap_bottom(Bitmap_wlq4, "获取时间");
  //获取时间 
}

//日历数据计算公式  d为几号，m为月份，y为年份
String week_calculate(int y, int m, int d)
{
  if (m == 1 || m == 2)  //一二月换算
  {
    m += 12;
    y--;
  }
  int week = (d + 2 * m + 3 * (m + 1) / 5 + y + y / 4 - y / 100 + y / 400 + 1) % 7;
  if (week == 1) return"周一";
  else if (week == 2 ) return"周二";
  else if (week == 3 ) return"周三";
  else if (week == 4 ) return"周四";
  else if (week == 5 ) return"周五";
  else if (week == 6 ) return"周六";
  else if (week == 0 ) return"周日";
  else return "计算出错";
  //其中1~6表示周一到周六 0表示星期天
}

String monthString(char mon)
{
  if (mon < 1 || mon >12)  //一二月换算
  {
    return "Error";
  }

  if (mon == 1)         return" January ";
  else if (mon == 2 )   return" February";
  else if (mon == 3 )   return"  March ";
  else if (mon == 4 )   return"  April ";
  else if (mon == 5 )   return"   May  ";
  else if (mon == 6 )   return"   June  ";
  else if (mon == 7 )   return"   July  ";
  else if (mon == 8 )   return" August  ";
  else if (mon == 9 )   return"September";
  else if (mon == 10 )  return" October ";
  else if (mon == 11 )  return"November ";
  else if (mon == 12 )  return"December ";
  else return "Error";
}

//****** 冒泡算法从小到大排序 ******
int get_max_num(int arr[], int len)
{
  int i, j;

  for (i = 0; i < len - 1; i++)
  {
    for (j = 0; j < len - 1 - i; j++)
    {
      if (arr[j] > arr[j + 1])
      {
        swap(&arr[j], &arr[j + 1]);
      }
    }
  }
  return arr[len - 1]; //获取最大值
}

void swap(int *t1, int *t2)
{
  int temp;
  temp = *t1;
  *t1 = *t2;
  *t2 = temp;
}
//ToxicSoul[random(ToxicSoulCount)]
void ShowToxicSoul()
{
  u16_t r = random(ToxicSoulCount);
  const char *soul = ToxicSoul[r];
  //Serial.printf("pick %u: %s\n", r, soul);
  //u8g2Fonts.setFont(u8g2_simhei_16_gb2312);
  //DrawMultiLineString(string(soul), 5, 270, 280, 26);
}
