#ifndef __DATA_POS_H__
#define __DATA_POS_H__

#include "bsp.h"


#define W25Q64_SECTOR_SIZE    4096    // 4KB per sector
#define W25Q64_SECTOR_COUNT   2048    // Total sectors
#define W25Q64_TOTAL_SIZE     (8 * 1024 * 1024)  // 8MB


#define YAW_KP_ADDR 0x1000
#define YAW_KI_ADDR 0x2000
#define YAW_KD_ADDR 0x3000

#define GRAY_KP_ADDR 0x4000
#define GRAY_KI_ADDR 0x5000
#define GRAY_KD_ADDR 0x6000


#define BLOBS_KP_ADDR 0x7000
#define BLOBS_KI_ADDR 0x8000
#define BLOBS_KD_ADDR 0x9000

#define GO_TARGET_KP_ADDR 0xA000
#define GO_TARGET_KI_ADDR 0xB000
#define GO_TARGET_KD_ADDR 0xC000

uint32_t W25Q64_GetSectorAddress(uint16_t sector_num);



#endif

