#include "ov2640_sharpness.h"

#include <stdint.h>


#define OV2640_MAXLEVEL_SHARPNESS 6

const static uint8_t OV2640_SHARPNESS_AUTO[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0x20, 0x20,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_MANUAL[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0x00, 0x20,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL0[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xc0, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL1[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xc1, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL2[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xc2, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL3[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xc4, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL4[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xc8, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL5[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xd0, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t OV2640_SHARPNESS_LEVEL6[]=
{
    0xFF, 0x00, 0xff,
    0x92, 0x01, 0xff,
    0x93, 0xdf, 0x1f,
    0x00, 0x00, 0x00
};

const static uint8_t *OV2640_SETTING_SHARPNESS[]=
{
    OV2640_SHARPNESS_LEVEL0, // -3 sharpness
    OV2640_SHARPNESS_LEVEL1,
    OV2640_SHARPNESS_LEVEL2,
    OV2640_SHARPNESS_LEVEL3,
    OV2640_SHARPNESS_LEVEL4,
    OV2640_SHARPNESS_LEVEL5,
    OV2640_SHARPNESS_LEVEL6  // +3 sharpness
};


static bool table_mask_write(sensor_t *_sensor, const uint8_t* _ptab)
{
    uint8_t address;
    uint8_t value;
    uint8_t orgval;
    uint8_t mask;
    const uint8_t *pdata = _ptab;

    if (pdata == NULL)
        return false;

    while (1)
    {   
        address = *pdata++;
        value = *pdata++;
        mask = *pdata++;
        if ((address == 0) && (value == 0) && (mask == 0))
            break;
        _sensor->set_reg(_sensor, address, mask, value);
    }   

    return true;
}


bool ov2640_enable_auto_sharpness(sensor_t *_sensor)
{
    return table_mask_write(_sensor, OV2640_SHARPNESS_AUTO);
}


bool ov2640_set_sharpness(sensor_t *_sensor, int _sharpness)
{
    if ((_sharpness < -3) || (_sharpness > OV2640_MAXLEVEL_SHARPNESS - 3))
        return false;

    bool retval = false;
    retval = table_mask_write(_sensor, OV2640_SHARPNESS_MANUAL);
    retval = table_mask_write(_sensor, OV2640_SETTING_SHARPNESS[_sharpness + 3]);

    return retval;
}
