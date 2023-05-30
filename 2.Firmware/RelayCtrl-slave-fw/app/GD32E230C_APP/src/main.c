#include "gd32e23x.h"
#include "systick.h"

#include "usart.h"
#include "command.h"
#include "motor_ctrl.h"
#include "other.h"

#include <stdio.h>

int main(void)
{
	nvic_vector_table_set(NVIC_VECTTAB_FLASH, 0x3C00);

	systick_config();
	usart0_init(115200);
	printf("starting...");

	/* device init */
	device_params_init();
	con_io_init();
	motor_Tim_Init();

	/* read axle offset data form flash */
	load_offset();

	/* set motor direction */
	motor_set_direction();

	/* enable motor chip */
	Mote_OutEnable();

	/* wait motor go into zero location */
	printf("goto zero...");
	zero_init();
	while (Moter_DriveGotoZero() == 0)
	{
		asm volatile("nop");
		// delay_ms(10);
	}
	iwdg_init();
	printf("start success");

	for (;;)
	{
		// usart0_send_data((uint8_t*)"hello", 5);
		iwdg_feed();

		comm_handle();
		motor_handle();
		other_handle();

		delay_ms(1);
	}
}
