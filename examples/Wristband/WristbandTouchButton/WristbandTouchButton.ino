/**
 * @file      WristbandTouchButton.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

LilyGo_Class amoled;

// Create a window into which all elements are loaded
lv_obj_t *window;

// The displayable area of Wristband is 126x250, which requires an offset of 44 pixels.
#define WindowViewableWidth              126
#define WindowViewableHeight             250

lv_obj_t *btn_value;
uint32_t inter_value;

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

    // Create button state label
    btn_value = lv_label_create(window);
    lv_obj_set_style_text_color(btn_value, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(btn_value, &lv_font_montserrat_22, LV_PART_MAIN);
    lv_label_set_text(btn_value, "0000");
    lv_obj_align(btn_value, LV_ALIGN_CENTER, 0, 0);

    // Remove the touch button interrupt
    touchDetachInterrupt(BOARD_TOUCH_BUTTON);
}


void loop()
{
    if (millis() > inter_value) {
        // Read the touch capacitance value
        touch_value_t value = touchRead(BOARD_TOUCH_BUTTON);
        Serial.println(value);
        lv_label_set_text_fmt(btn_value, "%u", value);
        lv_obj_align(btn_value, LV_ALIGN_CENTER, 0, 0);
        inter_value = millis() + 100;
    }
    lv_timer_handler();
    delay(5);
}
