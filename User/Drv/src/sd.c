#include "main.h"
#include <sd.h>
#include "myDiskio.h"
#include "stdio.h"
#include "string.h"
#include "fatfs.h"
#include "tim.h"
#include "uart.h"

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

#define SD_LOG_RECORD_SIZE		56		// 4B timestamp + 52B sensor data
#define SD_LOG_FLUSH_ITV		10000	// 10s flush interval (ms)
#define SD_LOG_BUF_MAX			100		// max records per flush (10s / 100ms)
#define SD_LOG_BUF_SIZE			(SD_LOG_RECORD_SIZE * SD_LOG_BUF_MAX)	// 5600 bytes

static FIL logFile;
static bool b_logOpen;
static uint8_t u8_logBuf[SD_LOG_BUF_SIZE];
static uint16_t u16_logBufIdx;
static uint32_t u32_logFlushRef;

static const char c_sensorFmt[] =
"PSA Sensor Binary Log Format\r\n"
"=============================\r\n"
"File: sensor.bin\r\n"
"Record Size: 56 bytes\r\n"
"Interval: 100ms\r\n"
"\r\n"
"Byte Layout (per record):\r\n"
"--------------------------\r\n"
"[0-3]   timestamp_ms    uint32  LE    System tick (ms since boot)\r\n"
"[4-9]   imu_l_gyro      int16x3 BE    Left IMU Gyroscope (X,Y,Z)\r\n"
"[10-15] imu_l_accel     int16x3 BE    Left IMU Accelerometer (X,Y,Z)\r\n"
"[16-21] imu_r_gyro      int16x3 BE    Right IMU Gyroscope (X,Y,Z)\r\n"
"[22-27] imu_r_accel     int16x3 BE    Right IMU Accelerometer (X,Y,Z)\r\n"
"[28-29] fsr_left        uint16  BE    Force Sensor Left\r\n"
"[30-31] fsr_right       uint16  BE    Force Sensor Right\r\n"
"[32-33] temp_out        [int,dec]     Outdoor Temp (byte0=integer, byte1=decimal*10)\r\n"
"[34-35] temp_in         [int,dec]     Indoor Temp\r\n"
"[36-37] temp_ir         [int,dec]     IR Temp\r\n"
"[38-39] tof1            uint16  BE    Time-of-Flight Sensor 1\r\n"
"[40-41] tof2            uint16  BE    Time-of-Flight Sensor 2\r\n"
"[42-43] battery         [int,dec]     Battery Voltage\r\n"
"[44]    imu_l_evt       uint8         Left IMU Event Flags\r\n"
"[45]    imu_r_evt       uint8         Right IMU Event Flags\r\n"
"[46-49] gps_lat         float   BE    GPS Latitude (IEEE754)\r\n"
"[50-53] gps_lon         float   BE    GPS Longitude (IEEE754)\r\n"
"[54]    gps_sat         uint8         Number of Satellites\r\n"
"[55]    gps_fix         uint8         Fix Type (0=none,1=2D,2=3D)\r\n"
"\r\n"
"Notes:\r\n"
"- LE = Little-Endian, BE = Big-Endian\r\n"
"- Temperature: value = byte0 + (byte1 / 10.0)\r\n"
"- GPS zeros when no fix available\r\n"
"- Total records = file_size / 56\r\n";


/*
 * brief	: open sensor log files (sensor.bin + sensor_fmt.txt)
 * date
 * - create	: 26.03.04
 */
bool b_SD_Log_Open(){
	if(!b_SdMount) return false;

	// create sensor_fmt.txt (overwrite)
	FIL fmtFile;
	UINT bw;
	if(f_open(&fmtFile, "sensor_fmt.txt", FA_CREATE_ALWAYS | FA_WRITE) == FR_OK){
		f_write(&fmtFile, c_sensorFmt, strlen(c_sensorFmt), (void*)&bw);
		f_close(&fmtFile);
	}

	// open sensor.bin (append)
	res = f_open(&logFile, "sensor.bin", FA_OPEN_APPEND | FA_WRITE);
	if(res != FR_OK){
		b_logOpen = false;
		return false;
	}

	b_logOpen = true;
	u16_logBufIdx = 0;
	u32_logFlushRef = u32_Tim_1msGet();
	return true;
}


/*
 * brief	: accumulate sensor data to buffer, flush every 10s
 * date
 * - create	: 26.03.04
 * param
 * - pu8_data	: sensor data (52 bytes, same as ESP format)
 * - u16_len	: data length
 */
void v_SD_Log_Write(uint8_t* pu8_data, uint16_t u16_len){
	if(!b_logOpen) return;
	if((u16_logBufIdx + 4 + u16_len) > SD_LOG_BUF_SIZE) return;

	// prepend timestamp (4B LE)
	uint32_t ts = u32_Tim_1msGet();
	u8_logBuf[u16_logBufIdx++] = ts & 0xFF;
	u8_logBuf[u16_logBufIdx++] = (ts >> 8) & 0xFF;
	u8_logBuf[u16_logBufIdx++] = (ts >> 16) & 0xFF;
	u8_logBuf[u16_logBufIdx++] = (ts >> 24) & 0xFF;

	// append sensor data
	memcpy(&u8_logBuf[u16_logBufIdx], pu8_data, u16_len);
	u16_logBufIdx += u16_len;

	// flush every 10s
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logFlushRef, SD_LOG_FLUSH_ITV)){
		v_SD_Log_Flush();
	}
}


/*
 * brief	: flush buffer to SD card and clear
 * date
 * - create	: 26.03.04
 */
void v_SD_Log_Flush(){
	if(!b_logOpen || u16_logBufIdx == 0) return;

	UINT bw;
	res = f_write(&logFile, u8_logBuf, u16_logBufIdx, (void*)&bw);
	if(res == FR_OK){
		f_sync(&logFile);
	}

	// clear buffer
	u16_logBufIdx = 0;
	u32_logFlushRef = u32_Tim_1msGet();
}


/*
 * brief	: close sensor log file
 * date
 * - create	: 26.03.04
 */
void v_SD_Log_Close(){
	if(!b_logOpen) return;

	// flush remaining data
	if(u16_logBufIdx > 0){
		v_SD_Log_Flush();
	}
	f_close(&logFile);
	b_logOpen = false;
}


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
