#include "motor_drive.h"
#include "systick.h"
#include "ConstCoor.h"

#include <stdio.h>
#include <string.h>

MotorDataType MotorData;

/* device state cache */
static DeviceStatus_t ReturnData;

void device_params_init()
{
	memset(&MotorData, 0, sizeof(MotorDataType));
	memset(&ReturnData, 0, sizeof(DeviceStatus_t));
	set_run_state(STATE_STOP);
}

/**************************************************************************************************
**************************************************************************************************/
void con_io_init(void)
{
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	gpio_mode_set(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_ALL);
	gpio_output_options_set(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_ALL);
	gpio_port_write(GPIOB, 0x0040);
	gpio_mode_set(GPIOA, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12);
}

void motor_Tim_Init(void)
{
	timer_parameter_struct timer_initpara;
	nvic_irq_enable(TIMER0_BRK_UP_TRG_COM_IRQn, 2);
	rcu_periph_clock_enable(RCU_TIMER0);
	timer_deinit(TIMER0);
	timer_initpara.period = MinSpeedUs;
	timer_initpara.prescaler = 72 - 1;
	timer_initpara.clockdivision = TIMER_CKDIV_DIV1;
	timer_initpara.counterdirection = TIMER_COUNTER_UP;
	timer_initpara.alignedmode = TIMER_COUNTER_EDGE;
	timer_initpara.repetitioncounter = 0;
	timer_init(TIMER0, &timer_initpara);
	timer_primary_output_config(TIMER0, ENABLE);
	timer_auto_reload_shadow_enable(TIMER0);
	timer_enable(TIMER0);
	timer_interrupt_enable(TIMER0, TIMER_INT_UP); // 使能定时器更新中断
}

static void water_pump_pwm_handle()
{
	if (MotorData.DCMOTE_PWM == 0)
	{
		MotorData.DCMOTE_PWMcount = 0;
		MOTOR_BI_L;
		MOTOR_FI_L;
	}
	else if (MotorData.DCMOTE_PWM > 0)
	{
		if (++MotorData.DCMOTE_PWMcount > 100)
			MotorData.DCMOTE_PWMcount = 0;

		if (MotorData.DCMOTE_PWMcount < MotorData.DCMOTE_PWM)
		{
			if (MotorData.motorM_DirUpSet[3] == 0)
			{
				MOTOR_BI_H;
				MOTOR_FI_L;
			}
			else
			{
				MOTOR_BI_L;
				MOTOR_FI_H;
			}
		}
		else
		{
			MOTOR_BI_L;
			MOTOR_FI_L;
		}
	}
	else
	{
		if (--MotorData.DCMOTE_PWMcount < -100)
			MotorData.DCMOTE_PWMcount = 0;

		if (MotorData.DCMOTE_PWMcount > MotorData.DCMOTE_PWM)
		{
			if (MotorData.motorM_DirUpSet[3] == 0)
			{
				MOTOR_BI_L;
				MOTOR_FI_H;
			}
			else
			{
				MOTOR_BI_H;
				MOTOR_FI_L;
			}
		}
		else
		{
			MOTOR_BI_L;
			MOTOR_FI_L;
		}
	}
}

static void axle_motor_pwm_handle()
{
	// read key
	if (ReadX0 == 0)
		MotorData.zeroKeyTir[0] = 1;
	else
		MotorData.zeroKeyTir[0] = 0;
	if (ReadY0 == 0)
		MotorData.zeroKeyTir[1] = 1;
	else
		MotorData.zeroKeyTir[1] = 0;
	if (ReadZ0 == 0)
		MotorData.zeroKeyTir[2] = 1;
	else
		MotorData.zeroKeyTir[2] = 0;

	// delay count
	MotorData.StepDelayUsCount[0] += MinSpeedUs;
	MotorData.StepDelayUsCount[1] += MinSpeedUs;
	MotorData.StepDelayUsCount[2] += MinSpeedUs;

	for (int i = 0; i < 3; i++)
	{
		if (MotorData.StepDelayUsCount[i] < MotorData.StepDelayUsSet[i])
			continue;
		MotorData.StepDelayUsCount[i] = 0;
		if (MotorData.zeroGotoMode[i] == 1)
		{
			if (MotorData.zeroKeyTir[i] != 0) // check key, yet trigger
			{
				MotorData.itCoor[i] = 0;
				MotorData.setCoor[i] = 0;
				MotorData.zeroGotoMode[i] = 0; // set normal mode
				MotorData.itStop[i] = 0;
			}
			else
			{
				MotorData.itStop[i] = 1;
				/* dir */
				if (MotorData.motorM_DirUpSet[i] == 0)
				{
					if (i == 0)
						MOTORX_DIR_H;
					else if (i == 1)
						MOTORY_DIR_H;
					else if (i == 2)
						MOTORZ_DIR_H;
				}
				else
				{
					if (i == 0)
						MOTORX_DIR_L;
					else if (i == 1)
						MOTORY_DIR_L;
					else if (i == 2)
						MOTORZ_DIR_L;
				}

				/* trigger pulse level */
				if (i == 0)
				{
					if (MotorData.itOutStep[i] == 0)
					{
						MOTORX_STEP_H;
						MotorData.itOutStep[i] = 1;
					}
					else
					{
						MOTORX_STEP_L;
						MotorData.itOutStep[i] = 0;
					}
				}
				else if (i == 1)
				{
					if (MotorData.itOutStep[i] == 0)
					{
						MOTORY_STEP_H;
						MotorData.itOutStep[i] = 1;
					}
					else
					{
						MOTORY_STEP_L;
						MotorData.itOutStep[i] = 0;
					}
				}
				else if (i == 2)
				{
					if (MotorData.itOutStep[i] == 0)
					{
						MOTORZ_STEP_H;
						MotorData.itOutStep[i] = 1;
					}
					else
					{
						MOTORZ_STEP_L;
						MotorData.itOutStep[i] = 0;
					}
				}
			}
		}
		else
		{
			if (MotorData.setCoor[i] != MotorData.itCoor[i])
			{
				MotorData.itStop[i] = 1;
				if (MotorData.setCoor[i] < MotorData.itCoor[i])
				{
					MotorData.itCoor[i]--;
					if (MotorData.motorM_DirUpSet[i] == 0)
					{
						if (i == 0)
							MOTORX_DIR_H;
						else if (i == 1)
							MOTORY_DIR_H;
						else if (i == 2)
							MOTORZ_DIR_H;
					}
					else
					{
						if (i == 0)
							MOTORX_DIR_L;
						else if (i == 1)
							MOTORY_DIR_L;
						else if (i == 2)
							MOTORZ_DIR_L;
					}
				}
				else
				{
					MotorData.itCoor[i]++;
					if (MotorData.motorM_DirUpSet[i] == 0)
					{
						if (i == 0)
							MOTORX_DIR_L;
						else if (i == 1)
							MOTORY_DIR_L;
						else if (i == 2)
							MOTORZ_DIR_L;
					}
					else
					{
						if (i == 0)
							MOTORX_DIR_H;
						else if (i == 1)
							MOTORY_DIR_H;
						else if (i == 2)
							MOTORZ_DIR_H;
					}
				}
				if (i == 0)
				{
					if (MotorData.itOutStep[i] == 0)
					{
						MOTORX_STEP_H;
						MotorData.itOutStep[i] = 1;
					}
					else
					{
						MOTORX_STEP_L;
						MotorData.itOutStep[i] = 0;
					}
				}
				else if (i == 1)
				{
					if (MotorData.itOutStep[i] == 0)
					{
						MOTORY_STEP_H;
						MotorData.itOutStep[i] = 1;
					}
					else
					{
						MOTORY_STEP_L;
						MotorData.itOutStep[i] = 0;
					}
				}
				else if (i == 2)
				{
					if (MotorData.itOutStep[i] == 0)
					{
						MOTORZ_STEP_H;
						MotorData.itOutStep[i] = 1;
					}
					else
					{
						MOTORZ_STEP_L;
						MotorData.itOutStep[i] = 0;
					}
				}
			}
			else
			{
				MotorData.itStop[i] = 0;
			}
		}
	}
}

void timer0_update_handle()
{
	if (RESET != timer_interrupt_flag_get(TIMER0, TIMER_INT_FLAG_UP))
	{
		timer_interrupt_flag_clear(TIMER0, TIMER_INT_FLAG_UP);

		water_pump_pwm_handle();

		/* check stop key status */
		if (get_device_state()->stop_state != 0)
		{
			return;
		}

		axle_motor_pwm_handle();
	}
}

///**************************************************************************
// ���õ������
//**************************************************************************/
void Mote_SetCoor(uint8_t MoterNaber, int32_t Coor, int32_t Speed)
{
	if (MoterNaber > 3)
		return;
	if (MoterNaber == 0)
	{
		if (Coor > MaxPlusX)
			Coor = MaxPlusX;
	}
	if (MoterNaber == 1)
	{
		if (Coor > MaxPlusY)
			Coor = MaxPlusY;
	}
	if (MoterNaber == 2)
	{
		if (Coor > MaxPlusZ)
			Coor = MaxPlusZ;
	}

	MotorData.setCoor[MoterNaber] = Coor;
	if (Speed < 100)
		Speed = 100;
	MotorData.StepDelayUsSet[MoterNaber] = Speed; // �ٶȣ�ԽСԽ��
	if (MotorData.setCoor[MoterNaber] != MotorData.itCoor[MoterNaber])
	{
		MotorData.itStop[MoterNaber] = 1;
	}
}

void Mote_OutDisable(void)
{
	MOTOR_EN_H;
}
void Mote_OutEnable(void)
{
	MOTOR_EN_L;
	delay_ms(1000);
}

int32_t Moter_ReadCoor(uint8_t MoterNaber)
{
	if (MoterNaber > 3)
		return 0;
	else
		return MotorData.itCoor[MoterNaber];
}

uint8_t Moter_ReadStatic(uint8_t MoterNaber)
{
	if (MoterNaber > 3)
		return 0;
	else
		return MotorData.itStop[MoterNaber];
}

uint8_t Moter_ReadAllStatic(void)
{
	if ((MotorData.itStop[0] == 0) &&
		(MotorData.itStop[1] == 0) &&
		(MotorData.itStop[2] == 0))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

/* set motor goto zero mode */
void Mote_SetGotoZero(uint8_t MoterNaber)
{
	if (MoterNaber > 3)
		return;

	if (MoterNaber == 0)
		MotorData.StepDelayUsSet[MoterNaber] = 500; // �ٶȣ�ԽСԽ��
	if (MoterNaber == 1)
		MotorData.StepDelayUsSet[MoterNaber] = 500;
	if (MoterNaber == 2)
		MotorData.StepDelayUsSet[MoterNaber] = 200;
	if (MoterNaber == 3)
		MotorData.StepDelayUsSet[MoterNaber] = 200;

	MotorData.zeroGotoMode[MoterNaber] = 1;
	if (MotorData.zeroKeyTir[MoterNaber] == 0)
	{
		MotorData.itStop[MoterNaber] = 1;
	}
}

void zero_init()
{
	set_run_state(STATE_GOTOZERO);
	MotorData.setCoor[0] = MotorData.itCoor[0];
	MotorData.setCoor[1] = MotorData.itCoor[1];
	MotorData.setCoor[2] = MotorData.itCoor[2];

	ReturnData.zero_stage = 0;
}

/* goto zero handle */
bool Moter_DriveGotoZero(void)
{
	if (Moter_ReadAllStatic() != 0)
	{
		return 0;
	}

	switch (ReturnData.zero_stage)
	{
	case 0:
		Mote_SetCoor(Moter_ReadCoor(0) + 500, 0, 1000); // x point
		// Mote_SetGotoZero(0);
		ReturnData.zero_stage += 1;
		break;
	case 1:
		Mote_SetCoor(Moter_ReadCoor(1) + 500, 1, 1000); // y point
		// Mote_SetGotoZero(1);
		ReturnData.zero_stage += 1;
		break;

	case 2:
		Mote_SetGotoZero(0); // x goto zero
		ReturnData.zero_stage += 1;
		break;

	case 3:
		Mote_SetGotoZero(1); // y goto zero
		Mote_SetGotoZero(2); // z goto zero
		ReturnData.zero_stage += 1;
		break;

	case 4:
		set_run_state(STATE_STOP_ZERO);
		return 1;
		break;

	default:
		break;
	}

	return 0;
}

/* checke motor at zero point */
uint8_t Moter_ReadZero(void)
{
	if ((MotorData.itCoor[0] == 0) &&
		(MotorData.itCoor[1] == 0) &&
		(MotorData.itCoor[2] == 0))
	{
		return 1;
	}
	else
		return 0;
}

void OUT_UV(uint8_t onoff)
{
	DeviceStatus_t *state = get_device_state();

	if (onoff == 0)
	{
		UV_LED_L;
		state->uv_light = 0;
	}
	else
	{
		UV_LED_H;
		state->uv_light = 1;
	}
}

void OUT_LED(uint8_t onoff)
{
	if (onoff == 0)
	{
		KEY_LED_L;
	}
	else
	{
		KEY_LED_H;
	}
}

void WaterPump_Ctrl(int32_t onoff)
{
	DeviceStatus_t *state = get_device_state();

	MotorData.DCMOTE_PWM = (int8_t)onoff - 100;
	if (onoff == 100)
	{
		state->water_pump = 0;
	}
	else
	{
		state->water_pump = 1;
	}
}

uint8_t INT_key_stop(void)
{
	return ReadStopKey();
}

/* timer interrupt update */
uint8_t INT_key_X(void)
{
	return MotorData.zeroKeyTir[0];
}
/* timer interrupt update */
uint8_t INT_key_Y(void)
{
	return MotorData.zeroKeyTir[1];
}
/* timer interrupt update */
uint8_t INT_key_Z(void)
{
	return MotorData.zeroKeyTir[2];
}

uint8_t INT_normalSalineLow(void)
{
	if (ReadSenser() != RESET)
		return 0;
	else
		return 1;
}

void out_beepset(uint8_t mode)
{
	if (mode != 0)
		BEEP_H;
	else
		BEEP_L;
}

DeviceStatus_t *get_device_state()
{
	return &ReturnData;
}

void set_device_AllState(DeviceStatus_t state)
{
	ReturnData = state;
}

void set_run_state(RunState_t state)
{
	DeviceStatus_t *dev_state = get_device_state();

	dev_state->run_state = state;
}

RunState_t get_run_state()
{
	DeviceStatus_t *dev_state = get_device_state();

	return dev_state->run_state;
}

/**
 * @brief stop swtich handle
 */
void stop_key_handle(bool stop)
{
	// static uint8_t key_flag = 0;

	// uint8_t *stop_sta = &get_device_state()->stop_state;

	// /* stop break */
	// if (!stop)
	// {
	// 	KEY_LED_L;			 // close key led
	// 	WaterPump_Ctrl(100); // close water pump
	// 	*stop_sta = CLOSE;	 // clear falg
	// 	return;
	// }

	// /* check key pressed */
	// if (ReadStopKey == RESET)
	// {
	// 	key_flag = 1;
	// }

	// /* check key release */
	// if (ReadStopKey != RESET && key_flag)
	// {
	// 	key_flag = 0;

	// 	if (*stop_sta == CLOSE)
	// 	{
	// 		*stop_sta = OPEN;
	// 		KEY_LED_H;
	// 		WaterPump_Ctrl(100);
	// 	}
	// 	else
	// 	{
	// 		*stop_sta = CLOSE;
	// 		KEY_LED_L;
	// 		if (get_device_state()->sys_state == 3)
	// 		{
	// 			WaterPump_Ctrl(65);
	// 		}
	// 	}
	// }
}

void run_old(uint32_t time)
{
	iwdg_feed();
	if (time == 0)
		return;
	else if (time > 8 * 3600 * 1000)
		time = 8 * 3600 * 1000;

	uint16_t back_time = 0;
	uint32_t remain_time = time;
	uint16_t count = 0;
	OUT_UV(1);		// uv��
	out_beepset(0); // �رշ�����
	while (1)
	{
		iwdg_feed();

		if (Moter_ReadAllStatic() == 0)
		{
			for (uint8_t i = 0; i < 2; i++)
			{
				Mote_SetCoor(i, test_point[count * 8 + i * 2], test_point[count * 8 + i * 2 + 1]);
			}
			Mote_SetCoor(2, 0, 500);

			if (++count > 5)
				count = 0;
		}

		if (++back_time > 1000)
		{
			back_time = 0;
			remain_time = time - HAL_GetTick();
			if (HAL_GetTick() > time)
				break;
			printf("old runing, remaining %d s\r\n", remain_time / 1000);
		}

		if (remain_time == 0)
			break;

		delay_ms(1);
	}

	Moter_DriveGotoZero(); // ����
}
