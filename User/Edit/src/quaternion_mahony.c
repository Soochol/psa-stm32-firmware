#include "main.h"
#include "quaternion_mahony.h"
#include "math.h"
#include "icm42670p_platform.h"

//static const float kP	= 6.0f;		//proportional gain
//static const float kI	= 0.05f;	//integral gain
//static const float kD	= 0.0f;		//derivative gain

static const float kRadToDeg = 180.0f / 3.14159265358979323846f;    // rad to deg
static const float kDegToRad = 3.14159265358979323846f / 180.0f;    // deg to rad

float x_angle_offset = 0.0f;
float y_angle_offset = 0.0f;
float z_angle_offset = 0.0f;



x_QUAT_t x_Quaternion_Mahony_Compute(_x_XYZ_t* intgral_error, x_QUAT_t prev, _x_XYZ_t acc, _x_XYZ_t gyro, float dt, float kP, float kI){
	// 1) Normalize accelerometer measurement
	float acc_norm = sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
	if(acc_norm < 1e-6f){return prev;}

	_x_XYZ_t acc_n = {
		.x = acc.x / acc_norm,
		.y = acc.y / acc_norm,
		.z = acc.z / acc_norm,
	};

	// 2) Estimated gravity direction from quaternion
	float vx = 2.0f * (prev.x * prev.z - prev.w * prev.y);
	float vy = 2.0f * (prev.w * prev.x + prev.y * prev.z);
	float vz = prev.w * prev.w - prev.x * prev.x - prev.y * prev.y + prev.z * prev.z;

	// 3) Error is cross product between estimated and measured gravity
	_x_XYZ_t error = {
		.x = acc_n.y * vz - acc_n.z * vy,
		.y = acc_n.z * vx - acc_n.x * vz,
		.z = acc_n.x * vy - acc_n.y * vx,
	};

	// 4) Integral feedback
	intgral_error->x += kI * error.x * dt;
	intgral_error->y += kI * error.y * dt;
	intgral_error->z += kI * error.z * dt;

	// 5) Convert gyro to rad/s
	_x_XYZ_t gyro_rad = {
		.x = gyro.x * kDegToRad,
		.y = gyro.y * kDegToRad,
		.z = gyro.z * kDegToRad,
	};

	// 6) Apply feedback terms
	_x_XYZ_t omega = {
		.x = gyro_rad.x + kP * error.x + intgral_error->x,
		.y = gyro_rad.y + kP * error.y + intgral_error->y,
		.z = gyro_rad.z + kP * error.z + intgral_error->z,
	};

	// 7) Compute quaternion derivative
	x_QUAT_t dot = {
		.w = 0.5 * (-prev.x * omega.x - prev.y * omega.y - prev.z * omega.z),
		.x = 0.5 * ( prev.w * omega.x + prev.y * omega.z - prev.z * omega.y),
		.y = 0.5 * ( prev.w * omega.y - prev.x * omega.z + prev.z * omega.x),
		.z = 0.5 * ( prev.w * omega.z + prev.x * omega.y - prev.y * omega.x),
	};

	// 8) Integrate to yield new quaternion
	x_QUAT_t new = {
		.w = prev.w + dot.w * dt,
		.x = prev.x + dot.x * dt,
		.y = prev.y + dot.y * dt,
		.z = prev.z + dot.z * dt,
	};

	// 9) Normalize quaternion
	float norm = sqrt(new.w * new.w + new.x * new.x + new.y * new.y + new.z * new.z);

	new.w /= norm;
	new.x /= norm;
	new.y /= norm;
	new.z /= norm;

	return new;
}


void v_Quaternion_Mahony_Compute(_x_XYZ_t* intgral_error, x_QUAT_t* prev, _x_XYZ_t acc, _x_XYZ_t gyro, float dt, float kP, float kI){
	// 1) Normalize accelerometer measurement
	float acc_norm = sqrt(acc.x * acc.x + acc.y * acc.y + acc.z * acc.z);
	if(acc_norm < 1e-6f){;}

	_x_XYZ_t acc_n = {
		.x = acc.x / acc_norm,
		.y = acc.y / acc_norm,
		.z = acc.z / acc_norm,
	};

	// 2) Estimated gravity direction from quaternion
	float vx = 2.0f * (prev->x * prev->z - prev->w * prev->y);
	float vy = 2.0f * (prev->w * prev->x + prev->y * prev->z);
	float vz = prev->w * prev->w - prev->x * prev->x - prev->y * prev->y + prev->z * prev->z;

	// 3) Error is cross product between estimated and measured gravity
	_x_XYZ_t error = {
		.x = acc_n.y * vz - acc_n.z * vy,
		.y = acc_n.z * vx - acc_n.x * vz,
		.z = acc_n.x * vy - acc_n.y * vx,
	};

	// 4) Integral feedback
	intgral_error->x += kI * error.x * dt;
	intgral_error->y += kI * error.y * dt;
	intgral_error->z += kI * error.z * dt;

	// 5) Convert gyro to rad/s
	_x_XYZ_t gyro_rad = {
		.x = gyro.x * kDegToRad,
		.y = gyro.y * kDegToRad,
		.z = gyro.z * kDegToRad,
	};

	// 6) Apply feedback terms
	_x_XYZ_t omega = {
		.x = gyro_rad.x + kP * error.x + intgral_error->x,
		.y = gyro_rad.y + kP * error.y + intgral_error->y,
		.z = gyro_rad.z + kP * error.z + intgral_error->z,
	};

	// 7) Compute quaternion derivative
	x_QUAT_t dot = {
		.w = 0.5 * (-prev->x * omega.x - prev->y * omega.y - prev->z * omega.z),
		.x = 0.5 * ( prev->w * omega.x + prev->y * omega.z - prev->z * omega.y),
		.y = 0.5 * ( prev->w * omega.y - prev->x * omega.z + prev->z * omega.x),
		.z = 0.5 * ( prev->w * omega.z + prev->x * omega.y - prev->y * omega.x),
	};

	// 8) Integrate to yield new quaternion
	x_QUAT_t new = {
		.w = prev->w + dot.w * dt,
		.x = prev->x + dot.x * dt,
		.y = prev->y + dot.y * dt,
		.z = prev->z + dot.z * dt,
	};

	// 9) Normalize quaternion
	float norm = sqrt(new.w * new.w + new.x * new.x + new.y * new.y + new.z * new.z);

	prev->w = new.w / norm;
	prev->x = new.x / norm;
	prev->y = new.y / norm;
	prev->z = new.z / norm;
}

float f_Quaternion_TiltAngleX_Compute(x_QUAT_t q){
	// Rotate body X-axis into world frame, take Z-component
	float vx = 2.0f * (q.x * q.z - q.w * q.y);
	// Clamp to [-1, 1]
	vx = vx > 1.0f ? 1.0f : (vx < -1.0f ? -1.0f : vx);
	// Angle between body X-axis and gravity (world Z-axis)
	return acos(vx) * kRadToDeg - x_angle_offset;
}

float f_Quaternion_TiltAngleY_Compute(x_QUAT_t q){
	// Rotate body Y-axis into world frame, take Z-component
	float vy = 2.0f * (q.y * q.z + q.w * q.x);
	// Clamp to [-1, 1]
	vy = vy > 1.0f ? 1.0f : (vy < -1.0f ? -1.0f : vy);
	// Angle between body Y-axis and gravity (world Z-axis)
	return acos(vy) * kRadToDeg - y_angle_offset;
}

float f_Quaternion_TiltAngleZ_Compute(x_QUAT_t q){
	// Compute the world-frame Z component of the sensor's Z-axis
	float vz = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;
	// Clamp to [-1, 1]
	vz = vz > 1.0f ? 1.0f : (vz < -1.0f ? -1.0f : vz);
	// Tilt angle between sensor Z-axis and gravity (world Z-axis)

	return acos(vz) * kRadToDeg - z_angle_offset;
}




//////////////////////////////
//			COPILOT			//
//////////////////////////////

typedef struct {
    float twoKp;      // 2 * proportional gain
    float twoKi;      // 2 * integral gain
    float q0, q1, q2, q3; // Quaternion
    float integralFBx, integralFBy, integralFBz;
} MahonyAHRS;

MahonyAHRS mahony = {
    .twoKp = 2.0f,
    .twoKi = 0.0f,
    .q0 = 1.0f, .q1 = 0.0f, .q2 = 0.0f, .q3 = 0.0f,
    .integralFBx = 0.0f, .integralFBy = 0.0f, .integralFBz = 0.0f
};





void MahonyAHRSupdateIMU(float gx, float gy, float gz, float ax, float ay, float az, float dt) {
    float recipNorm;
    float halfvx, halfvy, halfvz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    // Normalize accelerometer
    recipNorm = 1.0f / sqrtf(ax * ax + ay * ay + az * az);
    ax *= recipNorm;
    ay *= recipNorm;
    az *= recipNorm;

    // Estimated direction of gravity
    halfvx = mahony.q1 * mahony.q3 - mahony.q0 * mahony.q2;
    halfvy = mahony.q0 * mahony.q1 + mahony.q2 * mahony.q3;
    halfvz = mahony.q0 * mahony.q0 - 0.5f + mahony.q3 * mahony.q3;

    // Error is cross product between estimated and measured direction of gravity
    halfex = (ay * halfvz - az * halfvy);
    halfey = (az * halfvx - ax * halfvz);
    halfez = (ax * halfvy - ay * halfvx);

    // Apply integral feedback
    if (mahony.twoKi > 0.0f) {
        mahony.integralFBx += mahony.twoKi * halfex * dt;
        mahony.integralFBy += mahony.twoKi * halfey * dt;
        mahony.integralFBz += mahony.twoKi * halfez * dt;
        gx += mahony.integralFBx;
        gy += mahony.integralFBy;
        gz += mahony.integralFBz;
    }

    // Apply proportional feedback
    gx += mahony.twoKp * halfex;
    gy += mahony.twoKp * halfey;
    gz += mahony.twoKp * halfez;

    // Integrate rate of change of quaternion
    gx *= 0.5f * dt;
    gy *= 0.5f * dt;
    gz *= 0.5f * dt;

    qa = mahony.q0;
    qb = mahony.q1;
    qc = mahony.q2;

    mahony.q0 += -qb * gx - qc * gy - mahony.q3 * gz;
    mahony.q1 += qa * gx + qc * gz - mahony.q3 * gy;
    mahony.q2 += qa * gy - qb * gz + mahony.q3 * gx;
    mahony.q3 += qa * gz + qb * gy - qc * gx;

    // Normalize quaternion
    recipNorm = 1.0f / sqrtf(mahony.q0 * mahony.q0 + mahony.q1 * mahony.q1 +
                             mahony.q2 * mahony.q2 + mahony.q3 * mahony.q3);
    mahony.q0 *= recipNorm;
    mahony.q1 *= recipNorm;
    mahony.q2 *= recipNorm;
    mahony.q3 *= recipNorm;
}


float imu_roll, imu_pitch, imu_yaw;
void v_Mahony_Test(){
	float gyro_sensitivity = 2000.0f / 32768.0f;
	float accel_sensitivity = 2.0f / 32768.0f;
	//float g = 9.80665f;

	int16_t* imu = pi16_IMU_Get_Left();

	int cnt=0;
	for(int i=0; i<6; i++){
		if(imu[i] == 0){cnt++;}
	}
	if(cnt == 6){return;}


	float ax, ay, az;
	float gx, gy, gz;
	ax = imu[0] * accel_sensitivity;
	ay = imu[1] * accel_sensitivity;
	az = imu[2] * accel_sensitivity;
	gx = imu[3] * gyro_sensitivity * (M_PI / 180.0f);
	gy = imu[4] * gyro_sensitivity * (M_PI / 180.0f);
	gz = imu[5] * gyro_sensitivity * (M_PI / 180.0f);

	MahonyAHRSupdateIMU(gx, gy, gz, ax, ay, az, 0.1);


	float roll = atan2f(2.0f * (mahony.q0*mahony.q1 + mahony.q2*mahony.q3), 1.0f - 2.0f * (mahony.q1*mahony.q1 + mahony.q2*mahony.q2));
	float pitch = asinf(2.0f * (mahony.q0*mahony.q2 - mahony.q3*mahony.q1));
	float yaw = atan2f(2.0f * (mahony.q0*mahony.q3 + mahony.q1*mahony.q2), 1.0f - 2.0f * (mahony.q2*mahony.q2 + mahony.q3*mahony.q3));

	imu_roll = roll * 180.0f / M_PI;
	imu_pitch = pitch * 180.0f / M_PI;
	imu_yaw = yaw * 180.0f / M_PI;
}




x_QUAT_t Quaternion_Inverse(x_QUAT_t q){
	x_QUAT_t q_inv;
	q_inv.w = q.w;
	q_inv.x = -q.x;
	q_inv.y = -q.y;
	q_inv.z = -q.z;
	return q_inv;
}

x_QUAT_t Quaternion_Multiply(x_QUAT_t q1, x_QUAT_t q2){
	x_QUAT_t result;
	result.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
	result.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
	result.y = q1.w*q2.y - q1.x*q2.z + q1.y*q2.w + q1.z*q2.x;
	result.z = q1.w*q2.z + q1.x*q2.y - q1.y*q2.x + q1.z*q2.w;
	return result;
}

















