#include "motor_ctrl.h"

#include "other.h"

#include <string.h>

// 大面膜托盘测试点
// const int32_t testPoint_leftEye[3] = {10800, 500, 12000, 500, 0, 500};
// const int32_t testPoint_rigthEye[3] = {20500, 500, 12000, 500, 0, 500};
// const int32_t testPoint_mouth[3] = {15650, 500, 21100, 500, 0, 500};

// 小面膜托盘测试点
const int32_t testPoint_leftEye[6] = {10800, 500, 13500, 500, 0, 500};
const int32_t testPoint_rigthEye[6] = {20500, 500, 13500, 500, 0, 500};
const int32_t testPoint_mouth[6] = {15650, 500, 22600, 500, 0, 500};

/**
 * @brief move to test point
 */
void move_to_testPoint(uint8_t point, void *msg)
{
    static int32_t *pBuf = (int32_t *)testPoint_leftEye;

    OffSet_t *offset = get_offset();

    if (point == 1)
    {
        pBuf = (int32_t *)testPoint_leftEye;
    }
    else if (point == 2)
    {
        pBuf = (int32_t *)testPoint_rigthEye;
    }
    else if (point == 3)
    {
        pBuf = (int32_t *)testPoint_mouth;
    }

    MotorDataType *pMotor = (MotorDataType *)msg;

    /* single point move set coordinate point */
    pMotor->oneCoor[0] = pBuf[0] + offset->x; // "x" move speed
    pMotor->oneCoor[1] = pBuf[2] + offset->y; // "y" move speed
    pMotor->oneCoor[2] = pBuf[4] + offset->z; // "z" move speed
    pMotor->oneCoor[3] = 0;                   // reserve

    /* single point move set speed */
    pMotor->oneSpeed[0] = pBuf[1]; // "x" move speed
    pMotor->oneSpeed[1] = pBuf[3]; // "y" move speed
    pMotor->oneSpeed[2] = pBuf[5]; // "z" move speed
    pMotor->oneSpeed[3] = 100;     // close water pump

    pMotor->SetDoutConut = 0;
    pMotor->SetDoutNaber = 0; // single point move
    pMotor->oneCoorEn = 1;    // single point move set item
    pMotor->NewCom = 1;       // receive new command flag
}

void motor_set_direction()
{
    memset(&MotorData, 0, sizeof(MotorDataType));
    MotorData.motorM_DirUpSet[0] = 0;
    MotorData.motorM_DirUpSet[1] = 1;
    MotorData.motorM_DirUpSet[2] = 0;
    MotorData.motorM_DirUpSet[3] = 0;
}

uint8_t MOTOR_SetDoutIt(uint32_t Conut, uint32_t ConutAll, int16_t *buff)
{
    if (Conut > ConutAll - 1)
    {
        return 0;
    }

    OffSet_t *offset = get_offset();

    /* read "x" coordinate data */
    Mote_SetCoor(0, buff[Conut * 8] + offset->x, buff[Conut * 8 + 1]);

    /* read "y" coordinate data */
    Mote_SetCoor(1, buff[Conut * 8 + 2] + offset->y, buff[Conut * 8 + 3]);

    /* read "z" coordinate data, "z" coordinate data too big, so the buffer shrinks by a factor of 10 when saved */
    Mote_SetCoor(2, buff[Conut * 8 + 4] * 10 + 5 + offset->z, buff[Conut * 8 + 5]);

    OUT_UV(1);
    WaterPump_Ctrl(buff[Conut * 8 + 7]);

    return 1;
}

void check_motor_location()
{
    if (Moter_ReadZero())
    {
        set_run_state(STATE_STOP_ZERO);
    }
    else
    {
        set_run_state(STATE_STOP);
    }
}

uint8_t get_execution_schedule()
{
    return 0;
    return (uint8_t)(((float)MotorData.SetDoutConut / (float)MotorData.SetDoutConutAll) * 100.0f);
}

void motor_handle(void)
{
    /* device stop */
    if (get_device_state()->stop_state != 0)
    {
        return;
    }

    MotorDataType *pMotor = &MotorData;
    DeviceStatus_t *pState = get_device_state();

    /* goto zero */
    if (pState->run_state == STATE_GOTOZERO)
    {
        Moter_DriveGotoZero();
        return;
    }

    /* checke new command and motor all stop */
    if (pMotor->NewCom == 0 && Moter_ReadAllStatic() != 0)
    {
        return;
    }

    pMotor->NewCom = 0;

    switch (MotorData.SetDoutNaber)
    {
    case 0xFF:
        zero_init();
        OUT_UV(0);
        pMotor->SetDoutNaber = 0;
        break;

    case 0: /* one point move */
        if (pMotor->oneCoorEn == 0)
        {
						check_motor_location();
            break;
        }
        MotorData.oneCoorEn = 0;
        set_run_state(STATE_POINT_MOVE);
        for (uint8_t i = 0; i < 3; i++)
        {
            Mote_SetCoor(i, MotorData.oneCoor[i], MotorData.oneSpeed[i]); // one point move
        }
        WaterPump_Ctrl(MotorData.oneSpeed[3]);
        break;

    case 1: /* empty */
        set_run_state(STATE_EMPTY);
        if (MOTOR_SetDoutIt(MotorData.SetDoutConut, MotorData.SetDoutConutAll, MotorData.SetDoutBuff) == 0)
        {
            WaterPump_Ctrl(60);
						pMotor->SetDoutNaber = 0;
            pMotor->SetDoutConut = 0;
            pMotor->oneCoorEn = 0;
            pMotor->NewCom = 0;
            return;
        }
        break;

    default:
        if (pMotor->SetDoutNaber == 2)
        {
            set_run_state(STATE_BASIC);
        }
        else if (pMotor->SetDoutNaber == 3)
        {
            set_run_state(STATE_DROP);
        }

        if (MOTOR_SetDoutIt(MotorData.SetDoutConut, MotorData.SetDoutConutAll, MotorData.SetDoutBuff) == 0)
        {
            WaterPump_Ctrl(100);
            MotorData.SetDoutNaber = 0xFF;
            MotorData.SetDoutConut = 0;
            MotorData.oneCoorEn = 0;
            MotorData.NewCom = 1;
            break;
        }
        MotorData.SetDoutConut++;
        break;
    }

    // check_motor_location();
}
