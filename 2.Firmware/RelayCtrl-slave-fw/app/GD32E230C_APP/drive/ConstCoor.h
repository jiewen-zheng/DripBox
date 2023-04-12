
#ifndef __ConstCoor_H__
#define __ConstCoor_H__
#include <stdint.h>

//#define MedicineOut_1_len 25 - 2
//#define MedicineOut_2_len 25 - 2
//#define MedicineOut_3_len 30 - 2
//#define MedicineOut_4_len 30 - 2

//#define MedicineOut_5_len 30 - 2
//#define MedicineOut_6_len 176
//#define MedicineOut_7_len 16
//#define MedicineOut_8_len 142

//#define MedicineOut_9_len 72 - 2
//#define MedicineOut_10_len 108 - 2
//#define MedicineOut_11_len 144 - 2
//#define MedicineOut_12_len 178 - 2

//#define MedicineOut_13_len 72 - 2
//#define MedicineOut_14_len 108 - 2
//#define MedicineOut_15_len 144 - 2
//#define MedicineOut_16_len 180 - 2

//#define MedicineOut_17_len 9
//#define MedicineOut_18_len 9
//#define MedicineOut_19_len 9
//#define MedicineOut_20_len 9

//#define MedicineOut_21_len 9
//#define MedicineOut_22_len 9
//#define MedicineOut_23_len 9
//#define MedicineOut_24_len 9

//#define MedicineOut_25_len 6

//#define MedicineOut_26_len 22
//#define MedicineOut_27_len 22
//#define MedicineOut_28_len 16
//#define MedicineOut_29_len 16
//#define MedicineOut_30_len 68
//#define MedicineOut_31_len 106
//#define MedicineOut_32_len 142
//#define MedicineOut_33_len 176
//#define MedicineOut_34_len 68
//#define MedicineOut_35_len 106
//#define MedicineOut_36_len 142
//#define MedicineOut_37_len 176

//extern const int16_t MedicineOut_1[];
//extern const int16_t MedicineOut_2[];
//extern const int16_t MedicineOut_3[];
//extern const int16_t MedicineOut_4[];
//extern const int16_t MedicineOut_5[];
//extern const int16_t MedicineOut_6[];
//extern const int16_t MedicineOut_7[];
//extern const int16_t MedicineOut_8[];
//extern const int16_t MedicineOut_9[];
//extern const int16_t MedicineOut_10[];
//extern const int16_t MedicineOut_11[];
//extern const int16_t MedicineOut_12[];

//extern const int16_t MedicineOut_13[];
//extern const int16_t MedicineOut_14[];
//extern const int16_t MedicineOut_15[];
//extern const int16_t MedicineOut_16[];
//extern const int16_t MedicineOut_17[];
//extern const int16_t MedicineOut_18[];
//extern const int16_t MedicineOut_19[];
//extern const int16_t MedicineOut_20[];
//extern const int16_t MedicineOut_21[];
//extern const int16_t MedicineOut_22[];
//extern const int16_t MedicineOut_23[];
//extern const int16_t MedicineOut_24[];

//extern const int16_t MedicineOut_25[];

//extern const int16_t MedicineOut_26[];
//extern const int16_t MedicineOut_27[];
//extern const int16_t MedicineOut_28[];
//extern const int16_t MedicineOut_29[];
//extern const int16_t MedicineOut_30[];
//extern const int16_t MedicineOut_31[];
//extern const int16_t MedicineOut_32[];
//extern const int16_t MedicineOut_33[];
//extern const int16_t MedicineOut_34[];
//extern const int16_t MedicineOut_35[];
//extern const int16_t MedicineOut_36[];
//extern const int16_t MedicineOut_37[];

/**********************************************************************************************/


#define ARRAY_SIZE(p) (sizeof(p) / sizeof(*p) / 8)
	

#define test_point_len 6
extern const int16_t test_point[];

#define empty_point_len 1
extern const int16_t empty_point[];

extern const uint32_t basic_data_point[12 * 2];
extern const uint32_t drop_data_point[24 * 2];

#endif
