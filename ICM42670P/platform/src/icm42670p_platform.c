#include "main.h"
#include "icm42670p_platform.h"
#include "string.h"
#include "tim.h"
#include "i2c.h"
//#include "quaternion_mahony.h"
#include "mode.h"

extern I2C_HandleTypeDef hi2c2;
static I2C_HandleTypeDef* p_i2c = &hi2c2;



typedef struct {
	uint8_t header;
	int16_t acc_x;
	int16_t acc_y;
	int16_t acc_z;
	int16_t gyro_x;
	int16_t gyro_y;
	int16_t gyro_z;
	int16_t temp;
	uint8_t time;
} IMU_FIFO_t;

typedef enum {
	IMU_RET_OK=0,
	IMU_RET_DONE,
	IMU_RET_ERR,
} e_IMU_RET_t;




#define IMU_LEFT_ACTIVE		1
#define IMU_RIGHT_ACTIVE	1
#define IMU_FIFO_ACTIVE		0
#define IMU_RETRY_ID		0

//////////////////////
//		TILT		//
//////////////////////
#define IMU_TILT_SAMPLING_CNT	10
typedef struct {
	float f_buf[3][IMU_TILT_SAMPLING_CNT];
	uint16_t u16_in, u16_cnt;
	float f_last[3];
	float f_sum[3];
	float f_avg[3];
} x_IMU_TILT_AVG_t;

typedef struct {
	int16_t i16_buf[6][IMU_TILT_SAMPLING_CNT];
	uint16_t u16_in, u16_cnt;
	int16_t i16_last[6];
	int32_t i32_sum[6];
	int16_t i16_avg[6];
} x_IMU_RAW_AVG_t;

static void v_IMU_Tilt_Compute_Orientation(x_QUAT_t* p_orientation, _x_XYZ_t* p_intgral_error, int16_t* pi16_imu);
static void v_IMU_Tilt_Compute_Angle(_x_XYZ_t* p_angle, x_QUAT_t* p_orientation);
static void v_IMU_Tilt_Compute_Offset(_x_XYZ_t* p_offset, _x_XYZ_t* p_center, _x_XYZ_t* p_now);
static void v_IMU_Tilt_Compute_AVG(x_IMU_TILT_AVG_t* p_avg, _x_XYZ_t* p_angle);

static void v_IMU_Int_Handler();



//raw
uint8_t imu_arr[16 + 1];
//from register
int16_t imu_left[6 + 1];
int16_t imu_right[6 + 1];
//from fifo
IMU_FIFO_t imu_fifo_left, imu_fifo_right;
//id
uint8_t imu_id_left, imu_id_right;
//flag
static bool b_mclk;
//evt
uint8_t imu_evt_left, imu_evt_right;





#define IMU_RX_SIZE	32

static uint8_t u8_rd[IMU_RX_SIZE];
uint8_t u8_imuRX[IMU_RX_SIZE];
static bool b_imu_evt;

static uint8_t u8_arr_left[IMU_RX_SIZE];
static uint8_t u8_arr_right[IMU_RX_SIZE];

static uint32_t u32_toutRef_L, u32_toutRef_R;
static e_COMM_STAT_t e_imu_config;

static uint8_t u8_rd_L[IMU_RX_SIZE];
static uint8_t u8_rd_R[IMU_RX_SIZE];

//	FUNCT	//
e_IMU_RET_t e_IMU_MCLK_On(bool* pb_init, uint8_t u8_addr);



#if 0
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(GPIO_Pin == DI_IMUR_INT_Pin){

	}
	if(GPIO_Pin == DI_IMUL_INT_Pin){

	}
}
#endif

void IMU_Init(){
	//__HAL_I2C_ENABLE_IT(p_i2c, I2C_IT_ERRI);

}




static e_COMM_STAT_t e_imu_evt_L, e_imu_evt_R;



void v_IMU_RD_Done(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_cnt){
	memcpy(u8_rd, pu8_arr, u16_cnt);
	memcpy(u8_imuRX, pu8_arr, u16_cnt);
	b_imu_evt = true;

	if(u8_addr == ADDR_IMU_LEFT){
		e_imu_evt_L = COMM_STAT_DONE;
		memcpy(u8_rd_L, pu8_arr, u16_cnt);
	}
	else{
		e_imu_evt_R = COMM_STAT_DONE;
		memcpy(u8_rd_R, pu8_arr, u16_cnt);
	}

}

void v_IMU_WR_Done(uint8_t u8_addr){
	b_imu_evt = true;

	if(u8_addr == ADDR_IMU_LEFT){e_imu_evt_L = COMM_STAT_DONE;}
	else						{e_imu_evt_R = COMM_STAT_DONE;}
}



int i_IMU_Write(uint8_t u8_addr, uint16_t u16_memAddr, uint8_t* pu8, uint16_t u16_cnt){
	if(u8_addr == ADDR_IMU_LEFT){
		e_imu_evt_L = COMM_STAT_BUSY;
		u32_toutRef_L = u32_Tim_1msGet();
	}
	else{
		e_imu_evt_R = COMM_STAT_BUSY;
		u32_toutRef_R = u32_Tim_1msGet();
	}
	return i_I2C2_Write(u8_addr, u16_memAddr, pu8, u16_cnt);
}

int i_IMU_Read(uint8_t u8_addr, uint16_t u16_memAddr, uint16_t u16_cnt){
	if(u8_addr == ADDR_IMU_LEFT){
		e_imu_evt_L = COMM_STAT_BUSY;
		u32_toutRef_L = u32_Tim_1msGet();
	}
	else{
		e_imu_evt_R = COMM_STAT_BUSY;
		u32_toutRef_R = u32_Tim_1msGet();
	}
	return i_I2C2_Read(u8_addr, u16_memAddr, u16_cnt);
}

void v_IMU_Tout_Handler(){
	if((e_imu_evt_L == COMM_STAT_BUSY) && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef_L, 2000)){
		//timeout
		e_imu_evt_L = COMM_STAT_READY;
		e_imu_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_IMU);
		v_Mode_SetNext(modeERROR);
	}

	if((e_imu_evt_R == COMM_STAT_BUSY) && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef_R, 2000)){
		//timeout
		e_imu_evt_R = COMM_STAT_READY;
		e_imu_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_IMU);
		v_Mode_SetNext(modeERROR);
	}
}




e_IMU_RET_t e_IMU_Read_MREG(bool* pb_init, uint8_t u8_addr, uint32_t u32_reg, uint8_t* pu8_dest, uint16_t u16_len){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		if(b_mclk)	{order = 1;}
		else		{order = 0;}
	}
	uint8_t wr[4] = {0,};
	bool retry;

	do{
		retry = false;
		switch(order){
		case 0:
			if(e_IMU_MCLK_On(pb_init, u8_addr) != IMU_RET_DONE){
				order--;
			}
			else{
				retry = true;
			}
			break;
		case 1:
			//BLK_SEL_R + MADDR_R
			wr[0] = u32_reg >> 8;
			wr[1] = u32_reg;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_BLK_SEL_R, wr, 2);
			break;
		case 2:
			HAL_Delay(2);
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_M_R, u16_len);
			break;
		case 3:
			memcpy(pu8_dest, u8_rd, u16_len);
			HAL_Delay(2);
			//BLK_SEL_R
			if(u32_reg & 0x0000FF00){
				wr[0] = 0;
				i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_BLK_SEL_R, wr, 1);
				break;
			}
			else{
				return IMU_RET_DONE;
			}
		case 4:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Write_MREG(bool* pb_init, uint8_t u8_addr, uint32_t u32_reg, uint8_t* pu8_arr, uint16_t u16_len){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		if(b_mclk)	{order = 1;}
		else		{order = 0;}
	}
	uint8_t wr[16] = {0,};
	bool retry;

	do{
		retry = false;
		switch(order){
		case 0:
			if(e_IMU_MCLK_On(pb_init, u8_addr) != IMU_RET_DONE){
				order--;
			}
			else{
				retry = true;
			}
			break;
		case 1:
			//BLK_SEL_W
			wr[0] = u32_reg >> 8;
			//MADDR_W
			wr[1] = u32_reg;
			//M_W
			memcpy(wr + 2, pu8_arr, u16_len);
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_BLK_SEL_W, wr, u16_len + 2);
			break;
		case 2:
			//BLK_SEL_W
			if(u32_reg & 0x0000FF00){
				wr[0] = 0;
				i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_BLK_SEL_W, wr, 1);
				break;
			}
			else{
				return IMU_RET_DONE;
			}
		case 3:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}



e_IMU_RET_t e_IMU_Reset(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};

	switch(order){
	case 0:
		wr[0] = 0;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_BLK_SEL_R, wr, 1);
		break;
	case 1:
		wr[0] = 0;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_BLK_SEL_W, wr, 1);
		break;
	case 2:
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_SIGNAL_PATH_RESET, 1);
		break;
	case 3:
		wr[0] = u8_rd[0];
		wr[0] |= ICM42670P_BANK0_MASK_SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_SIGNAL_PATH_RESET, wr, 1);
		break;
	case 4:
		HAL_Delay(100);
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS, 1);
		break;
	case 5:
		if(u8_rd[0] & ICM42670P_BANK0_MASK_INT_STATUS_RESET_DONE_INT){
			return IMU_RET_DONE;
		}
		break;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}


e_IMU_RET_t e_IMU_Get_ID(bool* pb_init, uint8_t u8_addr, uint8_t* pu8_id){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}

	switch(order){
	case 0:
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_WHO_AM_I, 1);
		break;
	case 1:
		*pu8_id = u8_rd[0];
		return IMU_RET_DONE;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}


e_IMU_RET_t e_IMU_Config_INTF(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};

	switch(order){
	case 0:
		wr[0] = 0x09;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_DRIVE_CONFIG2, wr, 1);
		break;
	case 1:
		//INTF_CONFIG1
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INTF_CONFIG1, 1);
		break;
	case 2:
		wr[0] = u8_rd[0];
		wr[0] &= ~ICM42670P_BANK0_MASK_INTF_CONFIG1_I3C_SDR_EN;
		wr[0] &= ~ICM42670P_BANK0_MASK_INTF_CONFIG1_I3C_DDR_EN;

		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_INTF_CONFIG1, wr, 1);
		break;
	case 3:
		return IMU_RET_DONE;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}


uint8_t imu_config_int;
e_IMU_RET_t e_IMU_Config_INT(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	static bool mreg;
	if(*pb_init){
		*pb_init = false;
		mreg = true;
		order = 0;
	}
	uint8_t wr[4] = {0,};
	static uint8_t rd[4];
	bool retry;
	do{
		retry = false;
		switch(order){
		case 0:
			//INT_CONFIG
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_CONFIG, 1);
			break;
		case 1:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_INT_CONFIG_INT1_MODE;
			wr[0] &= ~ICM42670P_BANK0_MASK_INT_CONFIG_INT1_DRIVE_CIRCUIT;
			wr[0] &= ~ICM42670P_BANK0_MASK_INT_CONFIG_INT1_POLARITY;

			//wr[0] |= ICM42670P_BANK0_INT_CONFIG_INT1_MODE_PULSED;
			wr[0] |= ICM42670P_BANK0_INT_CONFIG_INT1_MODE_LATCHED;
			wr[0] |= ICM42670P_BANK0_INT_CONFIG_INT1_POLARITY_LOW;
			wr[0] |= ICM42670P_BANK0_INT_CONFIG_INT1_DRIVE_CIRCUIT_OD;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_INT_CONFIG, wr, 1);
			break;
		case 2:
			//INT_SOURCE0
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_SOURCE0, 1);
			break;
		case 3:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_INT_SOURCE0_DRDY_INT1_EN;

			wr[0] |= ICM42670P_BANK0_MASK_INT_SOURCE0_DRDY_INT1_EN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_INT_SOURCE0, wr, 1);
			break;
		case 4:
			//INT_SOURCE0
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_SOURCE0, 1);
			break;
		case 5:
			imu_config_int = u8_rd[0];
			break;
		case 6:
			if(e_IMU_Read_MREG(&mreg, u8_addr, ICM42670P_MREG1_REG_INT_SOURCE6, rd, 1) == IMU_RET_DONE){
				retry = true;
				mreg = true;
			}
			else{
				order--;
			}
			break;
		case 7:
			wr[0] = rd[0];
			wr[0] &= ~ICM42670P_MREG1_MASK_INT_SOURCE6_FF_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_LOWG_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_STEP_DET_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_STEP_CNT_OFL_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_TILT_DET_INT1_EN;
/*
			wr[0] |= ICM42670P_MREG1_MASK_INT_SOURCE6_FF_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_LOWG_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_STEP_DET_INT1_EN	|\
					ICM42670P_MREG1_MASK_INT_SOURCE6_STEP_CNT_OFL_INT1_EN;
*/
			if(e_IMU_Write_MREG(&mreg, u8_addr, ICM42670P_MREG1_REG_INT_SOURCE6, wr, 1) == IMU_RET_DONE){
				retry = true;
				mreg = true;
			}
			else{
				order--;
			}
			break;
		case 8:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Config_ACC(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};

	switch(order){
	case 0:
		//ACCEL_CONFIG0
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_CONFIG0, 1);
		break;
	case 1:
		wr[0] = u8_rd[0];
		wr[0] &= ~ICM42670P_BANK0_MASK_ACCEL_CONFIG0_ACCEL_UI_FS_SEL;
		wr[0] &= ~ICM42670P_BANK0_MASK_ACCEL_CONFIG0_ACCEL_ODR;

		wr[0] |= ICM42670P_BANK0_ACCEL_CONFIG0_FS_SEL_2g;
		wr[0] |= ICM42670P_BANK0_ACCEL_CONFIG0_ODR_100_HZ;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_ACCEL_CONFIG0, wr, 1);
		break;
	case 2:
		//ACCEL_CONFIG1
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_CONFIG1, 1);
		break;
	case 3:
		wr[0] = u8_rd[0];
		wr[0] &= ~ICM42670P_BANK0_MASK_ACCEL_CONFIG1_ACCEL_UI_FILT_BW;

		wr[0] |= ICM42670P_BANK0_ACCEL_CONFIG1_ACCEL_FILT_BW_25;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_ACCEL_CONFIG1, wr, 1);
		break;
	case 4:
		return IMU_RET_DONE;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Config_GYRO(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};

	switch(order){
	case 0:
		//GYRO_CONFIG0
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_GYRO_CONFIG0, 1);
		break;
	case 1:
		wr[0] = u8_rd[0];
		wr[0] &= ~ICM42670P_BANK0_MASK_GYRO_CONFIG0_GYRO_UI_FS_SEL;
		wr[0] &= ~ICM42670P_BANK0_MASK_GYRO_CONFIG0_GYRO_ODR;

		wr[0] |= ICM42670P_BANK0_GYRO_CONFIG0_FS_SEL_2000dps;
		wr[0] |= ICM42670P_BANK0_GYRO_CONFIG0_ODR_100_HZ;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_GYRO_CONFIG0, wr, 1);
		break;
	case 2:
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_GYRO_CONFIG1, 1);
		break;
	case 3:
		wr[0] = u8_rd[0];
		wr[0] &= ~ICM42670P_BANK0_MASK_GYRO_CONFIG1_GYRO_UI_FILT_BW;

		wr[0] |= ICM42670P_BANK0_GYRO_CONFIG1_GYRO_FILT_BW_73;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_GYRO_CONFIG1, wr, 1);
		break;
	case 4:
		return IMU_RET_DONE;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_MCLK_On(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};

	switch(order){
	case 0:
		//PWR_MGMT0
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, 1);
		break;
	case 1:
		wr[0] = u8_rd[0];

		wr[0] |= ICM42670P_BANK0_MASK_PWR_MGMT0_IDLE;
		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, wr, 1);
		break;
	case 2:
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, 1);
		break;
	case 3:
		if(u8_rd[0] & ICM42670P_BANK0_MASK_PWR_MGMT0_IDLE){
			b_mclk = true;
			return IMU_RET_DONE;
		}
		else{
			order -= 2;
		}
		break;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_MCLK_Off(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};

	switch(order){
	case 0:
		//PWR_MGMT0
		i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, 1);
		break;
	case 1:
		wr[0] = u8_rd[0];
		wr[0] &= ~ICM42670P_BANK0_MASK_PWR_MGMT0_IDLE;

		i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, wr, 1);
		break;
	case 2:
		return IMU_RET_DONE;
	default:
		return IMU_RET_ERR;
	}
	order++;
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Config_FIFO(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	static bool mclk_init;
	static bool mreg_init;
	if(*pb_init){
		*pb_init = false;
		if(b_mclk){
			order = 1;
		}
		else{
			mclk_init = true;
			order = 0;
		}
	}
	static uint8_t wr[4];
	static uint8_t rd[8];
	bool retry;

	do{
		retry = false;
		switch(order){
		case 0:
			//MCLK ON
			if(e_IMU_MCLK_On(&mclk_init, u8_addr) != IMU_RET_DONE){
				order--;
			}
			else{
				retry = true;
			}
			break;
		case 1:
			//INTF_CONFIG0
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INTF_CONFIG0, 1);
			break;
		case 2:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_INTF_CONFIG0_FIFO_COUNT_FORMAT;
			wr[0] &= ~ICM42670P_BANK0_MASK_INTF_CONFIG0_FIFO_COUNT_ENDIAN;

			wr[0] |= ICM42670P_BANK0_INTF_CONFIG0_FIFO_COUNT_REC_RECORD;
			wr[0] |= ICM42670P_BANK0_INTF_CONFIG0_FIFO_COUNT_LITTLE_ENDIAN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_INTF_CONFIG0, wr, 1);
			break;
		case 3:
			//FIFO_CONFIG0
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_FIFO_CONFIG1, 1);
			break;
		case 4:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_FIFO_CONFIG1_FIFO_MODE;
			wr[0] &= ~ICM42670P_BANK0_MASK_FIFO_CONFIG1_FIFO_BYPASS;

			wr[0] |= ICM42670P_BANK0_FIFO_CONFIG1_FIFO_MODE_SNAPSHOT;
			wr[0] |= ICM42670P_BANK0_FIFO_CONFIG1_FIFO_BYPASS_OFF;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_FIFO_CONFIG1, wr, 1);
			mreg_init = true;
			break;
		case 5:
			//TMST_CONFIG1_MREG1
			if(e_IMU_Read_MREG(&mreg_init, u8_addr, ICM42670P_MREG1_REG_TMST_CONFIG1, rd, 1) == IMU_RET_DONE){
				wr[0] = rd[0];
				wr[0] &= ~ICM42670P_MREG1_MASK_TMST_CONFIG1_TMST_EN;

				wr[0] |= ICM42670P_MREG1_TMST_CONFIG1_TMST_EN;
				retry = true;
				mreg_init = true;
			}
			else{
				order--;
			}
			break;
		case 6:
			if(e_IMU_Write_MREG(&mreg_init, u8_addr, ICM42670P_MREG1_REG_TMST_CONFIG1, wr, 1) == IMU_RET_DONE){
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 7:
			wr[0] = 0;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_FIFO_CONFIG2, wr, 1);
			mreg_init = true;
			break;
		case 8:
			//FIFO_CONFIG5_MREG1
			if(e_IMU_Read_MREG(&mreg_init, u8_addr, ICM42670P_MREG1_REG_FIFO_CONFIG5, rd, 1) == IMU_RET_DONE){
				wr[0] = rd[0];
				wr[0] &= ~ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_ACCEL_EN;
				wr[0] &= ~ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_HIRES_EN;
				wr[0] &= ~ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_GYRO_EN;
				wr[0] &= ~ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_TMST_FSYNC_EN;

				wr[0] |= ICM42670P_MREG1_FIFO_CONFIG5_GYRO_EN;
				wr[0] |= ICM42670P_MREG1_FIFO_CONFIG5_TMST_FSYNC_EN;
				wr[0] |= ICM42670P_MREG1_FIFO_CONFIG5_ACCEL_EN;
				retry = true;
				mreg_init = true;
			}
			else{
				order--;
			}
			break;
		case 9:
			if(e_IMU_Write_MREG(&mreg_init, u8_addr, ICM42670P_MREG1_REG_FIFO_CONFIG5, wr, 1) == IMU_RET_DONE){
				retry = true;
				mclk_init = true;
			}
			else{
				order--;
			}
			break;
		case 10:
			if(e_IMU_MCLK_Off(&mclk_init, u8_addr) == IMU_RET_DONE){
				return IMU_RET_DONE;
			}
			else{
				order--;
			}
			break;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

uint32_t u32_IMU_Get_ACC_ODR_toUS(e_ICM42670P_BANK0_ACCEL_CONFIG0_ODR_t e_odr){
	uint32_t us = 625;
	uint16_t pos = e_odr - ICM42670P_BANK0_ACCEL_CONFIG0_ODR_1600_HZ;
	if(pos > 10){
		//max value
		pos = 10;
	}
	us <<= pos;

	return us;
}

e_IMU_RET_t e_IMU_Config_PWR_Mode(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[4] = {0,};
	uint8_t acc, gyro;
	uint32_t accel_odr_us=0;
	e_ICM42670P_BANK0_ACCEL_CONFIG0_ODR_t odr;
	uint32_t delay;
	bool retry;

	do{
		retry = false;
		switch(order){
		case 0:
			//PWR_MGMT0
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, 1);
			break;
		case 1:
			wr[0] = u8_rd[0];

			acc = u8_rd[0] & ICM42670P_BANK0_MASK_PWR_MGMT0_ACCEL_MODE;
			gyro = u8_rd[0] & ICM42670P_BANK0_MASK_PWR_MGMT0_GYRO_MODE;
			if((acc == ICM42670P_BANK0_PWR_MGMT0_ACCEL_MODE_LP) && \
			  ((gyro == ICM42670P_BANK0_PWR_MGMT0_GYRO_MODE_OFF) || (gyro == ICM42670P_BANK0_PWR_MGMT0_GYRO_MODE_STANDBY))){
				//ACCEL_CONFIG0
				i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_CONFIG0, 1);
				break;
			}
			order += 3;
			retry = true;
			break;
		case 2:
			//RCOSC
			odr = u8_rd[0] & ICM42670P_BANK0_MASK_ACCEL_CONFIG0_ACCEL_ODR;
			accel_odr_us = u32_IMU_Get_ACC_ODR_toUS(odr);

			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, 1);
			break;
		case 3:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_PWR_MGMT0_ACCEL_LP_CLK_SEL;

			wr[0] |= ICM42670P_BANK0_PWR_MGMT0_ACCEL_LP_CLK_RCOSC;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, wr, 1);
			break;
		case 4:
			delay = (accel_odr_us / 1000) + 1;
			HAL_Delay(delay);	//continued
			retry = true;
			break;
		case 5:
			//PWR_MGMT0
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_PWR_MGMT0_ACCEL_MODE;
			wr[0] &= ~ICM42670P_BANK0_MASK_PWR_MGMT0_GYRO_MODE;

			wr[0] |= ICM42670P_BANK0_PWR_MGMT0_ACCEL_MODE_LN;
			wr[0] |= ICM42670P_BANK0_PWR_MGMT0_GYRO_MODE_LN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_PWR_MGMT0, wr, 1);
			break;
		case 6:
			HAL_Delay(50);
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}



e_IMU_RET_t e_IMU_Config_Parameter(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	static bool mreg;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;
	uint8_t wr[8] = {0,};
	do{
		retry = false;
		switch(order){
		case 0:
			//status reads
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, 1);
			break;
		case 1:
			if(u8_rd[0] & (ICM42670P_BANK0_MASK_APEX_CONFIG1_PED_ENABLE | ICM42670P_BANK0_MASK_APEX_CONFIG1_FF_ENABLE | ICM42670P_BANK0_MASK_APEX_CONFIG1_SMD_ENABLE | ICM42670P_BANK0_MASK_APEX_CONFIG1_TILT_ENABLE)){
				return IMU_RET_ERR;
			}
			else{
				retry = true;
				mreg = true;
			}
			break;
		case 2:
			//MREG1	- APEX_CONFIG2
			wr[0] = ICM42670P_MREG1_APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL_8_S | \
					ICM42670P_MREG1_APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL_80_MG;
			//MREG1	- APEX_CONFIG3
			wr[1] = ICM42670P_MREG1_APEX_CONFIG3_PEDO_AMP_TH_62_MG | \
					5;	//step counts : 5
			//MREG1	- APEX_CONFIG4
			wr[2] = ICM42670P_MREG1_APEX_CONFIG4_PEDO_SB_TIMER_TH_150_SAMPLES | \
					ICM42670P_MREG1_APEX_CONFIG4_PEDO_HI_ENRGY_TH_104_MG | \
					2 << ICM42670P_MREG1_POS_APEX_CONFIG4_PED_STEP_DET_TH_SEL;	//step : 2
			//MREG1	- APEX_CONFIG5
			wr[3] = ICM42670P_MREG1_APEX_CONFIG5_TILT_WAIT_TIME_4_S	|\
					ICM42670P_MREG1_APEX_CONFIG5_LOWG_PEAK_TH_HYST_156_MG	|\
					ICM42670P_MREG1_APEX_CONFIG5_HIGHG_PEAK_TH_HYST_156_MG;

			if(e_IMU_Write_MREG(&mreg, u8_addr, ICM42670P_MREG1_REG_APEX_CONFIG2, wr, 4) == IMU_RET_DONE){
				retry = true;
				mreg = true;
			}
			else{
				order--;
			}
			break;
		case 3:
			//APEX_CONFIG0	- read
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, 1);
			break;
		case 4:
			//APEX_CONFIG0	- write
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_POWER_SAVE_EN;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG0_DMP_POWER_SAVE_DIS;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, wr, 1);
			break;
		case 5:
			//MREG1 - APEX_CONFIG9 = write
			//APEX_CONFIG9
			wr[0] = ICM42670P_MREG1_APEX_CONFIG9_FF_DEBOUNCE_DURATION_2000_MS | \
					ICM42670P_MREG1_APEX_CONFIG9_SMD_SENSITIVITY_0 | \
					ICM42670P_MREG1_APEX_CONFIG9_SENSITIVITY_MODE_NORMAL;
			//APEX_CONFIG10
			wr[1] = ICM42670P_MREG1_APEX_CONFIG10_LOWG_PEAK_TH_563_MG | \
					ICM42670P_MREG1_APEX_CONFIG10_LOWG_TIME_TH_1_SAMPLE;
			//APEX_CONFIG11
			wr[2] = ICM42670P_MREG1_APEX_CONFIG11_HIGHG_PEAK_TH_2500_MG	|\
					ICM42670P_MREG1_APEX_CONFIG11_HIGHG_TIME_TH_1_SAMPLE;

			if(e_IMU_Write_MREG(&mreg, u8_addr, ICM42670P_MREG1_REG_APEX_CONFIG9, wr, 3) == IMU_RET_DONE){
				retry = true;
				mreg = true;
			}
			else{
				order--;
			}
			break;
		case 6:
			//MREG1 - APEX_CONFIG12	- write
			//APEX_CONFIG12
			wr[0] = ICM42670P_MREG1_APEX_CONFIG12_FF_MAX_DURATION_204_CM | \
					ICM42670P_MREG1_APEX_CONFIG12_FF_MIN_DURATION_10_CM;

			if(e_IMU_Write_MREG(&mreg, u8_addr, ICM42670P_MREG1_REG_APEX_CONFIG12, wr, 1) == IMU_RET_DONE){
				retry = true;
				mreg = true;
			}
			else{
				order--;
			}
			break;
		case 7:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}


e_IMU_RET_t e_IMU_Reset_DMP(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	static bool mclk;
	if(*pb_init){
		*pb_init = false;
		order = 0;
		mclk = true;
	}
	uint8_t wr[8];
	bool retry;

	//APEX_CONFIG0
	do{
		retry = false;
		switch(order){
		case 0:
			if(e_IMU_MCLK_On(&mclk, u8_addr) == IMU_RET_DONE){
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 1:
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, 1);
			break;
		case 2:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_MEM_RESET_EN;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG0_DMP_MEM_RESET_APEX_ST_EN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, wr, 1);
			break;
		case 3:
			delay_us(1000);
			order++;
		case 4:
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, 1);
			break;
		case 5:
			if((u8_rd[0] & ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_MEM_RESET_EN) == ICM42670P_BANK0_APEX_CONFIG0_DMP_MEM_RESET_DIS){
				retry = true;
				mclk = true;
			}
			else{
				order -= 2;
				delay_us(10);
			}
			break;
		case 6:
			if(e_IMU_MCLK_Off(&mclk, u8_addr) == IMU_RET_DONE){
				return IMU_RET_DONE;
			}
			else{
				order--;
			}
			break;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Resume_DMP(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	uint8_t wr[8];
	bool retry;

	//APEX_CONFIG0
	do{
		retry = false;
		switch(order){
		case 0:
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, 1);
			break;
		case 1:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_INIT_EN;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG0_DMP_INIT_EN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, wr, 1);
			break;
		case 2:
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG0, 1);
			break;
		case 3:
			if((u8_rd[0] & ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_INIT_EN) == ICM42670P_BANK0_APEX_CONFIG0_DMP_INIT_DIS){

			}
			else{
				order -= 2;
				delay_us(100);
			}
			retry = true;
			break;
		case 4:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}


e_IMU_RET_t e_IMU_Disable_Pedometer(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;
	uint8_t wr[8] = {0,};
	do{
		retry = false;
		switch(order){
		case 0:
			//read
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, 1);
			break;
		case 1:
			//disable
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG1_PED_ENABLE;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG1_PED_ENABLE_DIS;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, wr, 1);
			break;
		case 2:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Enable_Pedometer(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;
	uint8_t wr[8] = {0,};
	do{
		retry = false;
		switch(order){
		case 0:
			//enable pedometer
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, 1);
			break;
		case 1:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG1_PED_ENABLE;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG1_PED_ENABLE_EN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, wr, 1);
			break;
		case 2:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}



e_IMU_RET_t e_IMU_Enable_FreeFall(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	static bool init;
	if(*pb_init){
		*pb_init = false;
		order = 0;
		init = true;
	}
	bool retry;
	uint8_t wr[8] = {0,};
	do{
		retry = false;
		switch(order){
		case 0:
			if(e_IMU_Reset_DMP(&init, u8_addr) == IMU_RET_DONE){
				init = true;
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 1:
			if(e_IMU_Resume_DMP(&init, u8_addr) == IMU_RET_DONE){
				init = true;
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 2:
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, 1);
			break;
		case 3:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG1_FF_ENABLE;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG1_FF_ENABLE_EN;
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, wr, 1);
			break;
		case 4:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}



e_IMU_RET_t e_IMU_Config_EVT(bool* pb_init, uint8_t u8_addr){
	static uint16_t order;
	static bool init;
	if(*pb_init){
		*pb_init = false;
		order = 0;
		init = true;
	}
	bool retry;
	uint8_t wr[8] = {0,};
	do{
		retry = false;
		switch(order){
		case 0:
			if(e_IMU_Disable_Pedometer(&init, u8_addr) == IMU_RET_DONE){
				init = true;
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 1:
			//odr	: accel odr 100 Hz
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, 1);
			break;
		case 2:
			wr[0] = u8_rd[0];
			wr[0] &= ~ICM42670P_BANK0_MASK_APEX_CONFIG1_DMP_ODR;
			wr[0] |= ICM42670P_BANK0_APEX_CONFIG1_DMP_ODR_100Hz;	//same with accel odr
			i_IMU_Write(u8_addr, ICM42670P_BANK0_REG_APEX_CONFIG1, wr, 1);
			break;
		case 3:
			if(e_IMU_Config_Parameter(&init, u8_addr) == IMU_RET_DONE){
				init = true;
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 4:
			if(e_IMU_Enable_FreeFall(&init, u8_addr) == IMU_RET_DONE){
				init = true;
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 5:
			if(e_IMU_Enable_Pedometer(&init, u8_addr) == IMU_RET_DONE){
				init = true;
				retry = true;
			}
			else{
				order--;
			}
			break;
		case 6:
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}



uint32_t imu_tout;
e_IMU_RET_t e_IMU_Read_FromREG(bool* pb_init, uint8_t u8_addr, uint8_t* pu8_dest, bool* pb_evt){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;
	do{
		retry = false;
		switch(order){
		case 0:
			//INT_STATUS
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS_DRDY, 1);
			break;
		case 1:
			if(*pb_evt && u8_rd[0] & ICM42670P_BANK0_MASK_INT_STATUS_DRDY_DATA_RDY_INT){
				*pb_evt = false;
				i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_DATA_X1, 12);
			}
			else{
				if(*pb_evt){
					*pb_evt = false;
					order -= 2;
					retry = true;
				}
				else{
					order -= 2;
					imu_tout++;
					retry = true;
				}
			}
			break;
		case 2:
			if(*pb_evt){
				*pb_evt = false;
				memcpy(pu8_dest, u8_rd, 12);
				return IMU_RET_DONE;
			}
			else{
				order -=2;
				imu_tout++;
				break;
			}
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}


e_IMU_RET_t e_IMU_Read_FromREG_T(bool* pb_init, uint8_t u8_addr, bool* pb_evt){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;
	do{
		retry = false;
		switch(order){
		case 0:
			//INT_STATUS
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS_DRDY, 1);
			break;
		case 1:
			if(*pb_evt && u8_rd[0] & ICM42670P_BANK0_MASK_INT_STATUS_DRDY_DATA_RDY_INT){
				*pb_evt = false;
				i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_DATA_X1, 12);
			}
			else{
				if(*pb_evt){
					*pb_evt = false;
					order -= 2;
					retry = true;
				}
				else{
					order -= 2;
					imu_tout++;
					retry = true;
				}
			}
			break;
		case 2:
			if(*pb_evt){
				*pb_evt = false;
				return IMU_RET_DONE;
			}
			else{
				order -=2;
				imu_tout++;
				break;
			}
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

e_IMU_RET_t e_IMU_Read_DATA_EVT(bool* pb_init, uint8_t u8_addr, uint8_t* pu8_data, uint8_t* pu8_rd, uint8_t* pu8_evt){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;
	do{
		retry = false;
		switch(order){
		case 0:
			//INT_STATUS
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS_DRDY, 4);
			order++;
			break;
		case 1:
			if(pu8_rd[3] & ICM42670P_BANK0_MASK_INT_STATUS3_FF_DET_INT)			{*pu8_evt |= 1;}
			if(pu8_rd[3] & ICM42670P_BANK0_MASK_INT_STATUS3_LOWG_DET_INT)		{*pu8_evt |= 2;}
			if(pu8_rd[3] & ICM42670P_BANK0_MASK_INT_STATUS3_STEP_DET_INT)		{*pu8_evt |= 4;}
			if(pu8_rd[3] & ICM42670P_BANK0_MASK_INT_STATUS3_STEP_CNT_OVF_INT)	{*pu8_evt |= 8;}
			//data ready
			if(pu8_rd[0] & ICM42670P_BANK0_MASK_INT_STATUS_DRDY_DATA_RDY_INT){
				i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_DATA_X1, 12);
				order++;
			}
			else{
				order--;
			}
			break;
		case 2:
			memcpy(pu8_data, pu8_rd, 12);
			return IMU_RET_DONE;
		default:
			return IMU_RET_ERR;
		}
	}while(retry);
	return IMU_RET_OK;
}


e_IMU_RET_t e_IMU_Read_FromFIFO(bool* pb_init, uint8_t u8_addr, uint8_t* pu8_dest, bool* pb_evt){
	static uint16_t order;
	if(*pb_init){
		*pb_init = false;
		order = 0;
	}
	bool retry;

	do{
		retry = false;
		switch(order){
		case 0:
			//INT_STATUS
			i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS_DRDY, 1);
			break;
		case 1:
			if(*pb_evt && (u8_rd[0] & ICM42670P_BANK0_MASK_INT_STATUS_DRDY_DATA_RDY_INT)){
				*pb_evt = false;
				i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_FIFO_DATA, 16);
			}
			else{
				if(*pb_evt){
					*pb_evt = false;
					order -= 2;
					retry = true;
				}
				else{
					order -= 2;
					imu_tout++;
					retry = true;
				}
			}
			break;
		case 2:
			if(*pb_evt){
				*pb_evt = false;
				memcpy(pu8_dest, u8_rd, 12);
				return IMU_RET_DONE;
			}
			else{
				order -=2;
				imu_tout++;
				break;
			}
		default:
			return IMU_RET_ERR;
		}
		order++;
	}while(retry);
	return IMU_RET_OK;
}

void v_IMU_Decode_REG(int16_t* pi16_dest, uint8_t* pu8_src){
	for(uint16_t i=0; i<12; i++){
		if(i & 1){
			pi16_dest[i>>1] |= pu8_src[i] & 0x00FF;
		}
		else{
			pi16_dest[i>>1] = (pu8_src[i] << 8) & 0xFF00;
		}
	}
}

void v_IMU_Decode_FIFO(IMU_FIFO_t* px_dest, uint8_t* pu8_src){
	uint16_t cnt=0;
	//HEADER
	px_dest->header = pu8_src[cnt++];
	//ACCEL
	px_dest->acc_x = (pu8_src[cnt++] << 8) & 0xFF00;
	px_dest->acc_x |= pu8_src[cnt++] & 0x00FF;
	px_dest->acc_y = (pu8_src[cnt++] << 8) & 0xFF00;
	px_dest->acc_y |= pu8_src[cnt++] & 0x00FF;
	px_dest->acc_z = (pu8_src[cnt++] << 8) & 0xFF00;
	px_dest->acc_z |= pu8_src[cnt++] & 0x00FF;
	//GYRO
	px_dest->gyro_x = (pu8_src[cnt++] << 8) & 0xFF00;
	px_dest->gyro_x |= pu8_src[cnt++] & 0x00FF;
	px_dest->gyro_y = (pu8_src[cnt++] << 8) & 0xFF00;
	px_dest->gyro_y |= pu8_src[cnt++] & 0x00FF;
	px_dest->gyro_z = (pu8_src[cnt++] << 8) & 0xFF00;
	px_dest->gyro_z |= pu8_src[cnt++] & 0x00FF;
	//TEMP
	px_dest->temp = pu8_src[cnt++] & 0x00FF;
	px_dest->temp |= (pu8_src[cnt++] << 8) & 0xFF00;
	//TIME
	px_dest->time = pu8_src[cnt];
}




//////////////////////////
//		IMU HANDLER		//
//////////////////////////

void v_IMU_Init(){
	b_imu_evt = true;

	__HAL_I2C_ENABLE_IT(p_i2c, I2C_IT_ERRI);
}


uint8_t imu_data_rdy[4];
uint8_t imu_int_source0;
bool b_IMU_Config(bool* pb_init, uint8_t u8_addr, uint8_t* pu8_id){
	static uint16_t order;
	static bool config;
	if(*pb_init){
		*pb_init = 0;
		config = true;
		order = 0;
	}

	if(u8_addr == ADDR_IMU_LEFT){
		if(e_imu_evt_L == COMM_STAT_BUSY){return false;}
	}
	else{
		if(e_imu_evt_R == COMM_STAT_BUSY){return false;}
	}
	//do{
		switch(order){
		case 0:
			//RESET
			if(e_IMU_Reset(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
		case 1:
			//INTERFACE
			if(e_IMU_Config_INTF(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
		case 2:
			//GET ID
			if(e_IMU_Get_ID(&config, u8_addr, pu8_id) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
		case 3:
			//CONFIG INT	// add pedometer + step
			if(e_IMU_Config_INT(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
		case 4:
			//CONFIG ACC
			if(e_IMU_Config_ACC(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
		case 5:
			//CONFIG GYRO
			if(e_IMU_Config_GYRO(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
#if IMU_CONFIG_FIFO
		case 6:
			if(e_IMU_Config_FIFO(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
#endif
#if IMU_CONFIG_FIFO
		case 7:
#else
		case 6:
#endif
			//CONFIG PWR MODE
			if(e_IMU_Config_PWR_Mode(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
#if IMU_CONFIG_FIFO
		case 8:
#else
		case 7:
#endif
			if(e_IMU_Config_EVT(&config, u8_addr) == IMU_RET_DONE){
				config = true;
				order++;
			}
			break;
		case 8:
			if(i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS_DRDY, 4) == COMM_STAT_OK){
				order++;
			}
			break;
#if IMU_CONFIG_FIFO
		case 10:
#else
		case 9:
#endif
			memcpy(imu_data_rdy, u8_rd, 4);
			order++;
			break;
		case 10:
			if(i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_SOURCE0, 1) == COMM_STAT_OK){
				order++;
			}
			break;
		case 11:
			imu_int_source0 = u8_rd[0];
			return true;
		default:
			break;
		}
	//}while(*pb_init);
	return false;
}


void v_IMU_Handler_T(){
	static uint32_t timRef;
	static bool config;
	static bool init;
	static uint16_t toggle;

	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 100)){
		timRef = u32_Tim_1msGet();
		if(config == false){
			if(toggle & 1){
				//RIGHT
				if(b_IMU_Config(&init, ADDR_IMU_RIGHT, &imu_id_right) == true){
					toggle++;
					config = true;
				}
			}
			else{
				//LEFT
				if(b_IMU_Config(&init, ADDR_IMU_LEFT, &imu_id_left) == true){
					toggle++;
					//temporary
					b_imu_evt = true;
				}
			}
		}
		else{
#if IMU_CONFIG_FIFO
			if(toggle & 1){
				//RIGHT
				if(e_IMU_Read_FromFIFO(&init, ADDR_IMU_RIGHT, imu_arr, &b_imu_evt) == true){
					toggle++;
					init = true;
					v_IMU_Decode_FIFO(&imu_fifo_right, imu_arr);
				}
			}

			else{
				//LEFT
				if(e_IMU_Read_FromFIFO(&init, ADDR_IMU_LEFT, imu_arr, &b_imu_evt) == true){
					toggle++;
					init = true;
					v_IMU_Decode_FIFO(&imu_fifo_left, imu_arr);
				}
			}
#else
			if(toggle & 1){
				//RIGHT
				if(e_IMU_Read_FromREG(&init, ADDR_IMU_RIGHT, imu_arr, &b_imu_evt) == true){
					toggle++;
					init = true;
					v_IMU_Decode_REG(imu_right, imu_arr);
				}
			}
			else{
				//LEFT
				if(e_IMU_Read_FromREG(&init, ADDR_IMU_LEFT, imu_arr, &b_imu_evt) == true){
					toggle++;
					init = true;
					v_IMU_Decode_REG(imu_left, imu_arr);
				}
			}
#endif
		}
	}
}


static int i_tilt_center;
static int i_tilt_init_L, i_tilt_init_R;
void v_IMU_Tilt_Center_Enable(){
	i_tilt_center = 1;
	i_tilt_init_L = i_tilt_init_R = 1;
}

void v_IMU_Tilt_Center_Disable(){
	i_tilt_center = 0;
}

int i_IMU_Tilt_Is_Center(){
	return i_tilt_center;
}


float tilt_centerAngleX_L, tilt_centerAngleY_L, tilt_centerAngleZ_L;
float tilt_centerAngleX_R, tilt_centerAngleY_R, tilt_centerAngleZ_R;

float f_IMU_Get_Tilt_X_Center_L(){
	return tilt_centerAngleX_L;
}

float f_IMU_Get_Tilt_Y_Center_L(){
	return tilt_centerAngleY_L;
}

float f_IMU_Get_Tilt_Z_Center_L(){
	return tilt_centerAngleZ_L;
}

float f_IMU_Get_Tilt_X_Center_R(){
	return tilt_centerAngleX_R;
}

float f_IMU_Get_Tilt_Y_Center_R(){
	return tilt_centerAngleY_R;
}

float f_IMU_Get_Tilt_Z_Center_R(){
	return tilt_centerAngleZ_R;
}

#define IMU_TILT_SAMPLE_CNT	10

void v_IMU_Deinit(){
	e_imu_config = COMM_STAT_READY;
	v_IMU_Tilt_Center_Disable();
}

e_COMM_STAT_t e_IMU_Ready(){
	static uint32_t timRef, timItv;
	static bool init = true;
	static uint16_t mask;
	static int interface;
	if(e_imu_config == COMM_STAT_READY){
		if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
			timRef = u32_Tim_1msGet();
			if(mask == 0){
				if(!(interface & 0x01)){
					if(e_imu_evt_L != COMM_STAT_BUSY){
						if(e_IMU_Config_INTF(&init, ADDR_IMU_LEFT) == IMU_RET_DONE){
							init = true;
							interface |= 0x01;
						}
					}
				}
				else if(!(interface & 0x02)){
					if(e_imu_evt_R != COMM_STAT_BUSY){
						if(e_IMU_Config_INTF(&init, ADDR_IMU_RIGHT) == IMU_RET_DONE){
							init = true;
							interface |= 0x02;
						}
					}
				}
				else{
					mask = 1;
				}
			}
			//LEFT
			if(mask == 1){
				if(b_IMU_Config(&init, ADDR_IMU_LEFT, &imu_id_left) == true){
					b_imu_evt = true;
					init = true;
					mask = 2;
				}
				else{
					timItv = 10;
					return false;
				}
			}
			//RIGHT
			if(mask == 2){
				if(b_IMU_Config(&init, ADDR_IMU_RIGHT, &imu_id_right) == true){
					mask = 0;
					e_imu_config = COMM_STAT_DONE;
				}
				else{
					timItv = 10;
				}
			}
		}
	}
	return e_imu_config;
}


typedef enum {
	IMU_DATA_IDLE=0,
	IMU_DATA_WAIT,
	IMU_DATA_DONE,
} e_IMU_DATA_STAT_t;
uint16_t imu_handler_tout;

static x_QUAT_t orientation_left = {1.0f, 0.0f, 0.0f, 0.0f}, orientation_right = {1.0f, 0.0f, 0.0f, 0.0f};
static _x_XYZ_t	integral_error_left, integral_error_right;
static _x_XYZ_t angle_left, angle_right;
static _x_XYZ_t angle_center_left, angle_center_right;
static _x_XYZ_t angle_offset_left, angle_offset_right;
static x_IMU_TILT_AVG_t angle_avg_left, angle_avg_right;


static x_QUAT_t init_left, init_right;


void v_IMU_Handler(){
	//v_IMU_Int_Handler();

	static uint32_t timRef, timItv;
	static uint16_t mask;
	static bool init_L, init_R;

	if(e_imu_config != COMM_STAT_DONE){return;}
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
		if(mask == 0x00){
			mask = 0x80;
			timRef = u32_Tim_1msGet();
			timItv = 0;
			init_L = init_R = true;
		}

		//LEFT
		if(!(mask & 0x01) && e_imu_evt_L != COMM_STAT_BUSY){
			if(e_IMU_Read_DATA_EVT(&init_L, ADDR_IMU_LEFT, u8_arr_left, u8_rd_L, &imu_evt_left) == IMU_RET_DONE){
				mask |= 0x01;

				v_IMU_Decode_REG(imu_left, u8_arr_left);
				//tilt compute
				v_IMU_Tilt_Compute_Orientation(&orientation_left, &integral_error_left, imu_left);
				x_QUAT_t inv = Quaternion_Multiply(init_left, orientation_left);
				v_IMU_Tilt_Compute_Angle(&angle_left, &inv);
				if(i_tilt_center){
					init_left = Quaternion_Inverse(orientation_left);
				}
			}
			else{
				timItv += 5;
			}
		}
		//RIGHT
		if(mask & 0x01 && !(mask & 0x02) && e_imu_evt_R != COMM_STAT_BUSY){
			if(e_IMU_Read_DATA_EVT(&init_R, ADDR_IMU_RIGHT, u8_arr_right, u8_rd_R, &imu_evt_right) == IMU_RET_DONE){
				mask |= 0x02;

				v_IMU_Decode_REG(imu_right, u8_arr_right);
				//tilt compute
				v_IMU_Tilt_Compute_Orientation(&orientation_right, &integral_error_right, imu_right);
				x_QUAT_t inv = Quaternion_Multiply(init_right, orientation_right);
				v_IMU_Tilt_Compute_Angle(&angle_right, &inv);

				if(i_tilt_center){
					init_right = Quaternion_Inverse(orientation_right);
				}
			}
			else{
				timItv += 5;
			}
		}
		if(mask == 0x83){
			timItv = 100;
			mask = 0x00;
		}
	}

}




int16_t* pi16_IMU_Get_Left(){
	return imu_left;
}

int16_t* pi16_IMU_Get_Right(){
	return imu_right;
}

uint8_t u8_IMU_Get_EVT_Left(){
	return imu_evt_left;
}

uint8_t u8_IMU_Get_EVT_Right(){
	return imu_evt_right;
}

void v_IMU_Clear_EVT_Left(){
	imu_evt_left = 0;
}

void v_IMU_Clear_EVT_Right(){
	imu_evt_right = 0;
}

// angle_offset_left, angle_offset_right;
_x_XYZ_t x_IMU_Get_Offset_Left(){
	return angle_offset_left;
}

_x_XYZ_t x_IMU_Get_Offset_Right(){
	return angle_offset_right;
}

_x_XYZ_t x_IMU_Get_Angle_Left(){
	return angle_left;
}

_x_XYZ_t x_IMU_Get_Angle_Right(){
	return angle_right;
}

//////////////////////
//		TILT		//
//////////////////////
static const float kP	= 6.0f;		//proportional gain
static const float kI	= 0.05f;	//integral gain
//static const float kD	= 0.0f;		//derivative gain

const static float f_gyro_sensitivity = 2000.0f / 32768.0f;
const static float f_accel_sensitivity = 2.0f / 32768.0f;
const static float f_1degree = M_PI / 180.f;

//tilt 	now 	x, y, z
//tilt	center	x, y, z
//orientation
//integral error
//static _x_XYZ_t



static void v_IMU_Tilt_Compute_Orientation(x_QUAT_t* p_orientation, _x_XYZ_t* p_intgral_error, int16_t* pi16_imu){
	_x_XYZ_t acc, gyro;
	int cnt=0;

	for(int i=0; i<6; i++){
		if(pi16_imu[i] == 0){cnt++;}
	}
	if(cnt != 6){
		acc.x = pi16_imu[0] * f_accel_sensitivity;
		acc.y = pi16_imu[1] * f_accel_sensitivity;
		acc.z = pi16_imu[2] * f_accel_sensitivity;

		gyro.x = pi16_imu[3] * f_gyro_sensitivity * f_1degree;
		gyro.y = pi16_imu[4] * f_gyro_sensitivity * f_1degree;
		gyro.z = pi16_imu[5] * f_gyro_sensitivity * f_1degree;

		v_Quaternion_Mahony_Compute(p_intgral_error, p_orientation, acc, gyro, 0.1, kP, kI);
	}
}

static void v_IMU_Tilt_Compute_Angle(_x_XYZ_t* p_angle, x_QUAT_t* p_orientation){
	p_angle->x = f_Quaternion_TiltAngleX_Compute(*p_orientation) * 1.0f;
	p_angle->y = f_Quaternion_TiltAngleY_Compute(*p_orientation) * -1.0f;
	p_angle->z = f_Quaternion_TiltAngleZ_Compute(*p_orientation) * 1.0f;
}

static void v_IMU_Tilt_Compute_Offset(_x_XYZ_t* p_offset, _x_XYZ_t* p_center, _x_XYZ_t* p_now){
#if 0
	p_offset->x = p_now->x - p_center->x;
	p_offset->y = p_now->y - p_center->y;
	p_offset->z = p_now->z - p_center->z;
#endif
	p_offset->x = p_now->x - p_center->x;
	p_offset->y = p_now->y - p_center->y;
	p_offset->z = p_now->z - p_center->z;
}

static void v_IMU_Tilt_Compute_AVG(x_IMU_TILT_AVG_t* p_avg, _x_XYZ_t* p_angle){
	float angle[3] = {
		p_angle->x,
		p_angle->y,
		p_angle->z,
	};

	if(p_avg->u16_cnt < IMU_TILT_SAMPLING_CNT){p_avg->u16_cnt++;}
	for(int i=0; i<3; i++){
		//last
		p_avg->f_last[i] = p_avg->f_buf[i][p_avg->u16_in];
		//update buffer
		p_avg->f_buf[i][p_avg->u16_in] = angle[i];
		//sum
		p_avg->f_sum[i] += angle[i];
		p_avg->f_sum[i] -= p_avg->f_last[i];
		//avg
		p_avg->f_avg[i] = p_avg->f_sum[i] / p_avg->u16_cnt;
	}
	if(p_avg->u16_in < (IMU_TILT_SAMPLING_CNT - 1))	{p_avg->u16_in++;}
	else											{p_avg->u16_in = 0;}
}






e_COMM_STAT_t e_IMU_Read_Data(int* pi_int, uint8_t u8_addr, uint8_t* pu8_data, uint8_t* pu8_evt){
	static int order;
	if(*pi_int && order != 1){
		order = 0;
	}
	switch(order){
	case 0:
		if(i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_INT_STATUS_DRDY, 4) == COMM_STAT_OK){order++;}
		break;
	case 1:
		if(u8_imuRX[3] & ICM42670P_BANK0_MASK_INT_STATUS3_FF_DET_INT)		{*pu8_evt |= 1;}
		if(u8_imuRX[3] & ICM42670P_BANK0_MASK_INT_STATUS3_LOWG_DET_INT)		{*pu8_evt |= 2;}
		if(u8_imuRX[3] & ICM42670P_BANK0_MASK_INT_STATUS3_STEP_DET_INT)		{*pu8_evt |= 4;}
		if(u8_imuRX[3] & ICM42670P_BANK0_MASK_INT_STATUS3_STEP_CNT_OVF_INT)	{*pu8_evt |= 8;}
		if(u8_imuRX[0] & ICM42670P_BANK0_MASK_INT_STATUS_DRDY_DATA_RDY_INT){
			*pi_int = 0;
			order++;
		}
		else{
			order--;
		}
		break;
	case 2:
		if(i_IMU_Read(u8_addr, ICM42670P_BANK0_REG_ACCEL_DATA_X1, 12) == COMM_STAT_OK){order++;}
		break;
	case 3:
		memcpy(pu8_data, u8_imuRX, 12);
		order++;
		break;
	default:
		return COMM_STAT_DONE;
	}
	return COMM_STAT_OK;
}

uint32_t imu_int_cnt_L, imu_int_cnt_R;
static void v_IMU_Int_Handler(){

	static int int_L, int_R;
	static int mask;

	if(e_imu_config != COMM_STAT_DONE){return;}
	//active low
	//Left
	if(!CHK_BIT(DI_IMUL_INT_GPIO_Port->IDR, DI_IMUL_INT_Pin)){
		//read event + data
		int_L = 1;
		mask = 1;
	}
	//Right
	if(!CHK_BIT(DI_IMUR_INT_GPIO_Port->IDR, DI_IMUR_INT_Pin)){
		//read event + data
		int_R = 1;
	}

	if(mask == 1 && e_imu_evt_L != COMM_STAT_BUSY && e_IMU_Read_Data(&int_L, ADDR_IMU_LEFT, u8_arr_left, &imu_evt_left) == COMM_STAT_DONE){
		mask = 2;
		v_IMU_Decode_REG(imu_left, u8_arr_left);
		//tilt compute
		v_IMU_Tilt_Compute_Orientation(&orientation_left, &integral_error_left, imu_left);
		v_IMU_Tilt_Compute_Angle(&angle_left, &orientation_left);
		v_IMU_Tilt_Compute_Offset(&angle_offset_left, &angle_center_left, &angle_left);

		//v_IMU_Tilt_Compute_L();
		if(i_tilt_center){
			//v_IMU_Tilt_Cetner_Handler_L(&i_tilt_init_L);
			v_IMU_Tilt_Compute_AVG(&angle_avg_left, &angle_left);
			angle_center_left.x = angle_avg_left.f_avg[0];
			angle_center_left.y = angle_avg_left.f_avg[1];
			angle_center_left.z = angle_avg_left.f_avg[2];
		}

		imu_int_cnt_L++;
	}
	if(mask == 2 && e_imu_evt_R != COMM_STAT_BUSY && e_IMU_Read_Data(&int_R, ADDR_IMU_RIGHT, u8_arr_right, &imu_evt_right) == COMM_STAT_DONE){
		mask = 3;
		v_IMU_Decode_REG(imu_right, u8_arr_right);
		//tilt compute
		v_IMU_Tilt_Compute_Orientation(&orientation_right, &integral_error_right, imu_right);
		v_IMU_Tilt_Compute_Angle(&angle_right, &orientation_right);
		v_IMU_Tilt_Compute_Offset(&angle_offset_right, &angle_center_right, &angle_right);

		//v_IMU_Tilt_Compute_R();
		if(i_tilt_center){
			//v_IMU_Tilt_Cetner_Handler_R(&i_tilt_init_R);
			v_IMU_Tilt_Compute_AVG(&angle_avg_right, &angle_right);
			angle_center_right.x = angle_avg_right.f_avg[0];
			angle_center_right.y = angle_avg_right.f_avg[1];
			angle_center_right.z = angle_avg_right.f_avg[2];
		}

		imu_int_cnt_R++;
	}
}










