/**
 * @file      WristbandRtcDateTime.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-23
 *
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

// Create a window into which all elements are loaded
lv_obj_t *window;

// The displayable area of Wristband is 126x250, which requires an offset of 44 pixels.
#define WindowViewableWidth              126
#define WindowViewableHeight             250


LilyGo_Class amoled;
lv_obj_t *label_date;
lv_obj_t *label_time;


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
        Serial.println(buf);
        lv_label_set_text_fmt(label_time, "%s", buf);
    }

    written = strftime(buf, 64, "%Y %m %d", &timeinfo);
    if (written != 0) {
        Serial.println(buf);
        lv_label_set_text_fmt(label_date, "%s", buf);
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


    // Create a timer to update date time once a second
    lv_timer_create(update_datetime, 1000, NULL);

}



void loop()
{
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}






