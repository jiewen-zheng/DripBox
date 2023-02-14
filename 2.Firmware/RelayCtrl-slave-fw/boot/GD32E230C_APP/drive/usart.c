#include "usart.h"
#include "fifo.h"


#define ARRAYNUM(arr_nanme) (uint32_t)(sizeof(arr_nanme) / sizeof(*(arr_nanme)))
#define USART0_TDATA_ADDRESS ((uint32_t)0x40013828)
#define USART0_RDATA_ADDRESS ((uint32_t)0x40013824)

uint8_t usart0_txbuf[UART_TXBUFF_SIZE];
volatile uint8_t usart0_rxbuf[UART_RXBUFF_SIZE];

void usart0_gpio_init()
{
	/* enable COM GPIO clock */
	rcu_periph_clock_enable(RCU_GPIOA);

	/* connect port to USARTx_Tx */
	gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_9);
	/* connect port to USARTx_Rx */
	gpio_af_set(GPIOA, GPIO_AF_1, GPIO_PIN_10);

	/* configure USART Tx as alternate function push-pull */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_9);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_9);

	/* configure USART Rx as alternate function push-pull */
	gpio_mode_set(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO_PIN_10);
	gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_10);
}

void usart0_tx_dma_init()
{
	dma_parameter_struct dma_init_struct;

	/* enable DMA clock */
	rcu_periph_clock_enable(RCU_DMA);

	/* deinitialize DMA channel1 */
	dma_deinit(DMA_CH1);
	dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
	dma_init_struct.memory_addr = (uint32_t)usart0_txbuf;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = ARRAYNUM(usart0_txbuf);
	dma_init_struct.periph_addr = USART0_TDATA_ADDRESS;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
	dma_init(DMA_CH1, &dma_init_struct);
	/* configure DMA mode */
	dma_circulation_disable(DMA_CH1);
	dma_memory_to_memory_disable(DMA_CH1);
	/* enable DMA channel1 */
	dma_channel_enable(DMA_CH1);
	/* USART DMA enable for transmission and reception */
	usart_dma_transmit_config(USART0, USART_DENT_ENABLE);
}

void usart0_rx_dma_init()
{
	dma_parameter_struct dma_init_struct;

	/* enable DMA clock */
	rcu_periph_clock_enable(RCU_DMA);

	/* deinitialize DMA channel2 */
	dma_deinit(DMA_CH2);
	dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
	dma_init_struct.memory_addr = (uint32_t)usart0_rxbuf;
	dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.number = ARRAYNUM(usart0_rxbuf);
	;
	dma_init_struct.periph_addr = USART0_RDATA_ADDRESS;
	dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority = DMA_PRIORITY_ULTRA_HIGH;
	dma_init(DMA_CH2, &dma_init_struct);

	dma_circulation_disable(DMA_CH2);
	dma_memory_to_memory_disable(DMA_CH2);

	dma_channel_enable(DMA_CH2);
	usart_dma_receive_config(USART0, USART_DENR_ENABLE);
}

void usart0_init(uint32_t baud)
{

	/* initilize the gpio */
	usart0_gpio_init();

	/* enable USART clock */
	rcu_periph_clock_enable(RCU_USART0);
	/* USART configure */
	usart_deinit(USART0);
	usart_baudrate_set(USART0, baud);
	usart_receive_config(USART0, USART_RECEIVE_ENABLE);
	usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);

	usart_enable(USART0);

	/* initilize the dma */
	// usart0_tx_dma_init();
	usart0_rx_dma_init();

	/* USART interrupt configuration */
	nvic_irq_enable(USART0_IRQn, 0);
	/* enable USART interrupt */
	usart_interrupt_enable(USART0, USART_INT_IDLE);

	/* apply fifo */
	if (!fifo_dynamic_register(0, 1536))
	{
		printf("usart fifo register failed\r\n");
	}
}

void usart0_send_data(uint8_t *buf, uint16_t len)
{
	for (uint16_t i = 0; i < len; i++)
	{
		while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
			;
		usart_data_transmit(USART0, buf[i]);
	}
}

void usart0_dma_send_data(uint8_t *buf, uint16_t len)
{
	dma_channel_disable(DMA_CH1);

	dma_memory_address_config(DMA_CH1, (uint32_t)buf);
	dma_transfer_number_config(DMA_CH1, len);

	dma_channel_enable(DMA_CH1);
}

void usart0_idle_handle()
{
	if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
	{
		usart_interrupt_flag_clear(USART0, USART_INT_FLAG_IDLE);
		usart_data_receive(USART0);
		dma_channel_disable(DMA_CH2);

		uint32_t read_len = ARRAYNUM(usart0_rxbuf) - dma_transfer_number_get(DMA_CH2);

		// printf("read data: %.*s\r\n", read_len, usart0_rxbuf);
		// usart0_send_data(usart0_rxbuf, read_len);

		// FIFO
		fifo_dynamic_write(0, (uint8_t *)usart0_rxbuf, read_len);

		dma_memory_address_config(DMA_CH2, (uint32_t)usart0_rxbuf);
		dma_transfer_number_config(DMA_CH2, ARRAYNUM(usart0_rxbuf));
		dma_channel_enable(DMA_CH2);
	}
}

/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
	while (RESET == usart_flag_get(USART0, USART_FLAG_TBE))
		;
	usart_data_transmit(USART0, (uint8_t)ch);
	return ch;
}
