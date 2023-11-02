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
            return;
        }
        ESP_LOGE(TAG, "takeImage failed: %s", R503::errorMsg(ret));
        return;
    }
    ret = fp_sensor.extractFeatures(1);
    if (ret != R503_SUCCESS)
    {
        if (ret == R503_IMAGE_MESSY || ret == R503_FEATURE_FAIL)
        {
            ESP_LOGW(TAG, "extractFeatures failed: Bad quality: %s", R503::errorMsg(ret));
            return;
        }
        ESP_LOGE(TAG, "extractFeatures failed: %s", R503::errorMsg(ret));
        return;
    }
    uint16_t location, score;
    ret = fp_sensor.searchFinger(1, location, score);
    switch(ret)
    {
        case R503_NO_MATCH_IN_LIBRARY: push_action(Action{.type=AT_Fingerprint, .fp_index = -1}); break;
        case R503_SUCCESS: push_action(Action{.type=AT_Fingerprint, .fp_index = location}); break;
        default: ESP_LOGE(TAG, "searchFinger failed: %s", R503::errorMsg(ret));
    }
}

void FGInput::process_cmd(uint32_t color)
{
    if (prev_color == color) return;
    prev_color = color;
    fp_sensor.setAuraLED(color);
}

void FGInput::passivate()
{
    fp_sensor.setAuraLED(auraOff);
    prev_color = -1;
}
