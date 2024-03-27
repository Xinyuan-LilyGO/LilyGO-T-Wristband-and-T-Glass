/**
 * @file      WristbandDisplayRotation.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-23
 * @note      The JD9613 display RAM is only 1/2 the screen size and does not support rotation.
 *            Directions 0 and 1 can adjust the vertical direction, horizontal 1 and 3 are the same direction.
 *            The sketch tests whether the screen boundaries are normal ，
 *            The actual display area is not full screen. Due to the shell, the display area is a little smaller than the screen.
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

LilyGo_Class amoled;

// Create a window into which all elements are loaded
lv_obj_t *window;

// The displayable area of Wristband is 126x250, which requires an offset of 44 pixels.
#define WindowViewableWidth              126
#define WindowViewableHeight             250

void button_event_callback(ButtonState state)
{

    // amoled.vibration();

    switch (state) {
    case BTN_CLICK_EVENT: {
        lv_disp_drv_t *drv = lv_disp_get_default()->driver;
        uint8_t r = amoled.getRotation();
        r++;
        r %= 4;
        amoled.setRotation(r);
        drv->hor_res = amoled.width();
        drv->ver_res = amoled.height();

        switch (r) {
        case 0:
            // Set display window size
            lv_obj_set_size(window, WindowViewableWidth, WindowViewableHeight);
            // Set window position
            lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);
            break;
        case 1:
        case 3:
            // Set display window size
            lv_obj_set_size(window, WindowViewableHeight, WindowViewableWidth);
            // Set window position
            lv_obj_align(window, LV_ALIGN_RIGHT_MID, 0, 0);
            break;
        case 2:
            // Set display window size
            lv_obj_set_size(window, WindowViewableWidth, WindowViewableHeight);
            // Set window position
            lv_obj_align(window, LV_ALIGN_TOP_MID, 0, 0);
            break;
        default:
            break;
        }

        lv_disp_drv_update(lv_disp_get_default(), drv);
    }
    break;
    }
}

/*
 * @note      The JD9613 display RAM is only 1/2 the screen size and does not support rotation.
 *            Directions 0 and 1 can adjust the vertical direction, horizontal 1 and 3 are the same direction.
 *            The sketch tests whether the screen boundaries are normal ，
 *            The actual display area is not full screen. Due to the shell, the display area is a little smaller than the screen.
* */
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

    // Set the default touch threshold. Depending on the contact surface, the touch sensitivity may need to be adjusted.
    // Capacitance difference threshold You can use examples/GlassTouchButton to read the capacitance value and calculate the actual difference value
    amoled.setTouchThreshold(200);

    // Set touch button callback function , Pressing the touch will perform a rotation test
    amoled.setEventCallback(button_event_callback);


    // Set the page to all black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
    // Create a display window object
    window = lv_obj_create(lv_scr_act());
    // Set window pad all zero
    lv_obj_set_style_pad_all(window, 0, 0);
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set display window size
    lv_obj_set_size(window, WindowViewableWidth, WindowViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);

    // Test boundaries
    lv_align_t  align[] = {LV_ALIGN_TOP_LEFT,
                           LV_ALIGN_TOP_MID,
                           LV_ALIGN_TOP_RIGHT,
                           LV_ALIGN_BOTTOM_LEFT,
                           LV_ALIGN_BOTTOM_MID,
                           LV_ALIGN_BOTTOM_RIGHT,
                           LV_ALIGN_LEFT_MID,
                           LV_ALIGN_RIGHT_MID
                          };
    for (int i = 0; i < sizeof(align) / sizeof(*align); ++i) {
        lv_obj_t *btn = lv_btn_create(window);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text_fmt(label, "%d", i);
        lv_obj_center(btn);
        lv_obj_align(btn, align[i], 0, 0);
    }
}



void loop()
{
    // Check button state
    amoled.update();
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}






