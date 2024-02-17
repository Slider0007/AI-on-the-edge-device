#ifndef OV2640_SHARPNESS_H
#define OV2640_SHARPNESS_H

#include "esp_camera.h"

bool ov2640_enable_auto_sharpness(sensor_t *_sensor);
bool ov2640_set_sharpness(sensor_t *_sensor, int _sharpness); // -3 to +3, -4 for auto-sharpness

#endif
