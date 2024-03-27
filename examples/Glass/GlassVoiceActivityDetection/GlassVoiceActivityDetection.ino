/**
 * @file      GlassVoiceActivityDetection.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-02-22
 *
 */
#include <LilyGo_Wristband.h>
#include <LV_Helper.h>

// The displayable area of Glass is 126x126, which requires an offset of 168 pixels.
#define GlassViewable_X_Offset          168
#define GlassViewableWidth              126
#define GlassViewableHeight             126

LilyGo_Class amoled;

// The esp_vad function is currently only reserved for Arduino version 2.0.9
#if ESP_ARDUINO_VERSION_VAL(2,0,9) == ESP_ARDUINO_VERSION

#include <esp_vad.h>

#define VAD_FRAME_LENGTH_MS             30
#define VAD_BUFFER_LENGTH               (VAD_FRAME_LENGTH_MS * MIC_I2S_SAMPLE_RATE / 1000)

static int16_t *vad_buff = NULL;
static vad_handle_t vad_inst;
static uint32_t noise_count;
lv_obj_t *noise_cnt;

void setup(void)
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

    // Using glasses requires setting the screen to flip vertically
    amoled.flipHorizontal(true);

    // Initialize onboard PDM microphone
    amoled.initMicrophone();

    beginLvglHelper(amoled);

    // Set display background color to black
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);

    // Create a display window object
    lv_obj_t *window = lv_obj_create(lv_scr_act());
    // Set window background color to black
    lv_obj_set_style_bg_color(window, lv_color_black(), 0);
    // Set window border width zero
    lv_obj_set_style_border_width(window, 0, 0);
    // Set the display area offset to the glass window
    lv_obj_set_pos(window, 0, GlassViewable_X_Offset);
    // Set display window size
    lv_obj_set_size(window, GlassViewableWidth, GlassViewableHeight);
    // Set window position
    lv_obj_align(window, LV_ALIGN_RIGHT_MID, 0, 0);

    // Create Voice Activity Detection label
    lv_obj_t *noise = lv_label_create(window);
    lv_obj_set_style_text_color(noise, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(noise, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_label_set_text(noise, "Noise count");
    lv_obj_align(noise, LV_ALIGN_CENTER, 0, -10);

    noise_cnt = lv_label_create(window);
    lv_obj_set_style_text_color(noise_cnt, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_text_font(noise_cnt, &lv_font_montserrat_26, LV_PART_MAIN);
    lv_label_set_text_fmt(noise_cnt, "%d", 0);
    lv_obj_align_to(noise_cnt, noise, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

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
        while (1) {
            delay(1000);
        }
    }
}

void loop()
{
    size_t read_len;

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
            Serial.print(millis());
            Serial.println(" Noise detected!!!");
            lv_label_set_text_fmt(noise_cnt, "%lu", noise_count++);
        }
    }
    lv_timer_handler();
    delay(5);
}

#else


#include <Arduino.h>

void setup()
{
    Serial.begin(115200);
}

void loop()
{
    Serial.println("The currently used version does not support esp_vad and cannot run the noise detection function. If you need to use esp_vad, please change the version to 2.0.9");
    delay(1000);
}


#endif