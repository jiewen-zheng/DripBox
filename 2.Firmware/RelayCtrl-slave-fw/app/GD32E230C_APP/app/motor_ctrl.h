#ifndef __MOTOR_CTRL_H
#define __MOTOR_CTRL_H
#include "motor_drive.h"

void motor_set_direction();

void move_to_testPoint(uint8_t point, void *msg);
uint8_t get_execution_schedule();
void motor_handle(void);
#endif