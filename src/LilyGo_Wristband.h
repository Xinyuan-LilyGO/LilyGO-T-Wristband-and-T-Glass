/**
 * @file      LilyGo_Wristband.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-10-06
 *
 */
#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SensorPCF85063.hpp>
#include <SensorBHI260AP.hpp>
#include <esp_lcd_types.h>
#include "LilyGo_Display.h"
#include "LilyGo_Button.h"
#include <driver/i2s.h>

#if ARDUINO_USB_CDC_ON_BOOT != 1
#warning "If you need to monitor printed data, be sure to set USB_CDC_ON_BOOT to ENABLE, otherwise you will not see any data in the serial monitor"
#endif

#ifndef BOARD_HAS_PSRAM
#error "Detected that PSRAM is not turned on. Please set PSRAM to QSPI PSRAM in ArduinoIDE"
#endif //BOARD_HAS_PSRAM



#ifndef  SW_ROTATION
#define SW_ROTATION
#endif

#define BOARD_NONE_PIN      (-1)

#define BOARD_DISP_CS       (17)
#define BOARD_DISP_SCK      (15)
#define BOARD_DISP_MISO     (BOARD_NONE_PIN)
#define BOARD_DISP_MOSI     (16)
#define BOARD_DISP_DC       (21)
#define BOARD_DISP_RST      (18)

#define BOARD_I2C_SDA       (9)
#define BOARD_I2C_SCL       (8)

#define BOARD_BHI_IRQ       (37)
#define BOARD_BHI_CS        (36)
#define BOARD_BHI_SCK       (35)
#define BOARD_BHI_MISO      (34)
#define BOARD_BHI_MOSI      (33)
#define BOARD_BHI_RST       (47)
#define BOARD_BHI_EN        (48)

#define BOARD_RTC_IRQ       (7)

#define BOARD_TOUCH_BUTTON  (14)
#define BOARD_BOOT_PIN      (0)
#define BOARD_BAT_ADC       (13)
#define BOARD_VIBRATION_PIN (38)
#define DEFAULT_SCK_SPEED   (70 * 1000 * 1000)

#define BOARD_MIC_CLOCK     (6)
#define BOARD_MIC_DATA      (5)

// !The PDM microphone can only be up to 16KHZ and cannot be changed
#define MIC_I2S_SAMPLE_RATE         16000
// !The PDM microphone can only use I2S channel 0 and cannot be changed
#define MIC_I2S_PORT                I2S_NUM_0
#define MIC_I2S_BITS_PER_SAMPLE     I2S_BITS_PER_SAMPLE_16BIT


class LilyGo_Wristband :
    public LilyGo_Display,
    public SensorPCF85063,
    public SensorBHI260AP,
    public LilyGo_Button
{
public:
    LilyGo_Wristband();
    ~LilyGo_Wristband();

    bool begin();

    void update();

    void setTouchThreshold(uint32_t threshold);
    void detachTouch();
    bool getTouched();
    bool isPressed();

    void attachRTC(void (*rtc_alarm_cb)(void *arg), void *arg = NULL);

    void setBrightness(uint8_t level);
    uint8_t getBrightness();

    void setRotation(uint8_t rotation);
    uint8_t getRotation();
    void flipHorizontal(bool enable);

    // Direct writing without software rotation
    void setAddrWindow(uint16_t xs, uint16_t ys, uint16_t xe, uint16_t ye);
    void pushColors(uint16_t *data, uint32_t len);

    // Software rotation is possible
    void pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t hight, uint16_t *data);

    uint16_t  width();
    uint16_t  height();
    bool hasTouch();
    uint8_t getPoint(int16_t *x, int16_t *y, uint8_t get_point );

    uint16_t getBattVoltage();
    int getBatteryPercent();

    void vibration(uint8_t duty = 50, uint32_t delay_ms = 30);

    void enableTouchWakeup(int threshold = 2000);
    void sleep();
    void wakeup();
    bool needFullRefresh();

    bool initMicrophone();
    bool readMicrophone(void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait = portMAX_DELAY);

private:
    bool initBUS();
    void writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length);
    uint8_t _brightness;
    esp_lcd_panel_handle_t panel_handle ;
    int  threshold ;
};

#ifndef LilyGo_Class
#define LilyGo_Class LilyGo_Wristband
#endif
