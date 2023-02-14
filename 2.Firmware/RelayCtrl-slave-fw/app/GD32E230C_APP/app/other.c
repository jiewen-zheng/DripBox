#include "other.h"
#include "gd32e23x_flash.h"

#include "motor_ctrl.h"

#include <string.h>

static OffSet_t offset_config = {0};

bool load_offset()
{
    OffSet_union_t offSet_union;
    OffSet_t *offset = &offset_config;

    uint16_t *ptr = (uint16_t *)OFFSET_ADDR;
    if (*ptr != 0xABBA)
    {
        offset->x = 0;
        offset->y = 0;
        offset->z = 0;
        return false;
    }

    memset(offset, 0, sizeof(OffSet_t));

    memcpy(&offSet_union, ptr, sizeof(OffSet_union_t));
    memcpy(offset, &offSet_union.offset, sizeof(OffSet_t));

    if (!offset->xSetFlag)
        offset->x = 0;
    if (!offset->ySetFlag)
        offset->y = 0;
    if (!offset->zSetFlag)
        offset->z = 0;

    return true;
}

bool save_offset(OffSet_t *offset)
{
    if (offset == NULL)
        return -1;

    OffSet_union_t offSet_union;
    memset(&offSet_union, 0, sizeof(OffSet_union_t));
    memcpy(&offSet_union.offset, offset, sizeof(OffSet_t));

    erase_flash_page(OFFSET_ADDR, OFFSET_ADDR + 1);
    if (!program_flash(OFFSET_ADDR, (uint32_t *)&offSet_union, sizeof(OffSet_union_t)))
    {
        return false;
    }

    return false;
}

void set_offset(AxleType_t axle, int value)
{
    OffSet_t *offset = &offset_config;

    offset->flag = 0xABBA;

    switch (axle)
    {
    case X_OFFSET:
        offset->xSetFlag = 1;
        offset->x = value;
        break;

    case Y_OFFSET:
        offset->ySetFlag = 1;
        offset->y = value;
        break;

    case Z_OFFSET:
        offset->zSetFlag = 1;
        offset->z = value;
        break;

    default:
        break;
    }

    save_offset(offset);
}

OffSet_t *get_offset()
{
    return &offset_config;
}

void device_state_update(DeviceStatus_t *state)
{
    // state->sys_state = 0;
    state->progress = get_execution_schedule();
    // state->uv_light = 0;
    // state->water_pump = 0;
    // state->stop_state = 0;
    state->x_zero = INT_key_X();
    state->y_zero = INT_key_Y();
    state->z_zero = INT_key_Z();
    state->water_too_low = INT_normalSalineLow();
}

void other_handle()
{
    low_water_alarm();
    stop_key_handle(false);

    device_state_update(&ReturnData);
}