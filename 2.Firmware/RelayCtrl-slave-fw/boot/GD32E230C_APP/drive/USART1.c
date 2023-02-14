
#include "systick.h"
#include "stdio.h"
#include "string.h"
#include "gd32e23x_usart.h"


void USART2_Init(uint32_t BaudRate)
{
	nvic_irq_enable(USART1_IRQn, 3);/* USART interrupt configuration */
	rcu_periph_clock_enable(RCU_GPIOA);/* enable COM GPIO clock */
	gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_2);/* connect port to USARTx_Tx */
	gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_3);/* connect port to USARTx_Rx */
	
	/* configure USART Tx as alternate function push-pull */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_2);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_2);
	/* configure USART Rx as alternate function push-pull */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_3);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_3);
 
	/* enable USART clock */
	rcu_periph_clock_enable(RCU_USART1);
	usart_deinit(USART1);
	usart_baudrate_set(USART1, BaudRate);
	usart_receive_config(USART1, USART_RECEIVE_ENABLE);
	usart_transmit_config(USART1, USART_TRANSMIT_ENABLE);
	usart_enable(USART1);

  /* enable USART interrupt */  
  usart_interrupt_enable(USART1, USART_INT_RBNE);
}



/**
  * @brief  串口发数据
  * @param  无
  * @retval 无
  */
void USART2_SendBuf(uint8_t *buf,uint32_t len) 
{
	for(uint32_t i=0;i<len;i++)
	{
		while(RESET == usart_flag_get(USART1, USART_FLAG_TBE));
		usart_data_transmit(USART1, buf[i]);
	}
}




