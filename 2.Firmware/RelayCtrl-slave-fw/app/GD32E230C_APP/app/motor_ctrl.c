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

uint8_t MOTOR_SetDoutIt(uint8_t naber, uint32_t Conut, uint32_t ConutAll, int16_t *buff)
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

uint8_t get_execution_schedule()
{
    return (uint8_t)(((float)MotorData.SetDoutConut / (float)MotorData.SetDoutConutAll) * 100.0f);
}

void motor_handle(void)
{

    if ((MotorData.NewCom != 0) || (Moter_ReadAllStatic() == 0))
    {
        MotorData.NewCom = 0;
        switch (MotorData.SetDoutNaber)
        {
        case 0xFF:
            // ReturnData.sys_state = 2; // ��0��
            // backStatus();
            Moter_DriveGotoZero();
            OUT_UV(0);
            // backStatus();
            MotorData.SetDoutNaber = 0;
            break;

        case 0:
            if (MotorData.oneCoorEn == 0)
            {
                break;
            }
            ReturnData.sys_state = 1;
            MotorData.oneCoorEn = 0;
            for (uint8_t i = 0; i < 3; i++)
            {
                Mote_SetCoor(i, MotorData.oneCoor[i], MotorData.oneSpeed[i]); // one point move
            }
            WaterPump_Ctrl(MotorData.oneSpeed[3]);
            break;

        default:
            if (MotorData.SetDoutNaber < 9)
                ReturnData.sys_state = 3;
            else
                ReturnData.sys_state = 4;

            ReturnData.progress = (uint8_t)(((float)MotorData.SetDoutConut / (float)MotorData.SetDoutConutAll) * 100.0f);

            /* device stop */
            if (get_device_state()->stop_state != 0)
            {
                break;
            }

            if (MOTOR_SetDoutIt(MotorData.SetDoutNaber, MotorData.SetDoutConut, MotorData.SetDoutConutAll, MotorData.SetDoutBuff) == 0)
            {
                WaterPump_Ctrl(100);
                ReturnData.sys_state = 2;
                // backStatus();
                Moter_DriveGotoZero();
                OUT_UV(0);
                MotorData.SetDoutNaber = 0;
                ReturnData.sys_state = 0;
            }
            else
            {
                MotorData.SetDoutConut++;
            }
            break;
        }
    }
    else
    {
        ReturnData.progress = 100;
        if ((Moter_ReadAllStatic() == 0) && (MotorData.SetDoutNaber == 0))
        {
            if (Moter_ReadZero() != 0)
                ReturnData.sys_state = 0;
            else
                ReturnData.sys_state = 1;
        }
    }
}
