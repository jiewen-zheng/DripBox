#include <Arduino.h>
#include "APP/main_app.h"
#include "HAL/HAL.h"

SET_LOOP_TASK_STACK_SIZE(16 * 1024)

void setup()
{
  // put your setup code here, to run once:
  HAL::Init();
  // delay(2000);
  app_init();
}

void loop()
{
  // put your main code here, to run repeatedly:
  app_loop();
  // Serial.println("hello world");
  delay(10);
}
