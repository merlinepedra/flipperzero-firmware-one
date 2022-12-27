#pragma once
#include <stdint.h>
#include <stdbool.h>

#define __IO volatile

#define SD_TIMEOUT_MS (1000)

typedef enum {
    SDStatusOK,
    SDStatusError,
    SDStatusTimeout,
} SDStatus;

/**
 * @brief Card Specific Data: CSD Register
 */
typedef struct {
    /* Header part */
    uint8_t CSDStruct : 2; /* CSD structure */
    uint8_t Reserved1 : 6; /* Reserved */
    uint8_t TAAC : 8; /* Data read access-time 1 */
    uint8_t NSAC : 8; /* Data read access-time 2 in CLK cycles */
    uint8_t MaxBusClkFrec : 8; /* Max. bus clock frequency */
    uint16_t CardComdClasses : 12; /* Card command classes */
    uint8_t RdBlockLen : 4; /* Max. read data block length */
    uint8_t PartBlockRead : 1; /* Partial blocks for read allowed */
    uint8_t WrBlockMisalign : 1; /* Write block misalignment */
    uint8_t RdBlockMisalign : 1; /* Read block misalignment */
    uint8_t DSRImpl : 1; /* DSR implemented */

    /* v1 or v2 struct */
    union csd_version {
        struct {
            uint8_t Reserved1 : 2; /* Reserved */
            uint16_t DeviceSize : 12; /* Device Size */
            uint8_t MaxRdCurrentVDDMin : 3; /* Max. read current @ VDD min */
            uint8_t MaxRdCurrentVDDMax : 3; /* Max. read current @ VDD max */
            uint8_t MaxWrCurrentVDDMin : 3; /* Max. write current @ VDD min */
            uint8_t MaxWrCurrentVDDMax : 3; /* Max. write current @ VDD max */
            uint8_t DeviceSizeMul : 3; /* Device size multiplier */
        } v1;
        struct {
            uint8_t Reserved1 : 6; /* Reserved */
            uint32_t DeviceSize : 22; /* Device Size */
            uint8_t Reserved2 : 1; /* Reserved */
        } v2;
    } version;

    uint8_t EraseSingleBlockEnable : 1; /* Erase single block enable */
    uint8_t EraseSectorSize : 7; /* Erase group size multiplier */
    uint8_t WrProtectGrSize : 7; /* Write protect group size */
    uint8_t WrProtectGrEnable : 1; /* Write protect group enable */
    uint8_t Reserved2 : 2; /* Reserved */
    uint8_t WrSpeedFact : 3; /* Write speed factor */
    uint8_t MaxWrBlockLen : 4; /* Max. write data block length */
    uint8_t WriteBlockPartial : 1; /* Partial blocks for write allowed */
    uint8_t Reserved3 : 5; /* Reserved */
    uint8_t FileFormatGrouop : 1; /* File format group */
    uint8_t CopyFlag : 1; /* Copy flag (OTP) */
    uint8_t PermWrProtect : 1; /* Permanent write protection */
    uint8_t TempWrProtect : 1; /* Temporary write protection */
    uint8_t FileFormat : 2; /* File Format */
    uint8_t Reserved4 : 2; /* Reserved */
    uint8_t crc : 7; /* Reserved */
    uint8_t Reserved5 : 1; /* always 1*/

} SD_CSD;

/**
 * @brief Card Identification Data: CID Register
 */
typedef struct {
    __IO uint8_t ManufacturerID; /* ManufacturerID */
    __IO uint16_t OEM_AppliID; /* OEM/Application ID */
    __IO uint32_t ProdName1; /* Product Name part1 */
    __IO uint8_t ProdName2; /* Product Name part2*/
    __IO uint8_t ProdRev; /* Product Revision */
    __IO uint32_t ProdSN; /* Product Serial Number */
    __IO uint8_t Reserved1; /* Reserved1 */
    __IO uint16_t ManufactDate; /* Manufacturing Date */
    __IO uint8_t CID_CRC; /* CID CRC */
    __IO uint8_t Reserved2; /* always 1 */
} SD_CID;

/**
 * @brief SD Card information structure
 */
typedef struct {
    SD_CSD Csd;
    SD_CID Cid;
    uint64_t CardCapacity; /*!< Card Capacity */
    uint32_t CardBlockSize; /*!< Card Block Size */
    uint32_t LogBlockNbr; /*!< Specifies the Card logical Capacity in blocks   */
    uint32_t LogBlockSize; /*!< Specifies logical block size in bytes           */
} SD_CardInfo;

/**
 * @brief SD card max mount retry count
 * 
 * @return uint8_t 
 */
uint8_t sd_max_mount_retry_count();

/**
 * @brief Init sd card
 * 
 * @param power_reset reset card power
 * @return SDStatus 
 */
SDStatus sd_init(bool power_reset);

/**
 * @brief Get card state
 * 
 * @return SDStatus 
 */
SDStatus sd_get_card_state(void);

/**
 * @brief Get card info
 * 
 * @param card_info 
 * @return SDStatus 
 */
SDStatus sd_get_card_info(SD_CardInfo* card_info);

/**
 * @brief Read blocks
 * 
 * @param data 
 * @param address 
 * @param blocks 
 * @param timeout 
 * @return SDStatus 
 */
SDStatus sd_read_blocks(uint32_t* data, uint32_t address, uint32_t blocks, uint32_t timeout);

/**
 * @brief Write blocks
 * 
 * @param data 
 * @param address 
 * @param blocks 
 * @param timeout 
 * @return SDStatus 
 */
SDStatus sd_write_blocks(uint32_t* data, uint32_t address, uint32_t blocks, uint32_t timeout);