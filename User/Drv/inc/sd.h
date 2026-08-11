#ifndef __JH_SD_H
#define __JH_SD_H

#include "lib_def.h"


void v_SD_Deinit();

void v_SD_Init();
void v_SD_Test();

bool b_IsMountSD();
bool b_MountSD();
bool b_UnMountSD();

// SD Sensor Log — 80 B records in /LOG/<dev>/<bootId>_<fileIndex>.psa
// (SD logging spec sections 7 and 8.1).
//
// Files are created on the first sample that needs one, not at start-up, so a
// session that never samples leaves nothing behind.
bool b_SD_Log_Init();
void v_SD_Log_Write(const uint8_t* pu8_payload, uint16_t u16_len,
		uint8_t u8_devMode, uint16_t u16_errMask, bool b_txOk);
void v_SD_Log_Flush();
void v_SD_Log_Close();

// ctrlLogEnable(0x56). Stopping flushes and closes, so the card is safe to pull.
// Logging is on from boot — the ESP32 is not required to ask for it.
void v_SD_Log_SetEnabled(bool b_en);

// evtLogError(0x84) reason codes (spec section 6.7)
typedef enum {
	SD_LOG_ERR_MOUNT	= 0,	// mount failed, retried periodically
	SD_LOG_ERR_NO_CARD	= 1,	// absent at boot, or pulled at run time
	SD_LOG_ERR_NO_SPACE	= 2,	// full; logging stops, nothing is deleted
	SD_LOG_ERR_WRITE	= 3,	// write failed repeatedly, remount attempted
	SD_LOG_ERR_FS		= 4,	// filesystem damaged; logging stops
	SD_LOG_ERR_BACKFILL	= 5,	// backfill read failed; logging continues
} e_SD_LOG_ERR_t;

// Card presence, remount and capacity. Call from the main loop; it rate-limits
// itself. Media faults are reported through b_SD_Log_Get_Error rather than sent
// from here, so this driver keeps no dependency on the protocol layer.
void v_SD_Log_Media_Handler();

// Drains one pending fault. false when there is nothing to report.
bool b_SD_Log_Get_Error(uint8_t* pu8_reason, uint16_t* pu16_detail);

uint8_t  u8_SD_Log_Get_State();			// logState for reqLogStatus: 0 stop, 1 logging, 2 fault
uint16_t u16_SD_Log_Get_FreeMB();		// 0xFFFF when unmounted or not known

uint32_t u32_SD_Log_Get_Seq();			// next sample number
uint32_t u32_SD_Log_Get_FlushedSeq();	// last seq actually on the card
uint32_t u32_SD_Log_Get_FileIndex();
uint16_t u16_SD_Log_Get_WriteErrCnt();

#endif


