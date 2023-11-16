/*
  xdsp_12_ST7789.ino - Display ST7789 support for Tasmota

  Copyright (C) 2021  Gerhard Mutz and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

//#ifdef USE_SPI
#ifdef USE_SPI
#ifdef USE_DISPLAY
#ifdef USE_DISPLAY_ST7789

#define XDSP_12                12
#define XI2C_38                38  // See I2CDEVICES.md

#undef COLORED
#define COLORED                1
#undef UNCOLORED
#define UNCOLORED              0

// touch panel controller
#undef FT5206_address
#define FT5206_address 0x38

// using font 8 is opional (num=3)
// very badly readable, but may be useful for graphs
#undef USE_TINY_FONT
#define USE_TINY_FONT


//#include <Arduino_ST7789.h>
#include <Adafruit_ST7789.h>
#include <FT5206.h>

// currently fixed
#define BACKPLANE_PIN 2
  #ifdef USE_LANBON_L8
  #undef BACKPLANE_PIN
  #define BACKPLANE_PIN 5
  #endif // USE_LANBON_L8

extern uint8_t color_type;
//Arduino_ST7789 *st7789;
Adafruit_ST7789 *st7789;

#ifdef USE_FT5206
uint8_t st7789_ctouch_counter = 0;
#endif // USE_FT5206
bool st7789_init_done = false;

/*********************************************************************************************/

void ST7789_InitDriver(void) {
  if (PinUsed(GPIO_ST7789_DC) &&  // This device does not need CS which breaks SPI bus usage
     ((TasmotaGlobal.soft_spi_enabled & SPI_MOSI) || (TasmotaGlobal.spi_enabled & SPI_MOSI))) {
      Settings->display_model = XDSP_12;

      if (!Settings->display_width) {
        Settings->display_width = 240;
      }
      if (!Settings->display_height) {
        Settings->display_height = 240;
      }

      pinMode(5, OUTPUT);
      digitalWrite(5, LOW); // 백라이트 활성화
      //pinMode(Pin(GPIO_BACKLIGHT), OUTPUT);
      //digitalWrite(Pin(GPIO_BACKLIGHT), LOW); // 백라이트 활성화

      if (TasmotaGlobal.soft_spi_enabled) {
        //st7789 = new Arduino_ST7789(Pin(GPIO_ST7789_DC), Pin(GPIO_OLED_RESET), Pin(GPIO_SSPI_MOSI), Pin(GPIO_SSPI_SCLK), Pin(GPIO_ST7789_CS), bppin);
        st7789 = new Adafruit_ST7789(Pin(GPIO_ST7789_CS), Pin(GPIO_ST7789_DC), Pin(GPIO_SSPI_MOSI), Pin(GPIO_SSPI_SCLK), Pin(GPIO_OLED_RESET));
      } else if (TasmotaGlobal.spi_enabled) {
        st7789 = new Adafruit_ST7789(Pin(GPIO_ST7789_CS), Pin(GPIO_ST7789_DC), Pin(GPIO_OLED_RESET));
      }
      st7789->init(Settings->display_width, Settings->display_height, SPI_MODE2); 
      //tft.setSPISpeed(40000000);
      st7789->setRotation(2); // rotates the screen

      st7789->fillScreen(ST77XX_BLACK); // fills the screen with black colour
      st7789->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
#if 0
      st7789->setTextColor(ST77XX_WHITE, ST77XX_BLACK);
      //st7789->setTextColor(ST77XX_GREEN); // text colour to white you can use hex codes like 0xDAB420 too
      st7789->setTextFont(2);
      //st7789->setTextWrap(true);
      //st7789->setCursor(10, 10); // starts to write text at y10 x10
      st7789->DrawStringAt(30, 30, "ST7789 TFT!", ST77XX_WHITE,0);
      //st7789->print("HELLO WORLD!");
#endif
      renderer = st7789;
      renderer->DisplayInit(DISPLAY_INIT_MODE, Settings->display_size, Settings->display_rotate, Settings->display_font);
      renderer->dim(GetDisplayDimmer16());
#if 0
#ifdef SHOW_SPLASH
    if (!Settings->flag5.display_no_splash) {
      // Welcome text
      renderer->setTextColor(ST7789_WHITE,ST7789_BLACK);
      renderer->setTextFont(2);
      renderer->DrawStringAt(30, (Settings->display_height-12)/2, "ST7789 TFT!", ST7789_WHITE,0);
      delay(1000);
    }
#endif
#endif
    color_type = COLOR_COLOR;

#ifdef ESP32
#ifdef USE_FT5206
    // start digitizer with fixed adress and pins for esp32
    #undef SDA_2
    #undef SCL_2
    #define SDA_2 23
    #define SCL_2 32
  #ifdef USE_LANBON_L8
    #undef SDA_2
    #undef SCL_2
    #define SDA_2 4
    #define SCL_2 0
  #endif // USE_LANBON_L8
    Wire1.begin(SDA_2, SCL_2, (uint32_t)400000);
    FT5206_Touch_Init(Wire1);
#endif // USE_FT5206
#endif // ESP32

    st7789_init_done = true;
    AddLog(LOG_LEVEL_INFO, PSTR("DSP: ST7789"));
  }
}

#ifdef ESP32
#ifdef USE_FT5206
#ifdef USE_TOUCH_BUTTONS

void ST7789_RotConvert(int16_t *x, int16_t *y) {
int16_t temp;
  if (renderer) {
    uint8_t rot=renderer->getRotation();
    switch (rot) {
      case 0:
        break;
      case 1:
        temp=*y;
        *y=renderer->height()-*x;
        *x=temp;
        break;
      case 2:
        *x=renderer->width()-*x;
        *y=renderer->height()-*y;
        break;
      case 3:
        temp=*y;
        *y=*x;
        *x=renderer->width()-temp;
        break;
    }
  }
}

// check digitizer hit
void ST7789_CheckTouch() {
st7789_ctouch_counter++;
  if (2 == st7789_ctouch_counter) {
    // every 100 ms should be enough
    st7789_ctouch_counter = 0;
    Touch_Check(ST7789_RotConvert);
  }
}
#endif // USE_TOUCH_BUTTONS
#endif // USE_FT5206
#endif // ESP32

void ST7789Time(void) {
  static char time[12];
  static char date[12];
  char line[12];
  static int16_t x, y, y2;
  static uint16_t w, h;

  renderer->setTextSize(3);

  snprintf_P(line, sizeof(line), PSTR("%04d" D_YEAR_MONTH_SEPARATOR "%02d" D_MONTH_DAY_SEPARATOR "%02d"), RtcTime.year, RtcTime.month, RtcTime.day_of_month);   // [2018-11-22]
  
  if(strcmp(date, line) != 0) {

    strcpy(date, line);
  
    if(x == 0 && y == 0) {
      renderer->getTextBounds(date, 5, 20, &x, &y, &w, &h);
      x = (240 - w) >> 1;
      y = 240 - (h * 3) >> 1;
      y2 = 240 - (y + (h >> 1));
    }

    renderer->setCursor(x, y);
    //renderer->writeFillRect(x, y, w, h, ST77XX_GREEN);
    renderer->println(date);
  }

  snprintf_P(line, sizeof(line), PSTR(" %02d" D_HOUR_MINUTE_SEPARATOR "%02d" D_MINUTE_SECOND_SEPARATOR "%02d"), RtcTime.hour, RtcTime.minute, RtcTime.second);  // [ 12:34:56 ]
  if(strcmp(time, line) != 0) {
    strcpy(time, line);

    renderer->setCursor(x, y2);
    //renderer->writeFillRect(x, y2, w, h, ST77XX_GREEN);
    renderer->println(time);
  }
  
  renderer->Updateframe();
}

void ST7789Refresh(void) {     // Every second
  switch (Settings->display_mode) {
    case 1:  // Time
      ST7789Time();
      break;
    case 2:  // Local
    case 4:  // Mqtt
      //SSD1331PrintLog(false);
      break;
    case 3:  // Local + Time
    case 5:  // Mqtt + Time
      //SSD1331PrintLog(true);
      break;
    default:;
  }
}


/*********************************************************************************************/
/*********************************************************************************************\
 * Interface
\*********************************************************************************************/
bool Xdsp12(uint32_t function)
{
  bool result = false;

//AddLog(LOG_LEVEL_INFO, PSTR("touch %d - %d"), FT5206_found, function);

  if (FUNC_DISPLAY_INIT_DRIVER == function) {
    ST7789_InitDriver();
  }
  else if (st7789_init_done && (XDSP_12 == Settings->display_model)) {
      switch (function) {
        case FUNC_DISPLAY_MODEL:
          result = true;
          break;
        case FUNC_DISPLAY_EVERY_SECOND:
          ST7789Refresh();
          break;
        case FUNC_DISPLAY_EVERY_50_MSECOND:
#ifdef USE_FT5206
#ifdef USE_TOUCH_BUTTONS
          if (FT5206_found) {
            ST7789_CheckTouch();
          }
#endif
#endif // USE_FT5206
          break;
    }
  }
  return result;
}

#endif  // USE_DISPLAY_ST7789
#endif  // USE_DISPLAY
#endif  // USE_SPI
