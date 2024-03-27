/**
 * @file      WristbandFactory.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2024-02-23
 *
 */
#include <LilyGo_Wristband.h> //To use LilyGo Wristband S3, please include <LilyGo_Wristband.h>
#include <LV_Helper.h>
#include <time.h>
#include <cbuf.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <MadgwickAHRS.h>       //MadgwickAHRS from https://github.com/arduino-libraries/MadgwickAHRS
#include "particleSensor.h"

LilyGo_Class amoled;


#ifndef WIFI_SSID
#define WIFI_SSID             "Your WiFi SSID"
#endif
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD         "Your WiFi PASSWORD"
#endif

#define WIFI_MSG_ID             0x1001

// Adjust the time server and corresponding event offset according to your own situation
#define NTP_SERVER1           "pool.ntp.org"
#define NTP_SERVER2           "time.nist.gov"

/**
 * The time zone is used by default. If you need to use an offset, please change the synchronization method in the setup function.
 * A more convenient approach to handle TimeZones with daylightOffset
 * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
 * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
 */

#define CFG_TIME_ZONE         "CST-8"       // TZ_Asia_Shanghai 

// #define GMT_OFFSET_SEC        0
// #define DAY_LIGHT_OFFSET_SEC  0

static const char *week_char[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char *month_char[] = LV_CALENDAR_DEFAULT_MONTH_NAMES;

// LV_FONT_DECLARE(wallpaper);
LV_FONT_DECLARE(wallpaper_7);
LV_FONT_DECLARE(heart_rate);
LV_FONT_DECLARE(icon_space);


// Create a window into which all elements are loaded
lv_obj_t *window;

// The displayable area of Wristband is 126x250, which requires an offset of 44 pixels.
#define WindowViewableWidth              126
#define WindowViewableHeight             250


Madgwick filter;

static cbuf accel_data_buffer(1);
static cbuf gyro_data_buffer(1);
static lv_obj_t *tileview;
static lv_obj_t *chart;
static lv_chart_series_t *ser1 ;
static lv_obj_t  *month_label ;

static void lv_gui_init();
static void lv_gui_select_next_item();
static void lv_tileview_add_datetime(lv_obj_t *parent);
static void lv_tileview_add_dummy_heart(lv_obj_t *parent);
static void lv_tileview_add_dev_probe(lv_obj_t *parent);
static void lv_tileview_add_wifi(lv_obj_t *parent);
static void lv_tileview_add_sensor(lv_obj_t *parent);
static void timeavailable(struct timeval *t);
static void WiFiEvent(WiFiEvent_t event);


void button_event_callback(ButtonState state)
{
    amoled.vibration();
    switch (state) {
    case BTN_PRESSED_EVENT:
        Serial.println("pressed");
        break;
    case BTN_RELEASED_EVENT:
        Serial.print("released: ");
        Serial.println(amoled.wasPressedFor());
        break;
    case BTN_CLICK_EVENT:
        Serial.println("click\n");
        lv_gui_select_next_item();
        break;
    case BTN_LONG_PRESSED_EVENT:
        amoled.enableTouchWakeup();
        amoled.sleep();
        Serial.println("long Pressed\n");
        break;
    case BTN_DOUBLE_CLICK_EVENT:
        Serial.println("double click\n");
        break;
    case BTN_TRIPLE_CLICK_EVENT:
        Serial.println("triple click\n");
        break;
    default:
        break;
    }
}


static void lv_button_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    if (amoled.getTouched()) {
        amoled.vibration();
        lv_gui_select_next_item();
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void bindTouchButton()
{
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_BUTTON;
    indev_drv.read_cb = lv_button_read;
    static lv_indev_t *lv_encoder_indev = lv_indev_drv_register(&indev_drv);
    lv_group_t *g = lv_group_create();
    lv_indev_set_group(lv_encoder_indev, g);
    lv_group_set_default(g);
}


static void accel_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len)
{
    struct bhy2_data_xyz data;
    bhy2_parse_xyz(data_ptr, &data);

    // float scaling_factor = get_sensor_default_scaling(sensor_id);
    // Serial.print(amoled.getSensorName(sensor_id));
    // Serial.print(":");
    // Serial.printf("x: %f, y: %f, z: %f;\r\n",
    //               data.x * scaling_factor,
    //               data.y * scaling_factor,
    //               data.z * scaling_factor
    //  );
    accel_data_buffer.write((const char *)&data, sizeof(data));
}


static void gyro_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len)
{
    struct bhy2_data_xyz data;
    bhy2_parse_xyz(data_ptr, &data);
    // float scaling_factor = get_sensor_default_scaling(sensor_id);
    // Serial.print(amoled.getSensorName(sensor_id));
    // Serial.print(":");
    // Serial.printf("x: %f, y: %f, z: %f;\r\n",
    //               data.x * scaling_factor,
    //               data.y * scaling_factor,
    //               data.z * scaling_factor
    //              );
    gyro_data_buffer.write((const char *)&data, sizeof(data));
}

void orientation_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len)
{
    uint8_t direction = *data_ptr;
    Serial.print(amoled.getSensorName(sensor_id));
    Serial.print(":");
    Serial.println(direction);
}

void setup(void)
{
    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);


    setCpuFrequencyMhz(80);

    // Initialization screen and peripherals
    if (!amoled.begin()) {
        while (1) {
            Serial.println("The board model cannot be detected, please raise the Core Debug Level to an error");
            delay(1000);
        }
    }

    // Set the bracelet screen orientation to portrait
    amoled.setRotation(0);

    // Brightness range : 0 ~ 255
    amoled.setBrightness(255);

    // Initialize the heart rate sensor. You need to purchase the heart rate version to have a heart rate sensor.
    // It is not available by default.
    setupParticleSensor();

    // Initialize lvgl
    beginLvglHelper(amoled);

    // bindTouchButton();

    // Set touch button callback function
    amoled.setEventCallback(button_event_callback);

    // Initialize Sensor
    accel_data_buffer.resize(sizeof(struct bhy2_data_xyz) * 2);
    gyro_data_buffer.resize(sizeof(struct bhy2_data_xyz) * 2);

    // initialize variables to pace updates to correct rate
    filter.begin(100);

    float sample_rate = 100.0;      /* Read out hintr_ctrl measured at 100Hz */
    uint32_t report_latency_ms = 0; /* Report immediately */

    // Enable acceleration
    amoled.configure(SENSOR_ID_ACC_PASS, sample_rate, report_latency_ms);
    // Enable gyroscope
    amoled.configure(SENSOR_ID_GYRO_PASS, sample_rate, report_latency_ms);

    // Set the acceleration sensor result callback function
    amoled.onResultEvent(SENSOR_ID_ACC_PASS, accel_process_callback);

    // Set the gyroscope sensor result callback function
    amoled.onResultEvent(SENSOR_ID_GYRO_PASS, gyro_process_callback);


    // Set notification call-back function , After time synchronization is completed, synchronize the synchronized time to the hardware RTC
    sntp_set_time_sync_notification_cb( timeavailable );

    /**
    * This will set configured ntp servers and constant TimeZone/daylightOffset
    * should be OK if your time zone does not need to adjust daylightOffset twice a year,
    * in such a case time adjustment won't be handled automagicaly.
    */
    // configTime(GMT_OFFSET_SEC, DAY_LIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);

    /**
     * A more convenient approach to handle TimeZones with daylightOffset
     * would be to specify a environmnet variable with TimeZone definition including daylight adjustmnet rules.
     * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
     */
    configTzTime(CFG_TIME_ZONE, NTP_SERVER1, NTP_SERVER2);

    // Initialize WiFi
    WiFi.mode(WIFI_STA);
    WiFi.onEvent(WiFiEvent);    // Register WiFi event

    // For factory WiFi connection testing only
    Serial.print("Use default WiFi SSID & PASSWORD!!");

    Serial.print("SSID:"); Serial.println(WIFI_SSID);
    Serial.print("PASSWORD:"); Serial.println(WIFI_PASSWORD);
    if (String(WIFI_SSID) == "Your WiFi SSID" || String(WIFI_PASSWORD) == "Your WiFi PASSWORD" ) {
        Serial.println("[Error] : WiFi ssid and password are not configured correctly");
        Serial.println("[Error] : WiFi ssid and password are not configured correctly");
        Serial.println("[Error] : WiFi ssid and password are not configured correctly");
    }

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Initialize factory gui
    lv_gui_init();

}


void loop()
{
    // Update 6-axis sensor and button state
    amoled.update();

    lv_timer_handler();

    delay(5);
}

// Callback function (get's called when time adjusts via NTP)
static void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    // Synchronize the synchronized time to the hardware RTC
    amoled.hwClockWrite();
}

static void lv_gui_select_next_item()
{
    static int next = 0;
    next++; next %= 4;
    lv_obj_set_tile_id(tileview, next, 0, LV_ANIM_ON);
}

static void update_datetime(lv_timer_t *e)
{
    struct tm timeinfo;

    /*
    You can use localtime_r to directly read the system time.
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    */

    // Here need to test the hardware time, so get the time in the RTC PCF85063
    amoled.getDateTime(&timeinfo);

    lv_obj_t *time_label = (lv_obj_t *)(e->user_data);
    lv_label_set_text_fmt(time_label, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text_fmt(month_label, "%s", month_char[timeinfo.tm_mon]);

    lv_obj_t *batt = (lv_obj_t *) lv_obj_get_user_data(time_label);

    int percent = amoled.getBatteryPercent();

    lv_label_set_text_fmt(batt, "%d%%", percent);
}

static void lv_gui_init()
{
    // Set page background black color
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    // Create a display window object
    window = lv_obj_create(lv_scr_act());
    // Set window background color to black
    lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    // Set window pad all zero
    lv_obj_set_style_pad_all(window, 0, 0);
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, WindowViewableWidth, WindowViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Create display area
    tileview = lv_tileview_create(window);
    lv_obj_set_size(tileview, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(tileview, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);

    // Create the displayed UI
    lv_obj_t *t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t4 = lv_tileview_add_tile(tileview, 3, 0, LV_DIR_HOR | LV_DIR_BOTTOM);

    // Create date page
    lv_tileview_add_datetime(t1);

    // Create devices probe
    lv_tileview_add_dev_probe(t2);

    // Create sensor page
    lv_tileview_add_sensor(t3);

    // Create a WiFi quality page
    lv_tileview_add_wifi(t4);

    // Create the heart rate sensor page
    // You need to purchase the heart rate version to have a heart rate sensor.
    // It is not available by default.
    if (isParticleSensorOnline) {
        lv_obj_t *t5 = lv_tileview_add_tile(tileview, 4, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
        lv_tileview_add_dummy_heart(t5);
    }
}

static void lv_tileview_add_datetime(lv_obj_t *parent)
{
    lv_obj_set_style_bg_img_src(parent, &wallpaper_7, LV_PART_MAIN);
    lv_obj_t  *time_label = lv_label_create(parent);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_44, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 55);
    lv_label_set_text(time_label, "12:34");

    month_label = lv_label_create(parent);
    lv_obj_set_style_text_font(month_label, &lv_font_montserrat_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(month_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(month_label, LV_ALIGN_TOP_MID, 0, LV_PCT(40));
    lv_label_set_text(month_label, month_char[0]);

    lv_obj_t  *batt = lv_label_create(parent);
    lv_obj_set_style_text_font(batt, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(batt, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(batt, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_label_set_text(batt, "100%");

    lv_obj_set_user_data(time_label, batt);

    lv_timer_create(update_datetime, 1000, time_label);
}

static void lv_tileview_add_dummy_heart(lv_obj_t *parent)
{
    lv_obj_t *cont =  lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *img = lv_gif_create(cont);
    lv_gif_set_src(img, &heart_rate);
    lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 20);

    chart = lv_chart_create(cont);
    lv_obj_set_size(chart, LV_PCT(100), LV_PCT(30));
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE); /*Show lines and points too*/
    lv_obj_align_to(chart, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_set_style_bg_opa(chart, LV_OPA_TRANSP, LV_PART_MAIN);

    ser1 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 120);
    lv_chart_set_div_line_count(chart, 5, 5);

    lv_obj_t  *bpm_label = lv_label_create(parent);
    lv_obj_set_style_text_font(bpm_label, &lv_font_montserrat_28, LV_PART_MAIN);
    lv_label_set_text(bpm_label, "78Bpm");
    lv_obj_set_style_text_color(bpm_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align_to(bpm_label, chart, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);

    lv_obj_t  *val_label = lv_label_create(parent);
    lv_obj_set_style_text_font(val_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(val_label, "NULL");
    lv_obj_set_style_text_color(val_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align_to(val_label, bpm_label, LV_ALIGN_OUT_BOTTOM_MID, -25, 5);

    lv_obj_set_user_data(bpm_label, val_label);

    lv_timer_create([](lv_timer_t *t) {

        lv_obj_t  *bpm_label  = (lv_obj_t *)t->user_data;

        // This is virtual data. Heart rate and blood oxygen saturation need to be combined with algorithms. Leave it to the powerful.
        uint16_t val = random(65, 100);
        lv_label_set_text_fmt(bpm_label, "%uBpm", val);
        lv_chart_set_next_value(chart, ser1, val);

        lv_obj_t  *val_label = (lv_obj_t *)lv_obj_get_user_data(bpm_label);
        lv_label_set_text_fmt(val_label, "IR:%u\nRED:%u\nTemp:%.2f", getParticleSensorIR(), getParticleSensorRed(), getParticleSensorTemp());

    }, 1000, bpm_label);
}




static void serialToScreen(lv_obj_t *parent, String string, String result)
{
    const lv_font_t *font = &lv_font_montserrat_12;
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_pad_left(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_size(cont, LV_PCT(100), lv_font_get_line_height(font) + 6 );
    lv_obj_t *label1 = lv_label_create(cont);
    lv_obj_set_style_pad_left(label1, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(label1, 0, LV_PART_MAIN);

    lv_obj_set_style_text_font(label1, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label1, lv_color_white(), LV_PART_MAIN);
    lv_label_set_recolor(label1, true);
    lv_label_set_text(label1, string.c_str());
    lv_obj_align(label1, LV_ALIGN_LEFT_MID, 0, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_pad_left(label, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(label, 0, LV_PART_MAIN);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_label_set_recolor(label, true);
    lv_label_set_text(label, result.c_str());

    lv_obj_align(label, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_scroll_to_y(parent, LV_PCT(100), LV_ANIM_ON);
}


static void lv_tileview_add_dev_probe(lv_obj_t *parent)
{
    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_bg_color(parent, lv_color_black(), LV_PART_MAIN);
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_style_border_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cont, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_radius(cont, 0, 0);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(90));
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_left(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_right(cont, 0, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    // Shift down by 20 pixels and display in the visible area
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 20);

    float flash_size = abs(ESP.getFlashChipSize() / 1024.0 / 1024.0);
    float ram_size = abs(ESP.getPsramSize() / 1024.0 / 1024.0);

    Serial.printf("flash_size:%.2f\n", flash_size);
    Serial.printf("ram_size:%.2f\n", ram_size);
    char buffer[128];
    const char *fail = "#FFFFFF [# #ff0000  FAIL# #FFFFFF ]#";
    const char *pass = "#FFFFFF [# #00ff00 PASS# #FFFFFF ]#";

    snprintf(buffer, 128, "#FFFFFF [# #00ff00 %uMHz# #FFFFFF ]#", ESP.getCpuFreqMHz() );
    serialToScreen(cont, "CPU:", buffer);
    serialToScreen(cont, "RTC",  true ? pass : fail);
    serialToScreen(cont, "MAX3010x", isParticleSensorOnline() ? pass : fail);
    serialToScreen(cont, "BHI260AP", true ? pass : fail);
    snprintf(buffer, 128, "#FFFFFF [# #00ff00 %.0f MB# #FFFFFF ]#", flash_size);
    serialToScreen(cont, "FLASH:", buffer);
    snprintf(buffer, 128, "#FFFFFF [# #00ff00 %.0f MB# #FFFFFF ]#", ram_size);
    serialToScreen(cont, "PSRAM:", buffer);
    snprintf(buffer, 128, "#FFFFFF [# #00ff00 %.0f V# #FFFFFF ]#", amoled.getBattVoltage() / 1000.0);
    serialToScreen(cont, "VOLTAGE:", buffer);

}

static void lv_tileview_add_sensor(lv_obj_t *parent)
{
    lv_obj_t *attitude = lv_label_create(parent);
    lv_label_set_long_mode(attitude, LV_LABEL_LONG_WRAP); /*Break the long lines*/
    lv_label_set_recolor(attitude, true);                 /*Enable re-coloring by commands in the text*/
    lv_obj_set_style_text_color(attitude, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(attitude, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text_fmt(attitude, "Roll:% 3.2f\nPitch:% 3.2f\nHeading:% 3.2f\n", 0, 0, 0);
    lv_obj_align(attitude, LV_ALIGN_CENTER, 0, 0);


    lv_timer_create([](lv_timer_t *t) {

        lv_obj_t *label =   (lv_obj_t *)t->user_data;

        float roll, pitch, heading;

        // Read data from buffer
        struct bhy2_data_xyz gyr;
        gyro_data_buffer.read(( char *)&gyr, sizeof(gyr));

        struct bhy2_data_xyz acc;
        accel_data_buffer.read(( char *)&acc, sizeof(acc));

        float accel_scaling_factor = get_sensor_default_scaling(BHY2_SENSOR_ID_ACC_PASS);
        float gyro_scaling_factor = get_sensor_default_scaling(BHY2_SENSOR_ID_GYRO_PASS);

        // update the filter, which computes orientation
        filter.updateIMU(gyr.x * gyro_scaling_factor,
                         gyr.y * gyro_scaling_factor,
                         gyr.z * gyro_scaling_factor,
                         acc.x * accel_scaling_factor,
                         acc.y * accel_scaling_factor,
                         acc.z * accel_scaling_factor
                        );

        // get the heading, pitch and roll
        roll = filter.getRoll();
        pitch = filter.getPitch();
        heading = filter.getYaw();

        lv_label_set_text_fmt(label, "Roll:\n\t% 3.2f\nPitch:\n\t% 3.2f\nHeading:\n\t% 3.2f\n", roll, pitch, heading);

    }, 15, attitude);
}


static void lv_tileview_add_wifi(lv_obj_t *parent)
{
    static lv_obj_t *label_rssi;
    static lv_obj_t *label;
    static lv_obj_t *label_ip_addr;
    static lv_obj_t *meter;
    static lv_meter_indicator_t *indic2 ;
    static lv_meter_indicator_t *indic1 ;

    lv_obj_set_scrollbar_mode(parent, LV_SCROLLBAR_MODE_OFF);

    meter = lv_meter_create(parent);

    /*Remove the background and the circle from the middle*/
    lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
    lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);
    lv_obj_set_size(meter, LV_PCT(90), LV_PCT(45));
    lv_obj_align(meter, LV_ALIGN_CENTER, 0, -20);

    /*Add a scale first with no ticks.*/
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 0, 0, 0, lv_color_black());
    lv_meter_set_scale_range(meter, scale, -100, 10, 280, 130);

    /*Add a three arc indicator*/
    lv_coord_t indic_w = 25;

    indic1 = lv_meter_add_arc(meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_meter_set_indicator_start_value(meter, indic1, 0);
    lv_meter_set_indicator_end_value(meter, indic1, 100);

    indic2 = lv_meter_add_arc(meter, scale, indic_w, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, indic2, 0);
    lv_meter_set_indicator_end_value(meter, indic2, 90);

    label_rssi = lv_label_create(parent);
    lv_obj_set_style_text_color(label_rssi, lv_color_white(), 0);
    lv_label_set_text(label_rssi, "0");
    lv_obj_set_style_text_font(label_rssi, &lv_font_montserrat_36, 0);
    lv_obj_align_to(label_rssi, meter, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "RSSI");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
    lv_obj_align_to(label, label_rssi, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    label_ip_addr = lv_label_create(parent);
    lv_obj_set_style_text_color(label_ip_addr, lv_color_white(), 0);
    lv_obj_set_style_text_font(label_ip_addr, &lv_font_montserrat_16, 0);
    lv_label_set_text(label_ip_addr, "IP:N/A");
    lv_obj_align_to(label_ip_addr, label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_msg_subsribe_obj(WIFI_MSG_ID, label_ip_addr, NULL);

    // Added got ip address message cb
    lv_obj_add_event_cb( label_ip_addr, [](lv_event_t *e) {

        lv_obj_t *obj_align = (lv_obj_t *)lv_event_get_user_data(e);

        lv_obj_t *label = (lv_obj_t *)lv_event_get_target(e);

        Serial.print("-->>IP:"); Serial.println(WiFi.localIP());

        lv_label_set_text_fmt(label, "IP:%s",  WiFi.isConnected() ? (WiFi.localIP().toString().c_str()) : ("N/A") );

        lv_obj_align_to(label_ip_addr, obj_align, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    }, LV_EVENT_MSG_RECEIVED, label);

    lv_timer_create([](lv_timer_t *t) {

        if (!WiFi.isConnected()) {
            lv_label_set_text(label_rssi, "N/A");
            return;
        }
        int32_t rssi = WiFi.RSSI();

        lv_label_set_text_fmt(label_rssi, "%d",  rssi);
        lv_obj_align_to(label_rssi, meter, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        lv_meter_set_indicator_end_value(meter, indic2, rssi);

    }, 1000, NULL);
}

static void WiFiEvent(WiFiEvent_t event)
{
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
        Serial.println("WiFi interface ready");
        break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
        Serial.println("Completed scan for access points");
        break;
    case ARDUINO_EVENT_WIFI_STA_START:
        Serial.println("WiFi client started");
        break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
        Serial.println("WiFi clients stopped");
        break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.println("Connected to access point");
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.println("Disconnected from WiFi access point");
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
        Serial.println("Authentication mode of access point has changed");
        break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.print("Obtained IP address: ");
        Serial.println(WiFi.localIP());
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
        Serial.println("Lost IP address and IP address is reset to 0");
        lv_msg_send(WIFI_MSG_ID, NULL);
        break;
    default: break;
    }
}
