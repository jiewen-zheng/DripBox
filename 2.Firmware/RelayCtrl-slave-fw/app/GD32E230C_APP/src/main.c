#include "gd32e23x.h"
#include "systick.h"

#include "usart.h"

#include "command.h"
#include "motor_ctrl.h"
#include "other.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
	nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x3C00);

	systick_config();
	// iwdg_init();
	usart0_init(115200);

	printf("starting...");

	motor_Tim_Init();

	memset(&ReturnData, 0, sizeof(DeviceStatus_t));
	ReturnData.sys_state = 1;

	con_io_init();

	/* read axle offset data form flash */
	load_offset();

	/* set motor direction */
	motor_set_direction();

	/* wait motor go into zero point */
	// Mote_OutEnable();
	// delay_ms(1000);
	// Moter_DriveGotoZero();
	// delay_ms(1000);
	printf("start success");

	while (1)
	{
		iwdg_feed();

		comm_handle();
		motor_handle();
		other_handle();
		delay_ms(1000);
	}
}
