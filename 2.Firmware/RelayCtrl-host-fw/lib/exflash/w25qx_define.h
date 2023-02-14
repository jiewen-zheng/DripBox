#ifndef __W25QX_DEFINE_H
#define __W25QX_DEFINE_H

// #define W25Q80
// #define W25Q16
// #define W25Q32
// #define W25Q64
#define W25Q128

#define W25QX_SCLK 12
#define W25QX_MISO 13
#define W25QX_MOSI 11
#define W25QX_CS -1 // not connected

#if !defined(W25Q80) && !defined(W25Q16) && !defined(W25Q32) && !defined(W25Q64) && !defined(W25Q128)
#define W25Q80
#endif

#define W25QX_SECTOR_SIZE 4096     // w25qx sector size is 4kb
#define W25QX_BLOCK_SIZE 16 * 4096 // a block has 16 sectors

#define W25QX_JEDECID 0xEF40

#if defined(W25Q80)
#define W25QX_ID 0xEF13
#define W25QX_SECTOR_COUNT 16 * 16 // w25q80 have 16 block, a block has 16 sectors
#elif defined(W25Q16)
#define W25QX_ID 0xEF14
#define W25QX_SECTOR_COUNT 32 * 16 // w25q16 have 32 block, a block has 16 sectors
#elif defined(W25Q32)
#define W25QX_ID 0xEF15
#define W25QX_SECTOR_COUNT 64 * 16 // w25q32 have 64 block, a block has 16 sectors
#elif defined(W25Q64)
#define W25QX_ID 0xEF16
#define W25QX_SECTOR_COUNT 128 * 16 // w25q64 have 128 block, a block has 16 sectors
#elif defined(W25Q128)
#define W25QX_ID 0xEF17
#define W25QX_SECTOR_COUNT 256 * 16 // w25q128 have 256 block, a block has 16 sectors
#endif

#define W25QX_WriteEnable 0x06    // write enable
#define W25QX_WriteDisnable 0x04  // write disable
#define W25QX_SR_WriteEnable 0x50 // sr write enable

#define W25QX_ReleasePowerDown 0xAB // power wake up
#define W25QX_PowerDown 0xB9        // power down

#define W25QX_ReadManufactDeviceID 0x90 // Manufact id
#define W25QX_ReadJedecID 0x9F          // jedec id
#define W25QX_ReadUniqueID 0x4B         // unique id

#define W25QX_ReadData 0x03         // read data
#define W25QX_FastRead 0x0B         // fast read data
#define W25QX_PageProgram 0x02      // page program
#define W25QX_SectorErase_4KB 0x20  // setor erase 4kb
#define W25QX_SectorErase_32KB 0x52 // setor erase 32kb
#define W25QX_SectorErase_64KB 0xD8 // setor erase 64kb
#define W25QX_ChipErase 0xC7        // chip erase
// #define W25QX_ChipErase             0x60  // chip erase

#define W25QX_ReadStatusReg 0x05  // Read Status Register
#define W25QX_WriteStatusReg 0x01 // Read Status Register

#endif /* __W25QX_H */