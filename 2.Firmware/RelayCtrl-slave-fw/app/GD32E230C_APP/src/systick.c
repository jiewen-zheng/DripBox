#include "gd32e23x.h"
#include "systick.h"

volatile static uint32_t delay = 0;
volatile uint32_t SysTimMs = 0;

void delay_us(uint16_t us)
{
	int temp;
	while (us > 0)
	{
		while (temp > 0)
		{
			temp--;
			__NOP();
		}
		us--;
	}
}

void systick_config(void)
{
	/* setup systick timer for 1000Hz interrupts */
	if (SysTick_Config(SystemCoreClock / 1000U))
	{
		/* capture error */
		while (1)
		{
		}
	}
	/* configure the systick handler priority */
	NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

void delay_1ms(uint32_t count)
{
	delay = count;
	while (0U != delay)
	{
	}
}

void delay_ms(uint32_t count)
{
	delay = count;
	while (0U != delay)
	{
	}
}

/*!
	@brief  delay decrement
*/
void delay_decrement(void)
{
	if (0U != delay)
	{
		delay--;
	}
	SysTimMs++;
}

uint32_t HAL_GetTick(void)
{
	return SysTimMs;
}

void iwdg_init()
{
	uint16_t timeout = 0xffff;

	rcu_osci_on(RCU_IRC40K);

	while (rcu_osci_stab_wait(RCU_IRC40K) == ERROR)
	{
		if (timeout > 0)
			timeout--;
		else
			break;
	}
	/* configure fwdgt counter clock : 40kHz / 64 = 0.625kHz */
	fwdgt_config(2500, FWDGT_PSC_DIV64); // t = (1 / 625)*(2000) = 3.2s

	fwdgt_enable();
}

void iwdg_feed()
{

	fwdgt_counter_reload();
}
