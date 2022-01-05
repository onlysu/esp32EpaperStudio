// Display Library for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// based on Demo Example from Good Display, available here: http://www.e-paper-display.com/download_detail/downloadsId=806.html
// Panel: GDEH0154D67 : http://www.e-paper-display.com/products_detail/productId=455.html
// Controller : SSD1681 : http://www.e-paper-display.com/download_detail/downloadsId=825.html
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

#include "GxEPD2_154_D67.h"

GxEPD2_154_D67::GxEPD2_154_D67(int8_t cs, int8_t dc, int8_t rst, int8_t busy) :
  GxEPD2_EPD(cs, dc, rst, busy, HIGH, 10000000, WIDTH, HEIGHT, panel, hasColor, hasPartialUpdate, hasFastPartialUpdate)
{
}

void GxEPD2_154_D67::clearScreen(uint8_t value)
{
  writeScreenBuffer(value);
  refresh(true);
  writeScreenBufferAgain(value);
}

void GxEPD2_154_D67::writeScreenBuffer(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  if (_initial_write) _writeScreenBuffer(0x26, value); // set previous
  _writeScreenBuffer(0x24, value); // set current
  _initial_write = false; // initial full screen buffer clean done
}

void GxEPD2_154_D67::writeScreenBufferAgain(uint8_t value)
{
  if (!_using_partial_mode) _Init_Part();
  _writeScreenBuffer(0x24, value); // set current
}

void GxEPD2_154_D67::_writeScreenBuffer(uint8_t command, uint8_t value)
{
  _writeCommand(command);
  for (uint32_t i = 0; i < uint32_t(WIDTH) * uint32_t(HEIGHT) / 8; i++)
  {
    _writeData(value);
  }
}

void GxEPD2_154_D67::writeImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::writeImageForFullRefresh(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
  _writeImage(0x26, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::writeImageAgain(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImage(0x24, bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::_writeImage(uint8_t command, const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  int16_t wb = (w + 7) / 8; // width bytes, bitmaps are padded
  x -= x % 8; // byte boundary
  w = wb * 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb, h of bitmap for index!
      int16_t idx = mirror_y ? j + dx / 8 + ((h - 1 - (i + dy))) * wb : j + dx / 8 + (i + dy) * wb;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _writeData(data);
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_154_D67::writeImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::writeImagePartAgain(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  _writeImagePart(0x24, bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::_writeImagePart(uint8_t command, const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                     int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (_initial_write) writeScreenBuffer(); // initial full screen buffer clean
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
  if ((w_bitmap < 0) || (h_bitmap < 0) || (w < 0) || (h < 0)) return;
  if ((x_part < 0) || (x_part >= w_bitmap)) return;
  if ((y_part < 0) || (y_part >= h_bitmap)) return;
  int16_t wb_bitmap = (w_bitmap + 7) / 8; // width bytes, bitmaps are padded
  x_part -= x_part % 8; // byte boundary
  w = w_bitmap - x_part < w ? w_bitmap - x_part : w; // limit
  h = h_bitmap - y_part < h ? h_bitmap - y_part : h; // limit
  x -= x % 8; // byte boundary
  w = 8 * ((w + 7) / 8); // byte boundary, bitmaps are padded
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  int16_t dx = x1 - x;
  int16_t dy = y1 - y;
  w1 -= dx;
  h1 -= dy;
  if ((w1 <= 0) || (h1 <= 0)) return;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _writeCommand(command);
  for (int16_t i = 0; i < h1; i++)
  {
    for (int16_t j = 0; j < w1 / 8; j++)
    {
      uint8_t data;
      // use wb_bitmap, h_bitmap of bitmap for index!
      int16_t idx = mirror_y ? x_part / 8 + j + dx / 8 + ((h_bitmap - 1 - (y_part + i + dy))) * wb_bitmap : x_part / 8 + j + dx / 8 + (y_part + i + dy) * wb_bitmap;
      if (pgm)
      {
#if defined(__AVR) || defined(ESP8266) || defined(ESP32)
        data = pgm_read_byte(&bitmap[idx]);
#else
        data = bitmap[idx];
#endif
      }
      else
      {
        data = bitmap[idx];
      }
      if (invert) data = ~data;
      _writeData(data);
    }
  }
  delay(1); // yield() to avoid WDT on ESP8266 and ESP32
}

void GxEPD2_154_D67::writeImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::writeImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                    int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    writeImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::writeNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    writeImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::drawImage(const uint8_t bitmap[], int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImage(bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImageAgain(bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::drawImagePart(const uint8_t bitmap[], int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  writeImagePart(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  refresh(x, y, w, h);
  writeImagePartAgain(bitmap, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
}

void GxEPD2_154_D67::drawImage(const uint8_t* black, const uint8_t* color, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImage(black, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::drawImagePart(const uint8_t* black, const uint8_t* color, int16_t x_part, int16_t y_part, int16_t w_bitmap, int16_t h_bitmap,
                                   int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (black)
  {
    drawImagePart(black, x_part, y_part, w_bitmap, h_bitmap, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::drawNative(const uint8_t* data1, const uint8_t* data2, int16_t x, int16_t y, int16_t w, int16_t h, bool invert, bool mirror_y, bool pgm)
{
  if (data1)
  {
    drawImage(data1, x, y, w, h, invert, mirror_y, pgm);
  }
}

void GxEPD2_154_D67::refresh(bool partial_update_mode)
{
  if (partial_update_mode) refresh(0, 0, WIDTH, HEIGHT);
  else
  {
    if (_using_partial_mode) _Init_Full();
    _Update_Full();
    _initial_refresh = false; // initial full update done
  }
}

void GxEPD2_154_D67::refresh(int16_t x, int16_t y, int16_t w, int16_t h)
{
  if (_initial_refresh) return refresh(false); // initial update needs be full update
  x -= x % 8; // byte boundary
  w -= x % 8; // byte boundary
  int16_t x1 = x < 0 ? 0 : x; // limit
  int16_t y1 = y < 0 ? 0 : y; // limit
  int16_t w1 = x + w < int16_t(WIDTH) ? w : int16_t(WIDTH) - x; // limit
  int16_t h1 = y + h < int16_t(HEIGHT) ? h : int16_t(HEIGHT) - y; // limit
  w1 -= x1 - x;
  h1 -= y1 - y;
  if (!_using_partial_mode) _Init_Part();
  _setPartialRamArea(x1, y1, w1, h1);
  _Update_Part();
}

void GxEPD2_154_D67::powerOff()
{
  _PowerOff();
}

void GxEPD2_154_D67::hibernate()
{
  _PowerOff();
  if (_rst >= 0)
  {
    _writeCommand(0x10); // deep sleep mode
    _writeData(0x1);     // enter deep sleep
    _hibernating = true;
  }
}

void GxEPD2_154_D67::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  _writeCommand(0x11); // set ram entry mode
  _writeData(0x03);    // x increase, y increase : normal mode
  _writeCommand(0x44);
  _writeData(x / 8);
  _writeData((x + w - 1) / 8);
  _writeCommand(0x45);
  _writeData(y % 256);
  _writeData(y / 256);
  _writeData((y + h - 1) % 256);
  _writeData((y + h - 1) / 256);
  _writeCommand(0x4e);
  _writeData(x / 8);
  _writeCommand(0x4f);
  _writeData(y % 256);
  _writeData(y / 256);
}

void GxEPD2_154_D67::_PowerOn()
{
  if (!_power_is_on)
  {
    _writeCommand(0x22);
    _writeData(0xf8);
    _writeCommand(0x20);
    _waitWhileBusy("_PowerOn", power_on_time);
  }
  _power_is_on = true;
}

void GxEPD2_154_D67::_PowerOff()
{
  if (_power_is_on)
  {
    _writeCommand(0x22);
    _writeData(0x83);
    _writeCommand(0x20);
    _waitWhileBusy("_PowerOff", power_off_time);
  }
  _power_is_on = false;
  _using_partial_mode = false;
}

void GxEPD2_154_D67::_InitDisplay()
{
  if (_hibernating) _reset();
  delay(10); // 10ms according to specs
  _writeCommand(0x12); // soft reset
  delay(10); // 10ms according to specs
  
  /*
  _writeCommand(0x01); // Driver output control
  _writeData(0xC7);
  _writeData(0x00);
  _writeData(0x00);
  _writeCommand(0x3C); // BorderWavefrom
  _writeData(0x05);
  _writeCommand(0x18); // Read built-in temperature sensor
  _writeData(0x80);
  _setPartialRamArea(0, 0, WIDTH, HEIGHT);
  
  */
	_writeCommand(0x74);//  # set analog block control
	_writeData(0x54);
	_writeCommand(0x7E); // # set digital block control
	_writeData(0x3B);
	
	_writeCommand(0x2B);  //# Reduce glitch under ACVCOM
	_writeData(0x04);
	_writeData(0x63);

	_writeCommand(0x0C);  //# Soft start setting
	_writeData(0x8B);
	_writeData(0x9C);
	_writeData(0x96);
	_writeData(0x0F);

	_writeCommand(0x01);  //# Set MUX as 300
	_writeData(0x2B);
	_writeData(0x01);
	_writeData(0x00);

	_writeCommand(0x11);  //# data entry mode
	_writeData(0x01);

	_writeCommand(0x44);  //# set Ram-X address start/end position
	_writeData(0x00);
	_writeData(0x31); // # RAM x address end at 31h(49+1)*8->400

	_writeCommand(0x45);//  # set Ram-Y address start/end position
	_writeData(0x2B);  //# RAM y address start at 12Bh
	_writeData(0x01);//
	_writeData(0x00);  //# RAM y address end at 00h
	_writeData(0x00);//

	_writeCommand(0x3C);  //# BorderWavefrom
	_writeData(0x01);  //# HIZ

	_writeCommand(0x18);//
	_writeData(0X80);//
	
	_writeCommand(0x22);
    _writeData(0XB1);  //Load Temperature and waveform setting.
    _writeCommand(0x20);
    _waitWhileBusy("_Update_Full", full_refresh_time); //waiting for the electronic paper IC to release the idle signal
    

    _writeCommand(0x4E); 
    _writeData(0x00);
    _writeCommand(0x4F); 
    _writeData(0x2B);
    _writeData(0x01);
	
  
}
const uint8_t GxEPD2_154_D67::LUT_DATA_part[] PROGMEM =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //L0 BB R0 B/W 0 
    0x82,0x00,0x00,0x00,0x00,0x00,0x00,  //L1 BW R0 B/W 1
    0x50,0x00,0x00,0x00,0x00,0x00,0x00,  //L2 WB R1 B/W 0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //L3 WW R0 W/W 0
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,  //L4 VCOM
    //b1w1 b2          w2
    0x08,0x08,0x00,0x08,0x01,
    0x00,0x00,0x00,0x00,0x01,
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,
};
const uint8_t GxEPD2_154_D67::lut_init_data[] PROGMEM = {  
0x08,
0x48,
0x40,
0x00,
0x00,
0x00,
0x00,
0x08,
0x48,
0x00,
0x00,
0x00,
0x00,
0x00,
0x48,
0x48,
0x40,
0x00,
0x00,
0x00,
0x00,
0x48,
0x48,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x0F,
0x01,
0x0F,
0x01,
0x00,
0x0F,
0x01,
0x0F,
0x01,
0x00,
0x0F,
0x01,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
0x00,
};
void GxEPD2_154_D67::_InitPartDisplay()
{
	if (_hibernating) _reset();                    // 电子纸控制器复位

	_waitWhileBusy("_Update_Full", full_refresh_time);  
    _writeCommand(0x12);     //SWRESET
    _waitWhileBusy("_Update_Full", full_refresh_time);  
	
	_writeCommand(0x74);       //
	_writeData(0x54);    //
	_writeCommand(0x7E);       //
	_writeData(0x3B);    //
	
	_writeCommand(0x01);       //
	_writeData(0x2B);    //
	_writeData(0x01);
	_writeData(0x00);    //

	_writeCommand(0x0C);       //
	_writeData(0x8B);    //
	_writeData(0x9C);    //
	_writeData(0xD6);    //
	_writeData(0x0F);    //

	_writeCommand(0x3A);       //
	_writeData(0x2C);    //
	_writeCommand(0x3B);       //
	_writeData(0x0A);    //
	_writeCommand(0x3C);       //
	_writeData(0x03);    //

	_writeCommand(0x11);       // data enter mode
	_writeData(0x01);    // 01 –Y decrement, X increment,
	_writeCommand(0x44);       // set RAM x address start/end
	_writeData(0x00);    // RAM x address start at 00h;
	_writeData(0x31);    // RAM x address end at 31h (49+1)*8->400
	_writeCommand(0x45);       // set RAM y address start/end
	_writeData(0x2B);    // RAM y address start at 012Bh+1=300;
	_writeData(0x01);    // 高位地址=01
	_writeData(0x00);    // RAM y address end at 00h;
	_writeData(0x00);    // 高位地址=0

	_writeCommand(0x2C);       //
	_writeData(0x6C);    //


	_writeCommand(0x37);       //
	_writeData(0x00);    //
	_writeData(0x00);    //
	_writeData(0x00);    //
	_writeData(0x00);    //
	_writeData(0x80);    //
	//WRITE_LUT();
	//_writeCommand(0x32);
  //_writeDataPGM(lut_init_data, sizeof(lut_init_data));
  
	_writeCommand(0x21);       //
	_writeData(0x40);    //
	_writeCommand(0x22);
	_writeData(0xC7);    //
	//DIS_IMG(PIC_WHITE);         //

	_writeCommand(0x21);       //
	_writeData(0x00);    //
	_writeCommand(0x3C);       //
	_writeData(0xC0);    //
}


void GxEPD2_154_D67::_Init_Full()
{
  _InitDisplay();
  //_writeCommand(0x32);
  //_writeDataPGM(lut_init_data, sizeof(lut_init_data));
  _PowerOn();
  _using_partial_mode = false;
}

void GxEPD2_154_D67::_Init_Part()
{
  _InitPartDisplay();
        _writeCommand(0x21); 
  _writeData(0x00);
  
  //_writeCommand(0x2C); //VCOM Voltage
  //_writeData(0x26);    // NA ??
  _writeCommand(0x32);
  _writeDataPGM(LUT_DATA_part, sizeof(LUT_DATA_part));
  _PowerOn();
  _using_partial_mode = true;
}

void GxEPD2_154_D67::_Update_Full()
{
  _writeCommand(0x22);
  //_writeData(0xf4);
  _writeData(0xc7);
  _writeCommand(0x20);
  _waitWhileBusy("_Update_Full", full_refresh_time);
}

void GxEPD2_154_D67::_Update_Part()
{
  _writeCommand(0x22);
  //_writeData(0xfc);
  _writeData(0xc7);
  _writeCommand(0x20);
  _waitWhileBusy("_Update_Part", partial_refresh_time);
}
