#include "boxLock.h"

void HAL::BoxLock::init()
{
    pinMode(BLOCK_TRIGGER_PIN, INPUT_PULLDOWN);
    pinMode(BLOCK_LOCK_PIN, OUTPUT);
    digitalWrite(BLOCK_LOCK_PIN, LOW);
}

void HAL::BoxLock::lock()
{
    // Serial.printf("[lock] triggrt pin:%d", digitalRead(BLOCK_TRIGGER_PIN));
    lockState = true;
}

void HAL::BoxLock::unlock(bool trig)
{
    static unsigned long time = 0;

    if (lockState || trig)
    {
        time = millis();
        lockState = false;
        digitalWrite(BLOCK_LOCK_PIN, HIGH);
        return;
    }

    if (millis() - time > UnlockTime)
    {
        digitalWrite(BLOCK_LOCK_PIN, LOW);
    }
}

void HAL::BoxLock::unlock_delay(uint32_t time)
{
}

bool HAL::BoxLock::getClose()
{
    return (digitalRead(BLOCK_TRIGGER_PIN) == HIGH);
}
