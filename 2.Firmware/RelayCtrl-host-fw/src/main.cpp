#include <Arduino.h>
#include "APP/main_app.h"
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
  // Serial.println("hello world");
  delay(10);
}
