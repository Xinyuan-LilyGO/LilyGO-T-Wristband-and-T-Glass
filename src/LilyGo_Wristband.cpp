/**
 * @file      LilyGo_Wristband.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-10-06
 *
 */
#include <sys/cdefs.h>
#include <esp_lcd_panel_interface.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_commands.h>
#include <esp_check.h>
#include <hal/spi_types.h>
#include <driver/spi_common.h>
#include <esp_adc_cal.h>
#include "LilyGo_Wristband.h"
#include "initSequence.h"

static volatile bool touchDetected;
static void touchISR()
{
    touchDetected = true;
}


__BEGIN_DECLS


#define BOARD_DISP_HOST     SPI3_HOST

#define TAG  "jd9613"

typedef struct {
    esp_lcd_panel_t base;
    esp_lcd_panel_io_handle_t io;
    int reset_gpio_num;
    bool reset_level;
    uint8_t rotation;
    uint16_t *frame_buffer;
    uint16_t width;
    uint16_t height;
    bool flipHorizontal;
} jd9613_panel_t;

static esp_err_t panel_jd9613_del(esp_lcd_panel_t *panel);
static esp_err_t panel_jd9613_reset(esp_lcd_panel_t *panel);
static esp_err_t panel_jd9613_init(esp_lcd_panel_t *panel);
static esp_err_t panel_jd9613_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data);
static esp_err_t panel_jd9613_set_rotation(esp_lcd_panel_t *panel, uint8_t r);


static esp_err_t esp_lcd_new_panel_jd9613(const esp_lcd_panel_io_handle_t io, const esp_lcd_panel_dev_config_t *panel_dev_config, esp_lcd_panel_handle_t *ret_panel)
{
    esp_err_t ret = ESP_OK;
    jd9613_panel_t *jd9613 = NULL;

    ESP_GOTO_ON_FALSE(io && panel_dev_config && ret_panel, ESP_ERR_INVALID_ARG, err, TAG, "invalid argument");

    jd9613 = (jd9613_panel_t *)calloc(1, sizeof(jd9613_panel_t));

    ESP_GOTO_ON_FALSE(jd9613, ESP_ERR_NO_MEM, err, TAG, "no mem for jd9613 panel");


    jd9613->frame_buffer = (uint16_t *)heap_caps_malloc(JD9613_WIDTH * JD9613_HEIGHT * 2, MALLOC_CAP_DMA);
    if (!jd9613->frame_buffer) {
        free(jd9613);
        return ESP_FAIL;
    }

    if (panel_dev_config->reset_gpio_num >= 0) {
        pinMode(panel_dev_config->reset_gpio_num, OUTPUT);
    }

    switch (panel_dev_config->bits_per_pixel) {
    case 16: // RGB565
        // fb_bits_per_pixel = 16;
        break;
    default:
        ESP_GOTO_ON_FALSE(false, ESP_ERR_NOT_SUPPORTED, err, TAG, "unsupported pixel width");
        break;
    }

    jd9613->io = io;
    // jd9613->fb_bits_per_pixel = fb_bits_per_pixel;
    jd9613->reset_gpio_num = panel_dev_config->reset_gpio_num;
    jd9613->reset_level = panel_dev_config->flags.reset_active_high;
    jd9613->base.del = panel_jd9613_del;
    jd9613->base.reset = panel_jd9613_reset;
    jd9613->base.init = panel_jd9613_init;
    jd9613->base.draw_bitmap = panel_jd9613_draw_bitmap;
    jd9613->base.invert_color = NULL;
    jd9613->base.set_gap = NULL;
    jd9613->base.mirror = NULL;
    jd9613->base.swap_xy = NULL;
// #if ESP_IDF_VERSION <  ESP_IDF_VERSION_VAL(4,4,6)
//     jd9613->base.disp_off = NULL;
// #else
//     jd9613->base.disp_on_off = NULL;
// #endif

    *ret_panel = &(jd9613->base);
    log_d("new jd9613 panel @%p", jd9613);

    return ESP_OK;

err:
    if (jd9613) {
        if (panel_dev_config->reset_gpio_num >= 0) {
            pinMode(panel_dev_config->reset_gpio_num, OPEN_DRAIN);
        }
        free(jd9613);
    }
    return ret;
}


static esp_err_t panel_jd9613_del(esp_lcd_panel_t *panel)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);

    if (jd9613->reset_gpio_num >= 0) {
        pinMode(jd9613->reset_gpio_num, OPEN_DRAIN);
    }
    log_d("del jd9613 panel @%p", jd9613);
    free(jd9613);
    free(jd9613->frame_buffer);
    return ESP_OK;
}

static esp_err_t panel_jd9613_reset(esp_lcd_panel_t *panel)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;

    // perform hardware reset
    if (jd9613->reset_gpio_num >= 0) {
        digitalWrite(jd9613->reset_gpio_num, jd9613->reset_level);
        delay((100));
        digitalWrite(jd9613->reset_gpio_num, !jd9613->reset_level);
        delay((100));
    } else {
        // perform software reset
        esp_lcd_panel_io_tx_param(io, LCD_CMD_SWRESET, NULL, 0);
        delay((20)); // spec, wait at least 5ms before sending new command
    }

    return ESP_OK;
}


static esp_err_t panel_jd9613_init(esp_lcd_panel_t *panel)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;

    // vendor specific initialization, it can be different between manufacturers
    // should consult the LCD supplier for initialization sequence code
    int cmd = 0;
    while (jd9613_cmd[cmd].len != 0xff) {
        esp_lcd_panel_io_tx_param(io, jd9613_cmd[cmd].addr, jd9613_cmd[cmd].param, (jd9613_cmd[cmd].len - 1) & 0x1F);
        cmd++;
    }

    // jd9613->flipHorizontal = 1;
    jd9613->flipHorizontal = 0;
    jd9613->rotation = 1;

    panel_jd9613_set_rotation(panel, jd9613->rotation);

    esp_lcd_panel_io_tx_param(io, LCD_CMD_SLPOUT, NULL, 0);
    delay((120));

    esp_lcd_panel_io_tx_param(io, LCD_CMD_DISPON, NULL, 0);
    delay((120));

    return ESP_OK;
}

static esp_err_t panel_jd9613_draw_bitmap(esp_lcd_panel_t *panel, int x_start, int y_start, int x_end, int y_end, const void *color_data)
{
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    assert((x_start < x_end) && (y_start < y_end) && "start position must be smaller than end position");
    esp_lcd_panel_io_handle_t io = jd9613->io;

    uint32_t width = x_start + x_end;
    uint32_t height = y_start + y_end;
    uint32_t _x = x_start,
             _y = y_start,
             _xe = width,
             _ye = height;
    size_t write_colors_bytes = width * height * sizeof(uint16_t);
    uint16_t *data_ptr = (uint16_t *)color_data;

    // log_i("%s W:%lu H:%lu B:%lu\n", __func__, width, height, write_colors_bytes);

#ifdef SW_ROTATION
    bool sw_rotation = false;
    switch (jd9613->rotation) {
    case 1:
    case 3:
        sw_rotation = true;
        break;
    default:
        sw_rotation = false;
        break;
    }

    if (sw_rotation) {
        _x = JD9613_WIDTH - (y_start + height);
        _y = x_start;
        _xe = height;
        _ye = width;
    }
#endif

    // Direction 2 requires offset pixels
    if (jd9613->rotation == 2) {
        _x += 2;
        _xe += 2;
    }

    uint8_t data1[] = {lowByte(_x >> 8), lowByte(_x), lowByte((_xe - 1) >> 8), lowByte(_xe - 1)};
    esp_lcd_panel_io_tx_param(io, LCD_CMD_CASET, data1, 4);
    uint8_t data2[] = {lowByte(_y >> 8), lowByte(_y), lowByte((_ye - 1) >> 8), lowByte(_ye - 1)};
    esp_lcd_panel_io_tx_param(io, LCD_CMD_RASET, data2, 4);

#ifdef SW_ROTATION
    if (sw_rotation) {
        int index = 0;
        uint16_t *pdat = (uint16_t *)color_data;
        for (uint16_t j = 0; j < width; j++) {
            for (uint16_t i = 0; i < height; i++) {
                jd9613->frame_buffer[index++] = pdat[width * (height - i - 1) + j];
            }
        }
        data_ptr = jd9613->frame_buffer;
    }
#endif
    esp_lcd_panel_io_tx_color(io, LCD_CMD_RAMWR, data_ptr, write_colors_bytes);
    return ESP_OK;
}

#define  LCD_CMD_RGB 0x00
//There is only 1/2 RAM inside the JD9613 screen, and it cannot be rotated in directions 1 and 3.
static esp_err_t panel_jd9613_set_rotation(esp_lcd_panel_t *panel, uint8_t r)
{
// set_rotation:1 write reg :0x36 , data : 0x60 Width:294 Height:126
// panel_jd9613_draw_bitmap W:294 H:126 B:74088

// set_rotation:2 write reg :0x36 , data : 0xC0 Width:126 Height:294
// panel_jd9613_draw_bitmap W:126 H:294 B:74088

// set_rotation:3 write reg :0x36 , data : 0xA0 Width:294 Height:126
// panel_jd9613_draw_bitmap W:294 H:126 B:74088

// set_rotation:0 write reg :0x36 , data : 0x0 Width:126 Height:294
// panel_jd9613_draw_bitmap W:126 H:294 B:74088

#ifdef SW_ROTATION
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;
    uint8_t write_data = 0x00;
    switch (r) {
    case 1: // jd9613 only has 1/2RAM and cannot be rotated
        // write_data = LCD_CMD_MX_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        // jd9613->width = JD9613_HEIGHT;
        // jd9613->height = JD9613_WIDTH;

        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    case 2:
        write_data = LCD_CMD_MY_BIT | LCD_CMD_MX_BIT | LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    case 3: // jd9613 only has 1/2RAM and cannot be rotated
        // write_data = LCD_CMD_MY_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        // jd9613->width = JD9613_HEIGHT;
        // jd9613->height = JD9613_WIDTH;

        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    default: // case 0:
        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    }

    if (jd9613->flipHorizontal) {
        write_data |= (0x01 << 1); //Flip Horizontal
    }
    // write_data |= 0x01; //Flip Vertical
    jd9613->rotation = r;
    log_i("set_rotation:%d write reg :0x%X , data : 0x%X Width:%d Height:%d", r, LCD_CMD_MADCTL, write_data, jd9613->width, jd9613->height);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &write_data, 1);
    return ESP_OK;
#else
    jd9613_panel_t *jd9613 = __containerof(panel, jd9613_panel_t, base);
    esp_lcd_panel_io_handle_t io = jd9613->io;
    uint8_t write_data = 0x00;
    switch (r) {
    case 1: // jd9613 only has 1/2RAM and cannot be rotated
        write_data = LCD_CMD_MX_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        jd9613->width = JD9613_HEIGHT;
        jd9613->height = JD9613_WIDTH;
        break;
    case 2:
        write_data = LCD_CMD_MY_BIT | LCD_CMD_MX_BIT | LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    case 3: // jd9613 only has 1/2RAM and cannot be rotated
        write_data = LCD_CMD_MY_BIT | LCD_CMD_MV_BIT | LCD_CMD_RGB;
        jd9613->width = JD9613_HEIGHT;
        jd9613->height = JD9613_WIDTH;
        break;
    default: // case 0:
        write_data = LCD_CMD_RGB;
        jd9613->width = JD9613_WIDTH;
        jd9613->height = JD9613_HEIGHT;
        break;
    }
    write_data |= (0x01 << 1); //Flip Horizontal
    // write_data |= 0x01; //Flip Vertical
    jd9613->rotation = r;
    log_i("set_rotation:%d write reg :0x%X , data : 0x%X Width:%d Height:%d", r, LCD_CMD_MADCTL, write_data, jd9613->width, jd9613->height);
    esp_lcd_panel_io_tx_param(io, LCD_CMD_MADCTL, &write_data, 1);
    return ESP_OK;
#endif
}
__END_DECLS

static bool touchPadReadFunction()
{
    return touchInterruptGetLastStatus(BOARD_TOUCH_BUTTON) == 0;
}

LilyGo_Wristband::LilyGo_Wristband(): _brightness(AMOLED_DEFAULT_BRIGHTNESS), panel_handle(NULL), threshold(2000)
{
}

LilyGo_Wristband::~LilyGo_Wristband()
{
    esp_lcd_panel_del(panel_handle);
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
}

void LilyGo_Wristband::setTouchThreshold(uint32_t threshold)
{
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
    touchAttachInterrupt(BOARD_TOUCH_BUTTON, touchISR, threshold);
}

void LilyGo_Wristband::detachTouch()
{
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
}

bool LilyGo_Wristband::getTouched()
{
    if (touchDetected) {
        touchDetected = false;
        return touchInterruptGetLastStatus(BOARD_TOUCH_BUTTON);
    }
    return false;
}

bool LilyGo_Wristband::isPressed()
{
    return touchInterruptGetLastStatus(BOARD_TOUCH_BUTTON);
}

bool LilyGo_Wristband::begin()
{
    if (panel_handle) {
        return true;
    }

    // Initialize display
    initBUS();

    // Initialize touch button
    touchAttachInterrupt(BOARD_TOUCH_BUTTON, touchISR, threshold);

    LilyGo_Button::init(BOARD_TOUCH_BUTTON, 50, touchPadReadFunction);

    // Initialize vibration motor
    tone(BOARD_VIBRATION_PIN, 1000, 50);

    Wire.begin(BOARD_I2C_SDA, BOARD_I2C_SCL);

    // Initialize RTC PCF85063
    bool result = SensorPCF85063::init(Wire);
    if (!result) {
        log_e("Real time clock initialization failed!");
    }

    pinMode(BOARD_BHI_EN, OUTPUT);
    digitalWrite(BOARD_BHI_EN, HIGH);

    // Initialize Sensor
    SensorBHI260AP::setPins(BOARD_BHI_RST, BOARD_BHI_IRQ);
    result = SensorBHI260AP::init(SPI, BOARD_BHI_CS, BOARD_BHI_MOSI, BOARD_BHI_MISO, BOARD_BHI_SCK);
    if (!result) {
        log_e("Motion sensor initialization failed!");
    }

    return true;
}

void LilyGo_Wristband::update()
{
    SensorBHI260AP::update();
    LilyGo_Button::update();
}

void LilyGo_Wristband::attachRTC(void (*rtc_alarm_cb)(void *arg), void *arg)
{
    attachInterruptArg(BOARD_RTC_IRQ, rtc_alarm_cb, arg, FALLING);
}

void LilyGo_Wristband::setBrightness(uint8_t level)
{
    lcd_cmd_t t = {0x51, {level}, 1};
    writeCommand(t.addr, t.param, t.len);
    _brightness = level;
}

uint8_t LilyGo_Wristband::getBrightness()
{
    return _brightness;
}

void LilyGo_Wristband::setRotation(uint8_t rotation)
{
    assert(panel_handle);
    panel_jd9613_set_rotation(panel_handle, rotation);
}

uint8_t LilyGo_Wristband::getRotation()
{
    assert(panel_handle);
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);
    return (jd9613->rotation);
}

void LilyGo_Wristband::setAddrWindow(uint16_t xs, uint16_t ys, uint16_t w, uint16_t h)
{
    assert(panel_handle);
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);
    uint8_t data1[] = {lowByte(xs >> 8), lowByte(xs), lowByte((xs + w - 1) >> 8), lowByte(xs + w - 1)};
    esp_lcd_panel_io_tx_param(jd9613->io, LCD_CMD_CASET, data1, 4);
    uint8_t data2[] = {lowByte(ys >> 8), lowByte(ys), lowByte((ys + h - 1) >> 8), lowByte(ys + h - 1)};
    esp_lcd_panel_io_tx_param(jd9613->io, LCD_CMD_RASET, data2, 4);
}

void LilyGo_Wristband::flipHorizontal(bool enable)
{
    assert(panel_handle);
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);
    jd9613->flipHorizontal = enable;
    panel_jd9613_set_rotation(panel_handle, jd9613->rotation);
}

void LilyGo_Wristband::pushColors(uint16_t *data, uint32_t len)
{
    assert(panel_handle);
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);
    esp_lcd_panel_io_tx_color(jd9613->io, LCD_CMD_RAMWR, data, len);
}

void LilyGo_Wristband::pushColors(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t *data)
{
    assert(panel_handle);
    esp_lcd_panel_draw_bitmap(panel_handle, x, y, width, height, data);
}

bool LilyGo_Wristband::initBUS()
{
    spi_bus_config_t buscfg;
    buscfg.mosi_io_num = BOARD_DISP_MOSI;
    buscfg.miso_io_num = BOARD_DISP_MISO;
    buscfg.sclk_io_num = BOARD_DISP_SCK;
    buscfg.quadwp_io_num = BOARD_NONE_PIN;
    buscfg.quadhd_io_num = BOARD_NONE_PIN;
    buscfg.data4_io_num = 0;
    buscfg.data5_io_num = 0;
    buscfg.data6_io_num = 0;
    buscfg.data7_io_num = 0;
    buscfg.max_transfer_sz = JD9613_HEIGHT * 80 * sizeof(uint16_t);
    buscfg.flags = 0x00;
    buscfg.intr_flags = 0x00;

#if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(3,0,0)
    buscfg.isr_cpu_id = INTR_CPU_ID_AUTO;
#endif

    ESP_ERROR_CHECK(spi_bus_initialize(BOARD_DISP_HOST, &buscfg, SPI_DMA_CH_AUTO));

    log_i( "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config;
    io_config.cs_gpio_num = BOARD_DISP_CS;
    io_config.dc_gpio_num = BOARD_DISP_DC;
    io_config.spi_mode = 0;
    io_config.pclk_hz = DEFAULT_SCK_SPEED;
    io_config.trans_queue_depth = 10;
    io_config.on_color_trans_done = NULL;
    io_config.user_ctx = NULL;
    io_config.lcd_cmd_bits = 8;
    io_config.lcd_param_bits = 8;

    io_config.flags.dc_low_on_data = 0;
    io_config.flags.octal_mode = 0;
    io_config.flags.lsb_first = 0;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
    io_config.flags.quad_mode = 0;
    io_config.flags.sio_mode = 0;
    io_config.flags.cs_high_active = 0;
#else
    io_config.flags.dc_as_cmd_phase = 0;
#endif


    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BOARD_DISP_HOST, &io_config, &io_handle));


    esp_lcd_panel_dev_config_t panel_config;
    panel_config.reset_gpio_num = BOARD_DISP_RST;
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5,0,0)
    panel_config.color_space = ESP_LCD_COLOR_SPACE_RGB;
#else
    panel_config.color_space = LCD_RGB_ELEMENT_ORDER_RGB;
#endif
    panel_config.bits_per_pixel = 16;
    panel_config.flags.reset_active_high = 0;
    panel_config.vendor_config = NULL;

    log_i( "Install JD9613 panel driver");
    ESP_ERROR_CHECK(esp_lcd_new_panel_jd9613(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    return true;
}

void LilyGo_Wristband::writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length)
{
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);
    esp_lcd_panel_io_tx_param(jd9613->io, cmd, pdat, length);
}

void LilyGo_Wristband::enableTouchWakeup(int threshold)
{
    touchSleepWakeUpEnable(BOARD_TOUCH_BUTTON, threshold);

    esp_sleep_enable_touchpad_wakeup();
}

void LilyGo_Wristband::sleep()
{
    lcd_cmd_t t = {0x10, {0x00}, 1}; //Sleep in
    writeCommand(t.addr, t.param, t.len);

    detachInterrupt(BOARD_RTC_IRQ);

    Wire.end();

    esp_deep_sleep_start();
}

void LilyGo_Wristband::wakeup()
{
    lcd_cmd_t t = {0x11, {0x00}, 1};// Sleep Out
    writeCommand(t.addr, t.param, t.len);
}

uint16_t  LilyGo_Wristband::width()
{
    assert(panel_handle);
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);

#ifdef SW_ROTATION
    switch (jd9613->rotation) {
    case 1:
    case 3:
        return jd9613->height;
        break;
    default:
        break;
    }
#endif
    return jd9613->width;
}

uint16_t  LilyGo_Wristband::height()
{
    assert(panel_handle);
    jd9613_panel_t *jd9613 = __containerof(panel_handle, jd9613_panel_t, base);
#ifdef SW_ROTATION
    switch (jd9613->rotation) {
    case 1:
    case 3:
        return jd9613->width;
        break;
    default:
        break;
    }
#endif
    return jd9613->height;
}

bool LilyGo_Wristband::hasTouch()
{
    return false;
}

uint8_t LilyGo_Wristband::getPoint(int16_t *x, int16_t *y, uint8_t get_point )
{
    return 0;
}

uint16_t LilyGo_Wristband::getBattVoltage(void)
{
    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    const int number_of_samples = 20;
    uint32_t sum = 0;
    uint16_t raw_buffer[number_of_samples] = {0};
    for (int i = 0; i < number_of_samples; i++) {
        raw_buffer[i] =  analogRead(BOARD_BAT_ADC);
        delay(2);
    }
    for (int i = 0; i < number_of_samples; i++) {
        sum += raw_buffer[i];
    }
    sum = sum / number_of_samples;

    uint16_t volts =  esp_adc_cal_raw_to_voltage(sum, &adc_chars) * 2;
    return volts > 4200 ? 4200 : volts;
}


int LilyGo_Wristband::getBatteryPercent()
{
    uint16_t battery_voltage = getBattVoltage();
    return  (int)((((battery_voltage / 1000.0) - 3.0) / (4.2 - 3.0)) * 100);
}


void LilyGo_Wristband::vibration(uint8_t duty, uint32_t delay_ms)
{
    tone(BOARD_VIBRATION_PIN, 1000, delay_ms);
}

bool LilyGo_Wristband::needFullRefresh()
{
    return true;
}

bool LilyGo_Wristband::initMicrophone()
{
    static i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM),
        .sample_rate =  MIC_I2S_SAMPLE_RATE,
        .bits_per_sample = MIC_I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .communication_format = I2S_COMM_FORMAT_STAND_PCM_SHORT,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 512,
        .use_apll = true
    };

    static i2s_pin_config_t i2s_cfg = {0};
    i2s_cfg.bck_io_num   = I2S_PIN_NO_CHANGE;
    i2s_cfg.ws_io_num    = BOARD_MIC_CLOCK;
    i2s_cfg.data_out_num = I2S_PIN_NO_CHANGE;
    i2s_cfg.data_in_num  = BOARD_MIC_DATA;
    i2s_cfg.mck_io_num = I2S_PIN_NO_CHANGE;

    if (i2s_driver_install(MIC_I2S_PORT, &i2s_config, 0, NULL) != ESP_OK) {
        log_e("i2s_driver_install error");
        return false;
    }

    if (i2s_set_pin(MIC_I2S_PORT, &i2s_cfg) != ESP_OK) {
        log_e("i2s_set_pin error");
        return false;
    }
    log_i("Microphone init done .");
    return true;
}

bool LilyGo_Wristband::readMicrophone(void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait)
{
    return i2s_read(MIC_I2S_PORT, dest, size, bytes_read, ticks_to_wait) == ESP_OK;
}
