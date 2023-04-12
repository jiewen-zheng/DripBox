#include "gd32e23x.h"
#include "systick.h"
#include "usart.h"
#include <stdio.h>

#include "update.h"

int main(void)
{
	if (!update_check())
	{
		iap_load_app(USER_APP_START_ADDR);
	}

	systick_config();
	usart0_init(115200);

	update_init();

	while (1)
	{
		update_app();
		
//		usart0_send_data((uint8_t*)"hello mb \r\n", sizeof("hello mb \r\n"));
//		delay_ms(500);
	}
}
