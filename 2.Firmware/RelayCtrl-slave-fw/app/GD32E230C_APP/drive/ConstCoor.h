#ifndef __ConstCoor_H__
#define __ConstCoor_H__
#include <stdint.h>
#include <string.h>

#define ARRAY_SIZE(p) (sizeof(p) / sizeof(*p) / 8)



#define MedicineOut_99_len 6
extern const int16_t MedicineOut_99[];

extern const uint32_t TrackData[17 * 2];

#endif
