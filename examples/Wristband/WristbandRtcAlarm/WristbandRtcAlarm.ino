/**
 * @file      WristbandRtcAlarm.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-23
 *
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

LilyGo_Class amoled;


// Create a window into which all elements are loaded
lv_obj_t *window;

// The displayable area of Wristband is 126x250, which requires an offset of 44 pixels.
#define WindowViewableWidth              126
#define WindowViewableHeight             250


lv_obj_t *label_date;
lv_obj_t *label_time;
bool is_rtc_trigger = false;

void update_datetime(lv_timer_t *t)
{
    char buf[64] = {0};
    struct tm timeinfo;

    // Get the time C library structure
    amoled.getDateTime(&timeinfo);

    // Format the output using the strftime function
    // For more formats, please refer to :
    // https://man7.org/linux/man-pages/man3/strftime.3.html

    size_t written = strftime(buf, 64, "%H:%M:%S", &timeinfo);

    if (written != 0) {
        // Serial.println(buf);
        lv_label_set_text_fmt(label_time, "%s", buf);
    }

    written = strftime(buf, 64, "%Y %m %d", &timeinfo);
    if (written != 0) {
        // Serial.println(buf);
        lv_label_set_text_fmt(label_date, "%s", buf);
    }
}

void rtc_alarm_callback(void *arg)
{
    // Set interrupt flag
    is_rtc_trigger = true;
}


void flash_red_color(lv_timer_t *t)
{
    static int cnt = 0;
    // Flashing background color as alarm signal
    if ((cnt % 2) == 0) {
        lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    } else {
        lv_obj_set_style_bg_color(window, lv_color_make(255, 0, 0), 0);
    }
    if (cnt >= 4) {
        lv_timer_del(t);
        cnt = 0;
        return;
    }
    cnt++;
}


void rtc_set_alarm()
{
    amoled.resetAlarm();

    // An interrupt will be triggered whenever 30 seconds is reached
    amoled.setAlarmBySecond(30);

    // An interrupt will be triggered whenever 1 hour is reached
    // amoled.setAlarmByHours(1);

    // An interrupt will be triggered every time 1 minute is reached
    // amoled.setAlarmByMinutes(1);

    // Whenever the first of every month is reached, an interrupt will be triggered.
    // amoled.setAlarmByDays(1);

    // Set departure time manually
    // RTC_Alarm alarm_time(/*Hour*/11,/*Minutes*/ 20,/*Second*/ 30,/*day*/ 30,/*week*/ 1);
    // amoled.setAlarm(alarm_time);
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

    // Brightness range : 0 ~ 255
    amoled.setBrightness(255);

    // Initialize lvgl
    beginLvglHelper(amoled);

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

    // Create label
    label_date = lv_label_create(window);        /*Add a label the current screen*/
    lv_obj_set_style_text_color(label_date, lv_color_white(), LV_PART_MAIN); // Set text color white
    lv_label_set_text(label_date, "0000");                 /*Set label text*/
    lv_obj_center(label_date);                             /*Set  alignment*/

    label_time = lv_label_create(window);       /*Add a label the current screen*/
    lv_obj_set_style_text_color(label_time, lv_color_white(), LV_PART_MAIN); // Set text color white
    lv_label_set_text(label_time, "0000");                 /*Set label text*/
    lv_obj_align_to(label_time, label_date, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);         /*Set  alignment*/


    // Set datetime manually
    // amoled.setDateTime(2024, 2, 23, 0, 0, 0);

    // Use compile datetime
    amoled.setDateTime(RTC_DateTime(__DATE__, __TIME__));

    // Set alarm processing callback
    amoled.attachRTC(rtc_alarm_callback);

    // Set alarm
    rtc_set_alarm();

    // Enable alarm
    amoled.enableAlarm();

    // Create a timer to update date time once a second
    lv_timer_create(update_datetime, 1000, NULL);

}


void loop()
{
    // Check alarm flag
    if (is_rtc_trigger) {
        is_rtc_trigger = false;
        Serial.print("Trigger RTC Interrupt : ");
        Serial.println(amoled.strftime());

        // Create a timer as a flash reminder
        lv_timer_create(flash_red_color, 500, NULL);

        // Set alarm
        rtc_set_alarm();
    }

    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}






