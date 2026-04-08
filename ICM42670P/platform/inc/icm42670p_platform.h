#ifndef __JH_ICM42670P_PLATFORM_H
#define __JH_ICM42670P_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "icm42670p_driver.h"
#include "quaternion_mahony.h"

#define IMU_CONFIG_FIFO	0



void v_ICM42670P_Test();
void IMU_Init();



void v_IMU_RD_Done(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_cnt);
void v_IMU_WR_Done(uint8_t u8_addr);

void v_IMU_Tout_Handler();
void v_IMU_Reset_RetryCnt(void);

void v_IMU_Tilt_Center_Enable();
void v_IMU_Tilt_Center_Disable();
int i_IMU_Tilt_Is_Center();


void v_IMU_Init();
void v_IMU_Deinit();
e_COMM_STAT_t e_IMU_Ready();
int i_IMU_Is_Available();
void v_IMU_Handler();

void v_IMU_Handler_T();


int16_t* pi16_IMU_Get_Left();
int16_t* pi16_IMU_Get_Right();

uint8_t u8_IMU_Get_EVT_Left();
uint8_t u8_IMU_Get_EVT_Right();
void v_IMU_Clear_EVT_Left();
void v_IMU_Clear_EVT_Right();

// Drift-free accel-only tilt — the sole attitude source for trigger and ESP.
// Self-anchored at boot via a brief LPF warmup, so getters return ~0 once ready.
float f_IMU_Get_AccelTilt_X_Left(void);
float f_IMU_Get_AccelTilt_Y_Left(void);
float f_IMU_Get_AccelTilt_X_Right(void);
float f_IMU_Get_AccelTilt_Y_Right(void);


#ifdef __cplusplus
}
#endif

#endif

