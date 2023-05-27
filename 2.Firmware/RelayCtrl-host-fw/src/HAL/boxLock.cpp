#include "boxLock.h"

void HAL::BoxLock::init()
{
    pinMode(BLOCK_TRIGGER_PIN, INPUT_PULLDOWN);
    pinMode(BLOCK_LOCK_PIN, OUTPUT);
    digitalWrite(BLOCK_LOCK_PIN, LOW);
}

bool HAL::BoxLock::lock()
{
    Serial.printf("[lock] triggrt pin:%d", digitalRead(BLOCK_TRIGGER_PIN));

    if (digitalRead(BLOCK_TRIGGER_PIN) == HIGH)
    {
        digitalWrite(BLOCK_LOCK_PIN, HIGH);
        return true;
    }

    return false;
}

void HAL::BoxLock::unlock()
{
    digitalWrite(BLOCK_LOCK_PIN, LOW);
}
