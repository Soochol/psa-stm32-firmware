#include "main.h"
#include <sd.h>
#include "myDiskio.h"
#include "stdio.h"
#include "string.h"
#include "fatfs.h"
#include "tim.h"
#include "uart.h"
#include "lib_log.h"
#include "lib_crc.h"
#include "flash_cfg.h"		// bootId / deviceId for the .psa file header

#include "minimp3_platform.h"

extern SD_HandleTypeDef hsd2;
SD_HandleTypeDef* p_sd = &hsd2;


#define SD_LOG_ENABLED	0


#define SD_CAHCHED_USED

#ifdef SD_CAHCHED_USED
ALIGN_32BYTES(static FATFS fatFS __attribute__((section(".my_d2_section"))));
#else
static FATFS fatFS;
#endif
//ALIGN_32BYTES(static FATFS fatFS);

//static FATFS fatFS;    /* File system object for SD logical drive */
static FATFS* p_fatFs = &fatFS;
static TCHAR path = 0;	//logical drive name
static TCHAR* p_path = &path;
static FRESULT res;

static FIL myFile;

bool b_SdMount;

#define SDMMC_CK	(uint32_t)192000000


#define SD_INIT_CLK_CHANGE	1



//////////////////////////////////
//		TEST - PROCESS			//
//////////////////////////////////

/*
 * brief	: sd card data pin initialize
 * date
 * - create	: 25.05.26
 * - modify	: -
 *
 */
void v_SD_Init(){

}

void v_SD_Deinit(){
	HAL_SD_DeInit(p_sd);
}

/*
 * brief	: sd card mount
 * date
 * - create	: 25.05.20
 */
void v_MountSD(){
	if(BSP_SD_IsDetected() == SD_PRESENT){
#if SD_INIT_CLK_CHANGE
		p_sd->Init.ClockDiv = (SDMMC_CK / (2 * 400000)) - 2;  // 예: 96MHz → ClockDiv = 119
		if(HAL_SD_Init(p_sd) != HAL_OK){
			printf("sd init fail");
			b_SdMount = false;
			return;
		}
#endif
		memset(&fatFS, 0, sizeof(fatFS));
		//reinitialize to make sure the sd can be mounted several times
		disk_reinitialize(p_fatFs->drv);
		//check if mount was successful
		res = f_mount(p_fatFs, p_path, 1);
		if(res != FR_OK){
			printf("mount fail\r");
			MX_FATFS_DeInit();
			b_SdMount = false;
		}
		else{
#if SD_LOG_ENABLED
			printf("mount succ\r");
#endif
			b_SdMount = true;
#if SD_INIT_CLK_CHANGE
			//HAL_SD_DISABLE(p_sd);  // SDMMC 클럭 변경 전 비활성화
			HAL_SD_DeInit(p_sd);
			p_sd->Init.ClockDiv = 3;   // 예: 196MHz / (2 + 3) = 39.2MHz
			if(HAL_SD_Init(p_sd) != HAL_OK){
				printf("sd init fail");
				b_SdMount = false;
				return;
			}
			HAL_SD_ConfigWideBusOperation(p_sd, SDMMC_BUS_WIDE_4B);
			//HAL_SD_ConfigWideBusOperation(p_sd, SDMMC_BUS_WIDE_1B);
#endif
		}
	}
	else{
		b_SdMount = false;
	}
}


/*
 * brief	: sd card unmount
 * date
 * create	: 25.05.20
 */
void v_UnMountSD(){
	res = f_mount(NULL, "", 0);
	if(res != FR_OK){
		printf("unmount fail\r");
	}
	else{
#if SD_LOG_ENABLED
		printf("unmount succ\r");
#endif
	}
}


/*
 * brief	: open file
 * date
 * create	: 25.05.20
 */
void v_OpenFile(){
	res = f_open(&myFile, "withforce.txt", FA_WRITE|FA_READ);
	if(res != FR_OK){
		printf("open file fail\r");
		res = f_open(&myFile, "withforce.txt", FA_CREATE_ALWAYS | FA_WRITE | FA_READ);
		if(res != FR_OK){
			printf("file create fail\r");
		}
		else{
			printf("file create succ\r");
		}
	}
	else{
		printf("open file succ\r");
	}
}


/*
 * brief	: close file
 * date
 * create	: 25.05.20
 */
void v_CloseFile(){
	res = f_close(&myFile);
	if(res != FR_OK){
		printf("close file fail\r");
	}
	else{
		printf("close file succ\r");
	}
}




uint8_t fileText[32];
uint32_t bytesRead, bytesWrite;
/*
 * brief	: read file
 * date
 * create	: 25.05.20
 */
void v_ReadFile(){
	res = f_read(&myFile, fileText, sizeof(fileText), (void*)&bytesRead);
	if(res != FR_OK || bytesRead == 0){
		printf("read fail\r");
	}
	else{
		printf("read succ\r");
	}
}


/*
 * brief	: write file
 * date
 * create	: 25.05.20
 */
void v_WriteFile(){
	int16_t number = atoi((char const*)fileText);
	number++;
	memset(fileText, 0, sizeof(fileText));
	sprintf((char*)fileText, "%d", number);

	//move cursor
	res = f_lseek(&myFile, 0);
	if(res != FR_OK){
		printf("move cursor fail\r");
	}
	else{
		printf("move cursor succ\r");
	}

	//write number
	int16_t len = strlen((char const*)fileText);
	res = f_write(&myFile, fileText, len, (void*)&bytesWrite);
	if(res != FR_OK || bytesWrite == 0){
		printf("write fail\r");
	}
	else{
		printf("write succ\r");
	}
}




/*
 * brief	: sd card test
 * date
 * create	: 25.03.27
 */
void v_SD_Test(){
	static uint32_t timRef;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 5000)){
		timRef = u32_Tim_1msGet();

		if(b_SdMount == false){
			//v_MountSDIO();
			v_MountSD();
			//b_mount = true;
		}

		if(b_SdMount == true){
			v_OpenFile();
			v_ReadFile();
			v_WriteFile();
			v_CloseFile();
			printf("text : %s\r\n", fileText);
		}
	}
}


//////////////////////////////////
//			LIBRARY				//
//////////////////////////////////

/*
 * brief	: sd card mount check
 * date
 * - create	: 25.06.25
 */
bool b_IsMountSD(){
	return b_SdMount;
}

/*
 * brief	: sd card mount
 * date
 * - create	: 25.06.25
 */
bool b_MountSD(){
	if(BSP_SD_IsDetected() == SD_PRESENT){
#if SD_INIT_CLK_CHANGE
		p_sd->Init.ClockDiv = (SDMMC_CK / (2 * 400000)) - 2;  // 예: 96MHz → ClockDiv = 119
		if(HAL_SD_Init(p_sd) != HAL_OK){
			v_printf_poll("sd init fail");
			b_SdMount = false;
			return false;
		}
#endif
		//reinitialize to make sure the sd can be mounted several times
		disk_reinitialize(p_fatFs->drv);
		res = f_mount(p_fatFs, p_path, 1);
		if(res != FR_OK){
			v_printf_poll("mount fail\r");
			MX_FATFS_DeInit();
			b_SdMount = false;
		}
		else{
#if SD_LOG_ENABLED
			v_printf_poll("mount succ\r");
#endif
			b_SdMount = true;
#if SD_INIT_CLK_CHANGE
			HAL_SD_DeInit(p_sd);
			p_sd->Init.ClockDiv = 3;   // 예: 196MHz / (2 + 3) = 39.2MHz
			if(HAL_SD_Init(p_sd) != HAL_OK){
				v_printf_poll("sd init fail");
				b_SdMount = false;
				return false;
			}
			HAL_SD_ConfigWideBusOperation(p_sd, SDMMC_BUS_WIDE_4B);
#endif
		}
	}
	else{
		b_SdMount = false;
	}
	return b_SdMount;
}

/*
 * brief	: sd card unmount
 * date
 * - create	: 25.06.25
 */
bool b_UnMountSD(){
	bool unmount = false;
	res = f_mount(NULL, "", 0);
	if(res != FR_OK){
		printf("unmount fail\r");
	}
	else{
#if SD_LOG_ENABLED
		printf("unmount succ\r");
#endif
		unmount = true;
	}
	return unmount;
}


//////////////////////////////////
//		SENSOR LOG				//
//////////////////////////////////

// SD logging spec sections 7 / 8.1. Records are a fixed 80 B grid because two
// things depend on it: a merge tool must be able to find record boundaries
// arithmetically after a power cut, and backfill (spec section 6.4) seeks to a
// requested seq with 512 + (seq - firstSeq) * 80 instead of scanning.
//
// That arithmetic only holds while the n-th record in a file carries
// seq == firstSeq + n, so nothing here may ever skip a slot. Where a real sample
// cannot be produced, a placeholder takes its place; where even that is
// impossible, the file ends and a new one starts.
#define SD_LOG_PAYLOAD_SIZE		64			// STAT(0x70) DATA length
#define SD_LOG_REC_SIZE			80
#define SD_LOG_HDR_SIZE			512			// sector aligned; records start here
#define SD_LOG_FORMAT_VER		1
#define SD_LOG_SAMPLE_MHZ		10000U		// 10.000 Hz, in milli-hertz

// record field offsets (spec section 7)
#define SD_REC_SEQ				0
#define SD_REC_TICK				4
#define SD_REC_PAYLOAD			8
#define SD_REC_MODE				72
#define SD_REC_FLAGS			73
#define SD_REC_ERRMASK			74
#define SD_REC_CRC				78

// flags, record offset 73
#define SD_FLAG_TX_OK			(1U << 0)	// this sample's STAT reached the UART queue
#define SD_FLAG_INVALID			(1U << 1)	// placeholder: statPayload is not real data

// header field offsets (spec section 8.1)
#define SD_HDR_MAGIC			0
#define SD_HDR_FORMAT_VER		4
#define SD_HDR_REC_SIZE			6
#define SD_HDR_DEVID			8
#define SD_HDR_BOOTID			14
#define SD_HDR_FILEIDX			18
#define SD_HDR_FIRSTSEQ			22
#define SD_HDR_RATE				26
#define SD_HDR_CRC				30

// Flush policy (spec section 10.2). Two seconds bounds what an unexpected power
// loss costs at 20 samples, and it is also the cheaper option for the control
// loop: 1.6 kB per write against 8.8 kB at the old ten seconds, so the blocking
// peak drops with the loss window rather than trading against it.
#define SD_LOG_FLUSH_ITV		2000		// 2s
#define SD_LOG_FLUSH_REC		20			// ...or 20 records, whichever comes first
#define SD_LOG_BUF_MAX			24			// flush trigger + margin
#define SD_LOG_BUF_SIZE			(SD_LOG_REC_SIZE * SD_LOG_BUF_MAX)

// Rotation caps the damage a corrupt file can do (spec section 8.3). At 10 Hz an
// hour is 2.9 MB, so the time trigger always fires long before the size one —
// the size cap only matters if the sample rate ever rises.
#define SD_LOG_ROTATE_ITV		(60UL * 60UL * 1000UL)		// 1 hour
#define SD_LOG_ROTATE_BYTES		(32UL * 1024UL * 1024UL)	// 32 MB
#define SD_LOG_ROTATE_REC		((SD_LOG_ROTATE_BYTES - SD_LOG_HDR_SIZE) / SD_LOG_REC_SIZE)

#define SD_LOG_DIR				"/LOG"
#define SD_LOG_PATH_MAX			48

static FIL      logFile;
static bool     b_logCrcOk;			// CRC self test passed at start-up
static bool     b_logEnabled;		// on at boot, toggled by ctrlLogEnable(0x56)
static bool     b_logOpen;			// a file is open and its header is on the card
static uint8_t  u8_logBuf[SD_LOG_BUF_SIZE];
static uint16_t u16_logBufIdx;
static uint32_t u32_logFlushRef;
static uint32_t u32_logSeq;			// next sample number
static uint32_t u32_logFlushedSeq;	// last seq actually on the card, for reqLogStatus
static uint32_t u32_logFileIdx;
// Files on the card, which is not the same as files in the index once the cap
// is reached -- and the gap between the two is the only way a reader learns how
// much has fallen out of reach. Counted by the boot scan, then kept current as
// files are created, so reqLogStatus can answer without walking the card again.
static uint16_t u16_logFilesOnCard;
static uint32_t u32_logFileRef;		// tick when the current file was opened
static uint32_t u32_logFileRec;		// records placed in the current file
static uint16_t u16_logWrErr;		// reported as writeErrorCount in reqLogStatus(0x43)
static bool     b_logFault;			// unrecoverable: full card or damaged filesystem
static uint8_t  u8_logWrFail;		// consecutive failed flushes
static bool     b_logCardSeen;		// card was present at the last media poll
static uint32_t u32_logMediaRef;
static uint32_t u32_logMountRef;

// One slot is enough: the drain runs every main loop pass and faults arrive at
// media speed, so a second one cannot arrive before the first is taken.
static bool     b_logErrPend;
static uint8_t  u8_logErrReason;
static uint16_t u16_logErrDetail;

#define SD_LOG_MEDIA_ITV	1000	// card presence poll (a GPIO read)
#define SD_LOG_MOUNT_ITV	5000	// remount attempts; each one re-inits the SDMMC
#define SD_LOG_WRFAIL_MAX	3		// failed flushes before a remount is attempted

/*
 * Second fault injection point, for scenario 14. The write-failure hook in
 * sd_diskio.c produces a rotation, not a placeholder — the two are different
 * paths. A placeholder needs the length contract to trip, so forcing the length
 * here runs exactly the code the field would run rather than a special case.
 * Same rules as the other hook: no build configuration defines the symbol, and
 * it defaults to 0 so forgetting the flag fails safe.
 */
#ifndef PSA_FAULT_INJECT
#define PSA_FAULT_INJECT 0
#endif

#if PSA_FAULT_INJECT
volatile uint8_t u8_sd_fault_inject_len;
#endif

static void v_log_err_raise(uint8_t u8_reason, uint16_t u16_detail){
	if(b_logErrPend) return;		// keep the older one, it is closer to the cause
	u8_logErrReason  = u8_reason;
	u16_logErrDetail = u16_detail;
	b_logErrPend     = true;
}


static void v_put_u16be(uint8_t* p, uint16_t v){
	p[0] = (uint8_t)(v >> 8);
	p[1] = (uint8_t)(v);
}

static void v_put_u32be(uint8_t* p, uint32_t v){
	p[0] = (uint8_t)(v >> 24);
	p[1] = (uint8_t)(v >> 16);
	p[2] = (uint8_t)(v >> 8);
	p[3] = (uint8_t)(v);
}

static uint32_t u32_get_u32be(const uint8_t* p){
	return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
	       ((uint32_t)p[2] << 8)  |  (uint32_t)p[3];
}

/*
 * brief	: directory name for the current device id
 * note
 * - "UNKNOWN" until initLogIdentity(0x23) supplies a MAC, so a card written
 *   before the ESP32 ever talks is still self-describing (spec section 8.2).
 */
static void v_log_devid_dir(char* pc_out){
	static const char c_hex[] = "0123456789ABCDEF";
	const uint8_t* pu8_id = pu8_Flash_Cfg_Get_DeviceId();
	int i_known = 0;

	for(int i = 0; i < 6; i++){
		if(pu8_id[i] != 0xFF) i_known = 1;
	}
	if(!i_known){
		strcpy(pc_out, "UNKNOWN");
		return;
	}
	for(int i = 0; i < 6; i++){
		pc_out[i * 2]     = c_hex[pu8_id[i] >> 4];
		pc_out[i * 2 + 1] = c_hex[pu8_id[i] & 0x0F];
	}
	pc_out[12] = '\0';
}

static void v_log_build_header(uint8_t* pu8_hdr, uint32_t u32_firstSeq){
	memset(pu8_hdr, 0, SD_LOG_HDR_SIZE);
	memcpy(&pu8_hdr[SD_HDR_MAGIC], "PSA1", 4);
	v_put_u16be(&pu8_hdr[SD_HDR_FORMAT_VER], SD_LOG_FORMAT_VER);
	v_put_u16be(&pu8_hdr[SD_HDR_REC_SIZE],   SD_LOG_REC_SIZE);
	memcpy(&pu8_hdr[SD_HDR_DEVID], pu8_Flash_Cfg_Get_DeviceId(), 6);
	v_put_u32be(&pu8_hdr[SD_HDR_BOOTID],   u32_Flash_Cfg_Get_BootId());
	v_put_u32be(&pu8_hdr[SD_HDR_FILEIDX],  u32_logFileIdx);
	v_put_u32be(&pu8_hdr[SD_HDR_FIRSTSEQ], u32_firstSeq);
	v_put_u32be(&pu8_hdr[SD_HDR_RATE],     SD_LOG_SAMPLE_MHZ);
	v_put_u16be(&pu8_hdr[SD_HDR_CRC], u16_CRC16_CCITT(pu8_hdr, SD_HDR_CRC));
}

static void v_log_build_record(uint8_t* pu8_rec, uint32_t u32_seq,
		const uint8_t* pu8_payload, uint8_t u8_flags,
		uint8_t u8_devMode, uint16_t u16_errMask){
	memset(pu8_rec, 0, SD_LOG_REC_SIZE);
	v_put_u32be(&pu8_rec[SD_REC_SEQ],  u32_seq);
	v_put_u32be(&pu8_rec[SD_REC_TICK], u32_Tim_1msGet());
	if(pu8_payload != NULL){
		memcpy(&pu8_rec[SD_REC_PAYLOAD], pu8_payload, SD_LOG_PAYLOAD_SIZE);
	}
	pu8_rec[SD_REC_MODE]  = u8_devMode;
	pu8_rec[SD_REC_FLAGS] = u8_flags;
	v_put_u16be(&pu8_rec[SD_REC_ERRMASK], u16_errMask);
	// [76..77] reserved, left zero by the memset above
	v_put_u16be(&pu8_rec[SD_REC_CRC], u16_CRC16_CCITT(pu8_rec, SD_REC_CRC));
}

/*
 * brief	: create /LOG/<dev>/<bootId>_<fileIndex>.psa and write its header
 * note
 * - Called on the first record that needs a home, never at start-up. A session
 *   that samples nothing — cold boot straight into modeOFF, for one — therefore
 *   leaves no file at all, which is what keeps empty sessions out of
 *   reqLogFiles (spec section 8.3).
 */
static bool b_log_file_open(uint32_t u32_firstSeq){
	char c_dev[13];
	char c_dir[SD_LOG_PATH_MAX];
	char c_path[SD_LOG_PATH_MAX];
	uint8_t u8_hdr[SD_LOG_HDR_SIZE];
	UINT bw = 0;

	v_log_devid_dir(c_dev);
	f_mkdir(SD_LOG_DIR);					// FR_EXIST is the normal answer
	snprintf(c_dir, sizeof(c_dir), "%s/%s", SD_LOG_DIR, c_dev);
	f_mkdir(c_dir);
	snprintf(c_path, sizeof(c_path), "%s/%08lX_%04lX.psa", c_dir,
			(unsigned long)u32_Flash_Cfg_Get_BootId(), (unsigned long)u32_logFileIdx);

	if(f_open(&logFile, c_path, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK){
		u16_logWrErr++;
		return false;
	}

	v_log_build_header(u8_hdr, u32_firstSeq);
	if(f_write(&logFile, u8_hdr, SD_LOG_HDR_SIZE, &bw) != FR_OK || bw != SD_LOG_HDR_SIZE){
		f_close(&logFile);
		u16_logWrErr++;
		u32_logFileIdx++;
		return false;
	}
	f_sync(&logFile);

	b_logOpen = true;
	if(u16_logFilesOnCard < 0xFFFFU) u16_logFilesOnCard++;
	u32_logFlushRef = u32_Tim_1msGet();
	u32_logFileRef  = u32_logFlushRef;
	u32_logFileRec  = 0;
	LOG_INFO("SD_LOG", "%s firstSeq=%u", c_path, (unsigned)u32_firstSeq);
	return true;
}


static void v_log_idx_add(const char* pc_dir, const char* pc_name,
		uint32_t u32_boot, uint32_t u32_idx, uint32_t u32_size, uint8_t u8_unknown);

/*
 * brief	: end the current file so the next sample starts a fresh one
 * note
 * - close-then-open by construction (spec section 8.3): the replacement is not
 *   created here but lazily on the next record, so there is never a moment with
 *   two write handles competing with a backfill read for _FS_LOCK.
 * - Does not clear the record buffer. A flush that failed leaves the batch in
 *   RAM on purpose, and the new file re-records it from the start.
 */
/*
 * brief	: close the open file, register it, and move to the next index
 * note
 * - Every way a file ends comes through here -- rotation and a failed write --
 *   so the registration cannot be added to one path and forgotten in the other.
 * - The index is otherwise built once, by the boot scan, which leaves a file
 *   closed mid-session out of reqLogFiles and out of backfill until the next
 *   reset. That is exactly the data someone wants back after a dropout.
 * - Registration goes through v_log_idx_add, the routine the scan itself calls:
 *   it re-reads the header for firstSeq and derives the count from the file
 *   size rather than trusting counters this path would have to keep in step.
 *   A short file left by a failed write therefore indexes the records it does
 *   hold, which is the case where recovering them matters most.
 */
static void v_log_close_and_index(void){
	char c_dev[13];
	char c_dir[SD_LOG_PATH_MAX];
	char c_name[24];
	uint32_t u32_boot = u32_Flash_Cfg_Get_BootId();
	uint32_t u32_size = (uint32_t)f_size(&logFile);

	f_close(&logFile);
	b_logOpen = false;

	v_log_devid_dir(c_dev);
	snprintf(c_dir, sizeof(c_dir), "%s/%s", SD_LOG_DIR, c_dev);
	snprintf(c_name, sizeof(c_name), "%08lX_%04lX.psa",
			(unsigned long)u32_boot, (unsigned long)u32_logFileIdx);
	v_log_idx_add(c_dir, c_name, u32_boot, u32_logFileIdx, u32_size,
			(strcmp(c_dev, "UNKNOWN") == 0) ? 1U : 0U);

	// The count, not a success flag: v_log_idx_add drops a file that is too
	// short to hold a record or that would overflow the table, and it says so
	// by not moving this. It has to step by one here and match what reqLogFiles
	// then reports -- which is the whole check, without a card reader.
	LOG_INFO("SD_LOG", "closed %s size=%u indexed=%u", c_name,
			(unsigned)u32_size, (unsigned)u16_SD_Log_Idx_Count());

	u32_logFileIdx++;				// after the entry, which names the old index
}

static void v_log_rotate(const char* pc_reason){
	if(!b_logOpen) return;

	v_SD_Log_Flush();				// on failure this already closed and bumped
	if(b_logOpen){
		v_log_close_and_index();
	}
	LOG_INFO("SD_LOG", "rotate (%s) -> fileIndex=%u",
			pc_reason, (unsigned)u32_logFileIdx);
}

static const char* pc_log_rotate_reason(void){
	if(!b_logOpen) return NULL;
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logFileRef, SD_LOG_ROTATE_ITV)) return "1h";
	if(u32_logFileRec >= SD_LOG_ROTATE_REC) return "32MB";
	return NULL;
}


/*
 * brief	: arm sensor logging for this boot
 * date
 * - create	: 26.08.11
 */
bool b_SD_Log_Init(){
	u32_logSeq        = 0;
	u32_logFlushedSeq = 0;
	u32_logFileIdx    = 0;
	u16_logBufIdx     = 0;
	u16_logWrErr      = 0;
	b_logOpen         = false;
	b_logFault        = false;
	u8_logWrFail      = 0;
	b_logErrPend      = false;
	u32_logFlushRef   = u32_Tim_1msGet();
	u32_logMediaRef   = u32_logFlushRef;
	u32_logMountRef   = u32_logFlushRef;
	b_logCardSeen     = (BSP_SD_IsDetected() == SD_PRESENT);
	if(!b_logCardSeen) v_log_err_raise(SD_LOG_ERR_NO_CARD, 0);

	// A wrong polynomial is invisible on the device: records still look fine and
	// only the merge tool notices, by which point a whole card is unusable.
	// Refuse to log rather than fill a card with records nobody can validate.
	b_logCrcOk  = i_CRC16_SelfTest() ? true : false;
	b_logEnabled = true;			// on by default; ESP32 is not required to ask

	if(!b_logCrcOk){
		LOG_ERROR("SD_LOG", "CRC-16 self test failed, logging disabled");
		return false;
	}
	LOG_INFO("SD_LOG", "armed, bootId=%u", (unsigned)u32_Flash_Cfg_Get_BootId());
	return true;
}


/*
 * brief	: append one sample to the current file
 * date
 * - create	: 26.08.11
 * param
 * - pu8_payload	: STAT(0x70) DATA, must be SD_LOG_PAYLOAD_SIZE bytes
 * - u16_len		: length of pu8_payload
 * - u8_devMode		: protocol mode code (e_ESP_EVT_MODE_t), record offset 72
 * - u16_errMask	: errInit(0x90) bitmask snapshot, record offset 74
 * - b_txOk			: this sample's STAT frame was accepted by the UART queue
 */
void v_SD_Log_Write(const uint8_t* pu8_payload, uint16_t u16_len,
		uint8_t u8_devMode, uint16_t u16_errMask, bool b_txOk){
	// seq counts samples, not writes (spec section 4). It has to advance with no
	// card in the slot too, otherwise the next file's firstSeq would not line up
	// with the sample stream the ESP32 sees.
	uint32_t u32_seq = u32_logSeq++;
	uint8_t  u8_flags = b_txOk ? SD_FLAG_TX_OK : 0;

	if(!b_logCrcOk || !b_logEnabled || b_logFault) return;

#if PSA_FAULT_INJECT
	if(u8_sd_fault_inject_len){
		if(--u8_sd_fault_inject_len == 0) u16_len = 0;	// trips the length contract
	}
#endif

	// Length contract (spec section 7.4). Do not skip the slot: dropping one
	// record shifts every later record in the file by 80 B and backfill would
	// then answer a seq request with the wrong sample. Write a placeholder with
	// the right seq and let the reader discard it by flags bit1.
	if(pu8_payload == NULL || u16_len != SD_LOG_PAYLOAD_SIZE){
		pu8_payload = NULL;
		u8_flags |= SD_FLAG_INVALID;
		u16_logWrErr++;
		LOG_WARN("SD_LOG", "payload %u != %u, placeholder at seq=%u",
				u16_len, SD_LOG_PAYLOAD_SIZE, (unsigned)u32_seq);
	}

	// Rotate before placing the record, so it lands in the file it belongs to.
	const char* pc_reason = pc_log_rotate_reason();
	if(pc_reason != NULL) v_log_rotate(pc_reason);

	if(!b_logOpen){
		if(!b_SdMount) return;
		// A pending batch keeps its own first seq, so a file opened after a failed
		// flush still describes the records it actually contains.
		uint32_t u32_first = (u16_logBufIdx > 0)
				? u32_get_u32be(&u8_logBuf[SD_REC_SEQ]) : u32_seq;
		if(!b_log_file_open(u32_first)) return;
	}

	if((u16_logBufIdx + SD_LOG_REC_SIZE) > SD_LOG_BUF_SIZE){
		v_SD_Log_Flush();
		if((u16_logBufIdx + SD_LOG_REC_SIZE) > SD_LOG_BUF_SIZE) return;
	}

	v_log_build_record(&u8_logBuf[u16_logBufIdx], u32_seq, pu8_payload, u8_flags,
			u8_devMode, u16_errMask);
	u16_logBufIdx += SD_LOG_REC_SIZE;
	u32_logFileRec++;

	// Whichever comes first. At 10 Hz the two coincide, but the record count keeps
	// the 20-sample loss bound true if the rate ever changes or the tick jitters.
	if((u16_logBufIdx / SD_LOG_REC_SIZE) >= SD_LOG_FLUSH_REC
	|| _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logFlushRef, SD_LOG_FLUSH_ITV)){
		v_SD_Log_Flush();
	}
}


/*
 * brief	: write the buffered records to the card
 * date
 * - create	: 26.08.11
 */
void v_SD_Log_Flush(){
	if(!b_logOpen || u16_logBufIdx == 0) return;

	UINT bw = 0;
	FRESULT e_res = f_write(&logFile, u8_logBuf, u16_logBufIdx, (void*)&bw);

	if(e_res == FR_OK && bw == u16_logBufIdx){
		f_sync(&logFile);
		u32_logFlushedSeq = u32_get_u32be(&u8_logBuf[u16_logBufIdx - SD_LOG_REC_SIZE]);
		u16_logBufIdx = 0;
		u32_logFlushRef = u32_Tim_1msGet();
		u8_logWrFail = 0;
		return;
	}

	// FatFs distinguishes the two failures for us, which is why f_getfree is not
	// consulted here: a short write with FR_OK means the volume is full, anything
	// else is an I/O or filesystem problem. f_getfree would answer the same
	// question but scans the whole FAT when FSINFO is stale — seconds of blocking
	// against a 2 s watchdog.
	if(e_res == FR_OK){
		LOG_ERROR("SD_LOG", "card full at seq=%u, logging stopped",
				(unsigned)u32_logSeq);
		v_log_err_raise(SD_LOG_ERR_NO_SPACE, 0);
		b_logFault = true;			// spec 10.4: stop, never delete to make room
	}
	else{
		// Retries are simply the next flush, 2 s away, so they cost no blocking
		// time here. Only a persistent failure escalates.
		u8_logWrFail++;
		if(u8_logWrFail >= SD_LOG_WRFAIL_MAX){
			u8_logWrFail = 0;
			v_log_err_raise(SD_LOG_ERR_WRITE, (uint16_t)e_res);
			b_SdMount = false;		// media handler remounts on the next pass
		}
	}

	// Short or failed write. The file may now end part way through a record, and
	// carrying on would break "n-th record has seq firstSeq + n" for everything
	// after it. End the file here; the batch is still in RAM, so the next sample
	// opens a new file that re-records it from the start and nothing is lost.
	// (Full rotation policy — time, size, close-then-open — lands with 16.3-5.)
	u16_logWrErr++;
	LOG_ERROR("SD_LOG", "write %u/%u res=%d, rotating",
			(unsigned)bw, (unsigned)u16_logBufIdx, (int)e_res);
	v_log_close_and_index();		// the salvageable part stays reachable
	u32_logFlushRef = u32_Tim_1msGet();
}


/*
 * brief	: flush and close the current file
 * date
 * - create	: 26.08.11
 */
void v_SD_Log_Close(){
	v_log_rotate("close");
	// Unlike a rotation, nothing is going to re-record a pending batch after a
	// deliberate close — the card is about to go away — so drop it rather than
	// carry it into a file that may never be opened.
	u16_logBufIdx = 0;
}


/*
 * brief	: ctrlLogEnable(0x56) — stop means flush + close so the card is safe to pull
 */
void v_SD_Log_SetEnabled(bool b_en){
	if(b_en == b_logEnabled) return;
	if(!b_en) v_SD_Log_Close();
	b_logEnabled = b_en;
	LOG_INFO("SD_LOG", "%s", b_en ? "enabled" : "stopped (flushed and closed)");
}


/*
 * brief	: card presence, removal and remount (spec section 10.4)
 * note
 * - Nothing here touches the protocol layer. Faults are parked for
 *   b_SD_Log_Get_Error so this stays a driver.
 */
void v_SD_Log_Media_Handler(){
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logMediaRef, SD_LOG_MEDIA_ITV)) return;
	u32_logMediaRef = u32_Tim_1msGet();

	bool b_present = (BSP_SD_IsDetected() == SD_PRESENT);

	if(!b_present){
		if(b_logCardSeen || b_SdMount){
			// Abandon the handle rather than f_close it: closing writes the
			// directory entry, and the card it would be written to is gone.
			// Whatever the last flush committed stays valid on the card.
			b_logOpen     = false;
			u16_logBufIdx = 0;
			u32_logFileIdx++;			// a re-insert resumes in a new file
			b_SdMount     = false;
			v_log_err_raise(SD_LOG_ERR_NO_CARD, 0);
			LOG_WARN("SD_LOG", "card removed, next file will be #%u",
					(unsigned)u32_logFileIdx);
		}
		b_logCardSeen = false;
		return;
	}

	b_logCardSeen = true;
	if(b_SdMount) return;

	// Present but not mounted: a fresh insert, or recovery after a write fault.
	// Rate limited because each attempt re-inits the SDMMC peripheral.
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logMountRef, SD_LOG_MOUNT_ITV)) return;
	u32_logMountRef = u32_Tim_1msGet();

	if(b_MountSD()){
		LOG_INFO("SD_LOG", "card mounted");
		b_logFault = false;				// a new card clears a full/damaged one
		u8_logWrFail = 0;
	}
	else{
		v_log_err_raise(SD_LOG_ERR_MOUNT, 0);
	}
}


bool b_SD_Log_Get_Error(uint8_t* pu8_reason, uint16_t* pu16_detail){
	if(!b_logErrPend) return false;
	if(pu8_reason  != NULL) *pu8_reason  = u8_logErrReason;
	if(pu16_detail != NULL) *pu16_detail = u16_logErrDetail;
	b_logErrPend = false;
	return true;
}

/*
 * brief	: free space in MB for the reqLogStatus(0x43) response
 * note
 * - Reads FatFs' running free-cluster count rather than calling f_getfree.
 *   FatFs maintains it on every allocation (ff.c:1420) and seeds it from FSINFO
 *   at mount, so this is a struct read. f_getfree returns the same number but
 *   falls back to scanning the whole FAT when FSINFO is stale — seconds of
 *   blocking against a 2 s watchdog, and a stale FSINFO is exactly what a
 *   power cut during a write leaves behind.
 * - 0xFFFF means "not known", using the same validity test f_getfree makes.
 *   It overlaps the spec's "clamped at 65535" value; both mean "do not warn",
 *   and a card that really is running out is caught by evtLogError(2) instead.
 */
uint16_t u16_SD_Log_Get_FreeMB(){
	if(!b_SdMount) return 0xFFFFU;

	FATFS* fs = p_fatFs;
	if(fs->free_clst > fs->n_fatent - 2) return 0xFFFFU;	// FSINFO never was valid

	uint64_t u64_free = (uint64_t)fs->free_clst * (uint64_t)fs->csize * (uint64_t)fs->ssize;
	uint64_t u64_mb   = u64_free >> 20;
	return (u64_mb > 65535ULL) ? 65535U : (uint16_t)u64_mb;
}

uint8_t u8_SD_Log_Get_State(){
	if(b_logFault) return 2;
	if(!b_logCrcOk || !b_logEnabled) return 0;
	return 1;
}


//////////////////////////////////
//		LOG FILE INDEX			//
//////////////////////////////////

// Built once at start-up so reqLogFiles(0x45) can answer without touching the
// card. Rebuilding per request would mean an f_open per file, and f_open walks
// the directory, so the cost is quadratic in the number of files.
//
// The cap is a blocking-time budget, not a memory one: each entry costs one
// f_open plus a 512 B header read, and f_open's directory walk grows with the
// file count. 128 files is a few hundred ms at start-up, which fits inside the
// 2 s watchdog with room to spare. Files past the cap stay on the card and are
// still recoverable by pulling it (spec section 6.3).
#define SD_LOG_IDX_MAX		128

typedef struct {
	uint32_t u32_bootId;
	uint32_t u32_firstSeq;
	uint32_t u32_lastSeq;
	uint16_t u16_fileIdx;
	uint8_t  u8_unknownDir;		// written before initLogIdentity supplied a MAC
} x_log_idx_t;

static x_log_idx_t x_logIdx[SD_LOG_IDX_MAX];
static uint16_t    u16_logIdxCnt;

// ~300 B each; kept off the stack because the scan runs from the start-up path.
static DIR     x_logScanDir;
static FILINFO x_logScanInfo;

static int i_hex_nib(char c){
	if(c >= '0' && c <= '9') return c - '0';
	if(c >= 'A' && c <= 'F') return c - 'A' + 10;
	if(c >= 'a' && c <= 'f') return c - 'a' + 10;
	return -1;
}

static bool b_hex_parse(const char* pc, int i_len, uint32_t* pu32_out){
	uint32_t u32_v = 0;
	for(int i = 0; i < i_len; i++){
		int n = i_hex_nib(pc[i]);
		if(n < 0) return false;
		u32_v = (u32_v << 4) | (uint32_t)n;
	}
	*pu32_out = u32_v;
	return true;
}

/* "<bootId:08X>_<fileIndex:04X>.psa" — anything else is not ours. */
static bool b_log_name_parse(const char* pc_name, uint32_t* pu32_boot, uint32_t* pu32_idx){
	if(strlen(pc_name) != 17) return false;
	if(pc_name[8] != '_') return false;
	if(strcmp(&pc_name[13], ".psa") != 0) return false;
	if(!b_hex_parse(&pc_name[0], 8, pu32_boot)) return false;
	if(!b_hex_parse(&pc_name[9], 4, pu32_idx))  return false;
	return true;
}

/*
 * brief	: read one file's header and add it to the index
 * note
 * - A file whose header is present but which holds no record is skipped, per
 *   correction 26(a): reqLogFiles has no way to express "last record" for it,
 *   and firstSeq + N - 1 would underflow.
 */
static void v_log_idx_add(const char* pc_dir, const char* pc_name,
		uint32_t u32_boot, uint32_t u32_idx, uint32_t u32_size, uint8_t u8_unknown){
	char c_path[SD_LOG_PATH_MAX];
	uint8_t u8_hdr[SD_LOG_HDR_SIZE];
	FIL x_f;
	UINT br = 0;

	// Full. The file stays on the card and keeps its records, but nothing can ask
	// for it, so say so rather than dropping it quietly.
	if(u16_logIdxCnt >= SD_LOG_IDX_MAX){
		// Detail is the card's file count, the same figure the boot scan sends
		// and the same one reqLogStatus reports. Sending the index count here
		// instead made the event say "full, nothing beyond it": that count is
		// the cap by definition, so detail minus the cap was always zero.
		v_log_err_raise(SD_LOG_ERR_IDX_FULL, u16_logFilesOnCard);
		return;
	}
	if(u32_size < SD_LOG_HDR_SIZE + SD_LOG_REC_SIZE) return;	// header only, no record

	uint32_t u32_rec = (u32_size - SD_LOG_HDR_SIZE) / SD_LOG_REC_SIZE;
	if(u32_rec == 0) return;

	snprintf(c_path, sizeof(c_path), "%s/%s", pc_dir, pc_name);
	if(f_open(&x_f, c_path, FA_READ) != FR_OK) return;
	FRESULT e_res = f_read(&x_f, u8_hdr, SD_LOG_HDR_SIZE, &br);
	f_close(&x_f);
	if(e_res != FR_OK || br != SD_LOG_HDR_SIZE) return;

	if(memcmp(&u8_hdr[SD_HDR_MAGIC], "PSA1", 4) != 0) return;
	if(u16_CRC16_CCITT(u8_hdr, SD_HDR_CRC) !=
			(uint16_t)((u8_hdr[SD_HDR_CRC] << 8) | u8_hdr[SD_HDR_CRC + 1])) return;

	x_logIdx[u16_logIdxCnt].u32_bootId    = u32_boot;
	x_logIdx[u16_logIdxCnt].u16_fileIdx   = (uint16_t)u32_idx;
	x_logIdx[u16_logIdxCnt].u8_unknownDir = u8_unknown;
	x_logIdx[u16_logIdxCnt].u32_firstSeq = u32_get_u32be(&u8_hdr[SD_HDR_FIRSTSEQ]);
	x_logIdx[u16_logIdxCnt].u32_lastSeq  =
			x_logIdx[u16_logIdxCnt].u32_firstSeq + u32_rec - 1;
	u16_logIdxCnt++;
}

/*
 * brief	: build the log file index from the card
 * note
 * - Sorted bootId then fileIndex ascending, so the oldest unsent data is offered
 *   first (spec section 6.3).
 * - Also advances fileIndex past anything already present for the current
 *   bootId. That should be impossible, since bootId increments every reset — but
 *   if a flash write ever failed, the reused bootId would otherwise have
 *   FA_CREATE_ALWAYS overwrite a file that still held unsent data.
 */
static uint32_t u32_log_scan_dir(const char* pc_dir, uint8_t u8_unknown){
	uint32_t u32_curBoot = u32_Flash_Cfg_Get_BootId();
	uint32_t u32_seen = 0;

	if(f_opendir(&x_logScanDir, pc_dir) != FR_OK) return 0;

	// Counting and indexing are separate on purpose. The walk used to stop at
	// SD_LOG_IDX_MAX, which meant the count could never exceed the cap and so
	// could never report how far past it the card was -- the one moment the
	// number matters. Reading on costs a readdir step per file; the expensive
	// part is the header read inside v_log_idx_add, and that is what stops.
	//
	// The resume check stays outside the cap for a harder reason: it is what
	// keeps a new file from reusing an index this bootId already wrote. Skipping
	// it past 128 would overwrite records instead of merely hiding them.
	while(1){
		if(f_readdir(&x_logScanDir, &x_logScanInfo) != FR_OK) break;
		if(x_logScanInfo.fname[0] == 0) break;
		if(x_logScanInfo.fattrib & AM_DIR) continue;

		uint32_t u32_boot, u32_idx;
		if(!b_log_name_parse(x_logScanInfo.fname, &u32_boot, &u32_idx)) continue;
		u32_seen++;

		if(u32_boot == u32_curBoot && u32_idx >= u32_logFileIdx){
			u32_logFileIdx = u32_idx + 1;
			LOG_WARN("SD_LOG", "bootId %u already has files, resuming at #%u",
					(unsigned)u32_boot, (unsigned)u32_logFileIdx);
		}
		v_log_idx_add(pc_dir, x_logScanInfo.fname, u32_boot, u32_idx,
				(uint32_t)x_logScanInfo.fsize, u8_unknown);
	}
	f_closedir(&x_logScanDir);
	return u32_seen;
}

/*
 * brief	: build the log file index from the card
 * note
 * - Scans the UNKNOWN directory as well as this device's own. Every device
 *   records into /LOG/UNKNOWN/ until initLogIdentity(0x23) supplies a MAC, and
 *   once it does, those files would otherwise stop being visible to
 *   reqLogFiles — present on the card, unreachable by backfill, which is the
 *   silent loss this whole format exists to prevent.
 * - Sorted bootId then fileIndex ascending, so the oldest unsent data is offered
 *   first (spec section 6.3).
 */
void v_SD_Log_Scan(){
	char c_dev[13];
	char c_dir[SD_LOG_PATH_MAX];
	uint32_t u32_seen = 0;

	u16_logIdxCnt = 0;
	if(!b_SdMount) return;

	v_log_devid_dir(c_dev);
	snprintf(c_dir, sizeof(c_dir), "%s/%s", SD_LOG_DIR, c_dev);
	u32_seen += u32_log_scan_dir(c_dir, 0);

	if(strcmp(c_dev, "UNKNOWN") != 0){
		snprintf(c_dir, sizeof(c_dir), "%s/UNKNOWN", SD_LOG_DIR);
		uint32_t u32_orphan = u32_log_scan_dir(c_dir, 1);
		if(u32_orphan){
			LOG_INFO("SD_LOG", "%u file(s) from before the deviceId was known",
					(unsigned)u32_orphan);
		}
		u32_seen += u32_orphan;
	}

	// insertion sort: bootId, then fileIndex
	for(uint16_t i = 1; i < u16_logIdxCnt; i++){
		x_log_idx_t x_key = x_logIdx[i];
		int16_t j = (int16_t)i - 1;
		while(j >= 0 && (x_logIdx[j].u32_bootId > x_key.u32_bootId ||
				(x_logIdx[j].u32_bootId == x_key.u32_bootId &&
				 x_logIdx[j].u16_fileIdx > x_key.u16_fileIdx))){
			x_logIdx[j + 1] = x_logIdx[j];
			j--;
		}
		x_logIdx[j + 1] = x_key;
	}

	if(u32_seen > u16_logIdxCnt){
		LOG_WARN("SD_LOG", "index %u of %u files (cap %u)",
				(unsigned)u16_logIdxCnt, (unsigned)u32_seen, SD_LOG_IDX_MAX);
		// Detail is what is on the card, not what fit: the gap against the cap is
		// how many are already out of reach, and a reader that only sees 128 has
		// no way to work that out.
		v_log_err_raise(SD_LOG_ERR_IDX_FULL,
				(u32_seen > 0xFFFFU) ? 0xFFFFU : (uint16_t)u32_seen);
	}
	u16_logFilesOnCard = (u32_seen > 0xFFFFU) ? 0xFFFFU : (uint16_t)u32_seen;
	LOG_INFO("SD_LOG", "indexed %u of %u file(s) on card",
			(unsigned)u16_logIdxCnt, (unsigned)u16_logFilesOnCard);
}

/*
 * brief	: remove one indexed file at the PC's request (spec 6.8)
 * note
 * - The caller decides *whether* a file may go: only the PC knows the records
 *   reached it, passed CRC and merged, and neither this side nor the ESP32 can
 *   see any of that. What is checked here is only whether removing it now would
 *   break something in flight.
 * - The index entry is dropped rather than the card rescanned. A scan costs a
 *   header read per file and would say nothing this does not already know.
 */
uint8_t u8_SD_Log_Delete(uint32_t u32_boot, uint16_t u16_idx){
	char c_dev[13];
	char c_path[SD_LOG_PATH_MAX];
	uint16_t i;

	if(!b_SdMount) return 4;

	// A backfill holds an open handle on a file it is part way through. Refusing
	// is enough: the PC asks again once the transfer it started has finished.
	if(b_SD_Log_Backfill_Active()) return 3;

	// The file being written now. Deleting it would strand logFile on an entry
	// that no longer exists, and this session's records with it.
	if(b_logOpen && u32_boot == u32_Flash_Cfg_Get_BootId()
			&& (uint32_t)u16_idx == u32_logFileIdx){
		return 2;
	}

	for(i = 0; i < u16_logIdxCnt; i++){
		if(x_logIdx[i].u32_bootId == u32_boot
				&& x_logIdx[i].u16_fileIdx == u16_idx) break;
	}
	if(i >= u16_logIdxCnt) return 1;			// not listed, so not ours to delete

	if(x_logIdx[i].u8_unknownDir) strcpy(c_dev, "UNKNOWN");
	else                          v_log_devid_dir(c_dev);
	snprintf(c_path, sizeof(c_path), "%s/%s/%08lX_%04lX.psa", SD_LOG_DIR, c_dev,
			(unsigned long)u32_boot, (unsigned long)u16_idx);

	if(f_unlink(c_path) != FR_OK){
		v_log_err_raise(SD_LOG_ERR_BACKFILL, 0);
		return 4;
	}

	// Close the gap rather than leaving a hole: b_bf_find walks this array in
	// order and takes the last match, which only holds while it stays sorted.
	for(uint16_t j = i; j + 1 < u16_logIdxCnt; j++){
		x_logIdx[j] = x_logIdx[j + 1];
	}
	u16_logIdxCnt--;
	if(u16_logFilesOnCard > 0) u16_logFilesOnCard--;

	LOG_INFO("SD_LOG", "deleted %08lX_%04lX.psa indexed=%u onCard=%u",
			(unsigned long)u32_boot, (unsigned long)u16_idx,
			(unsigned)u16_logIdxCnt, (unsigned)u16_logFilesOnCard);
	return 0;
}

uint16_t u16_SD_Log_Files_On_Card(){
	return u16_logFilesOnCard;
}

uint16_t u16_SD_Log_Idx_Count(){
	return u16_logIdxCnt;
}

uint16_t u16_SD_Log_Idx_Capacity(){
	return SD_LOG_IDX_MAX;
}

bool b_SD_Log_Idx_Get(uint16_t u16_n, uint32_t* pu32_boot, uint32_t* pu32_first,
		uint32_t* pu32_last, uint16_t* pu16_idx){
	if(u16_n >= u16_logIdxCnt) return false;
	if(pu32_boot)  *pu32_boot  = x_logIdx[u16_n].u32_bootId;
	if(pu32_first) *pu32_first = x_logIdx[u16_n].u32_firstSeq;
	if(pu32_last)  *pu32_last  = x_logIdx[u16_n].u32_lastSeq;
	if(pu16_idx)   *pu16_idx   = x_logIdx[u16_n].u16_fileIdx;
	return true;
}


//////////////////////////////////
//		BACKFILL READER			//
//////////////////////////////////

// Serves reqLogRead(0x44). The protocol layer pulls records from here one at a
// time and frames them; this file never transmits.
static FIL      bfFile;
static bool     b_bfOpen;
static bool     b_bfActive;
static uint32_t u32_bfNext;			// next seq to hand out
static uint32_t u32_bfLast;			// last seq of the request, inclusive
static uint32_t u32_bfSent;			// last seq actually handed out
static uint8_t  u8_bfResult;

/*
 * brief	: pick the file holding u32_seq of u32_boot
 * note
 * - Ranges can overlap after a failure rotation, because the batch still in RAM
 *   is re-recorded into the new file. The higher fileIndex wins (correction 15):
 *   it is the copy that was written after the failure, so it is the intact one.
 */
static bool b_bf_find(uint32_t u32_boot, uint32_t u32_seq, uint16_t* pu16_idx,
		uint32_t* pu32_first, uint8_t* pu8_unknown){
	bool b_found = false;
	for(uint16_t i = 0; i < u16_logIdxCnt; i++){
		if(x_logIdx[i].u32_bootId != u32_boot) continue;
		if(u32_seq < x_logIdx[i].u32_firstSeq) continue;
		if(u32_seq > x_logIdx[i].u32_lastSeq)  continue;
		*pu16_idx    = x_logIdx[i].u16_fileIdx;		// index is sorted ascending,
		*pu32_first  = x_logIdx[i].u32_firstSeq;	// so the last match is the highest
		*pu8_unknown = x_logIdx[i].u8_unknownDir;
		b_found = true;
	}
	return b_found;
}

static void v_bf_close(){
	if(b_bfOpen){
		f_close(&bfFile);
		b_bfOpen = false;
	}
	b_bfActive = false;
}

uint8_t u8_SD_Log_Backfill_Start(uint32_t u32_boot, uint32_t u32_startSeq, uint16_t u16_count){
	char c_dev[13];
	char c_path[SD_LOG_PATH_MAX];
	uint16_t u16_idx;
	uint32_t u32_first;
	uint8_t  u8_unknown = 0;

	v_bf_close();						// a new request supersedes any in flight
	u32_bfSent = (u32_startSeq == 0) ? 0 : (u32_startSeq - 1);

	if(u16_count == 0) return 2;		// nothing asked for
	if(!b_SdMount)     return 3;

	// Is this bootId on the card at all?
	bool b_boot = false;
	for(uint16_t i = 0; i < u16_logIdxCnt; i++){
		if(x_logIdx[i].u32_bootId == u32_boot){ b_boot = true; break; }
	}
	if(!b_boot) return 1;				// no such bootId

	if(!b_bf_find(u32_boot, u32_startSeq, &u16_idx, &u32_first, &u8_unknown)) return 2;

	// Read from wherever the file actually is. Once a MAC arrives the current
	// directory changes, but the files recorded before it do not move.
	if(u8_unknown) strcpy(c_dev, "UNKNOWN");
	else           v_log_devid_dir(c_dev);
	snprintf(c_path, sizeof(c_path), "%s/%s/%08lX_%04lX.psa", SD_LOG_DIR, c_dev,
			(unsigned long)u32_boot, (unsigned long)u16_idx);
	if(f_open(&bfFile, c_path, FA_READ) != FR_OK){
		v_log_err_raise(SD_LOG_ERR_BACKFILL, 0);
		return 3;
	}
	// Fixed records are the whole point: the offset is arithmetic, no scanning.
	if(f_lseek(&bfFile, SD_LOG_HDR_SIZE + (uint32_t)(u32_startSeq - u32_first) * SD_LOG_REC_SIZE)
			!= FR_OK){
		f_close(&bfFile);
		v_log_err_raise(SD_LOG_ERR_BACKFILL, 0);
		return 3;
	}

	b_bfOpen   = true;
	b_bfActive = true;
	u32_bfNext = u32_startSeq;
	u32_bfLast = u32_startSeq + u16_count - 1;
	u8_bfResult = 0;
	return 0xFF;						// accepted; streaming begins
}

/*
 * brief	: hand out the next record, or report the stream is finished
 * retval	: true  = pu8_rec holds one 80 B record
 *            false = done; *pu8_result and u32_SD_Log_Backfill_LastSent() apply
 */
bool b_SD_Log_Backfill_Next(uint8_t* pu8_rec, uint8_t* pu8_result){
	UINT br = 0;

	if(!b_bfActive){
		if(pu8_result) *pu8_result = u8_bfResult;
		return false;
	}
	if(u32_bfNext > u32_bfLast){
		u8_bfResult = 0;				// requested range delivered
		v_bf_close();
		if(pu8_result) *pu8_result = u8_bfResult;
		return false;
	}

	if(f_read(&bfFile, pu8_rec, SD_LOG_REC_SIZE, &br) != FR_OK){
		u8_bfResult = 3;
		v_log_err_raise(SD_LOG_ERR_BACKFILL, 0);
		v_bf_close();
		if(pu8_result) *pu8_result = u8_bfResult;
		return false;
	}
	if(br != SD_LOG_REC_SIZE){
		// End of file, or a partial record left by a power cut. Never send a
		// fragment (spec section 10.3); result 0 means "all we hold", and the PC
		// resumes from lastSeqSent + 1.
		u8_bfResult = 0;
		v_bf_close();
		if(pu8_result) *pu8_result = u8_bfResult;
		return false;
	}

	u32_bfSent = u32_bfNext;
	u32_bfNext++;
	return true;
}

void v_SD_Log_Backfill_Abort(uint8_t u8_result){
	if(!b_bfActive) return;
	u8_bfResult = u8_result;
	v_bf_close();
}

bool     b_SD_Log_Backfill_Active(){	return b_bfActive; }
uint32_t u32_SD_Log_Backfill_LastSent(){ return u32_bfSent; }


uint32_t u32_SD_Log_Get_Seq(){			return u32_logSeq; }
uint32_t u32_SD_Log_Get_FlushedSeq(){	return u32_logFlushedSeq; }
uint32_t u32_SD_Log_Get_FileIndex(){	return u32_logFileIdx; }
uint16_t u16_SD_Log_Get_WriteErrCnt(){	return u16_logWrErr; }


/*
 * brief	: fatfs read
 * date
 * - create	: 25.06.25
 * note
 * -
 */
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *pData, uint32_t ReadAddr, uint32_t NumOfBlocks)
{
  uint8_t sd_state = MSD_OK;
#ifdef SD_CAHCHED_USED
  uint32_t alignedAddr = (uint32_t)pData & ~0x1F;
  SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, BLOCKSIZE * NumOfBlocks + ((uint32_t)pData - alignedAddr));
#endif
  /* Read block(s) in DMA transfer mode */
  if (HAL_SD_ReadBlocks_DMA(&hsd2, (uint8_t *)pData, ReadAddr, NumOfBlocks) != HAL_OK)
  {
    sd_state = MSD_ERROR;
  }

  return sd_state;
}
