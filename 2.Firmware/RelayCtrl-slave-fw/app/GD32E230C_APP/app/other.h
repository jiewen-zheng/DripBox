#ifndef __OTHER_H
#define __OTHER_H

#include <stdint.h>
#include <stdbool.h>

#define OFFSET_ADDR (GD32E23X_FLASH_BASE_ADDR + 60 * GD32E23X_FLASH_PAGE_SIZE)

enum
{
    X_OFFSET = 0,
    Y_OFFSET,
    Z_OFFSET,
};
typedef uint8_t AxleType_t;

typedef struct
{
    uint16_t flag;
    uint8_t xSetFlag;
    uint8_t ySetFlag;
    uint8_t zSetFlag;
    int x;
    int y;
    int z;
} OffSet_t;

typedef union
{
    OffSet_t offset;
    uint32_t data[17];
} OffSet_union_t;

bool load_offset();
bool save_offset(OffSet_t *offset);
void set_offset(AxleType_t axle, int value);
OffSet_t *get_offset();

void other_handle();
#endif
