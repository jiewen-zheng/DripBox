#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include <stdint.h>

/* configure systick */
void systick_config(void);
/* delay a time in milliseconds */
void delay_1ms(uint32_t count);
void delay_ms(uint32_t count);

void delay_decrement(void);

uint32_t HAL_GetTick(void);
#endif /* SYS_TICK_H */
