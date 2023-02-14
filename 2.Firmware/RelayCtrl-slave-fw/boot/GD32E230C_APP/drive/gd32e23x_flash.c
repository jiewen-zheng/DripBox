#include "gd32e23x_flash.h"

static uint32_t get_page_start_addr(uint32_t addr)
{
    addr = (addr / GD32E23X_FLASH_PAGE_SIZE) * GD32E23X_FLASH_PAGE_SIZE;

    return addr;
}

bool erase_flash_page(uint32_t startAddr, uint32_t endAddr)
{
    if (endAddr < startAddr)
    {
        return false;
    }

    if (startAddr < GD32E23X_FLASH_BASE_ADDR || (endAddr > (GD32E23X_FLASH_BASE_ADDR + GD32E23X_FLASH_PAGE_NUM * GD32E23X_FLASH_PAGE_SIZE)))
    {
        return false;
    }

    uint32_t page_num = (endAddr - startAddr) / GD32E23X_FLASH_PAGE_SIZE + 1;
    uint32_t erase_addr = 0;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
    for (uint16_t i = 0; i < page_num; i++)
    {
        erase_addr = get_page_start_addr(startAddr);
        fmc_page_erase(erase_addr);
        startAddr += GD32E23X_FLASH_PAGE_SIZE;
    }
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
    fmc_lock();

    return true;
}

bool program_flash(uint32_t startAddr, const uint32_t *wbuff, uint32_t len)
{

    uint32_t program_addr = startAddr;

    fmc_unlock();
    fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);

    while (program_addr < (startAddr + len))
    {
        if (fmc_word_program(program_addr, *wbuff) != FMC_READY)
        {
            fmc_flag_clear(FMC_FLAG_END | FMC_FLAG_WPERR | FMC_FLAG_PGERR);
            fmc_lock();
            return false;
        }
        program_addr += 4;
        wbuff++;
    }

    fmc_lock();

    return true;
}

bool read_flash(uint32_t startAddr, uint32_t *rbuff, uint32_t len)
{
    uint32_t read_addr = startAddr;

    if (startAddr < GD32E23X_FLASH_BASE_ADDR || (startAddr > (GD32E23X_FLASH_BASE_ADDR + GD32E23X_FLASH_PAGE_NUM * GD32E23X_FLASH_PAGE_SIZE)))
    {
        return false;
    }

    while (read_addr < (startAddr + len))
    {
        *rbuff++ = *(uint32_t *)(read_addr);
        read_addr += 4;
    }

    return true;
}