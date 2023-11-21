#include "common.h"
#include "input_hw_fg.h"
#include "hadrware.h"
#include "activity.h"

const char* TAG = "FGInput";

void FGInput::press()
{
    vTaskDelay(2);
    int ret = fp_sensor.takeImage();
    if (ret != R503_SUCCESS) 
    {
        if (ret == R503_NO_FINGER)
        {
            ESP_LOGW(TAG, "takeImage failed: Finger gone");
            push_action(Action{.type=AT_Fingerprint, .fp_index = -1, .fp_score = uint16_t(ret), .fp_error = "Палец убрали слишком быстро"});
            return;
        }
        ESP_LOGE(TAG, "takeImage failed: %s", R503::errorMsg(ret));
        return;
    }
    ret = fp_sensor.extractFeatures(fp_sensor.active_page);
    if (ret != R503_SUCCESS)
    {
        if (ret == R503_IMAGE_MESSY || ret == R503_FEATURE_FAIL)
        {
            ESP_LOGW(TAG, "extractFeatures failed: Bad quality: %s", R503::errorMsg(ret));
            push_action(Action{.type=AT_Fingerprint, .fp_index = -1, .fp_score = uint16_t(ret), .fp_error= ret == R503_IMAGE_MESSY ? "Плохое качество отпечатка" : "Слишком маленький отпечаток"});
            return;
        }
        ESP_LOGE(TAG, "extractFeatures failed: %s", R503::errorMsg(ret));
        return;
    }
    uint16_t location, score;
    ret = fp_sensor.searchFinger(fp_sensor.active_page, location, score);
    switch(ret)
    {
        case R503_NO_MATCH_IN_LIBRARY: push_action(Action{.type=AT_Fingerprint, .fp_index = -1, .fp_score = uint16_t(ret), .fp_error = "Нет такого отпечатка"}); break;
        case R503_SUCCESS: push_action(Action{.type=AT_Fingerprint, .fp_index = int16_t(location), .fp_score = score}); break;
        default: ESP_LOGE(TAG, "searchFinger failed: %s", R503::errorMsg(ret));
    }
}

void FGInput::process_input() 
{
    press();     
    fp_sensor.setAuraLED(prev_color);
    ei();
}

void FGInput::process_cmd(uint32_t color)
{
    if (prev_color == color) return;
    prev_color = color;
    fp_sensor.setAuraLED(color);
    ei();
}

void FGInput::passivate()
{
    fp_sensor.setAuraLED(auraOff);
    prev_color = -1;
    di();
}
