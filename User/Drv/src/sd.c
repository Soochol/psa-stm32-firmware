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

#define SD_LOG_FLUSH_ITV		10000		// 10s; spec section 10.2 tightens this to 2s
#define SD_LOG_BUF_MAX			110			// 10s / 100ms + 10% margin
#define SD_LOG_BUF_SIZE			(SD_LOG_REC_SIZE * SD_LOG_BUF_MAX)

#define SD_LOG_DIR				"/LOG"
#define SD_LOG_PATH_MAX			48

static FIL      logFile;
static bool     b_logArmed;			// logging wanted
static bool     b_logOpen;			// a file is open and its header is on the card
static uint8_t  u8_logBuf[SD_LOG_BUF_SIZE];
static uint16_t u16_logBufIdx;
static uint32_t u32_logFlushRef;
static uint32_t u32_logSeq;			// next sample number
static uint32_t u32_logFlushedSeq;	// last seq actually on the card, for reqLogStatus
static uint32_t u32_logFileIdx;
static uint16_t u16_logWrErr;		// reported as writeErrorCount in reqLogStatus(0x43)


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
	u32_logFlushRef = u32_Tim_1msGet();
	LOG_INFO("SD_LOG", "%s firstSeq=%u", c_path, (unsigned)u32_firstSeq);
	return true;
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
	u32_logFlushRef   = u32_Tim_1msGet();

	// A wrong polynomial is invisible on the device: records still look fine and
	// only the merge tool notices, by which point a whole card is unusable.
	// Refuse to log rather than fill a card with records nobody can validate.
	if(!i_CRC16_SelfTest()){
		LOG_ERROR("SD_LOG", "CRC-16 self test failed, logging disabled");
		b_logArmed = false;
		return false;
	}

	b_logArmed = true;
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

	if(!b_logArmed) return;

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

	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logFlushRef, SD_LOG_FLUSH_ITV)){
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
		return;
	}

	// Short or failed write. The file may now end part way through a record, and
	// carrying on would break "n-th record has seq firstSeq + n" for everything
	// after it. End the file here; the batch is still in RAM, so the next sample
	// opens a new file that re-records it from the start and nothing is lost.
	// (Full rotation policy — time, size, close-then-open — lands with 16.3-5.)
	u16_logWrErr++;
	LOG_ERROR("SD_LOG", "write %u/%u res=%d, rotating",
			(unsigned)bw, (unsigned)u16_logBufIdx, (int)e_res);
	f_close(&logFile);
	b_logOpen = false;
	u32_logFileIdx++;
	u32_logFlushRef = u32_Tim_1msGet();
}


/*
 * brief	: flush and close the current file
 * date
 * - create	: 26.08.11
 */
void v_SD_Log_Close(){
	if(!b_logOpen) return;

	v_SD_Log_Flush();
	if(b_logOpen){					// still open means the flush succeeded
		f_close(&logFile);
		b_logOpen = false;
		u32_logFileIdx++;
	}
	u16_logBufIdx = 0;
}


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
