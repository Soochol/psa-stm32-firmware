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


#define SD_LOG_ENABLED	1


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
