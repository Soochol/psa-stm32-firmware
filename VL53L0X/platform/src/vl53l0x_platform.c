//?????#include "hal.h"
#include <string.h>
#include <vl53l0x_platform.h>

#include "../../core/inc/vl53l0x_api.h"
#include "i2c.h"
#include "tim.h"
#include "uart.h"

#define I2C_TIME_OUT_BASE   10
#define I2C_TIME_OUT_BYTE   1
#define VL53L0X_OsDelay(...) HAL_Delay(2)

#define TOF_LOG_ENABLED	0

#ifndef HAL_I2C_MODULE_ENABLED
#warning "HAL I2C module must be enable "
#endif
//extern I2C_HandleTypeDef hi2c1;
//#define VL53L0X_pI2cHandle    (&hi2c1)

/* when not customized by application define dummy one */
#ifndef VL53L0X_GetI2cBus
/** This macro can be overloaded by user to enforce i2c sharing in RTOS context
 */
#   define VL53L0X_GetI2cBus(...) (void)0
#endif

#ifndef VL53L0X_PutI2cBus
/** This macro can be overloaded by user to enforce i2c sharing in RTOS context
 */
#   define VL53L0X_PutI2cBus(...) (void)0
#endif

#ifndef VL53L0X_OsDelay
#   define  VL53L0X_OsDelay(...) (void)0
#endif

#if 0

uint8_t _I2CBuffer[64];

static int _I2CWrite(VL53L0X_DEV Dev, uint8_t *pdata, uint32_t count) {
    int status;
    int i2c_time_out = I2C_TIME_OUT_BASE+ count* I2C_TIME_OUT_BYTE;

    status = HAL_I2C_Master_Transmit(Dev->I2cHandle, Dev->I2cDevAddr, pdata, count, i2c_time_out);
    if (status) {
        //VL6180x_ErrLog("I2C error 0x%x %d len", dev->I2cAddr, len);
        //XNUCLEO6180XA1_I2C1_Init(&hi2c1);
    }
    return status;
}

static int _I2CRead(VL53L0X_DEV Dev, uint8_t *pdata, uint32_t count) {
    int status;
    int i2c_time_out = I2C_TIME_OUT_BASE+ count* I2C_TIME_OUT_BYTE;

    status = HAL_I2C_Master_Receive(Dev->I2cHandle, Dev->I2cDevAddr|1, pdata, count, i2c_time_out);
    if (status) {
        //VL6180x_ErrLog("I2C error 0x%x %d len", dev->I2cAddr, len);
        //XNUCLEO6180XA1_I2C1_Init(&hi2c1);
    }
    return status;
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    int status_int;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    if (count > sizeof(_I2CBuffer) - 1) {
        return VL53L0X_ERROR_INVALID_PARAMS;
    }
    _I2CBuffer[0] = index;
    memcpy(&_I2CBuffer[1], pdata, count);
    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, _I2CBuffer, count + 1);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    VL53L0X_PutI2cBus();
    return Status;
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;
    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, &index, 1);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        goto done;
    }
    status_int = _I2CRead(Dev, pdata, count);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
done:
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    _I2CBuffer[0] = index;
    _I2CBuffer[1] = data;

    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    _I2CBuffer[0] = index;
    _I2CBuffer[1] = data >> 8;
    _I2CBuffer[2] = data & 0x00FF;

    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, _I2CBuffer, 3);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;
    _I2CBuffer[0] = index;
    _I2CBuffer[1] = (data >> 24) & 0xFF;
    _I2CBuffer[2] = (data >> 16) & 0xFF;
    _I2CBuffer[3] = (data >> 8)  & 0xFF;
    _I2CBuffer[4] = (data >> 0 ) & 0xFF;
    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, _I2CBuffer, 5);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t data;

    Status = VL53L0X_RdByte(Dev, index, &data);
    if (Status) {
        goto done;
    }
    data = (data & AndData) | OrData;
    Status = VL53L0X_WrByte(Dev, index, data);
done:
    return Status;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, &index, 1);
    if( status_int ){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        goto done;
    }
    status_int = _I2CRead(Dev, data, 1);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }
done:
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, &index, 1);

    if( status_int ){
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        goto done;
    }
    status_int = _I2CRead(Dev, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        goto done;
    }

    *data = ((uint16_t)_I2CBuffer[0]<<8) + (uint16_t)_I2CBuffer[1];
done:
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    VL53L0X_GetI2cBus();
    status_int = _I2CWrite(Dev, &index, 1);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        goto done;
    }
    status_int = _I2CRead(Dev, _I2CBuffer, 4);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
        goto done;
    }

    *data = ((uint32_t)_I2CBuffer[0]<<24) + ((uint32_t)_I2CBuffer[1]<<16) + ((uint32_t)_I2CBuffer[2]<<8) + (uint32_t)_I2CBuffer[3];

done:
    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev) {
    VL53L0X_Error status = VL53L0X_ERROR_NONE;

    // do nothing
    VL53L0X_OsDelay();
    return status;
}

//end of file
#endif


///////////////////////////////////////////////////////
extern I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef* p_i2c_tof1 = &hi2c1;

bool b_tof2_gpio;

uint8_t _I2CBuffer[64];
uint32_t i2c_tof2_ok, i2c_tof2_busy, i2c_tof2_err;

//DMA polling

void v_TOF2_GPIO_Callback(){
	b_tof2_gpio = true;
}




void v_TOF2_SHUT_High(){
	HAL_GPIO_WritePin(DO_TOF2_SHUT_GPIO_Port, DO_TOF2_SHUT_Pin, GPIO_PIN_SET);
}

void v_TOF2_SHUT_Low(){
	HAL_GPIO_WritePin(DO_TOF2_SHUT_GPIO_Port, DO_TOF2_SHUT_Pin, GPIO_PIN_RESET);
}

bool b_TOF1_GPIO(){
	return HAL_GPIO_ReadPin(DI_ACT_TOF1_GPIO_GPIO_Port, DI_ACT_TOF1_GPIO_Pin);
}

void v_TOF1_SHUT_High(){
	HAL_GPIO_WritePin(DO_ACT_TOF1_SHUT_GPIO_Port, DO_ACT_TOF1_SHUT_Pin, GPIO_PIN_SET);
}

void v_TOF1_SHUT_Low(){
	HAL_GPIO_WritePin(DO_ACT_TOF1_SHUT_GPIO_Port, DO_ACT_TOF1_SHUT_Pin, GPIO_PIN_RESET);
}

#define VL53L0X_FN_MEM	1

//	Function	//
static int _I2CWrite(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
#if VL53L0X_FN_MEM
	int status = HAL_I2C_Mem_Write(Dev->I2cHandle, Dev->I2cDevAddr, index, I2C_MEMADD_SIZE_8BIT, pdata, count, 1000);
    if(status == HAL_OK)		{i2c_tof2_ok++;}
    else{
    	if(status == HAL_BUSY)	{i2c_tof2_busy++;}
    	else					{i2c_tof2_err++;}
    }
    return status;
#else
    _I2CBuffer[0] = index;
    memcpy(&_I2CBuffer[1], pdata, count);
    int status = HAL_I2C_Master_Transmit(Dev->I2cHandle, Dev->I2cDevAddr, _I2CBuffer, count + 1, 1000);
    if(status == HAL_OK)		{i2c_tof2_ok++;}
	else{
		if(status == HAL_BUSY)	{i2c_tof2_busy++;}
		else					{i2c_tof2_err++;}
	}
    return status;
#endif
}

static int _I2CRead(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
#if VL53L0X_FN_MEM
	int status = HAL_I2C_Mem_Read(Dev->I2cHandle, Dev->I2cDevAddr, index, I2C_MEMADD_SIZE_8BIT, pdata, count, 1000);
    if(status == HAL_OK)		{i2c_tof2_ok++;}
	else{
		if(status == HAL_BUSY)	{i2c_tof2_busy++;}
		else					{i2c_tof2_err++;}
	}
    return status;
#else
    //write
    _I2CBuffer[0] = index;
    int status = HAL_I2C_Master_Transmit(Dev->I2cHandle, Dev->I2cDevAddr, _I2CBuffer, 1, 1000);
	if(status == HAL_OK)		{i2c_tof2_ok++;}
	else{
		if(status == HAL_BUSY)	{i2c_tof2_busy++;}
		else					{i2c_tof2_err++;}
		return status;
	}
	//read
	status = HAL_I2C_Master_Receive(Dev->I2cHandle, Dev->I2cDevAddr, pdata, count, 1000);
	if(status == HAL_OK)		{i2c_tof2_ok++;}
	else{
		if(status == HAL_BUSY)	{i2c_tof2_busy++;}
		else					{i2c_tof2_err++;}
	}
	return status;
#endif
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    int status_int;
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    if (count > sizeof(_I2CBuffer) - 1) {
        return VL53L0X_ERROR_INVALID_PARAMS;
    }

    memcpy(_I2CBuffer, pdata, count);

    VL53L0X_GetI2cBus();

    status_int = _I2CWrite(Dev, index, _I2CBuffer, count);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    VL53L0X_PutI2cBus();
    return Status;
}

// the ranging_sensor_comms.dll will take care of the page selection
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;
    VL53L0X_GetI2cBus();

    status_int = _I2CRead(Dev, index, pdata, count);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    _I2CBuffer[0] = data;

    VL53L0X_GetI2cBus();

    status_int = _I2CWrite(Dev, index, _I2CBuffer, 1);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    _I2CBuffer[0] = data >> 8;
    _I2CBuffer[1] = data & 0x00FF;

    VL53L0X_GetI2cBus();

    status_int = _I2CWrite(Dev, index, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    _I2CBuffer[0] = (data >> 24) & 0xFF;
    _I2CBuffer[1] = (data >> 16) & 0xFF;
    _I2CBuffer[2] = (data >> 8)  & 0xFF;
    _I2CBuffer[3] = (data >> 0 ) & 0xFF;

    VL53L0X_GetI2cBus();

    status_int = _I2CWrite(Dev, index, _I2CBuffer, 4);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    uint8_t data;

    Status = VL53L0X_RdByte(Dev, index, &data);
    if (Status) {
        goto done;
    }
    data = (data & AndData) | OrData;
    Status = VL53L0X_WrByte(Dev, index, data);
done:
    return Status;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    VL53L0X_GetI2cBus();

    status_int = _I2CRead(Dev, index, data, 1);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    VL53L0X_GetI2cBus();

    status_int = _I2CRead(Dev, index, _I2CBuffer, 2);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    *data = ((uint16_t)_I2CBuffer[0]<<8) + (uint16_t)_I2CBuffer[1];

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data) {
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;

    VL53L0X_GetI2cBus();

    status_int = _I2CRead(Dev, index, _I2CBuffer, 4);
    if (status_int != 0) {
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;
    }

    *data = ((uint32_t)_I2CBuffer[0]<<24) + ((uint32_t)_I2CBuffer[1]<<16) + ((uint32_t)_I2CBuffer[2]<<8) + (uint32_t)_I2CBuffer[3];

    VL53L0X_PutI2cBus();
    return Status;
}

VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev) {
    VL53L0X_Error status = VL53L0X_ERROR_NONE;

    // do nothing
    VL53L0X_OsDelay();
    return status;
}




//	user	//
VL53L0X_Dev_t tof1;
VL53L0X_Dev_t tof2;


VL53L0X_DEV p_dev1 = &tof1;
VL53L0X_DEV p_dev2 = &tof2;
VL53L0X_Version_t tof_ver1, tof_ver2;
VL53L0X_DeviceInfo_t tof_info1, tof_info2;


uint32_t tof_err;



int i_TOF_Init(VL53L0X_DEV dev, VL53L0X_Version_t* ver, VL53L0X_DeviceInfo_t* info){
	VL53L0X_Error status;
	status = VL53L0X_GetVersion(ver);
	if(status != VL53L0X_ERROR_NONE){
#if TOF_LOG_ENABLED
		v_printf_poll("VL53L0X GetVersion fail\n");
#endif
		return -1;
	}

	status = VL53L0X_DataInit(dev);
	if(status != VL53L0X_ERROR_NONE){
#if TOF_LOG_ENABLED
		v_printf_poll("VL53L0X DataInit fail\n");
#endif
		return -1;
	}

	status = VL53L0X_GetDeviceInfo(dev, info);
	if(status != VL53L0X_ERROR_NONE){
#if TOF_LOG_ENABLED
		v_printf_poll("VL53L0X GetDeviceInfo fail\n");
#endif
		return -1;
	}
	return 0;
}


uint8_t VhvSettings;
uint32_t refSpadCount;
uint8_t isAperture;
uint8_t phaseCal;

VL53L0X_RangingMeasurementData_t singleMeas[16];
FixPoint1616_t LimitCheckCurrent[16];

#define TOF_RAING_DELAY_ADD	0

void v_TOF_RangingTest(VL53L0X_DEV dev){
	VL53L0X_Error status;


	//VL53L0X_STATE_WAIT_STATICINIT to VL53L0X_STATE_IDLE.
	status = VL53L0X_StaticInit(dev);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif
	status = VL53L0X_PerformRefCalibration(dev, &VhvSettings, &phaseCal);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif
	status = VL53L0X_PerformRefSpadManagement(dev, &refSpadCount, &isAperture);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif
	status = VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_SINGLE_RANGING);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif
	//Enable/Disable Sigma and Signal cheeck
	status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE, 1);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif
	status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE, 1);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif

	//RANGE_IGNORE_THRESHOLD
	status = VL53L0X_SetLimitCheckEnable(dev, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, 1);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif
	status = VL53L0X_SetLimitCheckValue(dev, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, (FixPoint1616_t)(1.5*0.023*65536));
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
#if TOF_RAING_DELAY_ADD
	HAL_Delay(2);
#endif

#if 0
	for(int i=0; i<10; i++){
		status = VL53L0X_PerformSingleRangingMeasurement(p_dev1, &singleMeas[i]);
		VL53L0X_GetLimitCheckCurrent(p_dev1, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &LimitCheckCurrent[i]);
	}
#endif

}

int i_TOF_RangingTest_Cont(VL53L0X_DEV dev){
	//VL53L0X_STATE_WAIT_STATICINIT to VL53L0X_STATE_IDLE.
	if(VL53L0X_StaticInit(dev) != VL53L0X_ERROR_NONE){return -1;}
	if(VL53L0X_PerformRefCalibration(dev, &VhvSettings, &phaseCal) != VL53L0X_ERROR_NONE){return -1;}
	if(VL53L0X_PerformRefSpadManagement(dev, &refSpadCount, &isAperture) != VL53L0X_ERROR_NONE){return -1;}
	if(VL53L0X_SetDeviceMode(dev, VL53L0X_DEVICEMODE_CONTINUOUS_TIMED_RANGING) != VL53L0X_ERROR_NONE){return -1;}
	if(VL53L0X_SetInterMeasurementPeriodMilliSeconds(dev, 100) != VL53L0X_ERROR_NONE){return -1;}
	if(VL53L0X_StartMeasurement(dev) != VL53L0X_ERROR_NONE){return -1;}
	return 0;
}



uint32_t tof2_measure;
VL53L0X_RangingMeasurementData_t tof2_m;

uint32_t tof_retry;

uint8_t prodMajor1, prodMinor1;
uint8_t prodMajor2, prodMinor2;

#define TOF_TOGGLE_TEST	1


uint8_t rdBuf[16];


void v_TOF_CommCheck(){
	static uint32_t timRef;
	static bool config;
#if TOF_TOGGLE_TEST
	static uint16_t toggle;
#endif
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 500)){
		timRef = u32_Tim_1msGet();

		if(config == false){
			tof1.I2cDevAddr = ADDR_TOF1;
			tof1.I2cHandle = p_i2c_tof1;

			v_TOF1_SHUT_High();
			HAL_Delay(200);
			config = true;
			toggle = 0;
		}
		else{
			VL53L0X_Error status;
			//status = VL53L0X_GetDeviceInfo(p_dev2, &tof_info);
#if TOF_TOGGLE_TEST
			if(toggle & 1){
				//TOF2
				//status = VL53L0X_GetProductRevision(p_dev2, &prodMajor2, &prodMinor2);
				status = _I2CRead(p_dev2, 0xC0, rdBuf, 1);
				if(status != VL53L0X_ERROR_NONE){
					tof_err++;
					v_printf_poll("VL53L0X_2 GetDeviceInfo fail\n");
				}
				else{
					v_printf_poll("VL53L0X_2 GetDeviceInfo succ : %2X\n", rdBuf[0]);
				}
			}
			else{
				//TOF1
				//status = VL53L0X_GetProductRevision(p_dev1, &prodMajor1, &prodMinor1);
				status = _I2CRead(p_dev1, 0xC0, rdBuf, 1);
				if(status != VL53L0X_ERROR_NONE){
					tof_err++;
					v_printf_poll("VL53L0X_1 GetDeviceInfo fail\n");
				}
				else{
					v_printf_poll("VL53L0X_1 GetDeviceInfo succ : %2X\n", rdBuf[0]);
				}
			}
			toggle++;
#else
			status = VL53L0X_GetProductRevision(p_dev1, &prodMajor1, &prodMinor1);
			if(status != VL53L0X_ERROR_NONE){
				tof_err++;
				v_printf_poll("VL53L0X GetDeviceInfo fail\n");
			}
			else{
				v_printf_poll("VL53L0X GetDeviceInfo succ\n");
			}
#endif
		}
	}
}

#include "mode.h"
#define TOF_SINGLE	0
#define TOF2_ACTIVE	0


VL53L0X_RangingMeasurementData_t tof1_meas, tof2_meas;
FixPoint1616_t lmtCurrent1, lmtCurrent2;
/*
 * tBOOT	: XSHUT HIGH stable time	: 1.2 ms (maximum)
 *
 *
 */



e_COMM_STAT_t e_tof_config;

void v_TOF_Deinit(){
	e_tof_config = COMM_STAT_READY;
}

e_COMM_STAT_t e_TOF_Ready(){
	if(e_tof_config == COMM_STAT_READY){
		p_dev1->I2cDevAddr = ADDR_TOF1;
		p_dev1->I2cHandle = p_i2c_tof1;

		v_TOF1_SHUT_High();
		HAL_Delay(100);

		if(i_TOF_Init(p_dev1, &tof_ver1, &tof_info1) != 0){
			return e_tof_config = COMM_STAT_ERR;
		}
#if TOF_SINGLE
		v_TOF_RangingTest(p_dev1);
		v_TOF_RangingTest(p_dev2);
#else
		if(i_TOF_RangingTest_Cont(p_dev1) != 0){
			return e_tof_config = COMM_STAT_ERR;
		}
#endif
		e_tof_config = COMM_STAT_DONE;
	}
	return e_tof_config;
}


uint8_t tof_ready1;

void v_TOF_Handler(){
	static uint32_t timRef;
	if(e_tof_config != COMM_STAT_DONE){return;}
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 100)){return;}
	timRef = u32_Tim_1msGet();
	//static uint32_t timRef;

	int status;
#if TOF_SINGLE
	status = VL53L0X_PerformSingleRangingMeasurement(p_dev1, &tof1_meas);
	VL53L0X_GetLimitCheckCurrent(p_dev1, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &lmtCurrent1);

	status = VL53L0X_PerformSingleRangingMeasurement(p_dev2, &tof2_meas);
	VL53L0X_GetLimitCheckCurrent(p_dev1, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &lmtCurrent2);
#else
	status = VL53L0X_GetMeasurementDataReady(p_dev1, &tof_ready1);
	if(status != VL53L0X_ERROR_NONE){tof_err++;}
	if(tof_ready1 == 1){
		status = VL53L0X_GetRangingMeasurementData(p_dev1, &tof1_meas);
		if(status != VL53L0X_ERROR_NONE){tof_err++;}
		VL53L0X_ClearInterruptMask(p_dev1, 0);
	}
#endif
}


uint16_t u16_TOF_Get_1(){
	return tof1_meas.RangeMilliMeter;
}

uint16_t u16_TOF_Get_2(){
	return tof2_meas.RangeMilliMeter;
}



#if 0

void v_TOF_Handler(){
	static uint32_t timRef;
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 100)){return;}
	timRef = u32_Tim_1msGet();
	static bool config;
	//static uint32_t timRef;
	if(config == false){
		config = true;
		p_dev1->I2cDevAddr = ADDR_TOF1;
		p_dev1->I2cHandle = p_i2c_tof1;

		p_dev2->I2cDevAddr = ADDR_TOF2;
		p_dev2->I2cHandle = p_i2c_tof2;

		v_TOF2_SHUT_High();
		v_TOF1_SHUT_High();

		HAL_Delay(100);

		v_TOF_Init(p_dev1, &tof_ver1, &tof_info1);
		v_TOF_Init(p_dev2, &tof_ver2, &tof_info2);
#if TOF_SINGLE
		v_TOF_RangingTest(p_dev1);
		v_TOF_RangingTest(p_dev2);
#else
		i_TOF_RangingTest_Cont(p_dev1);
		i_TOF_RangingTest_Cont(p_dev2);
#endif
	}
	else{
		int status;
#if TOF_SINGLE
		status = VL53L0X_PerformSingleRangingMeasurement(p_dev1, &tof1_meas);
		VL53L0X_GetLimitCheckCurrent(p_dev1, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &lmtCurrent1);

		status = VL53L0X_PerformSingleRangingMeasurement(p_dev2, &tof2_meas);
		VL53L0X_GetLimitCheckCurrent(p_dev1, VL53L0X_CHECKENABLE_RANGE_IGNORE_THRESHOLD, &lmtCurrent2);
#else
		status = VL53L0X_GetRangingMeasurementData(p_dev1, &tof1_meas);
		if(status != VL53L0X_ERROR_NONE){tof_err++;}
		VL53L0X_ClearInterruptMask(p_dev1, 0);

		status = VL53L0X_GetRangingMeasurementData(p_dev2, &tof2_meas);
		if(status != VL53L0X_ERROR_NONE){tof_err++;}
		VL53L0X_ClearInterruptMask(p_dev2, 0);
#endif
	}
}

#endif

