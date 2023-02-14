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
	}
}
