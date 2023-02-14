#include "gd32e23x_it.h"
#include "systick.h"

#include "usart.h"

/**
	@brief	this function handles NMI exception
*/
void NMI_Handler(void)
{
}

/**
	@brief	this function handles HardFault exception
*/
void HardFault_Handler(void)
{
	/* if Hard Fault exception occurs, go to infinite loop */
	while (1)
	{
	}
}

/*!
	@brief	this function handles MemManage exception
*/
void MemManage_Handler(void)
{
	/* if Memory Manage exception occurs, go to infinite loop */
	while (1)
	{
	}
}

/*!
	@brief	this function handles BusFault exception
*/
void BusFault_Handler(void)
{
	/* if Bus Fault exception occurs, go to infinite loop */
	while (1)
	{
	}
}

/*!
	@brief	this function handles UsageFault exception
*/
void UsageFault_Handler(void)
{
	/* if Usage Fault exception occurs, go to infinite loop */
	while (1)
	{
	}
}

/*!
	@brief	this function handles SVC exception
*/
void SVC_Handler(void)
{
}

/*!
	@brief	this function handles DebugMon exception
*/
void DebugMon_Handler(void)
{
}

/*!
	@brief	this function handles PendSV exception
*/
void PendSV_Handler(void)
{
}

/**
	@brief	this function handles SysTick exception
*/
void SysTick_Handler(void)
{
	delay_decrement();
}

/******************************************************************************/
/* gd32e23x Peripheral Interrupt Handlers  	                                  */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_gd32e23x.s).                     */
/******************************************************************************/

void USART0_IRQHandler(void)
{
	usart0_idle_handle();
}
