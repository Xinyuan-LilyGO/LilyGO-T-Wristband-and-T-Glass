/**
 * @file      GlassFactory.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2023-10-23
 * @note      Arduino Setting
 *            Tools ->
 *                  Board:"ESP32S3 Dev Module"
 *                  USB CDC On Boot:"Enable"
 *                  USB DFU On Boot:"Disable"
 *                  Flash Size : "4MB(32Mb)"
 *                  Flash Mode"QIO 80MHz
 *                  Partition Scheme:"Huge APP (3MB No OTA/1MB SPIFFS)"
 *                  PSRAM:"QSPI PSRAM"
 *                  Upload Mode:"UART0/Hardware CDC"
 *                  USB Mode:"Hardware CDC and JTAG"
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>
#include <MadgwickAHRS.h>       //MadgwickAHRS from https://github.com/arduino-libraries/MadgwickAHRS
#include <cbuf.h>
#include <WiFi.h>
#include <esp_sntp.h>

// The esp_vad function is currently only reserved for Arduino version 2.0.9
#if ESP_ARDUINO_VERSION_VAL(2,0,9) == ESP_ARDUINO_VERSION

#include <esp_vad.h>

// For noise detection
#define VAD_FRAME_LENGTH_MS             30
#define VAD_BUFFER_LENGTH               (VAD_FRAME_LENGTH_MS * MIC_I2S_SAMPLE_RATE / 1000)

static int16_t *vad_buff = NULL;
static vad_handle_t vad_inst;
static uint32_t noise_count;

#define TILEVIEW_CNT            6
#else
#define TILEVIEW_CNT            5
#endif //Version check


// The resolution of the non-magnified side of the glasses reflection area is about 126x126, 
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

LilyGo_Class amoled;

LV_IMG_DECLARE(img_down);
LV_IMG_DECLARE(img_left);
LV_IMG_DECLARE(img_right);
LV_IMG_DECLARE(img_up);



static lv_color_t font_color = lv_color_white();
static lv_color_t bg_color = lv_color_black();

static lv_obj_t *touch_label;

static lv_obj_t *tileview;
static lv_obj_t *time_label;
static lv_obj_t *day_label;
static lv_obj_t *week_label;
static lv_obj_t *month_label;
static bool colon;
static cbuf accel_data_buffer(1);
static cbuf gyro_data_buffer(1);

static lv_timer_t *noise_timer;
static lv_timer_t *sensor_timer;
static lv_timer_t *datetime_timer;

Madgwick filter;
LilyGo_Button bootPin;

//! You can use EspTouch to configure the network key without changing the WiFi password below
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


const char *week_char[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *month_char[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sept", "Oct", "Nov", "Dec"};

static void lv_gui_init();
static void lv_gui_select_next_item();
static void gyro_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len);
static void accel_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len);
static void WiFiEvent(WiFiEvent_t event);
static void timeavailable(struct timeval *t);

void button_event_callback(ButtonState state)
{
    // amoled.vibration();
    switch (state) {
    case BTN_PRESSED_EVENT:
        Serial.println("pressed");
        break;
    case BTN_RELEASED_EVENT:
        Serial.print("released: ");
        Serial.println(amoled.wasPressedFor());
        break;
    case BTN_CLICK_EVENT: {

        lv_gui_select_next_item(); return;
        // Serial.println("click\n");
        lv_disp_drv_t *drv = lv_disp_get_default()->driver;
        uint8_t r = amoled.getRotation();
        r++;
        r %= 4;
        amoled.setRotation(r);
        drv->hor_res = amoled.width();
        drv->ver_res = amoled.height();
        lv_disp_drv_update(lv_disp_get_default(), drv);
    }
    break;
    case BTN_LONG_PRESSED_EVENT:
        Serial.println("long Pressed\n");
        break;
    case BTN_DOUBLE_CLICK_EVENT:
        Serial.println("double click\n");
        /*
        if (amoled.getBrightness()) {
            amoled.setBrightness(0);
        } else {
            amoled.setBrightness(255);
        }
        */
        break;
    case BTN_TRIPLE_CLICK_EVENT:
        Serial.println("triple click\n");
        break;
    default:
        break;
    }
}

void bootPinFunctionCallback(ButtonState state)
{
    switch (state) {
    case BTN_PRESSED_EVENT:
        Serial.println("pressed");
        break;
    case BTN_RELEASED_EVENT:
        Serial.print("released: ");
        Serial.println(amoled.wasPressedFor());
        break;
    case BTN_CLICK_EVENT:
        break;
    case BTN_LONG_PRESSED_EVENT:
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


void setup()
{
    bool rslt = false;

    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);


    // Initialization screen and peripherals
    rslt = amoled.begin();
    if (!rslt) {
        while (1) {
            Serial.println("The board model cannot be detected, please raise the Core Debug Level to an error");
            delay(1000);
        }
    }

    // Set the bracelet screen orientation to portrait
    amoled.setRotation(0);

    // Initialize boot pin
    bootPin.init(BOARD_BOOT_PIN);

    // Set boot button callback function
    bootPin.setEventCallback(button_event_callback);

    // Set the default touch threshold. Depending on the contact surface, the touch sensitivity may need to be adjusted.
    amoled.setTouchThreshold(200);

    // Using glasses requires setting the screen to flip vertically
    // amoled.flipHorizontal(true);

    // Brightness range : 0 ~ 255
    amoled.setBrightness(255);

    // Initialize lvgl
    beginLvglHelper(amoled);

    // Set touch button callback function
    amoled.setEventCallback(button_event_callback);

    // Initialize onboard PDM microphone
    amoled.initMicrophone();

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

// The esp_vad function is currently only reserved for Arduino version 2.0.9
#if ESP_ARDUINO_VERSION_VAL(2,0,9) == ESP_ARDUINO_VERSION
    // Initialize esp-sr vad detected
#if ESP_IDF_VERSION_VAL(4,4,1) == ESP_IDF_VERSION
    vad_inst = vad_create(VAD_MODE_0, MIC_I2S_SAMPLE_RATE, VAD_FRAME_LENGTH_MS);
#elif  ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4,4,1)
    vad_inst = vad_create(VAD_MODE_0);
#else
#error "No support this version."
#endif
    if (psramFound()) {
        vad_buff = (int16_t *)ps_malloc(VAD_BUFFER_LENGTH * sizeof(short));
    } else {
        vad_buff = (int16_t *)malloc(VAD_BUFFER_LENGTH * sizeof(short));
    }
    if (vad_buff == NULL) {
        Serial.println("Memory allocation failed!");
    }
#else
    Serial.println("The currently used version does not support esp_vad and cannot run the noise detection function. If you need to use esp_vad, please change the version to 2.0.9");
#endif


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
    } else {
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }

    // Initialize factory gui
    lv_gui_init();
}

uint32_t threshold = 1500;
uint32_t touchReadInterval = 0;
void loop()
{
    // Update 6-axis sensor and button state
    amoled.update();
    // Update BOOT state
    bootPin.update();

    // if (millis() > touchReadInterval) {
    //     uint32_t value = touchRead(BOARD_TOUCH_BUTTON);
    //     update_touch_label(value);
    //     touchReadInterval = millis() + 100;
    // }

    lv_timer_handler();
    delay(1);
}

// Callback function (get's called when time adjusts via NTP)
static void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    // Synchronize the synchronized time to the hardware RTC
    amoled.hwClockWrite();
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

    lv_label_set_text_fmt(time_label, "%02d%s%02d", timeinfo.tm_hour, colon != 0 ? "#ffffff :#" : "#000000 :#", timeinfo.tm_min);
    colon = !colon;
    lv_label_set_text_fmt(week_label, "%s", week_char[timeinfo.tm_wday]);
    lv_label_set_text_fmt(month_label, "%s", month_char[timeinfo.tm_mon]);


}

static void lv_tileview_add_datetime(lv_obj_t *parent)
{
    lv_obj_set_scroll_dir(parent, LV_DIR_NONE);
    // TIME
    lv_obj_t *time_cont = lv_obj_create(parent);
    lv_obj_set_size(time_cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(time_cont, bg_color, 0);
    lv_obj_set_style_border_width(time_cont, 0, 0);
    lv_obj_set_scrollbar_mode(time_cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(time_cont, LV_DIR_NONE);

    time_label = lv_label_create(time_cont);
    lv_label_set_recolor(time_label, 1);
    lv_label_set_text(time_label, "12:34");
    lv_obj_set_style_text_color(time_label, font_color, 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -15);

    week_label = lv_label_create(time_cont);
    lv_obj_set_style_text_color(week_label, font_color, 0);
    lv_obj_set_style_text_font(week_label, &lv_font_montserrat_10, 0);
    lv_label_set_text(week_label, "Thu");
    lv_obj_align_to(week_label, time_label, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);

    month_label = lv_label_create(time_cont);
    lv_obj_set_style_text_color(month_label, font_color, 0);
    lv_obj_set_style_text_font(month_label, &lv_font_montserrat_10, 0);
    lv_label_set_text(month_label, "Feb");
    lv_obj_align_to(month_label, time_label, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);

    datetime_timer = lv_timer_create(update_datetime, 500, NULL);
}

static void lv_tileview_add_sensor(lv_obj_t *parent)
{
    lv_obj_t *attitude = lv_label_create(parent);
    lv_label_set_long_mode(attitude, LV_LABEL_LONG_WRAP); /*Break the long lines*/
    lv_label_set_recolor(attitude, true);                 /*Enable re-coloring by commands in the text*/
    lv_obj_set_style_text_color(attitude, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(attitude, &lv_font_montserrat_10, LV_PART_MAIN);
    lv_label_set_text_fmt(attitude, "Roll:% 3.2f\nPitch:% 3.2f\nHeading:% 3.2f\n", 0, 0, 0);
    lv_obj_align(attitude, LV_ALIGN_CENTER, 0, 0);

    sensor_timer = lv_timer_create([](lv_timer_t *t) {
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


// The esp_vad function is currently only reserved for Arduino version 2.0.9
#if ESP_ARDUINO_VERSION_VAL(2,0,9) == ESP_ARDUINO_VERSION
static void lv_tileview_add_noise_detect(lv_obj_t *parent)
{
    lv_obj_t *noise = lv_label_create(parent);
    lv_obj_set_style_text_color(noise, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(noise, &lv_font_montserrat_20, LV_PART_MAIN);
    lv_label_set_text(noise, "Mic");
    lv_obj_align(noise, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t *noise_cnt = lv_label_create(parent);
    lv_obj_set_style_text_color(noise_cnt, font_color, LV_PART_MAIN);
    lv_obj_set_style_text_font(noise_cnt, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_label_set_text_fmt(noise_cnt, "%d", 0);
    lv_obj_align_to(noise_cnt, noise, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

    noise_timer =  lv_timer_create([](lv_timer_t *t) {
        lv_obj_t *label =   (lv_obj_t *)t->user_data;

        size_t read_len;

        if (!vad_buff)return ;
        if (amoled.readMicrophone((char *) vad_buff, VAD_BUFFER_LENGTH * sizeof(short), &read_len)) {
            // Feed samples to the VAD process and get the result
#if   ESP_IDF_VERSION_VAL(4,4,1) == ESP_IDF_VERSION
            vad_state_t vad_state = vad_process(vad_inst, vad_buff);
#elif ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4,4,1)
            vad_state_t vad_state = vad_process(vad_inst, vad_buff, MIC_I2S_SAMPLE_RATE, VAD_FRAME_LENGTH_MS);
#else
#error "No support this version."
#endif
            if (vad_state == VAD_SPEECH) {
                lv_label_set_text_fmt(label, "%lu", noise_count++);
            }
        }

    }, 100, noise_cnt);
}
#endif // Version check

static void lv_tileview_add_wifi(lv_obj_t *parent)
{
    static lv_obj_t *label_rssi;
    static lv_obj_t *label;
    static lv_obj_t *meter;
    static lv_meter_indicator_t *indic2 ;
    static lv_meter_indicator_t *indic1 ;

    meter = lv_meter_create(parent);

    /*Remove the background and the circle from the middle*/
    lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
    lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);

    lv_obj_set_size(meter, LV_PCT(50), LV_PCT(50));
    lv_obj_center(meter);

    /*Add a scale first with no ticks.*/
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 0, 0, 0, lv_color_black());
    lv_meter_set_scale_range(meter, scale, -100, 10, 280, 130);

    /*Add a three arc indicator*/
    lv_coord_t indic_w = 15;

    indic1 = lv_meter_add_arc(meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_meter_set_indicator_start_value(meter, indic1, 0);
    lv_meter_set_indicator_end_value(meter, indic1, 100);

    indic2 = lv_meter_add_arc(meter, scale, indic_w, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_meter_set_indicator_start_value(meter, indic2, 0);
    lv_meter_set_indicator_end_value(meter, indic2, 90);


    label_rssi = lv_label_create(parent);
    lv_obj_set_style_text_color(label_rssi, lv_color_white(), 0);
    lv_label_set_text(label_rssi, "0");
    lv_obj_set_style_text_font(label_rssi, &lv_font_montserrat_12, 0);
    lv_obj_center(label_rssi);

    label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "RSSI");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_align_to(label, label_rssi, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_timer_create([](lv_timer_t *t) {

        if (!WiFi.isConnected()) {
            lv_label_set_text(label_rssi, "N/A");
            return;
        }
        int32_t rssi = WiFi.RSSI();

        lv_label_set_text_fmt(label_rssi, "%d",  rssi);

        lv_meter_set_indicator_end_value(meter, indic2, rssi);

    }, 1000, NULL);

}

static void ofs_y_anim(void *img, int32_t v)
{
    lv_img_set_offset_y((lv_obj_t *)img, v);
}

static void ofs_x_anim(void *img, int32_t v)
{
    lv_img_set_offset_x((lv_obj_t *)img, v);
}

typedef void (*offset_cb)(void *img, int32_t v);
static const char *str[] = {"backward", "left", "right", "forward"};
static const void *ptr_img[]  = {&img_down, &img_left, &img_right, &img_up};
static const offset_cb ptr_cb[]  = {ofs_y_anim, ofs_x_anim, ofs_x_anim, ofs_y_anim};

// Just for testing images
static void lv_tileview_add_img(lv_obj_t *parent)
{
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(cont, bg_color, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(cont, LV_DIR_NONE);

    lv_obj_t *img = lv_img_create(cont);
    lv_img_set_src(img, &img_down);
    lv_obj_center(img);

    lv_obj_t *label = lv_label_create(cont);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label, font_color, 0);
    lv_label_set_text(label, str[0]);
    lv_obj_align_to(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    lv_obj_set_user_data(label, img);

    lv_timer_create([](lv_timer_t *t) {

        static int i = 0;

        lv_obj_t *label =   (lv_obj_t *)t->user_data;
        lv_obj_t *img = (lv_obj_t *)lv_obj_get_user_data(label);

        lv_anim_del(img, NULL);

        lv_img_set_src(img, ptr_img[i]);
        lv_obj_center(img);

        lv_img_set_offset_x(img, 0);
        lv_img_set_offset_y(img, 0);

        lv_label_set_text_fmt(label, "%s",  str[i]);
        lv_obj_align_to(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, img);
        lv_anim_set_exec_cb(&a, ptr_cb[i]);

        switch (i) {
        case 0:
            lv_anim_set_values(&a, 0, 100);
            break;
        case 1:
            lv_anim_set_values(&a, 100, 0);
            break;
        case 2:
            lv_anim_set_values(&a, 0, 100);
            break;
        case 3:
            lv_anim_set_values(&a, 100, 0);
            break;
        default:
            break;
        }
        lv_anim_set_time(&a, 3000);
        lv_anim_set_playback_time(&a, 500);
        lv_anim_set_repeat_count(&a, 3000);
        lv_anim_start(&a);
        i++;
        i %= (sizeof(str) / sizeof(str[0]));
    }, 3000, label);
}

static void lv_tileview_add_battery_voltage(lv_obj_t *parent)
{
    static lv_obj_t *label_voltage;
    static lv_obj_t *label;
    static lv_obj_t *meter;
    static lv_meter_indicator_t *indic2 ;
    static lv_meter_indicator_t *indic1 ;

    meter = lv_meter_create(parent);

    /*Remove the background and the circle from the middle*/
    lv_obj_remove_style(meter, NULL, LV_PART_MAIN);
    lv_obj_remove_style(meter, NULL, LV_PART_INDICATOR);

    lv_obj_set_size(meter, LV_PCT(50), LV_PCT(50));
    lv_obj_center(meter);

    /*Add a scale first with no ticks.*/
    lv_meter_scale_t *scale = lv_meter_add_scale(meter);
    lv_meter_set_scale_ticks(meter, scale, 0, 0, 0, lv_color_black());
    lv_meter_set_scale_range(meter, scale, 0, 100, 280, 130);

    /*Add a three arc indicator*/
    lv_coord_t indic_w = 15;

    indic1 = lv_meter_add_arc(meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_meter_set_indicator_start_value(meter, indic1, 0);
    lv_meter_set_indicator_end_value(meter, indic1, 100);

    indic2 = lv_meter_add_arc(meter, scale, indic_w, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_meter_set_indicator_start_value(meter, indic2, 0);
    lv_meter_set_indicator_end_value(meter, indic2, 90);


    label_voltage = lv_label_create(parent);
    lv_obj_set_style_text_color(label_voltage, lv_color_white(), 0);
    lv_label_set_text(label_voltage, "0");
    lv_obj_set_style_text_font(label_voltage, &lv_font_montserrat_12, 0);
    lv_obj_center(label_voltage);

    label = lv_label_create(parent);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_label_set_text(label, "Volts");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_12, 0);
    lv_obj_align_to(label, label_voltage, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    lv_timer_create([](lv_timer_t *t) {

        // Obtain battery voltage, based on ADC reading, there is a certain error
        uint16_t battery_voltage = amoled.getBattVoltage();

        // Calculate battery percentage.
        int percentage = amoled.getBatteryPercent();

        lv_label_set_text_fmt(label_voltage, "%.2f", battery_voltage / 1000.0);

        lv_meter_set_indicator_end_value(meter, indic2, percentage);

    }, 1000, NULL);

}

static void tileview_change_cb(lv_event_t *e)
{
    lv_obj_t *tileview = lv_event_get_target(e);
    uint8_t pageId = lv_obj_get_index(lv_tileview_get_tile_act(tileview));
    lv_event_code_t c = lv_event_get_code(e);
    Serial.print("Code : ");
    Serial.print(c);
    uint32_t count =  lv_obj_get_child_cnt(tileview);
    Serial.print(" Count:");
    Serial.print(count);
    Serial.print(" pageId:");
    Serial.println(pageId);
}

static void lv_gui_select_next_item()
{
    static int id = 0;
    id++;
    id %= TILEVIEW_CNT;
    lv_obj_set_tile_id(tileview, id, 0, LV_ANIM_ON);
}

// void update_touch_label(uint32_t value)
// {
//     lv_label_set_text_fmt(touch_label, "%u", value);
// }


static void lv_gui_init()
{
    // Set the page to all black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // Create display area 126 x 126
    tileview = lv_tileview_create(lv_scr_act());
    lv_obj_set_style_bg_color(tileview, lv_color_black(), 0);
    lv_obj_set_scrollbar_mode(tileview, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(tileview, tileview_change_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Set display window size
    lv_obj_set_size(tileview, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(tileview, LV_ALIGN_BOTTOM_MID, 0, 0);


    // Create the displayed UI
    lv_obj_t *t1 = lv_tileview_add_tile(tileview, 0, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t2 = lv_tileview_add_tile(tileview, 1, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t3 = lv_tileview_add_tile(tileview, 2, 0, LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t4 = lv_tileview_add_tile(tileview, 3, 0,  LV_DIR_HOR | LV_DIR_BOTTOM);
    lv_obj_t *t5 = lv_tileview_add_tile(tileview, 4, 0,  LV_DIR_HOR | LV_DIR_BOTTOM);

    // Create date page
    lv_tileview_add_datetime(t1);

    // Create sensor page
    lv_tileview_add_sensor(t2);

    // Create battery voltage page
    lv_tileview_add_battery_voltage(t3);

    // Create a WiFi quality page
    lv_tileview_add_wifi(t4);

    // Create image page
    lv_tileview_add_img(t5);


    // The esp_vad function is currently only reserved for Arduino version 2.0.9
#if ESP_ARDUINO_VERSION_VAL(2,0,9) == ESP_ARDUINO_VERSION

    lv_obj_t *t6 = lv_tileview_add_tile(tileview, 5, 0,  LV_DIR_HOR | LV_DIR_BOTTOM);

    // Create noise detect page
    lv_tileview_add_noise_detect(t6);

#endif  //Version check


    // touch_label =  lv_label_create(lv_scr_act());
    // lv_obj_set_pos(touch_label, 0, GlassViewable_X_Offset);
    // lv_label_set_text(touch_label, "0");
    // lv_obj_set_style_text_color(touch_label, lv_color_white(), LV_PART_MAIN);
    // lv_obj_set_style_text_font(touch_label, &lv_font_montserrat_14, LV_PART_MAIN);
    // lv_obj_align(touch_label, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

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
