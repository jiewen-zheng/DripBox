#include <Arduino.h>
#include "APP/app.h"
#include "HAL/HAL.h"

void setup()
{
  // put your setup code here, to run once:
  HAL::Init();
  app_init();
}

void loop()
{
  // put your main code here, to run repeatedly:
  app_loop();

  delay(10);
}
