#include "flash_cfg.h"
#include "main.h"
#include "stm32h7xx_hal.h"
#include "lib_log.h"
#include <string.h>

// STM32H723VG single-bank flash layout:
//   8 sectors × 128 KB = 1 MB total at 0x08000000
//   Sector 7 (last 128 KB, 0x080E0000..0x080FFFFF) reserved here for user cfg.
//
// Current firmware uses sectors 0..6 (~855 KB / 896 KB). As long as code stays
// under 896 KB this sector is free. If size grows past that, reduce LENGTH in
// STM32H723VGTX_FLASH.ld or move this region.
#define FLASH_CFG_SECTOR       FLASH_SECTOR_7
#define FLASH_CFG_BANK         FLASH_BANK_1
#define FLASH_CFG_BASE         0x080E0000U
#define FLASH_CFG_SIZE         0x00020000U                       // 128 KB
#define FLASH_CFG_END          (FLASH_CFG_BASE + FLASH_CFG_SIZE)

// STM32H7 program unit = 256 bits = 32 bytes ("flash word")
#define FLASH_WORD_SIZE        32U

// Bumped from 'PSVS' when bootId and deviceId joined the entry. Entries written
// by the older layout keep the old magic and simply read as invalid, so the
// append log continues past them and no migration step is needed.
#define FLASH_CFG_MAGIC        0x32565350U   // 'PSV2' little-endian

// Erase once fewer than this many slots remain, so the erase always happens in
// v_Flash_Cfg_Init() — the only point where it does not race the watchdog. One
// slot is consumed per boot plus the occasional setting change, so 64 slots is
// well over a week of field use.
#define FLASH_CFG_ERASE_MARGIN 64U

typedef struct __attribute__((aligned(32))) {
	uint32_t u32_magic;
	uint32_t u32_bootId;
	uint32_t u32_bootId_inv;                  // XOR check (bootId ^ inv == 0xFFFFFFFF)
	uint8_t  u8_devId[FLASH_CFG_DEVID_LEN];   // 0xFF.. until initLogIdentity(0x23)
	uint8_t  u8_vol;
	uint8_t  u8_vol_inv;                      // XOR check (vol ^ vol_inv == 0xFF)
	uint8_t  u8_pad[12];                      // 0xFF fill keeps the slot distinguishable from erased state
} x_flash_entry_t;

_Static_assert(sizeof(x_flash_entry_t) == FLASH_WORD_SIZE,
		"flash cfg entry must match flash word size");

// RAM snapshot. Every write persists all of it, so a partial update cannot leave
// two fields disagreeing across entries.
static uint8_t  u8_state_vol = 1U;
static uint8_t  u8_state_devId[FLASH_CFG_DEVID_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint32_t u32_state_bootId;
static int      i_state_loaded;

static int b_entry_valid(const x_flash_entry_t* p){
	return (p->u32_magic == FLASH_CFG_MAGIC) &&
	       ((uint8_t)(p->u8_vol ^ p->u8_vol_inv) == 0xFFU) &&
	       ((p->u32_bootId ^ p->u32_bootId_inv) == 0xFFFFFFFFU);
}

static int b_entry_erased(uint32_t u32_addr){
	const uint32_t* p = (const uint32_t*)u32_addr;
	for(uint32_t i = 0; i < FLASH_WORD_SIZE / sizeof(uint32_t); i++){
		if(p[i] != 0xFFFFFFFFU) return 0;
	}
	return 1;
}

static uint32_t u32_find_last_entry(void){
	for(uint32_t addr = FLASH_CFG_END - FLASH_WORD_SIZE;
	    addr >= FLASH_CFG_BASE;
	    addr -= FLASH_WORD_SIZE){
		const x_flash_entry_t* p = (const x_flash_entry_t*)addr;
		if(b_entry_valid(p)) return addr;
		if(addr == FLASH_CFG_BASE) break;   // guard against uint underflow
	}
	return 0U;
}

static uint32_t u32_find_next_free(void){
	for(uint32_t addr = FLASH_CFG_BASE; addr < FLASH_CFG_END; addr += FLASH_WORD_SIZE){
		if(b_entry_erased(addr)) return addr;
	}
	return 0U;
}

static uint32_t u32_free_slots(void){
	uint32_t u32_addr = u32_find_next_free();
	if(u32_addr == 0U) return 0U;
	return (FLASH_CFG_END - u32_addr) / FLASH_WORD_SIZE;
}

// Flash is cacheable: MPU region 0 disables subregion 0, so 0x08000000 falls back
// to the default memory map. Without this the next scan reads pre-erase or
// pre-program content out of the D-Cache.
static void v_invalidate(uint32_t u32_addr, uint32_t u32_size){
	SCB_InvalidateDCache_by_Addr((uint32_t*)u32_addr, (int32_t)u32_size);
}

static HAL_StatusTypeDef e_erase_sector(void){
	FLASH_EraseInitTypeDef x_erase;
	uint32_t u32_err;
	memset(&x_erase, 0, sizeof(x_erase));
	x_erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
	x_erase.Banks        = FLASH_CFG_BANK;
	x_erase.Sector       = FLASH_CFG_SECTOR;
	x_erase.NbSectors    = 1;
	x_erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

	HAL_StatusTypeDef e_res = HAL_FLASHEx_Erase(&x_erase, &u32_err);
	v_invalidate(FLASH_CFG_BASE, FLASH_CFG_SIZE);
	return e_res;
}

/*
 * brief	: persist the whole RAM snapshot into the next free slot
 * note
 * - Never erases. v_Flash_Cfg_Init() keeps a margin of free slots precisely so a
 *   runtime save cannot land on an erase, which would block for 1-4 s and trip
 *   the 2 s watchdog.
 */
static void v_write_snapshot(void){
	uint32_t u32_addr = u32_find_next_free();
	if(u32_addr == 0U){
		LOG_ERROR("FLASH_CFG", "sector full, snapshot not stored");
		return;
	}

	x_flash_entry_t x_entry;
	memset(&x_entry, 0xFF, sizeof(x_entry));
	x_entry.u32_magic      = FLASH_CFG_MAGIC;
	x_entry.u32_bootId     = u32_state_bootId;
	x_entry.u32_bootId_inv = ~u32_state_bootId;
	memcpy(x_entry.u8_devId, u8_state_devId, FLASH_CFG_DEVID_LEN);
	x_entry.u8_vol     = u8_state_vol;
	x_entry.u8_vol_inv = (uint8_t)(~u8_state_vol);

	if(HAL_FLASH_Unlock() != HAL_OK) return;
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_FLASHWORD, u32_addr, (uint32_t)&x_entry);
	HAL_FLASH_Lock();
	v_invalidate(u32_addr, FLASH_WORD_SIZE);
}

void v_Flash_Cfg_Init(void){
	if(i_state_loaded) return;

	uint32_t u32_addr = u32_find_last_entry();
	if(u32_addr != 0U){
		const x_flash_entry_t* p = (const x_flash_entry_t*)u32_addr;
		u32_state_bootId = p->u32_bootId;
		memcpy(u8_state_devId, p->u8_devId, FLASH_CFG_DEVID_LEN);
		if(p->u8_vol >= 1U && p->u8_vol <= 3U) u8_state_vol = p->u8_vol;
	}
	i_state_loaded = 1;

	u32_state_bootId++;

	// Erase now, while the watchdog is still stopped, so no runtime save has to.
	if(u32_free_slots() < FLASH_CFG_ERASE_MARGIN){
		LOG_INFO("FLASH_CFG", "%u slots left, erasing sector", (unsigned)u32_free_slots());
		if(HAL_FLASH_Unlock() == HAL_OK){
			if(e_erase_sector() != HAL_OK){
				LOG_ERROR("FLASH_CFG", "sector erase failed");
			}
			HAL_FLASH_Lock();
		}
	}

	v_write_snapshot();
	LOG_INFO("FLASH_CFG", "bootId=%u vol=%u slots=%u",
			(unsigned)u32_state_bootId, u8_state_vol, (unsigned)u32_free_slots());
}

uint32_t u32_Flash_Cfg_Get_BootId(void){
	return u32_state_bootId;
}

const uint8_t* pu8_Flash_Cfg_Get_DeviceId(void){
	return u8_state_devId;
}

void v_Flash_Cfg_Set_DeviceId(const uint8_t* pu8_id){
	if(pu8_id == NULL) return;
	if(memcmp(u8_state_devId, pu8_id, FLASH_CFG_DEVID_LEN) == 0) return;
	memcpy(u8_state_devId, pu8_id, FLASH_CFG_DEVID_LEN);
	v_write_snapshot();
}

void v_Flash_Cfg_Save_VolLevel(uint8_t u8_lv){
	if(u8_state_vol == u8_lv) return;   // nothing changed, spend no slot
	u8_state_vol = u8_lv;
	v_write_snapshot();
}

uint8_t u8_Flash_Cfg_Load_VolLevel(void){
	return u8_state_vol;                // v_Flash_Cfg_Init() runs first, from main()
}
