/**
 * @file      WristbandBatteryVoltage.ino
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

lv_obj_t *label_voltage;


void read_battery_voltage(lv_timer_t *t)
{

    // Obtain battery voltage, based on ADC reading, there is a certain error
    uint16_t battery_voltage = amoled.getBattVoltage();

    // Calculate battery percentage.
    int percentage = amoled.getBatteryPercent();

    lv_label_set_text_fmt(label_voltage, "Volts: %.2f\n   %d%%", battery_voltage / 1000.0, percentage);

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
    label_voltage = lv_label_create(window);        /*Add a label the current screen*/
    lv_obj_set_style_text_color(label_voltage, lv_color_white(), LV_PART_MAIN); // Set text color white
    lv_label_set_text(label_voltage, "0000");                 /*Set label text*/
    lv_obj_center(label_voltage);                             /*Set center alignment*/


    // Create a timer to update battery information once a second
    lv_timer_create(read_battery_voltage, 1000, NULL);

}



void loop()
{
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}






