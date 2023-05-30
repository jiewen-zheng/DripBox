#include "command.h"
#include "systick.h"

#include "usart.h"
#include "crc16.h"

#include "ConstCoor.h"
#include "motor_ctrl.h"

#include "other.h"
#include "update.h"

#include <string.h>

/* command check save cache */
static CommType_t params;

void CommSend(uint8_t com, uint8_t *dataBuf, uint16_t len)
{
    uint8_t *comm_back_buf = usart0_txbuf;

    uint16_t f_len = len + 3;

    comm_back_buf[0] = 0x5A;
    comm_back_buf[1] = f_len >> 8;
    comm_back_buf[2] = f_len;
    comm_back_buf[3] = com;

    memcpy(comm_back_buf + 4, dataBuf, len);

    /* crc check is "data + cmd" */
    uint16_t crc = ym_crc16(comm_back_buf + 3, len + 1);
    comm_back_buf[len + 4] = crc >> 8;
    comm_back_buf[len + 5] = crc;

    usart0_send_data(comm_back_buf, len + 6);
}

void back_device_status()
{
    uint8_t status[10] = {0};

    DeviceStatus_t *state = get_device_state();

    status[0] = state->run_state;     // running mode:
    status[1] = state->progress;      // run schedule 0~100
    status[2] = state->uv_light;      // "uv" light switch state (0 close, 1 open)
    status[3] = state->water_pump;    // water pump state
    status[4] = state->stop_state;    // stop switch state (0 close, 1 open)
    status[5] = state->x_zero;        // "x" make zero switch (0 not zero, 1 zero)
    status[6] = state->y_zero;        // "y" make zero switch (0 not zero, 1 zero)
    status[7] = state->z_zero;        // "z" make zero switch (0 not zero, 1 zero)
    status[8] = state->water_too_low; // basic liquid too little
    status[9] = state->error + 1;     //	run error
    CommSend(0X89, status, 10);
}

void back_offset()
{
    OffSet_t *offset = get_offset();
    uint8_t back_offset[3] = {0};

    back_offset[0] = offset->x / 100;
    back_offset[1] = offset->y / 100;
    back_offset[2] = offset->z / 100;

    CommSend(0X90, back_offset, 3);
}

void comm_execute(uint8_t cmd, uint8_t *data, uint16_t len, void *msg)
{
    MotorDataType *pMotor = (MotorDataType *)msg;
    DeviceStatus_t *pState = get_device_state();

    switch (cmd)
    {
        /* basic liquid */
    case 0x81:
        if (data[0] > MAX_MASK_TYPE * 2 + 1) // check maks type
            break;
        pMotor->SetDoutNaber = 2;
        pMotor->SetDoutBuff = (int16_t *)basic_data_point[data[0] * 2 - 2];
        pMotor->SetDoutConutAll = basic_data_point[data[0] * 2 - 1];

        pMotor->SetDoutConut = 0;
        pMotor->oneCoorEn = 0;
        pMotor->NewCom = 1;
        break;

        /* dropping liquid */
    case 0x82:
        if (data[0] > MAX_MASK_TYPE * 4 + 1) // check maks type
            break;
        pMotor->SetDoutNaber = 3;
        pMotor->SetDoutBuff = (int16_t *)drop_data_point[data[0] * 2 - 2];
        pMotor->SetDoutConutAll = drop_data_point[data[0] * 2 - 1];

        pMotor->SetDoutConut = 0;
        pMotor->oneCoorEn = 0;
        pMotor->NewCom = 1;
        break;

        /* pipe line row empty */
    case 0x83:
        pMotor->SetDoutNaber = 1;
        pMotor->SetDoutBuff = (int16_t *)empty_point;
        pMotor->SetDoutConutAll = empty_point_len;
        // emptying_function(data[0]);

        pMotor->SetDoutConut = 0;
        pMotor->oneCoorEn = 0;
        pMotor->NewCom = 1;
        break;

        /* goto zero point */
    case 0x85:
        usart0_send_data((uint8_t *)"OK", 2);
        /* stop water pump */
        if (pState->run_state == STATE_EMPTY)
        {
            WaterPump_Ctrl(160);
            delay_ms(2000);
        }
        WaterPump_Ctrl(100);
        pMotor->SetDoutNaber = 0xFF;
        pMotor->SetDoutConut = 0;
        pMotor->oneCoorEn = 0;
        pMotor->NewCom = 1;
        break;

        /* "UV" light control */
    case 0x86:
        OUT_UV(data[0]);
        break;

    /* light control */
    case 0x87:
        OUT_LED(data[0]);
        break;

    /* water pump control */
    case 0x88:
        if (data[0] == 0)
            WaterPump_Ctrl(100);
        else
            WaterPump_Ctrl(60);
        break;

    /* device status feedback */
    case 0x89:
        back_device_status();
        break;

    /* single point move */
    case 0x8A:
        pMotor->SetDoutNaber = 0;
        pMotor->oneCoorEn = 1;
        pMotor->NewCom = 1;
        for (int n = 0; n < 4; n++)
        {
            pMotor->oneCoor[n] = ((int32_t)data[n * 8 + 0]) |
                                 ((int32_t)data[n * 8 + 1] << 8) |
                                 ((int32_t)data[n * 8 + 2] << 16) |
                                 ((int32_t)data[n * 8 + 3] << 24);
            pMotor->oneSpeed[n] = ((int32_t)data[n * 8 + 4]) |
                                  ((int32_t)data[n * 8 + 5] << 8) |
                                  ((int32_t)data[n * 8 + 6] << 16) |
                                  ((int32_t)data[n * 8 + 7] << 24);
        }
        break;

    /* stop now running */
    case 0x8B:
        stop_device(data[0]);
        break;

    /* system reset*/
    case 0x8F:
        NVIC_SystemReset();
        break;

    /* read offset data */
    case 0x90:
        back_offset();
        break;

    /* write offset data */
    case 0x91:
        set_offset(X_OFFSET, (int8_t)data[0] * 100);
        set_offset(Y_OFFSET, (int8_t)data[1] * 100);
        set_offset(Z_OFFSET, (int8_t)data[2] * 100);
        break;

    /* move to test point, eye or month */
    case 0x92:
        move_to_testPoint(data[0], msg);
        break;

    /* set "x" offset */
    case 0x93:
        set_offset(X_OFFSET, (int8_t)data[0] * 100);
        move_to_testPoint(0, msg); // offset move
        break;

    /* set "y" offset */
    case 0x94:
        set_offset(Y_OFFSET, (int8_t)data[0] * 100);
        move_to_testPoint(0, msg); // offset move
        break;

    /* set "z" offset */
    case 0x95:
        set_offset(Z_OFFSET, (int8_t)data[0] * 100);
        move_to_testPoint(0, msg); // offset move
        break;

    /* auto test */
    case 0x96:
        OUT_UV(data[0]);
        MotorData.SetDoutNaber = 99;
        MotorData.SetDoutBuff = (int16_t *)test_point;
        MotorData.SetDoutConutAll = test_point_len;

        MotorData.SetDoutConut = 0;
        MotorData.oneCoorEn = 0;
        MotorData.NewCom = 1;
        break;

    /* aging test */
    case 0xA0:
        // run_old(newComm.data[0] * 3600 * 1000);
        break;

    default:
        break;
    }

    if (cmd != 0x85 && cmd != 0x89 && cmd != 0x90)
    {
        usart0_send_data((uint8_t *)"OK", 2);
    }
}

/**
 * @brief check usart frame
 * @attention 仅当串口空闲时使用
 */
CommType_t *comm_check(uint8_t *pbuf, uint16_t len)
{
    /* check head */
    uint8_t head = pbuf[0];
    if (head != 0x5A)
    {
        return NULL;
    }

    /* get data len */
    params.dataLen = (((uint16_t)pbuf[1] << 8) | pbuf[2]) - 3; // lost crc and cmd.

    /* check crc */
    params.crc = ((uint16_t)pbuf[len - 2] << 8) | pbuf[len - 1];
    uint16_t crc = ym_crc16(&pbuf[3], params.dataLen + 1); // cmd + data

#if COMMAND_CRC_EN
    if (crc != params.crc)
    {
        return NULL;
        printf("crc error");
    }
#endif

    /* get data */
    params.frameLen = len;
    params.cmd = pbuf[3];
    params.data = &pbuf[4];

    return &params;
}

void comm_handle()
{
    /* check fifo buffer data length */
    uint32_t oc_size = fifo_dynamic_get_occupy_size(USART0_FIFO_NUM);
    if (oc_size == 0)
        return;

    /* read out fifo buffer data */
    memset(&params, 0, sizeof(CommType_t));
    fifo_dynamic_read(USART0_FIFO_NUM, params.frame, oc_size);

    /* check update cmd */
    if (memcmp("upgrade", params.frame, 7) == 0)
    {
        update_setFlag();
        NVIC_SystemReset();
    }

    /* get version */
    if (memcmp("version", params.frame, 7) == 0)
    {
        printf("%s", version);
    }

    /* check command */
    CommType_t *comm = comm_check(params.frame, oc_size);
    if (comm == NULL)
        return;

    /* execute command */
    comm_execute(comm->cmd, comm->data, comm->dataLen, &MotorData);
}