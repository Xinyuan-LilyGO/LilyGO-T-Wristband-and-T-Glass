/**
 * @file      WristbandLightSleep.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 * @note      Serial will be invalid during light sleep. To re-upload the sketch, the board needs to be put into download mode to download.
 *            To re-upload the sketch, you need to put the board into download mode to download. How to enter download mode, please see the FAQ section in the README.
 *            For sleep documentation, please refer to: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

LilyGo_Class amoled;

// Create a window into which all elements are loaded
lv_obj_t *window;

// The displayable area of Wristband is 126x250, which requires an offset of 44 pixels.
#define WindowViewableWidth              126
#define WindowViewableHeight             250


//Time-to-Sleep
#define uS_TO_MS_FACTOR 1000ULL    /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP   10           /* Time ESP32 will go to sleep (in microseconds); multiplied by above conversion to achieve seconds*/




void periodic_update_display(lv_timer_t *timer)
{
    lv_obj_t *label = (lv_obj_t *)timer->user_data;
    String millis_str = String(millis() / 1000);           /*Be realistic about something*/
    lv_label_set_text(label, millis_str.c_str());          /*Set label text*/
    lv_obj_center(label);                                  /*Set center alignment*/

}


/*
* @note      Serial will be invalid during light sleep. To re-upload the sketch, the board needs to be put into download mode to download.
*            To re-upload the sketch, you need to put the board into download mode to download. How to enter download mode, please see the FAQ section in the README.
*            For sleep documentation, please refer to: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html
* */
void setup()
{
    // Turn on debugging message output, Arduino IDE users please put
    // Tools -> USB CDC On Boot -> Enable, otherwise there will be no output
    Serial.begin(115200);

    // Initialization screen and peripherals
    bool rslt = amoled.begin();
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


    lv_obj_t *label = lv_label_create(window);        /*Add a label the current screen*/
    lv_label_set_text(label, "Light Sleep");                /*Set label text*/
    lv_obj_center(label);                                   /*Set center alignment*/

    // Refresh the screen periodically
    lv_timer_create(periodic_update_display, 1000, label);
}



void loop()
{
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();

    // Set the light sleep time, here set the time to 10ms
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_MS_FACTOR);

    /*
    * @note      Serial will be invalid during light sleep. To re-upload the sketch, the board needs to be put into download mode to download.
    *            To re-upload the sketch, you need to put the board into download mode to download. How to enter download mode, please see the FAQ section in the README.
    *            For sleep documentation, please refer to: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/system/sleep_modes.html
    * */
    esp_light_sleep_start();
}






