#ifndef __MOTOR_DRIVE_H
#define __MOTOR_DRIVE_H
#include "gd32e23x.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define MinSpeedUs 100
#define MaxPlusX 30000
#define MaxPlusY 30000
#define MaxPlusZ 198480

// IN
#define ReadY0 gpio_input_bit_get(GPIOA, GPIO_PIN_0)
#define ReadX0 gpio_input_bit_get(GPIOA, GPIO_PIN_1)
#define ReadZ0 gpio_input_bit_get(GPIOA, GPIO_PIN_8)
#define ReadStopKey() gpio_input_bit_get(GPIOA, GPIO_PIN_11)
#define ReadSenser() (gpio_input_bit_get(GPIOA, GPIO_PIN_12))
// OUT
#define UV_LED_H gpio_bit_set(GPIOB, GPIO_PIN_0)
#define KEY_LED_H gpio_bit_set(GPIOB, GPIO_PIN_1)
#define BEEP_H gpio_bit_set(GPIOB, GPIO_PIN_2)
#define MOTOR_BI_H gpio_bit_set(GPIOB, GPIO_PIN_4)
#define MOTOR_FI_H gpio_bit_set(GPIOB, GPIO_PIN_5)
#define MOTOR_EN_H gpio_bit_set(GPIOB, GPIO_PIN_6)
#define MOTORX_CLK_H gpio_bit_set(GPIOB, GPIO_PIN_7)
#define MOTORX_STEP_H gpio_bit_set(GPIOB, GPIO_PIN_8)
#define MOTORX_DIR_H gpio_bit_set(GPIOB, GPIO_PIN_9)
#define MOTORY_CLK_H gpio_bit_set(GPIOB, GPIO_PIN_10)
#define MOTORY_STEP_H gpio_bit_set(GPIOB, GPIO_PIN_11)
#define MOTORY_DIR_H gpio_bit_set(GPIOB, GPIO_PIN_12)
#define MOTORZ_CLK_H gpio_bit_set(GPIOB, GPIO_PIN_13)
#define MOTORZ_STEP_H gpio_bit_set(GPIOB, GPIO_PIN_14)
#define MOTORZ_DIR_H gpio_bit_set(GPIOB, GPIO_PIN_15)

#define UV_LED_L gpio_bit_reset(GPIOB, GPIO_PIN_0)
#define KEY_LED_L gpio_bit_reset(GPIOB, GPIO_PIN_1)
#define BEEP_L gpio_bit_reset(GPIOB, GPIO_PIN_2)
#define MOTOR_BI_L gpio_bit_reset(GPIOB, GPIO_PIN_4)
#define MOTOR_FI_L gpio_bit_reset(GPIOB, GPIO_PIN_5)
#define MOTOR_EN_L gpio_bit_reset(GPIOB, GPIO_PIN_6)
#define MOTORX_CLK_L gpio_bit_reset(GPIOB, GPIO_PIN_7)
#define MOTORX_STEP_L gpio_bit_reset(GPIOB, GPIO_PIN_8)
#define MOTORX_DIR_L gpio_bit_reset(GPIOB, GPIO_PIN_9)
#define MOTORY_CLK_L gpio_bit_reset(GPIOB, GPIO_PIN_10)
#define MOTORY_STEP_L gpio_bit_reset(GPIOB, GPIO_PIN_11)
#define MOTORY_DIR_L gpio_bit_reset(GPIOB, GPIO_PIN_12)
#define MOTORZ_CLK_L gpio_bit_reset(GPIOB, GPIO_PIN_13)
#define MOTORZ_STEP_L gpio_bit_reset(GPIOB, GPIO_PIN_14)
#define MOTORZ_DIR_L gpio_bit_reset(GPIOB, GPIO_PIN_15)

typedef struct
{
	int32_t setCoor[3];		 // 目标坐标
	int32_t itCoor[3];		 // 当前坐标
	uint8_t itStop[3];		 // 当前是否在运动0静止，1运动
	uint8_t zeroGotoMode[3]; // 归模式，1为归零模式。0为匀速运行模式
	uint8_t zeroKeyTir[3];	 // 零点按键，0没触发，1触发
	uint16_t StepDelayUsSet[3];
	uint16_t StepDelayUsCount[3];
	uint8_t motorM_DirUpSet[4]; // 方向调整
	int8_t DCMOTE_PWM;			// 水泵PWM  -100~100
	int8_t DCMOTE_PWMcount;		// 水泵PWM  -100~100
	uint8_t Error;				// 错误信号
	int32_t itOutStep[3];		// 当前输出脉冲状态

	uint8_t SetDoutNaber;	  // 模式 0 单点移动， ff 归零， 1 排空， 2基础夜， 3 滴液
	uint32_t SetDoutConut;	  // 当前读取点位
	uint32_t SetDoutConutAll; // 总点位
	int16_t *SetDoutBuff;	  // 点位数据地址

	uint8_t oneCoorEn;	 // 单点移动标记
	int32_t oneCoor[4];	 // 移动脉冲值
	int32_t oneSpeed[4]; // 移动速度
	uint8_t NewCom;		 // 有新指令
} MotorDataType;

enum
{
	STATE_STOP_ZERO = 0,
	STATE_STOP = 1,
	STATE_GOTOZERO = 2,
	STATE_EMPTY = 3,
	STATE_BASIC = 4,
	STATE_DROP = 5,
	STATE_POINT_MOVE = 6,
};
typedef uint8_t RunState_t;

typedef struct
{
	uint8_t zero_stage;	   // goto zero stage
	uint8_t run_state;	   // running mode: 0 stop zero, 1 stop not zero, 2 go to zero, 3 empot, 4 basic, 5 drop
	uint8_t progress;	   // run schedule 0~100
	uint8_t uv_light;	   // "uv" light switch state (0 close, 1 open)
	uint8_t water_pump;	   // water pump state(0 close, 1 open)
	uint8_t stop_state;	   // stop switch state (0 close, 1 open)
	uint8_t x_zero;		   // "x" make zero switch (0 not zero, 1 zero)
	uint8_t y_zero;		   // "y" make zero switch (0 not zero, 1 zero)
	uint8_t z_zero;		   // "z" make zero switch (0 not zero, 1 zero)
	uint8_t water_too_low; // basic liquid too little(0 not low, 1 low)
	uint8_t error;		   // run error
} DeviceStatus_t;

extern MotorDataType MotorData;

void device_params_init();
void con_io_init(void);
void motor_Tim_Init(void);
void Mote_SetCoor(uint8_t MoterNaber, int32_t Coor, int32_t Speed); // ���õ������
void Mote_SetGotoZero(uint8_t MoterNaber);							// ���õ������
void Mote_OutDisable(void);											// ���õ��ͣ��ʧЧ
void Mote_OutEnable(void);											// ���õ��ͣ��ʧЧ
int32_t Moter_ReadCoor(uint8_t MoterNaber);							// ��ȡ�����ǰ����
uint8_t Moter_ReadStatic(uint8_t MoterNaber);						// ��ȡ��ǰ����Ƿ�ֹ 0��ֹ��1�˶�
uint8_t Moter_ReadAllStatic(void);
bool Moter_DriveGotoZero(void); // �豸����
uint8_t Moter_ReadZero(void);	// ��ȡ�豸�Ƿ����
void zero_init();

void OUT_UV(uint8_t onoff);
void OUT_LED(uint8_t onoff);
void WaterPump_Ctrl(int32_t onoff);
uint8_t INT_key_stop(void);
uint8_t INT_key_X(void);
uint8_t INT_key_Y(void);
uint8_t INT_key_Z(void);
uint8_t INT_normalSalineLow(void);
void out_beepset(uint8_t mode);

DeviceStatus_t *get_device_state();
void set_device_AllState(DeviceStatus_t state);
void set_run_state(RunState_t state);
RunState_t get_run_state();

void emptying_function(uint8_t sta);
void stop_key_handle(bool stop);
void low_water_alarm();
void run_old(uint32_t time);

void timer0_update_handle();

#endif
