#include "fifo.h"
#include <stdlib.h>

_fifo_t *fifo_num[FIFO_NUM];

/* fifo 上锁函数 */
static void fifo_lock(void)
{
    //__disable_irq();
}

/* fifo 解锁函数 */
static void fifo_unlock(void)
{
    //__enable_irq();
}

/**
 * dynamic fifo function.
 */

static bool fifo_check(uint8_t num)
{
    if (num > FIFO_NUM - 1)
    {
        return false;
    }

    if (fifo_num[num] == NULL || fifo_num[num]->buf == NULL)
    {
        return false;
    }

    return true;
}

/**
 * @brief  动态申请一个fifo缓冲区
 * @param  num: fifo编号 0 ~ FIFO_NUM-1
 * @param  size: 申请内存大小，默认为 unsigned char 类型.
 * @retval 成功返回true, 失败返回false.
 */
bool fifo_dynamic_register(uint8_t num, uint32_t size)
{
    if (num > FIFO_NUM - 1 || fifo_num[num] != NULL)
    {
        return false;
    }

    _fifo_t *fifo = (_fifo_t *)malloc(sizeof(_fifo_t));
    if (!fifo)
    {
        return false;
    }

    uint8_t *fifo_buf = (uint8_t *)malloc(sizeof(uint8_t) * size);
    if (!fifo_buf)
    {
        free(fifo);
        return false;
    }

    fifo_num[num] = fifo;

    fifo_register(fifo_num[num], fifo_buf, size, NULL, NULL);

    return true;
}

/**
 * @brief  释放申请的fifo内存
 * @param  num: fifo编号 0 ~ FIFO_NUM-1
 * @retval None.
 */
void fifo_dynamic_release(uint8_t num)
{
    if (!fifo_check(num))
    {
        return;
    }

    free(fifo_num[num]->buf);
    free(fifo_num[num]);

    fifo_num[num] = NULL;
}

void fifo_dynamic_clear(uint8_t num)
{
    if (!fifo_check(num))
    {
        return;
    }
    _fifo_t *pfifo = fifo_num[num];

    fifo_clear(pfifo);
}

uint32_t fifo_dynamic_write(uint8_t num, const uint8_t *pbuf, uint32_t size)
{
    if (!fifo_check(num))
    {
        return 0;
    }

    _fifo_t *pfifo = fifo_num[num];

    return fifo_write(pfifo, pbuf, size);
}

uint32_t fifo_dynamic_read(uint8_t num, uint8_t *pbuf, uint32_t size)
{
    if (!fifo_check(num))
    {
        return 0;
    }

    _fifo_t *pfifo = fifo_num[num];

    return fifo_read(pfifo, pbuf, size);
}

uint32_t fifo_dynamic_get_occupy_size(uint8_t num)
{
    if (!fifo_check(num))
    {
        return 0;
    }

    _fifo_t *pfifo = fifo_num[num];

    return fifo_get_occupy_size(pfifo);
}

/**
 * static fifo function
 */

/**
 * @brief  注册一个fifo
 * @param  pfifo: fifo结构体指针
 * @param  pfifo_buf: fifo内存块
 * @param  size:   内存大小
 * @retval none
 */
void fifo_register(_fifo_t *pfifo, uint8_t *pfifo_buf, uint32_t size, lock_fun lock, lock_fun unlock)
{
    pfifo->buf_size = size;
    pfifo->buf = pfifo_buf;
    pfifo->pwrite = pfifo->buf;
    pfifo->pread = pfifo->buf;
    pfifo->occupy_size = 0;
    pfifo->lock = lock;
    pfifo->unlock = unlock;
}

/**
 * @brief  释放fifo
 * @param  pfifo: fifo结构体指针
 * @retval none
 */
void fifo_release(_fifo_t *pfifo)
{
    pfifo->buf_size = 0;
    pfifo->occupy_size = 0;
    pfifo->buf = NULL;
    pfifo->pwrite = 0;
    pfifo->pread = 0;
    pfifo->lock = NULL;
    pfifo->unlock = NULL;
}

/**
 * @brief 清空fifo缓冲区到初始状态
 * @param pfifo: fifo结构体指针
 * @return None
 */
void fifo_clear(_fifo_t *pfifo)
{
    pfifo->pwrite = pfifo->buf;
    pfifo->pread = pfifo->buf;
    pfifo->occupy_size = 0;
}

/**
 * @brief  往fifo写数据
 * @param  pfifo: fifo结构体指针
 * @param  pbuf: 待写数据
 * @param  size: 待写数据大小
 * @retval 实际写入大小
 */
uint32_t fifo_write(_fifo_t *pfifo, const uint8_t *pbuf, uint32_t size)
{
    uint32_t w_size = 0, free_size = 0;

    if ((size == 0) || (pfifo == NULL) || (pbuf == NULL))
    {
        return 0;
    }

    free_size = fifo_get_free_size(pfifo);
    if (free_size == 0)
    {
        return 0;
    }

    if (free_size < size)
    {
        size = free_size;
    }
    w_size = size;
    if (pfifo->lock != NULL)
        pfifo->lock();
    while (w_size-- > 0)
    {
        *pfifo->pwrite++ = *pbuf++;
        if (pfifo->pwrite >= (pfifo->buf + pfifo->buf_size))
        {
            pfifo->pwrite = pfifo->buf;
        }
        pfifo->occupy_size++;
    }
    if (pfifo->unlock != NULL)
        pfifo->unlock();
    return size;
}

/**
 * @brief  从fifo读数据
 * @param  pfifo: fifo结构体指针
 * @param  pbuf: 待读数据缓存
 * @param  size: 待读数据大小
 * @retval 实际读大小
 */
uint32_t fifo_read(_fifo_t *pfifo, uint8_t *pbuf, uint32_t size)
{
    uint32_t r_size = 0, occupy_size = 0;

    if ((size == 0) || (pfifo == NULL) || (pbuf == NULL))
    {
        return 0;
    }

    occupy_size = fifo_get_occupy_size(pfifo);
    if (occupy_size == 0)
    {
        return 0;
    }

    if (occupy_size < size)
    {
        size = occupy_size;
    }
    if (pfifo->lock != NULL)
        pfifo->lock();
    r_size = size;
    while (r_size-- > 0)
    {
        *pbuf++ = *pfifo->pread++;
        if (pfifo->pread >= (pfifo->buf + pfifo->buf_size))
        {
            pfifo->pread = pfifo->buf;
        }
        pfifo->occupy_size--;
    }
    if (pfifo->unlock != NULL)
        pfifo->unlock();
    return size;
}

/**
 * @brief  获取fifo空间大小
 * @param  pfifo: fifo结构体指针
 * @retval fifo内存总大小
 */
uint32_t fifo_get_total_size(_fifo_t *pfifo)
{
    if (pfifo == NULL)
        return 0;

    return pfifo->buf_size;
}

/**
 * @brief  获取fifo空闲空间大小
 * @param  pfifo: fifo结构体指针
 * @retval 空闲空间大小
 */
uint32_t fifo_get_free_size(_fifo_t *pfifo)
{
    uint32_t size;

    if (pfifo == NULL)
        return 0;

    size = pfifo->buf_size - fifo_get_occupy_size(pfifo);

    return size;
}

/**
 * @brief  获取fifo已用空间大小
 * @param  pfifo: fifo结构体指针
 * @retval fifo已用大小
 */
uint32_t fifo_get_occupy_size(_fifo_t *pfifo)
{
    if (pfifo == NULL)
        return 0;

    return pfifo->occupy_size;
}
