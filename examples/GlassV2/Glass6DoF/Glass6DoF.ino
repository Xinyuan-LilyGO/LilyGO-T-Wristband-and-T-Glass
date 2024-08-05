/**
 * @file      Glass6DoF.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 *
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>
#include <cbuf.h>

// The resolution of the non-magnified side of the glasses reflection area is about 126x126, 
// and the magnified area is smaller than 126x126
#define GlassViewableWidth              126
#define GlassViewableHeight             126

LilyGo_Class amoled;

cbuf accel_data_buffer(1);
cbuf gyro_data_buffer(1);
lv_obj_t *label;
uint32_t inter_value = 0;

static void gyro_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len);
static void accel_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len);

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

    // Initialize Sensor
    accel_data_buffer.resize(sizeof(struct bhy2_data_xyz) * 2);
    gyro_data_buffer.resize(sizeof(struct bhy2_data_xyz) * 2);

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


    // Set the page to all black
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
    lv_obj_align(window, LV_ALIGN_BOTTOM_MID, 0, 0);

    label = lv_label_create(window);                  /*Add a label the current screen*/
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(label, "N/A");                  /*Set label text*/
    lv_obj_center(label);                             /*Set center alignment*/
}

void loop()
{
    if (millis() > inter_value) {
        // Read data from buffer
        struct bhy2_data_xyz gyr;
        gyro_data_buffer.read(( char *)&gyr, sizeof(gyr));

        struct bhy2_data_xyz acc;
        accel_data_buffer.read(( char *)&acc, sizeof(acc));

        float accel_scaling_factor = get_sensor_default_scaling(BHY2_SENSOR_ID_ACC_PASS);
        float gyro_scaling_factor = get_sensor_default_scaling(BHY2_SENSOR_ID_GYRO_PASS);

        // update the filter, which computes orientation
        Serial.printf("Gyro: X:% 3.2f Y:% 3.2f Z:% 3.2f Accel: X:% 3.2f Y:% 3.2f Z:% 3.2f \n",
                      gyr.x * gyro_scaling_factor,
                      gyr.y * gyro_scaling_factor,
                      gyr.z * gyro_scaling_factor,
                      acc.x * accel_scaling_factor,
                      acc.y * accel_scaling_factor,
                      acc.z * accel_scaling_factor
                     );

        lv_label_set_text_fmt(label,
                              "Gyro:\nX:% 3.2f\nY:% 3.2f\nZ:% 3.2f\nAccel: \nX:% 3.2f\nY:% 3.2f\nZ:% 3.2f",
                              gyr.x * gyro_scaling_factor,
                              gyr.y * gyro_scaling_factor,
                              gyr.z * gyro_scaling_factor,
                              acc.x * accel_scaling_factor,
                              acc.y * accel_scaling_factor,
                              acc.z * accel_scaling_factor
                             );

        inter_value = millis() + 100;
    }


    // Update 6-axis sensor and button state
    amoled.update();
    // lvgl task processing should be placed in the loop function
    lv_timer_handler();
    delay(2);
}





static void accel_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len)
{
    struct bhy2_data_xyz data;
    bhy2_parse_xyz(data_ptr, &data);
    accel_data_buffer.write((const char *)&data, sizeof(data));
}


static void gyro_process_callback(uint8_t sensor_id, uint8_t *data_ptr, uint32_t len)
{
    struct bhy2_data_xyz data;
    bhy2_parse_xyz(data_ptr, &data);
    gyro_data_buffer.write((const char *)&data, sizeof(data));
}

