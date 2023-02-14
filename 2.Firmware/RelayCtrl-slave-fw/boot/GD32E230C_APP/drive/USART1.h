
#ifndef __BSP_USART2_H
#define __BSP_USART2_H

#include "gd32e23x.h"
#include <stdio.h>
 
/*******************************************************************************/
void USART2_Init(uint32_t BaudRate);
void USART2_SendBuf(uint8_t *buf,uint32_t len) ;
/*******************************************************************************/	
#endif

	
/*****END OF FILE****/	
