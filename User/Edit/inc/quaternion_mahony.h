#ifndef __JH_QUATERNION_MAHONY_H
#define __JH_QUATERNION_MAHONY_H

#include "lib_def.h"

typedef struct {
	float x;
	float y;
	float z;
} _x_XYZ_t;

typedef struct {
	float w;
	float x;
	float y;
	float z;
} x_QUAT_t;

x_QUAT_t x_Quaternion_Mahony_Compute(_x_XYZ_t* intgral_error, x_QUAT_t prev, _x_XYZ_t acc, _x_XYZ_t gyro, float dt, float kP, float kI);
void v_Quaternion_Mahony_Compute(_x_XYZ_t* intgral_error, x_QUAT_t* prev, _x_XYZ_t acc, _x_XYZ_t gyro, float dt, float kP, float kI);

float f_Quaternion_TiltAngleX_Compute(x_QUAT_t q);
float f_Quaternion_TiltAngleY_Compute(x_QUAT_t q);
float f_Quaternion_TiltAngleZ_Compute(x_QUAT_t q);


void v_IMU_Tilt_Compute_L();
void v_IMU_Tilt_Compute_R();
void v_Mahony_Test();

float f_IMU_Get_Tilt_X_L();
float f_IMU_Get_Tilt_Y_L();
float f_IMU_Get_Tilt_Z_L();

float f_IMU_Get_Tilt_X_R();
float f_IMU_Get_Tilt_Y_R();
float f_IMU_Get_Tilt_Z_R();

float f_IMU_Get_Tilt_X_Offset_L();
float f_IMU_Get_Tilt_Y_Offset_L();
float f_IMU_Get_Tilt_Z_Offset_L();

float f_IMU_Get_Tilt_X_Offset_R();
float f_IMU_Get_Tilt_Y_Offset_R();
float f_IMU_Get_Tilt_Z_Offset_R();


x_QUAT_t Quaternion_Inverse(x_QUAT_t q);
x_QUAT_t Quaternion_Multiply(x_QUAT_t q1, x_QUAT_t q2);

#endif



