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
	// The index is full, so a file on the card is not in reqLogFiles and cannot
	// be asked for. Logging is unaffected -- the records are written, they just
	// have no way out. Detail carries the file count so the reader can see how
	// far past the cap it is. This is reported because the alternative is a file
	// that disappears from the listing with nothing said, which reads as "never
	// recorded" and would only surface long after the fact.
	SD_LOG_ERR_IDX_FULL	= 6,	// index cap reached; that file is unreachable
} e_SD_LOG_ERR_t;

// Card presence, remount and capacity. Call from the main loop; it rate-limits
// itself. Media faults are reported through b_SD_Log_Get_Error rather than sent
// from here, so this driver keeps no dependency on the protocol layer.
void v_SD_Log_Media_Handler();

// Drains one pending fault. false when there is nothing to report.
bool b_SD_Log_Get_Error(uint8_t* pu8_reason, uint16_t* pu16_detail);

uint8_t  u8_SD_Log_Get_State();			// logState for reqLogStatus: 0 stop, 1 logging, 2 fault
uint16_t u16_SD_Log_Get_FreeMB();		// 0xFFFF when unmounted or not known

// Log file index, built once at start-up for reqLogFiles(0x45). Sorted by
// bootId then fileIndex ascending, so the oldest unsent data comes first.
// Files holding no record are excluded — reqLogFiles cannot express a lastSeq
// for them.
void     v_SD_Log_Scan();
// Files on the card, against u16_SD_Log_Idx_Count() which stops at the cap.
// The two are equal until the index fills; after that the difference is how
// many files exist but cannot be listed or backfilled.
// Remove one indexed file, for ctrlLogDelete(0x57). The decision belongs to the
// PC -- only it knows the records arrived, verified and merged -- so this checks
// nothing about recovery, only that the file is not in use right now.
// 0 deleted, 1 not indexed, 2 currently being written, 3 backfill running,
// 4 filesystem error.
uint8_t  u8_SD_Log_Delete(uint32_t u32_boot, uint16_t u16_idx);
uint16_t u16_SD_Log_Files_On_Card();
uint16_t u16_SD_Log_Idx_Count();
// The cap itself, reported so the reader does not have to hold a copy of it.
// A reduced build used to leave the two sides disagreeing until both were
// edited by hand; carrying it on the wire is what stops that.
uint16_t u16_SD_Log_Idx_Capacity();
bool     b_SD_Log_Idx_Get(uint16_t u16_n, uint32_t* pu32_boot, uint32_t* pu32_first,
		uint32_t* pu32_last, uint16_t* pu16_idx);

// Backfill reader for reqLogRead(0x44). Records are handed out one at a time;
// the protocol layer frames and paces them, this driver never transmits.
//
// Start returns 0xFF when streaming begins, otherwise the statLogChunk END
// result code to send straight back: 1 no such bootId, 2 range not held,
// 3 read error.
uint8_t  u8_SD_Log_Backfill_Start(uint32_t u32_boot, uint32_t u32_startSeq, uint16_t u16_count);
bool     b_SD_Log_Backfill_Next(uint8_t* pu8_rec, uint8_t* pu8_result);
void     v_SD_Log_Backfill_Abort(uint8_t u8_result);
bool     b_SD_Log_Backfill_Active();
uint32_t u32_SD_Log_Backfill_LastSent();

uint32_t u32_SD_Log_Get_Seq();			// next sample number
uint32_t u32_SD_Log_Get_FlushedSeq();	// last seq actually on the card
uint32_t u32_SD_Log_Get_FileIndex();
uint16_t u16_SD_Log_Get_WriteErrCnt();

#endif


