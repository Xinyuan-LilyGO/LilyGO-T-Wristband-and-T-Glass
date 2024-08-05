/**
 * @file      GlassBatteryVoltage.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-23
 *
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

// The resolution of the non-magnified side of the glasses reflection area is about 126x126, 
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126


LilyGo_Class amoled;
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

    // Using glasses requires setting the screen to flip vertically
    amoled.flipHorizontal(true);

    // Brightness range : 0 ~ 255
    amoled.setBrightness(255);

    // Initialize lvgl
    beginLvglHelper(amoled);

    // Set display background color to black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // Create a display window object
    lv_obj_t *window = lv_obj_create(lv_scr_act());
    // Set window background color to black
    lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_RIGHT_MID, 0, 0);


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






