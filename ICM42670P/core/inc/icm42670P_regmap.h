#ifndef __JH_ICM42670P_REGMAP_H
#define __JH_ICM42670P_REGMAP_H


#ifdef __cplusplus
extern "C" {
#endif


#include "lib_def.h"

#define ICM42670P_MREG_MASK		0x00010000


/****************************************/
//	Register Map
/****************************************/

/*
 * BANK0
 */
typedef enum {
	ICM42670P_BANK0_REG_MCLK_RDY			=0x00,
	ICM42670P_BANK0_REG_DEVICE_CONFIG		=0x01,
	ICM42670P_BANK0_REG_SIGNAL_PATH_RESET 	=0x02,
	ICM42670P_BANK0_REG_DRIVE_CONFIG1     	=0x03,
	ICM42670P_BANK0_REG_DRIVE_CONFIG2     	=0x04,
	ICM42670P_BANK0_REG_DRIVE_CONFIG3     	=0x05,
	ICM42670P_BANK0_REG_INT_CONFIG        	=0x06,
	ICM42670P_BANK0_REG_TEMP_DATA1        	=0x09,
	ICM42670P_BANK0_REG_TEMP_DATA0        	=0x0a,
	ICM42670P_BANK0_REG_ACCEL_DATA_X1     	=0x0b,
	ICM42670P_BANK0_REG_ACCEL_DATA_X0     	=0x0c,
	ICM42670P_BANK0_REG_ACCEL_DATA_Y1     	=0x0d,
	ICM42670P_BANK0_REG_ACCEL_DATA_Y0     	=0x0e,
	ICM42670P_BANK0_REG_ACCEL_DATA_Z1     	=0x0f,
	ICM42670P_BANK0_REG_ACCEL_DATA_Z0     	=0x10,
	ICM42670P_BANK0_REG_GYRO_DATA_X1      	=0x11,
	ICM42670P_BANK0_REG_GYRO_DATA_X0      	=0x12,
	ICM42670P_BANK0_REG_GYRO_DATA_Y1      	=0x13,
	ICM42670P_BANK0_REG_GYRO_DATA_Y0      	=0x14,
	ICM42670P_BANK0_REG_GYRO_DATA_Z1      	=0x15,
	ICM42670P_BANK0_REG_GYRO_DATA_Z0      	=0x16,
	ICM42670P_BANK0_REG_TMST_FSYNCH       	=0x17,
	ICM42670P_BANK0_REG_TMST_FSYNCL       	=0x18,
	ICM42670P_BANK0_REG_APEX_DATA4        	=0x1d,
	ICM42670P_BANK0_REG_APEX_DATA5        	=0x1e,
	ICM42670P_BANK0_REG_PWR_MGMT0         	=0x1f,
	ICM42670P_BANK0_REG_GYRO_CONFIG0      	=0x20,
	ICM42670P_BANK0_REG_ACCEL_CONFIG0     	=0x21,
	ICM42670P_BANK0_REG_TEMP_CONFIG0      	=0x22,
	ICM42670P_BANK0_REG_GYRO_CONFIG1      	=0x23,
	ICM42670P_BANK0_REG_ACCEL_CONFIG1     	=0x24,
	ICM42670P_BANK0_REG_APEX_CONFIG0      	=0x25,
	ICM42670P_BANK0_REG_APEX_CONFIG1      	=0x26,
	ICM42670P_BANK0_REG_WOM_CONFIG        	=0x27,
	ICM42670P_BANK0_REG_FIFO_CONFIG1      	=0x28,
	ICM42670P_BANK0_REG_FIFO_CONFIG2      	=0x29,
	ICM42670P_BANK0_REG_FIFO_CONFIG3      	=0x2a,
	ICM42670P_BANK0_REG_INT_SOURCE0       	=0x2b,
	ICM42670P_BANK0_REG_INT_SOURCE1       	=0x2c,
	ICM42670P_BANK0_REG_INT_SOURCE3       	=0x2d,
	ICM42670P_BANK0_REG_INT_SOURCE4       	=0x2e,
	ICM42670P_BANK0_REG_FIFO_LOST_PKT0    	=0x2f,
	ICM42670P_BANK0_REG_FIFO_LOST_PKT1    	=0x30,
	ICM42670P_BANK0_REG_APEX_DATA0        	=0x31,
	ICM42670P_BANK0_REG_APEX_DATA1        	=0x32,
	ICM42670P_BANK0_REG_APEX_DATA2        	=0x33,
	ICM42670P_BANK0_REG_APEX_DATA3        	=0x34,
	ICM42670P_BANK0_REG_INTF_CONFIG0      	=0x35,
	ICM42670P_BANK0_REG_INTF_CONFIG1      	=0x36,
	ICM42670P_BANK0_REG_INT_STATUS_DRDY   	=0x39,
	ICM42670P_BANK0_REG_INT_STATUS        	=0x3a,
	ICM42670P_BANK0_REG_INT_STATUS2       	=0x3b,
	ICM42670P_BANK0_REG_INT_STATUS3       	=0x3c,
	ICM42670P_BANK0_REG_FIFO_COUNTH       	=0x3d,
	ICM42670P_BANK0_REG_FIFO_COUNTL       	=0x3e,
	ICM42670P_BANK0_REG_FIFO_DATA         	=0x3f,
	ICM42670P_BANK0_REG_WHO_AM_I          	=0x75,
	ICM42670P_BANK0_REG_BLK_SEL_W         	=0x79,
	ICM42670P_BANK0_REG_MADDR_W           	=0x7a,
	ICM42670P_BANK0_REG_M_W               	=0x7b,
	ICM42670P_BANK0_REG_BLK_SEL_R         	=0x7c,
	ICM42670P_BANK0_REG_MADDR_R           	=0x7d,
	ICM42670P_BANK0_REG_M_R               	=0x7e,
} e_ICM42670P_BANK0_t;

/*
 * MREG1
 * BLK_SEL_W/R	: 0x00
 * MADDR_W/R	: target address
 * M_W/R		: write or read
 */
typedef enum {
	ICM42670P_MREG1_REG_TMST_CONFIG1	=0x00010000,
	ICM42670P_MREG1_REG_FIFO_CONFIG5	=0x00010001,
	ICM42670P_MREG1_REG_FIFO_CONFIG6	=0x00010002,
	ICM42670P_MREG1_REG_FSYNC_CONFIG    =0x00010003,
	ICM42670P_MREG1_REG_INT_CONFIG0		=0x00010004,
	ICM42670P_MREG1_REG_INT_CONFIG1     =0x00010005,
	ICM42670P_MREG1_REG_SENSOR_CONFIG3	=0x00010006,
	ICM42670P_MREG1_REG_ST_CONFIG       =0x00010013,
	ICM42670P_MREG1_REG_SELFTEST		=0x00010014,
	ICM42670P_MREG1_REG_INTF_CONFIG6    =0x00010023,
	ICM42670P_MREG1_REG_INTF_CONFIG10	=0x00010025,
	ICM42670P_MREG1_REG_INTF_CONFIG7    =0x00010028,
	ICM42670P_MREG1_REG_OTP_CONFIG		=0x0001002b,
	ICM42670P_MREG1_REG_INT_SOURCE6     =0x0001002f,
	ICM42670P_MREG1_REG_INT_SOURCE7		=0x00010030,
	ICM42670P_MREG1_REG_INT_SOURCE8     =0x00010031,
	ICM42670P_MREG1_REG_INT_SOURCE9		=0x00010032,
	ICM42670P_MREG1_REG_INT_SOURCE10	=0x00010033,
	ICM42670P_MREG1_REG_APEX_CONFIG2	=0x00010044,
	ICM42670P_MREG1_REG_APEX_CONFIG3	=0x00010045,
	ICM42670P_MREG1_REG_APEX_CONFIG4	=0x00010046,
	ICM42670P_MREG1_REG_APEX_CONFIG5    =0x00010047,
	ICM42670P_MREG1_REG_APEX_CONFIG9    =0x00010048,
	ICM42670P_MREG1_REG_APEX_CONFIG10	=0x00010049,
	ICM42670P_MREG1_REG_APEX_CONFIG11	=0x0001004a,
	ICM42670P_MREG1_REG_ACCEL_WOM_X_THR	=0x0001004b,
	ICM42670P_MREG1_REG_ACCEL_WOM_Y_THR	=0x0001004c,
	ICM42670P_MREG1_REG_ACCEL_WOM_Z_THR	=0x0001004d,
	ICM42670P_MREG1_REG_OFFSET_USER0	=0x0001004e,
	ICM42670P_MREG1_REG_OFFSET_USER1	=0x0001004f,
	ICM42670P_MREG1_REG_OFFSET_USER2	=0x00010050,
	ICM42670P_MREG1_REG_OFFSET_USER3	=0x00010051,
	ICM42670P_MREG1_REG_OFFSET_USER4	=0x00010052,
	ICM42670P_MREG1_REG_OFFSET_USER5	=0x00010053,
	ICM42670P_MREG1_REG_OFFSET_USER6	=0x00010054,
	ICM42670P_MREG1_REG_OFFSET_USER7    =0x00010055,
	ICM42670P_MREG1_REG_OFFSET_USER8    =0x00010056,
	ICM42670P_MREG1_REG_ST_STATUS1      =0x00010063,
	ICM42670P_MREG1_REG_ST_STATUS2      =0x00010064,
	ICM42670P_MREG1_REG_FDR_CONFIG      =0x00010066,
	ICM42670P_MREG1_REG_APEX_CONFIG12	=0x00010067,
} e_ICM42670P_MREG1_t;


/*
 * MREG2
 * BLK_SEL_W/R	: 0x28
 * MADDR_W/R	: target address
 * M_W/R		: write or read
 */
typedef enum {
	ICM42670P_MREG2_REG_OTP_CTRL7	=0x00012806,
} e_ICM42670P_MREG2_t;


/*
 * MREG3
 * BLK_SEL_W/R	: 0x50
 * MADDR_W/R	: target address
 * M_W/R		: write or read
 */
typedef enum {
	ICM42670P_MREG3_REG_XA_ST_DATA	=0x00015000,
    ICM42670P_MREG3_REG_YA_ST_DATA	=0x00015001,
    ICM42670P_MREG3_REG_ZA_ST_DATA	=0x00015002,
    ICM42670P_MREG3_REG_XG_ST_DATA	=0x00015003,
    ICM42670P_MREG3_REG_YG_ST_DATA	=0x00015004,
    ICM42670P_MREG3_REG_ZG_ST_DATA	=0x00015005,
} e_ICM42670P_MREG3_t;


/****************************************/
//	BANK0 Bits Position
/****************************************/

/*
 * MCLK_RDY
 * Register Name : MCLK_RDY
 */

/*
 * mclk_rdy
 * 0: Indicates internal clock is currently not running
 * 1: Indicates internal clock is currently running
 */
#define ICM42670P_BANK0_POS_MCLK_RDY_MCLK_RDY   0x03
#define ICM42670P_BANK0_MASK_MCLK_RDY_MCLK_RDY	(0x01 << ICM42670P_BANK0_POS_MCLK_RDY_MCLK_RDY)

/*
 * DEVICE_CONFIG
 * Register Name : DEVICE_CONFIG
 */

/*
 * spi_ap_4wire
 * 0: AP interface uses 3-wire SPI mode
 * 1: AP interface uses 4-wire SPI mode
 */
#define ICM42670P_BANK0_POS_DEVICE_CONFIG_SPI_AP_4WIRE	0x02
#define ICM42670P_BANK0_MASK_DEVICE_CONFIG_SPI_AP_4WIRE	(0x01 << ICM42670P_BANK0_POS_DEVICE_CONFIG_SPI_AP_4WIRE)

/*
 * spi_mode
 * SPI mode selection
 *
 * 0: Mode 0 and Mode 3
 * 1: Mode 1 and Mode 2
 *
 * If device is operating in non-SPI mode, user is not allowed to change the power-on default setting of this register. Change of this register setting will not take effect till AP_CS = 1.
 */
#define ICM42670P_BANK0_POS_DEVICE_CONFIG_SPI_MODE     0x00
#define ICM42670P_BANK0_MASK_DEVICE_CONFIG_SPI_MODE    0x01



/*
 * SIGNAL_PATH_RESET
 * Register Name : SIGNAL_PATH_RESET
 */

/*
 * soft_reset_device_config
 * Software Reset (auto clear bit)
 *
 * 0: Software reset not enabled
 * 1: Software reset enabled
 */
#define ICM42670P_BANK0_POS_SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG	0x04
#define ICM42670P_BANK0_MASK_SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG	(0x01 << ICM42670P_BANK0_POS_SIGNAL_PATH_RESET_SOFT_RESET_DEVICE_CONFIG)

/*
 * fifo_flush
 * When set to 1, FIFO will get flushed.
 * FIFO flush requires the following programming sequence:
 * • Write FIFO_FLUSH =1
 * • Wait for 1.5 µs
 * • Read FIFO_FLUSH, it should now be 0
 * Host can only program this register bit to 1.
 */
#define ICM42670P_BANK0_POS_SIGNAL_PATH_RESET_FIFO_FLUSH	0x02
#define ICM42670P_BANK0_MASK_SIGNAL_PATH_RESET_FIFO_FLUSH	(0x01 << ICM42670P_BANK0_POS_SIGNAL_PATH_RESET_FIFO_FLUSH)



/*
 * DRIVE_CONFIG1
 * Register Name : DRIVE_CONFIG1
 */

/*
 * i3c_ddr_slew_rate
 * Controls slew rate for output pin 14 when device is in I3CSM DDR protocol.
 * While in I3CSM operation, the device automatically switches to use I3C_DDR_SLEW_RATE after receiving ENTHDR0 ccc command from the host. The device automatically switches back to I3C_SDR_SLEW_RATE after the host issues HDR_EXIT pattern.
 *
 * 000: MIN: 20 ns; TYP: 40 ns; MAX: 60 ns
 * 001: MIN: 12 ns; TYP: 24 ns; MAX: 36 ns
 * 010: MIN: 6 ns; TYP: 12 ns; MAX: 19 ns
 * 011: MIN: 4 ns; TYP: 8 ns; MAX: 14 ns
 * 100: MIN: 2 ns; TYP: 4 ns; MAX: 8 ns
 * 101: MAX: 2 ns
 * 110: Reserved
 * 111: Reserved
 *
 * This register field should not be programmed in I3C/DDR mode.
 */
#define ICM42670P_BANK0_POS_DRIVE_CONFIG1_I3C_DDR_SLEW_RATE		0x03
#define ICM42670P_BANK0_MASK_DRIVE_CONFIG1_I3C_DDR_SLEW_RATE	(0x07 << ICM42670P_BANK0_POS_DRIVE_CONFIG1_I3C_DDR_SLEW_RATE)

/*
 * i3c_sdr_slew_rate
 * Controls slew rate for output pin 14 in I3CSM SDR protocol.
 * After device reset, I2C_SLEW_RATE is used by default. If I3CSM feature is enabled, the device automatically switches to use I3C_SDR_SLEW_RATE after receiving 0x7E+W message (an I3CSM broadcast message).
 *
 * 000: MIN: 20 ns; TYP: 40 ns; MAX: 60 ns
 * 001: MIN: 12 ns; TYP: 24 ns; MAX: 36 ns
 * 010: MIN: 6 ns; TYP: 12 ns; MAX: 19 ns
 * 011: MIN: 4 ns; TYP: 8 ns; MAX: 14 ns
 * 100: MIN: 2 ns; TYP: 4 ns; MAX: 8 ns
 * 101: MAX: 2 ns
 * 110: Reserved
 * 111: Reserved
 *
 * This register field should not be programmed in I3C/DDR mode
 */
#define ICM42670P_BANK0_POS_DRIVE_CONFIG1_I3C_SDR_SLEW_RATE		0x00
#define ICM42670P_BANK0_MASK_DRIVE_CONFIG1_I3C_SDR_SLEW_RATE	0x07



/*
 * DRIVE_CONFIG2
 * Register Name : DRIVE_CONFIG2
 */

/*
 * i2c_slew_rate
 * Controls slew rate for output pin 14 in I2C mode.
 * After device reset, the I2C_SLEW_RATE is used by default. If the 1st write operation from host is an SPI transaction, the device automatically switches to SPI_SLEW_RATE. If I3CSM feature is enabled, the device automatically switches to I3C_SDR_SLEW_RATE after receiving 0x7E+W message (an I3C broadcast message).
 *
 * 000: MIN: 20 ns; TYP: 40 ns; MAX: 60 ns
 * 001: MIN: 12 ns; TYP: 24 ns; MAX: 36 ns
 * 010: MIN: 6 ns; TYP: 12 ns; MAX: 19 ns
 * 011: MIN: 4 ns; TYP: 8 ns; MAX: 14 ns
 * 100: MIN: 2 ns; TYP: 4 ns; MAX: 8 ns
 * 101: MAX: 2 ns
 * 110: Reserved
 * 111: Reserved
 *
 * This register field should not be programmed in I3C/DDR mode
 */
#define ICM42670P_BANK0_POS_DRIVE_CONFIG2_I2C_SLEW_RATE		0x03
#define ICM42670P_BANK0_MASK_DRIVE_CONFIG2_I2C_SLEW_RATE	(0x07 << ICM42670P_BANK0_POS_DRIVE_CONFIG2_I2C_SLEW_RATE)

/*
 * all_slew_rate
 * Configure drive strength for all output pins in all modes (SPI3, SPI4, I2C, I3CSM) excluding pin 14.
 *
 * 000: MIN: 20 ns; TYP: 40 ns; MAX: 60 ns
 * 001: MIN: 12 ns; TYP: 24 ns; MAX: 36 ns
 * 010: MIN: 6 ns; TYP: 12 ns; MAX: 19 ns
 * 011: MIN: 4 ns; TYP: 8 ns; MAX: 14 ns
 * 100: MIN: 2 ns; TYP: 4 ns; MAX: 8 ns
 * 101: MAX: 2 ns
 * 110: Reserved
 * 111: Reserved
 *
 * This register field should not be programmed in I3C/DDR mode
 */
#define ICM42670P_BANK0_POS_DRIVE_CONFIG2_ALL_SLEW_RATE		0x00
#define ICM42670P_BANK0_MASK_DRIVE_CONFIG2_ALL_SLEW_RATE	0x07



/*
 * DRIVE_CONFIG3
 * Register Name : DRIVE_CONFIG3
 */

/*
 * spi_slew_rate
 * Controls slew rate for output pin 14 in SPI 3-wire mode. In SPI 4-wire mode this register controls the slew rate of pin 1 as it is used as an output in SPI 4-wire mode only. After chip reset, the I2C_SLEW_RATE is used by default for pin 14 pin. If the 1st write operation from the host is an SPI3/4 transaction, the device automatically switches to SPI_SLEW_RATE.
 *
 * 000: MIN: 20 ns; TYP: 40 ns; MAX: 60 ns
 * 001: MIN: 12 ns; TYP: 24 ns; MAX: 36 ns
 * 010: MIN: 6 ns; TYP: 12 ns; MAX: 19 ns
 * 011: MIN: 4 ns; TYP: 8 ns; MAX: 14 ns
 * 100: MIN: 2 ns; TYP: 4 ns; MAX: 8 ns
 * 101: MAX: 2 ns
 * 110: Reserved
 * 111: Reserved
 *
 * This register field should not be programmed in I3C/DDR mode
 */
#define ICM42670P_BANK0_POS_DRIVE_CONFIG3_SPI_SLEW_RATE		0x00
#define ICM42670P_BANK0_MASK_DRIVE_CONFIG3_SPI_SLEW_RATE	0x07



/*
 * INT_CONFIG
 * Register Name : INT_CONFIG
 */

/*
 * int2_mode
 * Interrupt mode and drive circuit shall be configurable by register.
 * Interrupt Mode
 * 1: Latched Mode
 * 0: Pulsed Mode
 */
#define ICM42670P_BANK0_POS_INT_CONFIG_INT2_MODE	0x05
#define ICM42670P_BANK0_MASK_INT_CONFIG_INT2_MODE	(0x01 << ICM42670P_BANK0_POS_INT_CONFIG_INT2_MODE)

/*
 * int2_drive_circuit
 * Interrupt mode and drive circuit shall be configurable by register.
 * Drive Circuit
 * 1: Push-Pull
 * 0: Open drain
 */
#define ICM42670P_BANK0_POS_INT_CONFIG_INT2_DRIVE_CIRCUIT	0x04
#define ICM42670P_BANK0_MASK_INT_CONFIG_INT2_DRIVE_CIRCUIT	(0x01 << ICM42670P_BANK0_POS_INT_CONFIG_INT2_DRIVE_CIRCUIT)

/*
 * int2_polarity
 * Interrupt mode and drive circuit shall be configurable by register.
 * Interrupt Polarity
 * 1: Active High
 * 0: Active Low
 */
#define ICM42670P_BANK0_POS_INT_CONFIG_INT2_POLARITY	0x03
#define ICM42670P_BANK0_MASK_INT_CONFIG_INT2_POLARITY	(0x01 << ICM42670P_BANK0_MASK_INT_CONFIG_INT2_POLARITY)

/*
 * int1_mode
 * Interrupt mode and drive circuit shall be configurable by register.
 * Interrupt Mode
 * 1: Latched Mode
 * 0: Pulsed Mode
 */
#define ICM42670P_BANK0_POS_INT_CONFIG_INT1_MODE	0x02
#define ICM42670P_BANK0_MASK_INT_CONFIG_INT1_MODE	(0x01 << ICM42670P_BANK0_POS_INT_CONFIG_INT1_MODE)

/*
 * int1_drive_circuit
 * Interrupt mode and drive circuit shall be configurable by register.
 * Drive Circuit
 * 1: Push-Pull
 * 0: Open drain
 */
#define ICM42670P_BANK0_POS_INT_CONFIG_INT1_DRIVE_CIRCUIT	0x01
#define ICM42670P_BANK0_MASK_INT_CONFIG_INT1_DRIVE_CIRCUIT	(0x01 << ICM42670P_BANK0_POS_INT_CONFIG_INT1_DRIVE_CIRCUIT)

/*
 * int1_polarity
 * Interrupt mode and drive circuit shall be configurable by register.
 * Interrupt Polarity
 * 1: Active High
 * 0: Active Low
 */
#define ICM42670P_BANK0_POS_INT_CONFIG_INT1_POLARITY	0x00
#define ICM42670P_BANK0_MASK_INT_CONFIG_INT1_POLARITY	0x01



/*
 * TEMP_DATA1
 * Register Name : TEMP_DATA1
 */

/*
 * temp_data
 * Temperature data
 */
#define ICM42670P_BANK0_POS_TEMP_DATA1_TEMP_DATA	0x00
#define ICM42670P_BANK0_MASK_TEMP_DATA1_TEMP_DATA	0xff



/*
 * TEMP_DATA0
 * Register Name : TEMP_DATA0
 */

/*
 * temp_data
 * Temperature data
 */
#define ICM42670P_BANK0_POS_TEMP_DATA0_TEMP_DATA	0x00
#define ICM42670P_BANK0_MASK_TEMP_DATA0_TEMP_DATA	0xff



/*
 * ACCEL_DATA_X1
 * Register Name : ACCEL_DATA_X1
 */

/*
 * accel_data_x
 * Accel X axis data
 */
#define ICM42670P_BANK0_POS_ACCEL_DATA_X1_ACCEL_DATA_X	0x00
#define ICM42670P_BANK0_MASK_ACCEL_DATA_X1_ACCEL_DATA_X	0xff



/*
 * ACCEL_DATA_X0
 * Register Name : ACCEL_DATA_X0
 */

/*
 * accel_data_x
 * Accel X axis data
 */
#define ICM42670P_BANK0_POS_ACCEL_DATA_X0_ACCEL_DATA_X	0x00
#define ICM42670P_BANK0_MASK_ACCEL_DATA_X0_ACCEL_DATA_X	0xff



/*
 * ACCEL_DATA_Y1
 * Register Name : ACCEL_DATA_Y1
 */

/*
 * accel_data_y
 * Accel Y axis data
 */
#define ICM42670P_BANK0_POS_ACCEL_DATA_Y1_ACCEL_DATA_Y	0x00
#define ICM42670P_BANK0_MASK_ACCEL_DATA_Y1_ACCEL_DATA_Y	0xff



/*
 * ACCEL_DATA_Y0
 * Register Name : ACCEL_DATA_Y0
 */

/*
 * accel_data_y
 * Accel Y axis data
 */
#define ICM42670P_BANK0_POS_ACCEL_DATA_Y0_ACCEL_DATA_Y	0x00
#define ICM42670P_BANK0_MASK_ACCEL_DATA_Y0_ACCEL_DATA_Y	0xff



/*
 * ACCEL_DATA_Z1
 * Register Name : ACCEL_DATA_Z1
 */

/*
 * accel_data_z
 * Accel Z axis data
 */
#define ICM42670P_BANK0_POS_ACCEL_DATA_Z1_ACCEL_DATA_Z	0x00
#define ICM42670P_BANK0_MASK_ACCEL_DATA_Z1_ACCEL_DATA_Z	0xff



/*
 * ACCEL_DATA_Z0
 * Register Name : ACCEL_DATA_Z0
 */

/*
 * accel_data_z
 * Accel Z axis data
 */
#define ICM42670P_BANK0_POS_ACCEL_DATA_Z0_ACCEL_DATA_Z	0x00
#define ICM42670P_BANK0_MASK_ACCEL_DATA_Z0_ACCEL_DATA_Z	0xff



/*
 * GYRO_DATA_X1
 * Register Name : GYRO_DATA_X1
 */

/*
 * gyro_data_x
 * Gyro X axis data
 */
#define ICM42670P_BANK0_POS_GYRO_DATA_X1_GYRO_DATA_X	0x00
#define ICM42670P_BANK0_MASK_GYRO_DATA_X1_GYRO_DATA_X	0xff



/*
 * GYRO_DATA_X0
 * Register Name : GYRO_DATA_X0
 */

/*
 * gyro_data_x
 * Gyro X axis data
 */
#define ICM42670P_BANK0_POS_GYRO_DATA_X0_GYRO_DATA_X	0x00
#define ICM42670P_BANK0_MASK_GYRO_DATA_X0_GYRO_DATA_X	0xff



/*
 * GYRO_DATA_Y1
 * Register Name : GYRO_DATA_Y1
 */

/*
 * gyro_data_y
 * Gyro Y axis data
 */
#define ICM42670P_BANK0_POS_GYRO_DATA_Y1_GYRO_DATA_Y	0x00
#define ICM42670P_BANK0_MASK_GYRO_DATA_Y1_GYRO_DATA_Y	0xff



/*
 * GYRO_DATA_Y0
 * Register Name : GYRO_DATA_Y0
 */

/*
 * gyro_data_y
 * Gyro Y axis data
 */
#define ICM42670P_BANK0_POS_GYRO_DATA_Y0_GYRO_DATA_Y	0x00
#define ICM42670P_BANK0_MASK_GYRO_DATA_Y0_GYRO_DATA_Y	0xff



/*
 * GYRO_DATA_Z1
 * Register Name : GYRO_DATA_Z1
 */

/*
 * gyro_data_z
 * Gyro Z axis data
 */
#define ICM42670P_BANK0_POS_GYRO_DATA_Z1_GYRO_DATA_Z	0x00
#define ICM42670P_BANK0_MASK_GYRO_DATA_Z1_GYRO_DATA_Z	0xff



/*
 * GYRO_DATA_Z0
 * Register Name : GYRO_DATA_Z0
 */

/*
 * gyro_data_z
 * Gyro Z axis data
 */
#define ICM42670P_BANK0_POS_GYRO_DATA_Z0_GYRO_DATA_Z	0x00
#define ICM42670P_BANK0_MASK_GYRO_DATA_Z0_GYRO_DATA_Z	0xff



/*
 * TMST_FSYNCH
 * Register Name : TMST_FSYNCH
 */

/*
 * tmst_fsync_data
 * Stores the time delta from the  rising edge of FSYNC to the latest ODR until the UI Interface reads the FSYNC tag in the status register
 */
#define ICM42670P_BANK0_POS_TMST_FSYNCH_TMST_FSYNC_DATA		0x00
#define ICM42670P_BANK0_MASK_TMST_FSYNCH_TMST_FSYNC_DATA	0xff



/*
 * TMST_FSYNCL
 * Register Name : TMST_FSYNCL
 */

/*
 * tmst_fsync_data
 * Stores the time delta from the  rising edge of FSYNC to the latest ODR until the UI Interface reads the FSYNC tag in the status register
 */
#define ICM42670P_BANK0_POS_TMST_FSYNCL_TMST_FSYNC_DATA		0x00
#define ICM42670P_BANK0_MASK_TMST_FSYNCL_TMST_FSYNC_DATA	0xff



/*
 * APEX_DATA4
 * Register Name : APEX_DATA4
 */

/*
 * ff_dur
 * Free Fall duration. The duration is given in number of samples and it can be converted to freefall distance by applying the following formula:
 * ff_distance = 0.5*9.81*(ff_duration*dmp_odr_s)^2)
 * Note: dmp_odr_s in the duration of DMP_ODR expressed in seconds.
 */
#define ICM42670P_BANK0_POS_APEX_DATA4_FF_DUR	0x00
#define ICM42670P_BANK0_MASK_APEX_DATA4_FF_DUR	0xff



/*
 * APEX_DATA5
 * Register Name : APEX_DATA5
 */

/*
 * ff_dur
 * Free Fall duration. The duration is given in number of samples and it can be converted to freefall distance by applying the following formula:
 * ff_distance = 0.5*9.81*(ff_duration*dmp_odr_s)^2)
 * Note: dmp_odr_s in the duration of DMP_ODR expressed in seconds.
 */
#define ICM42670P_BANK0_POS_APEX_DATA5_FF_DUR	0x00
#define ICM42670P_BANK0_MASK_APEX_DATA5_FF_DUR	0xff



/*
 * PWR_MGMT0
 * Register Name : PWR_MGMT0
 */

/*
 * accel_lp_clk_sel
 * 0: Accelerometer LP mode uses Wake Up oscillator clock. This is the lowest power consumption mode and it is the recommended setting.
 * 1: Accelerometer LP mode uses RC oscillator clock.
 *
 * This field can be changed on-the-fly even if accel sensor is on.
 */
#define ICM42670P_BANK0_POS_PWR_MGMT0_ACCEL_LP_CLK_SEL     0x07
#define ICM42670P_BANK0_MASK_PWR_MGMT0_ACCEL_LP_CLK_SEL    (0x01 << ICM42670P_BANK0_POS_PWR_MGMT0_ACCEL_LP_CLK_SEL)

/*
 * idle
 * If this bit is set to 1, the  RC oscillator is powered on even if Accel and Gyro are powered off.
 * Nominally this bit is set to 0, so when Accel and Gyro are powered off,
 * the  chip will go to OFF state , since the RC oscillator will also be powered off.
 *
 * This field can be changed on-the-fly even if a sensor is already on
 */
#define ICM42670P_BANK0_POS_PWR_MGMT0_IDLE     0x04
#define ICM42670P_BANK0_MASK_PWR_MGMT0_IDLE    (0x01 << ICM42670P_BANK0_POS_PWR_MGMT0_IDLE)

/*
 * gyro_mode
 * 00: Turns gyroscope off
 * 01: Places gyroscope in Standby Mode
 * 10: Reserved
 * 11: Places gyroscope in Low Noise (LN) Mode
 *
 * Gyroscope needs to be kept ON for a minimum of 45ms. When transitioning from OFF to any of the other modes, do not issue any register writes for 200 µs.
 *
 * This field can be changed on-the-fly even if gyro sensor is on
 */
#define ICM42670P_BANK0_POS_PWR_MGMT0_GYRO_MODE     0x02
#define ICM42670P_BANK0_MASK_PWR_MGMT0_GYRO_MODE	(0x03 << ICM42670P_BANK0_POS_PWR_MGMT0_GYRO_MODE)

/*
 * accel_mode
 * 00: Turns accelerometer off
 * 01: Turns accelerometer off
 * 10: Places accelerometer in Low Power (LP) Mode
 * 11: Places accelerometer in Low Noise (LN) Mode
 *
 * When selecting LP Mode please refer to ACCEL_LP_CLK_SEL setting, bit[7] of this register.
 *
 * Before entering LP mode and during LP Mode the following combinations of ODR and averaging are not permitted:
 * 1) ODR=1600 Hz or ODR=800 Hz: any averaging.
 * 2) ODR=400 Hz: averaging=16x, 32x or 64x.
 * 3) ODR=200 Hz: averaging=64x.
 *
 * When transitioning from OFF to any of the other modes, do not issue any register writes for 200 µs.
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_PWR_MGMT0_ACCEL_MODE	0x00
#define ICM42670P_BANK0_MASK_PWR_MGMT0_ACCEL_MODE    0x03



/*
 * GYRO_CONFIG0
 * Register Name : GYRO_CONFIG0
 */

/*
 * gyro_ui_fs_sel
 * Full scale select for gyroscope UI interface output
 *
 * 00: ±2000 dps
 * 01: ±1000 dps
 * 10: ±500 dps
 * 11: ±250 dps
 *
 * This field can be changed on-the-fly even if gyro sensor is on
 */
#define ICM42670P_BANK0_POS_GYRO_CONFIG0_GYRO_UI_FS_SEL     0x05
#define ICM42670P_BANK0_MASK_GYRO_CONFIG0_GYRO_UI_FS_SEL    (0x03 << ICM42670P_BANK0_POS_GYRO_CONFIG0_GYRO_UI_FS_SEL)

/*
 * gyro_odr
 * Gyroscope ODR selection for UI interface output
 *
 * 0000: Reserved
 * 0001: Reserved
 * 0010: Reserved
 * 0011: Reserved
 * 0100: Reserved
 * 0101: 1.6k Hz
 * 0110: 800 Hz
 * 0111: 400 Hz
 * 1000: 200 Hz
 * 1001: 100 Hz
 * 1010: 50 Hz
 * 1011: 25 Hz
 * 1100: 12.5 Hz
 * 1101: Reserved
 * 1110: Reserved
 * 1111: Reserved
 *
 * This field can be changed on-the-fly even if gyro sensor is on
 */
#define ICM42670P_BANK0_POS_GYRO_CONFIG0_GYRO_ODR     0x00
#define ICM42670P_BANK0_MASK_GYRO_CONFIG0_GYRO_ODR    0x0f



/*
 * ACCEL_CONFIG0
 * Register Name : ACCEL_CONFIG0
 */

/*
 * accel_ui_fs_sel
 * Full scale select for accelerometer UI interface output
 *
 * 00: ±16g
 * 01: ±8g
 * 10: ±4g
 * 11: ±2g
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_ACCEL_CONFIG0_ACCEL_UI_FS_SEL     0x05
#define ICM42670P_BANK0_MASK_ACCEL_CONFIG0_ACCEL_UI_FS_SEL    (0x03 << ICM42670P_BANK0_POS_ACCEL_CONFIG0_ACCEL_UI_FS_SEL)

/*
 * accel_odr
 * Accelerometer ODR selection for UI interface output
 *
 * 0000: Reserved
 * 0001: Reserved
 * 0010: Reserved
 * 0011: Reserved
 * 0100: Reserved
 * 0101: 1.6 kHz (LN mode)
 * 0110: 800 Hz (LN mode)
 * 0111: 400 Hz (LP or LN mode)
 * 1000: 200 Hz (LP or LN mode)
 * 1001: 100 Hz (LP or LN mode)
 * 1010: 50 Hz (LP or LN mode)
 * 1011: 25 Hz (LP or LN mode)
 * 1100: 12.5 Hz (LP or LN mode)
 * 1101: 6.25 Hz (LP mode)
 * 1110: 3.125 Hz (LP mode)
 * 1111: 1.5625 Hz (LP mode)
 *
 * This field can be changed on-the-fly when accel sensor is on
 */
#define ICM42670P_BANK0_POS_ACCEL_CONFIG0_ACCEL_ODR     0x00
#define ICM42670P_BANK0_MASK_ACCEL_CONFIG0_ACCEL_ODR    0x0f



/*
 * TEMP_CONFIG0
 * Register Name : TEMP_CONFIG0
 */

/*
 * temp_filt_bw
 * Sets the bandwidth of the temperature signal DLPF
 *
 * 000: DLPF bypassed
 * 001: DLPF BW = 180 Hz
 * 010: DLPF BW = 72 Hz
 * 011: DLPF BW = 34 Hz
 * 100: DLPF BW = 16 Hz
 * 101: DLPF BW = 8 Hz
 * 110: DLPF BW = 4 Hz
 * 111: DLPF BW = 4 Hz
 *
 * This field can be changed on-the-fly even if sensor is on
 */
#define ICM42670P_BANK0_POS_TEMP_CONFIG0_TEMP_FILT_BW     0x04
#define ICM42670P_BANK0_MASK_TEMP_CONFIG0_TEMP_FILT_BW    (0x07 << ICM42670P_BANK0_POS_TEMP_CONFIG0_TEMP_FILT_BW)



/*
 * GYRO_CONFIG1
 * Register Name : GYRO_CONFIG1
 */

/*
 * gyro_ui_filt_bw
 * Selects GYRO UI low pass filter bandwidth
 *
 * 000: Low pass filter bypassed
 * 001: 180 Hz
 * 010: 121 Hz
 * 011: 73 Hz
 * 100: 53 Hz
 * 101: 34 Hz
 * 110: 25 Hz
 * 111: 16 Hz
 *
 * This field can be changed on-the-fly even if gyro sensor is on
 */
#define ICM42670P_BANK0_POS_GYRO_CONFIG1_GYRO_UI_FILT_BW     0x00
#define ICM42670P_BANK0_MASK_GYRO_CONFIG1_GYRO_UI_FILT_BW    0x07



/*
 * ACCEL_CONFIG1
 * Register Name : ACCEL_CONFIG1
 */

/*
 * accel_ui_avg
 * Selects averaging filter setting to create accelerometer output in accelerometer low power mode (LPM)
 *
 * 000: 2x average
 * 001: 4x average
 * 010: 8x average
 * 011: 16x average
 * 100: 32x average
 * 101: 64x average
 * 110: 64x average
 * 111: 64x average
 *
 * This field cannot be changed when the accel sensor is in LPM
 */
#define ICM42670P_BANK0_POS_ACCEL_CONFIG1_ACCEL_UI_AVG     0x04
#define ICM42670P_BANK0_MASK_ACCEL_CONFIG1_ACCEL_UI_AVG    (0x07 << ICM42670P_BANK0_POS_ACCEL_CONFIG1_ACCEL_UI_AVG)

/*
 * accel_ui_filt_bw
 * Selects ACCEL UI low pass filter bandwidth
 *
 * 000: Low pass filter bypassed
 * 001: 180 Hz
 * 010: 121 Hz
 * 011: 73 Hz
 * 100: 53 Hz
 * 101: 34 Hz
 * 110: 25 Hz
 * 111: 16 Hz
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_ACCEL_CONFIG1_ACCEL_UI_FILT_BW     0x00
#define ICM42670P_BANK0_MASK_ACCEL_CONFIG1_ACCEL_UI_FILT_BW    0x07



/*
 * APEX_CONFIG0
 * Register Name : APEX_CONFIG0
 */

/*
 * dmp_power_save_en
 * When this bit is set to 1, power saving is enabled for DMP algorithms
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG0_DMP_POWER_SAVE_EN     0x03
#define ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_POWER_SAVE_EN    (0x01 << ICM42670P_BANK0_POS_APEX_CONFIG0_DMP_POWER_SAVE_EN)

/*
 * dmp_init_en
 * When this bit is set to 1, DMP runs DMP SW initialization procedure. Bit is reset by hardware when the procedure is finished. All other APEX features are ignored as long as DMP_INIT_EN is set.
 *
 * This field can be changed on-the-fly even if accel sensor is on.
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG0_DMP_INIT_EN     0x02
#define ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_INIT_EN    (0x01 << ICM42670P_BANK0_POS_APEX_CONFIG0_DMP_INIT_EN)

/*
 * dmp_mem_reset_en
 * When this bit is set to 1, it clears DMP SRAM for APEX operation or Self-test operation.
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG0_DMP_MEM_RESET_EN     0x00
#define ICM42670P_BANK0_MASK_APEX_CONFIG0_DMP_MEM_RESET_EN    0x03



/*
 * APEX_CONFIG1
 * Register Name : APEX_CONFIG1
 */

/*
 * smd_enable
 * 0: Significant Motion Detection not enabled
 * 1: Significant Motion Detection enabled
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG1_SMD_ENABLE     0x06
#define ICM42670P_BANK0_MASK_APEX_CONFIG1_SMD_ENABLE    (0x01 << ICM42670P_BANK0_POS_APEX_CONFIG1_SMD_ENABLE)

/*
 * ff_enable
 * 0: Freefall Detection not enabled
 * 1: Freefall Detection enabled
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG1_FF_ENABLE     0x05
#define ICM42670P_BANK0_MASK_APEX_CONFIG1_FF_ENABLE    (0x01 << ICM42670P_BANK0_POS_APEX_CONFIG1_FF_ENABLE)

/*
 * tilt_enable
 * 0: Tilt Detection not enabled
 * 1: Tilt Detection enabled
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG1_TILT_ENABLE     0x04
#define ICM42670P_BANK0_MASK_APEX_CONFIG1_TILT_ENABLE    (0x01 << ICM42670P_BANK0_POS_APEX_CONFIG1_TILT_ENABLE)

/*
 * ped_enable
 * 0: Pedometer not enabled
 * 1: Pedometer enabled
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG1_PED_ENABLE     0x03
#define ICM42670P_BANK0_MASK_APEX_CONFIG1_PED_ENABLE    (0x01 << ICM42670P_BANK0_POS_APEX_CONFIG1_PED_ENABLE)

/*
 * dmp_odr
 * 00: 25 Hz
 * 01: 400 Hz
 * 10: 50 Hz
 * 11: 100 Hz
 *
 * The ACCEL_ODR field must be configured to an ODR equal or greater to the DMP_ODR field, for correct device operation.
 *
 * This field can be changed on-the-fly even if accel sensor is on
 */
#define ICM42670P_BANK0_POS_APEX_CONFIG1_DMP_ODR     0x00
#define ICM42670P_BANK0_MASK_APEX_CONFIG1_DMP_ODR    0x03



/*
 * WOM_CONFIG
 * Register Name : WOM_CONFIG
 */

/*
 * wom_int_dur
 * Selects Wake on Motion interrupt assertion from among the following options
 *
 * 00: WoM interrupt asserted at first overthreshold event
 * 01: WoM interrupt asserted at second overthreshold event
 * 10: WoM interrupt asserted at third overthreshold event
 * 11: WoM interrupt asserted at fourth overthreshold event
 *
 * This field can be changed on-the-fly even if accel sensor is on, but it cannot be changed if WOM_EN is already enabled
 */
#define ICM42670P_BANK0_POS_WOM_CONFIG_WOM_INT_DUR     0x03
#define ICM42670P_BANK0_MASK_WOM_CONFIG_WOM_INT_DUR    (0x03 << ICM42670P_BANK0_POS_WOM_CONFIG_WOM_INT_DUR)

/*
 * wom_int_mode
 * 0: Set WoM interrupt on the OR of all enabled accelerometer thresholds
 * 1: Set WoM interrupt on the AND of all enabled accelerometer thresholds
 *
 * This field can be changed on-the-fly even if accel sensor is on, but it cannot be changed if WOM_EN is already enabled
 */
#define ICM42670P_BANK0_POS_WOM_CONFIG_WOM_INT_MODE     0x02
#define ICM42670P_BANK0_MASK_WOM_CONFIG_WOM_INT_MODE    (0x01 << ICM42670P_BANK0_POS_WOM_CONFIG_WOM_INT_MODE)

/*
 * wom_mode
 * 0 - Initial sample is stored. Future samples are compared to initial sample
 * 1 - Compare current sample to previous sample
 *
 * This field can be changed on-the-fly even if accel sensor is already on, but it cannot be changed if wom_en is already enabled.
 */
#define ICM42670P_BANK0_POS_WOM_CONFIG_WOM_MODE     0x01
#define ICM42670P_BANK0_MASK_WOM_CONFIG_WOM_MODE    (0x01 << ICM42670P_BANK0_POS_WOM_CONFIG_WOM_MODE)

/*
 * wom_en
 * 1: enable wake-on-motion detection.
 * 0: disable wake-on-motion detection.
 *
 * This field can be changed on-the-fly even if accel sensor is already on.
 */
#define ICM42670P_BANK0_POS_WOM_CONFIG_WOM_EN     0x00
#define ICM42670P_BANK0_MASK_WOM_CONFIG_WOM_EN    0x01



/*
 * FIFO_CONFIG1
 * Register Name : FIFO_CONFIG1
 */

/*
 * fifo_mode
 * FIFO mode control
 *
 * 0: Stream-to-FIFO Mode
 * 1: STOP-on-FULL Mode
 */
#define ICM42670P_BANK0_POS_FIFO_CONFIG1_FIFO_MODE     0x01
#define ICM42670P_BANK0_MASK_FIFO_CONFIG1_FIFO_MODE    (0x01 << ICM42670P_BANK0_POS_FIFO_CONFIG1_FIFO_MODE)

/*
 * fifo_bypass
 * FIFO bypass control
 * 0: FIFO is not bypassed
 * 1: FIFO is bypassed
 */
#define ICM42670P_BANK0_POS_FIFO_CONFIG1_FIFO_BYPASS     0x00
#define ICM42670P_BANK0_MASK_FIFO_CONFIG1_FIFO_BYPASS    0x01



/*
 * FIFO_CONFIG2
 * Register Name : FIFO_CONFIG2
 */

/*
 * fifo_wm
 * FIFO watermark. Generate interrupt when the FIFO reaches or exceeds FIFO_WM size in bytes or records according to FIFO_COUNT_FORMAT setting. FIFO_WM_EN must be zero before writing this register. Interrupt only fires once. This register should be set to non-zero value, before choosing this interrupt source.
 *
 * This field should be changed when FIFO is empty to avoid spurious interrupts.
 */
#define ICM42670P_BANK0_POS_FIFO_CONFIG2_FIFO_WM     0x00
#define ICM42670P_BANK0_MASK_FIFO_CONFIG2_FIFO_WM    0xff



/*
 * FIFO_CONFIG3
 * Register Name : FIFO_CONFIG3
 */

/*
 * fifo_wm
 * FIFO watermark. Generate interrupt when the FIFO reaches or exceeds FIFO_WM size in bytes or records according to FIFO_COUNT_FORMAT setting. FIFO_WM_EN must be zero before writing this register. Interrupt only fires once. This register should be set to non-zero value, before choosing this interrupt source.
 *
 * This field should be changed when FIFO is empty to avoid spurious interrupts.
 */
#define ICM42670P_BANK0_POS_FIFO_CONFIG3_FIFO_WM     0x00
#define ICM42670P_BANK0_MASK_FIFO_CONFIG3_FIFO_WM    0x0f



/*
 * INT_SOURCE0
 * Register Name : INT_SOURCE0
 */

/*
 * st_int1_en
 * 0: Self-Test Done interrupt not routed to INT1
 * 1: Self-Test Done interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_ST_INT1_EN     0x07
#define ICM42670P_BANK0_MASK_INT_SOURCE0_ST_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_ST_INT1_EN)

/*
 * fsync_int1_en
 * 0: FSYNC interrupt not routed to INT1
 * 1: FSYNC interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_FSYNC_INT1_EN     0x06
#define ICM42670P_BANK0_MASK_INT_SOURCE0_FSYNC_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_FSYNC_INT1_EN)

/*
 * pll_rdy_int1_en
 * 0: PLL ready interrupt not routed to INT1
 * 1: PLL ready interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_PLL_RDY_INT1_EN     0x05
#define ICM42670P_BANK0_MASK_INT_SOURCE0_PLL_RDY_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_PLL_RDY_INT1_EN)

/*
 * reset_done_int1_en
 * 0: Reset done interrupt not routed to INT1
 * 1: Reset done interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_RESET_DONE_INT1_EN     0x04
#define ICM42670P_BANK0_MASK_INT_SOURCE0_RESET_DONE_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_RESET_DONE_INT1_EN)

/*
 * drdy_int1_en
 * 0: Data Ready interrupt not routed to INT1
 * 1: Data Ready interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_DRDY_INT1_EN     0x03
#define ICM42670P_BANK0_MASK_INT_SOURCE0_DRDY_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_DRDY_INT1_EN)

/*
 * fifo_ths_int1_en
 * 0: FIFO threshold interrupt not routed to INT1
 * 1: FIFO threshold interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_FIFO_THS_INT1_EN     0x02
#define ICM42670P_BANK0_MASK_INT_SOURCE0_FIFO_THS_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_FIFO_THS_INT1_EN)

/*
 * fifo_full_int1_en
 * 0: FIFO full interrupt not routed to INT1
 * 1: FIFO full interrupt routed to INT1
 * To avoid FIFO FULL interrupts while reading FIFO, this bit should be disabled while reading FIFO
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_FIFO_FULL_INT1_EN     0x01
#define ICM42670P_BANK0_MASK_INT_SOURCE0_FIFO_FULL_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE0_FIFO_FULL_INT1_EN)

/*
 * agc_rdy_int1_en
 * 0: UI AGC ready interrupt not routed to INT1
 * 1: UI AGC ready interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE0_AGC_RDY_INT1_EN     0x00
#define ICM42670P_BANK0_MASK_INT_SOURCE0_AGC_RDY_INT1_EN    0x01



/*
 * INT_SOURCE1
 * Register Name : INT_SOURCE1
 */

/*
 * i3c_protocol_error_int1_en
 * 0: I3CSM protocol error interrupt not routed to INT1
 * 1: I3CSM protocol error interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE1_I3C_PROTOCOL_ERROR_INT1_EN     0x06
#define ICM42670P_BANK0_MASK_INT_SOURCE1_I3C_PROTOCOL_ERROR_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE1_I3C_PROTOCOL_ERROR_INT1_EN)

/*
 * smd_int1_en
 * 0: SMD interrupt not routed to INT1
 * 1: SMD interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE1_SMD_INT1_EN     0x03
#define ICM42670P_BANK0_MASK_INT_SOURCE1_SMD_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE1_SMD_INT1_EN)

/*
 * wom_z_int1_en
 * 0: Z-axis WOM interrupt not routed to INT1
 * 1: Z-axis WOM interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE1_WOM_Z_INT1_EN     0x02
#define ICM42670P_BANK0_MASK_INT_SOURCE1_WOM_Z_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE1_WOM_Z_INT1_EN)

/*
 * wom_y_int1_en
 * 0: Y-axis WOM interrupt not routed to INT1
 * 1: Y-axis WOM interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE1_WOM_Y_INT1_EN     0x01
#define ICM42670P_BANK0_MASK_INT_SOURCE1_WOM_Y_INT1_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE1_WOM_Y_INT1_EN)

/*
 * wom_x_int1_en
 * 0: X-axis WOM interrupt not routed to INT1
 * 1: X-axis WOM interrupt routed to INT1
 */
#define ICM42670P_BANK0_POS_INT_SOURCE1_WOM_X_INT1_EN     0x00
#define ICM42670P_BANK0_MASK_INT_SOURCE1_WOM_X_INT1_EN    0x01



/*
 * INT_SOURCE3
 * Register Name : INT_SOURCE3
 */

/*
 * st_int2_en
 * 0: Self-Test Done interrupt not routed to INT2
 * 1: Self-Test Done interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_ST_INT2_EN     0x07
#define ICM42670P_BANK0_MASK_INT_SOURCE3_ST_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_ST_INT2_EN)

/*
 * fsync_int2_en
 * 0: FSYNC interrupt not routed to INT2
 * 1: FSYNC interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_FSYNC_INT2_EN     0x06
#define ICM42670P_BANK0_MASK_INT_SOURCE3_FSYNC_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_FSYNC_INT2_EN)

/*
 * pll_rdy_int2_en
 * 0: PLL ready interrupt not routed to INT2
 * 1: PLL ready interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_PLL_RDY_INT2_EN     0x05
#define ICM42670P_BANK0_MASK_INT_SOURCE3_PLL_RDY_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_PLL_RDY_INT2_EN)

/*
 * reset_done_int2_en
 * 0: Reset done interrupt not routed to INT2
 * 1: Reset done interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_RESET_DONE_INT2_EN     0x04
#define ICM42670P_BANK0_MASK_INT_SOURCE3_RESET_DONE_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_RESET_DONE_INT2_EN)

/*
 * drdy_int2_en
 * 0: Data Ready interrupt not routed to INT2
 * 1: Data Ready interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_DRDY_INT2_EN     0x03
#define ICM42670P_BANK0_MASK_INT_SOURCE3_DRDY_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_DRDY_INT2_EN)

/*
 * fifo_ths_int2_en
 * 0: FIFO threshold interrupt not routed to INT2
 * 1: FIFO threshold interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_FIFO_THS_INT2_EN     0x02
#define ICM42670P_BANK0_MASK_INT_SOURCE3_FIFO_THS_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_FIFO_THS_INT2_EN)

/*
 * fifo_full_int2_en
 * 0: FIFO full interrupt not routed to INT2
 * 1: FIFO full interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_FIFO_FULL_INT2_EN     0x01
#define ICM42670P_BANK0_MASK_INT_SOURCE3_FIFO_FULL_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE3_FIFO_FULL_INT2_EN)

/*
 * agc_rdy_int2_en
 * 0: AGC ready interrupt not routed to INT2
 * 1: AGC ready interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE3_AGC_RDY_INT2_EN     0x00
#define ICM42670P_BANK0_MASK_INT_SOURCE3_AGC_RDY_INT2_EN    0x01



/*
 * INT_SOURCE4
 * Register Name : INT_SOURCE4
 */

/*
 * i3c_protocol_error_int2_en
 * 0: I3CSM protocol error interrupt not routed to INT2
 * 1: I3CSM protocol error interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE4_I3C_PROTOCOL_ERROR_INT2_EN     0x06
#define ICM42670P_BANK0_MASK_INT_SOURCE4_I3C_PROTOCOL_ERROR_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE4_I3C_PROTOCOL_ERROR_INT2_EN)

/*
 * smd_int2_en
 * 0: SMD interrupt not routed to INT2
 * 1: SMD interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE4_SMD_INT2_EN     0x03
#define ICM42670P_BANK0_MASK_INT_SOURCE4_SMD_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE4_SMD_INT2_EN)

/*
 * wom_z_int2_en
 * 0: Z-axis WOM interrupt not routed to INT2
 * 1: Z-axis WOM interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE4_WOM_Z_INT2_EN     0x02
#define ICM42670P_BANK0_MASK_INT_SOURCE4_WOM_Z_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE4_WOM_Z_INT2_EN)

/*
 * wom_y_int2_en
 * 0: Y-axis WOM interrupt not routed to INT2
 * 1: Y-axis WOM interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE4_WOM_Y_INT2_EN     0x01
#define ICM42670P_BANK0_MASK_INT_SOURCE4_WOM_Y_INT2_EN    (0x01 << ICM42670P_BANK0_POS_INT_SOURCE4_WOM_Y_INT2_EN)

/*
 * wom_x_int2_en
 * 0: X-axis WOM interrupt not routed to INT2
 * 1: X-axis WOM interrupt routed to INT2
 */
#define ICM42670P_BANK0_POS_INT_SOURCE4_WOM_X_INT2_EN     0x00
#define ICM42670P_BANK0_MASK_INT_SOURCE4_WOM_X_INT2_EN    0x01



/*
 * FIFO_LOST_PKT0
 * Register Name : FIFO_LOST_PKT0
 */

/*
 * fifo_lost_pkt_cnt
 * Stores the  number of packets lost in the FIFO
 */
#define ICM42670P_BANK0_POS_FIFO_LOST_PKT0_FIFO_LOST_PKT_CNT     0x00
#define ICM42670P_BANK0_MASK_FIFO_LOST_PKT0_FIFO_LOST_PKT_CNT    0xff



/*
 * FIFO_LOST_PKT1
 * Register Name : FIFO_LOST_PKT1
 */

/*
 * fifo_lost_pkt_cnt
 * Stores the  number of packets lost in the FIFO
 */
#define ICM42670P_BANK0_POS_FIFO_LOST_PKT1_FIFO_LOST_PKT_CNT     0x00
#define ICM42670P_BANK0_MASK_FIFO_LOST_PKT1_FIFO_LOST_PKT_CNT    0xff



/*
 * APEX_DATA0
 * Register Name : APEX_DATA0
 */

/*
 * step_cnt
 * This status register indicates number of step taken.
 */
#define ICM42670P_BANK0_POS_APEX_DATA0_STEP_CNT     0x00
#define ICM42670P_BANK0_MASK_APEX_DATA0_STEP_CNT    0xff



/*
 * APEX_DATA1
 * Register Name : APEX_DATA1
 */

/*
 * step_cnt
 * This status register indicates number of step taken.
 */
#define ICM42670P_BANK0_POS_APEX_DATA1_STEP_CNT     0x00
#define ICM42670P_BANK0_MASK_APEX_DATA1_STEP_CNT    0xff



/*
 * APEX_DATA2
 * Register Name : APEX_DATA2
 */

/*
 * step_cadence
 * Pedometer step cadence.Walk/run cadency in number of samples. Format is u6.2.
 * E.g, At 50Hz and 2Hz walk frequency, the cadency is 25 samples. The register will output 100.
 */
#define ICM42670P_BANK0_POS_APEX_DATA2_STEP_CADENCE     0x00
#define ICM42670P_BANK0_MASK_APEX_DATA2_STEP_CADENCE    0xff



/*
 * APEX_DATA3
 * Register Name : APEX_DATA3
 */

/*
 * dmp_idle
 * 0: Indicates DMP is running
 * 1: Indicates DMP is idle
 */
#define ICM42670P_BANK0_POS_APEX_DATA3_DMP_IDLE     0x02
#define ICM42670P_BANK0_MASK_APEX_DATA3_DMP_IDLE    (0x01 << ICM42670P_BANK0_POS_APEX_DATA3_DMP_IDLE)

/*
 * activity_class
 * Pedometer Output: Detected activity
 *
 * 00: Unknown
 * 01: Walk
 * 10: Run
 * 11: Reserved
 */
#define ICM42670P_BANK0_POS_APEX_DATA3_ACTIVITY_CLASS     0x00
#define ICM42670P_BANK0_MASK_APEX_DATA3_ACTIVITY_CLASS    0x03



/*
 * INTF_CONFIG0
 * Register Name : INTF_CONFIG0
 */

/*
 * fifo_count_format
 * 0: FIFO count is reported in bytes
 * 1: FIFO count is reported in records (1 record = 16 bytes for header + gyro + accel + temp sensor data + time stamp, or 8 bytes for header + gyro/accel + temp sensor data)
 */
#define ICM42670P_BANK0_POS_INTF_CONFIG0_FIFO_COUNT_FORMAT     0x06
#define ICM42670P_BANK0_MASK_INTF_CONFIG0_FIFO_COUNT_FORMAT    (0x01 << ICM42670P_BANK0_POS_INTF_CONFIG0_FIFO_COUNT_FORMAT)

/*
 * fifo_count_endian
 * This bit applies to both fifo_count and lost_pkt_count
 * 0 : Little Endian (The LSByte data is read first, followed by MSByte data).
 * 1 : Big Endian (The MSByte data is read first, followed by LSByte data).
 */
#define ICM42670P_BANK0_POS_INTF_CONFIG0_FIFO_COUNT_ENDIAN     0x05
#define ICM42670P_BANK0_MASK_INTF_CONFIG0_FIFO_COUNT_ENDIAN    (0x01 << ICM42670P_BANK0_POS_INTF_CONFIG0_FIFO_COUNT_ENDIAN)

/*
 * sensor_data_endian
 * This bit applies to sensor data to AP, and fifo data.
 * 0 : Little Endian (The LSByte data is read first, followed by MSByte data).
 * 1 : Big Endian (The MSByte data is read first, followed by LSByte data).
 */
#define ICM42670P_BANK0_POS_INTF_CONFIG0_SENSOR_DATA_ENDIAN     0x04
#define ICM42670P_BANK0_MASK_INTF_CONFIG0_SENSOR_DATA_ENDIAN    (0x01 << ICM42670P_BANK0_POS_INTF_CONFIG0_SENSOR_DATA_ENDIAN)

/*
 * INTF_CONFIG1
 * Register Name : INTF_CONFIG1
 */

/*
 * i3c_sdr_en
 * 0: I3CSM SDR mode not enabled
 * 1: I3CSM SDR mode enabled
 *
 * Device will be in pure I2C mode if {I3C_SDR_EN, I3C_DDR_EN} = 00
 */
#define ICM42670P_BANK0_POS_INTF_CONFIG1_I3C_SDR_EN     0x03
#define ICM42670P_BANK0_MASK_INTF_CONFIG1_I3C_SDR_EN    (0x01 << ICM42670P_BANK0_POS_INTF_CONFIG1_I3C_SDR_EN)

/*
 * i3c_ddr_en
 * 0: I3CSM DDR mode not enabled
 * 1: I3CSM DDR mode enabled
 *
 * This bit will not take effect unless I3C_SDR_EN = 1.
 */
#define ICM42670P_BANK0_POS_INTF_CONFIG1_I3C_DDR_EN     0x02
#define ICM42670P_BANK0_MASK_INTF_CONFIG1_I3C_DDR_EN    (0x01 << ICM42670P_BANK0_POS_INTF_CONFIG1_I3C_DDR_EN)

/*
 * clksel
 * 00 Alway select internal RC oscillator
 * 01 Select PLL when available, else select RC oscillator (default)
 * 10 (Reserved)
 * 11 Disable all clocks
 */
#define ICM42670P_BANK0_POS_INTF_CONFIG1_CLKSEL     0x00
#define ICM42670P_BANK0_MASK_INTF_CONFIG1_CLKSEL    0x03



/*
 * INT_STATUS_DRDY
 * Register Name : INT_STATUS_DRDY
 */

/*
 * data_rdy_int
 * This bit automatically sets to 1 when a Data Ready interrupt is generated. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_DRDY_DATA_RDY_INT     0x00
#define ICM42670P_BANK0_MASK_INT_STATUS_DRDY_DATA_RDY_INT    0x01



/*
 * INT_STATUS
 * Register Name : INT_STATUS
 */

/*
 * st_int
 * This bit automatically sets to 1 when a Self Test done interrupt is generated. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_ST_INT     0x07
#define ICM42670P_BANK0_MASK_INT_STATUS_ST_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS_ST_INT)

/*
 * fsync_int
 * This bit automatically sets to 1 when an FSYNC interrupt is generated. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_FSYNC_INT     0x06
#define ICM42670P_BANK0_MASK_INT_STATUS_FSYNC_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS_FSYNC_INT)

/*
 * pll_rdy_int
 * This bit automatically sets to 1 when a PLL Ready interrupt is generated. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_PLL_RDY_INT     0x05
#define ICM42670P_BANK0_MASK_INT_STATUS_PLL_RDY_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS_PLL_RDY_INT)

/*
 * reset_done_int
 * This bit automatically sets to 1 when software reset is complete. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_RESET_DONE_INT     0x04
#define ICM42670P_BANK0_MASK_INT_STATUS_RESET_DONE_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS_RESET_DONE_INT)

/*
 * fifo_ths_int
 * This bit automatically sets to 1 when the FIFO buffer reaches the threshold value. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_FIFO_THS_INT     0x02
#define ICM42670P_BANK0_MASK_INT_STATUS_FIFO_THS_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS_FIFO_THS_INT)

/*
 * fifo_full_int
 * This bit automatically sets to 1 when the FIFO buffer is full. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_FIFO_FULL_INT     0x01
#define ICM42670P_BANK0_MASK_INT_STATUS_FIFO_FULL_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS_FIFO_FULL_INT)

/*
 * agc_rdy_int
 * This bit automatically sets to 1 when an AGC Ready interrupt is generated. The bit clears to 0 after the register has been read.
 */
#define ICM42670P_BANK0_POS_INT_STATUS_AGC_RDY_INT     0x00
#define ICM42670P_BANK0_MASK_INT_STATUS_AGC_RDY_INT    0x01



/*
 * INT_STATUS2
 * Register Name : INT_STATUS2
 */

/*
 * smd_int
 * Significant Motion Detection Interrupt, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS2_SMD_INT     0x03
#define ICM42670P_BANK0_MASK_INT_STATUS2_SMD_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS2_SMD_INT)

/*
 * wom_x_int
 * Wake on Motion Interrupt on X-axis, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS2_WOM_X_INT     0x02
#define ICM42670P_BANK0_MASK_INT_STATUS2_WOM_X_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS2_WOM_X_INT)

/*
 * wom_y_int
 * Wake on Motion Interrupt on Y-axis, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS2_WOM_Y_INT     0x01
#define ICM42670P_BANK0_MASK_INT_STATUS2_WOM_Y_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS2_WOM_Y_INT)

/*
 * wom_z_int
 * Wake on Motion Interrupt on Z-axis, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS2_WOM_Z_INT     0x00
#define ICM42670P_BANK0_MASK_INT_STATUS2_WOM_Z_INT    0x01



/*
 * INT_STATUS3
 * Register Name : INT_STATUS3
 */

/*
 * step_det_int
 * Step Detection Interrupt, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS3_STEP_DET_INT     0x05
#define ICM42670P_BANK0_MASK_INT_STATUS3_STEP_DET_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS3_STEP_DET_INT)

/*
 * step_cnt_ovf_int
 * Step Count Overflow Interrupt, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS3_STEP_CNT_OVF_INT     0x04
#define ICM42670P_BANK0_MASK_INT_STATUS3_STEP_CNT_OVF_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS3_STEP_CNT_OVF_INT)

/*
 * tilt_det_int
 * Tilt Detection Interrupt, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS3_TILT_DET_INT     0x03
#define ICM42670P_BANK0_MASK_INT_STATUS3_TILT_DET_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS3_TILT_DET_INT)

/*
 * ff_det_int
 * Freefall Interrupt, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS3_FF_DET_INT     0x02
#define ICM42670P_BANK0_MASK_INT_STATUS3_FF_DET_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS3_FF_DET_INT)

/*
 * lowg_det_int
 * LowG Interrupt, clears on read
 */
#define ICM42670P_BANK0_POS_INT_STATUS3_LOWG_DET_INT     0x01
#define ICM42670P_BANK0_MASK_INT_STATUS3_LOWG_DET_INT    (0x01 << ICM42670P_BANK0_POS_INT_STATUS3_LOWG_DET_INT)



/*
 * FIFO_COUNTH
 * Register Name : FIFO_COUNTH
 */

/*
 * fifo_count
 * Number of bytes in FIFO when fifo_count_format=0.
 * Number of records in FIFO when fifo_count_format=1.
 */
#define ICM42670P_BANK0_POS_FIFO_COUNTH_FIFO_COUNT     0x00
#define ICM42670P_BANK0_MASK_FIFO_COUNTH_FIFO_COUNT    0xff



/*
 * FIFO_COUNTL
 * Register Name : FIFO_COUNTL
 */

/*
 * fifo_count
 * Number of bytes in FIFO when fifo_count_format=0.
 * Number of records in FIFO when fifo_count_format=1.
 */
#define ICM42670P_BANK0_POS_FIFO_COUNTL_FIFO_COUNT     0x00
#define ICM42670P_BANK0_MASK_FIFO_COUNTL_FIFO_COUNT    0xff



/*
 * FIFO_DATA
 * Register Name : FIFO_DATA
 */

/*
 * fifo_data
 * FIFO data port
 */
#define ICM42670P_BANK0_POS_FIFO_DATA_FIFO_DATA     0x00
#define ICM42670P_BANK0_MASK_FIFO_DATA_FIFO_DATA    0xff



/*
 * WHO_AM_I
 * Register Name : WHO_AM_I
 */

/*
 * whoami
 * Register to indicate to user which device is being accessed
 */
#define ICM42670P_BANK0_POS_WHO_AM_I_WHOAMI     0x00
#define ICM42670P_BANK0_MASK_WHO_AM_I_WHOAMI    0xff



/*
 * BLK_SEL_W
 * Register Name : BLK_SEL_W
 */

/*
 * blk_sel_w
 * For write operation, select a 256-byte MCLK space, or 128-byte SCLK space.
 * Automatically reset when OTP copy operation is triggered.
 */
#define ICM42670P_BANK0_POS_BLK_SEL_W_BLK_SEL_W     0x00
#define ICM42670P_BANK0_MASK_BLK_SEL_W_BLK_SEL_W    0xff



/*
 * MADDR_W
 * Register Name : MADDR_W
 */

/*
 * maddr_w
 * For MREG write operation, the lower 8-bit address for accessing MCLK domain registers.
 */
#define ICM42670P_BANK0_POS_MADDR_W_MADDR_W     0x00
#define ICM42670P_BANK0_MASK_MADDR_W_MADDR_W    0xff



/*
 * M_W
 * Register Name : M_W
 */

/*
 * m_w
 * For MREG write operation, the write port for accessing MCLK domain registers.
 */
#define ICM42670P_BANK0_POS_M_W_M_W     0x00
#define ICM42670P_BANK0_MASK_M_W_M_W    0xff



/*
 * BLK_SEL_R
 * Register Name : BLK_SEL_R
 */

/*
 * blk_sel_r
 * For read operation, select a 256-byte MCLK space, or 128-byte SCLK space.
 * Automatically reset when OTP copy operation is triggered.
 */
#define ICM42670P_BANK0_POS_BLK_SEL_R_BLK_SEL_R     0x00
#define ICM42670P_BANK0_MASK_BLK_SEL_R_BLK_SEL_R    0



/****************************************/
//	MREG1 Bits Position
/****************************************/

/*
 * TMST_CONFIG1
 * Register Name : TMST_CONFIG1
 */

/*
 * tmst_res
 * Time Stamp resolution; When set to 0 (default), time stamp resolution is 1 us. When set to 1, resolution is 16us
 */
#define ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_RES     0x03
#define ICM42670P_MREG1_MASK_TMST_CONFIG1_TMST_RES    (0x01 << ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_RES)

/*
 * tmst_delta_en
 * Time Stamp delta Enable : When set to 1, the Time stamp field contains the  measurement of time since  the last occurrence of ODR.
 */
#define ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_DELTA_EN     0x02
#define ICM42670P_MREG1_MASK_TMST_CONFIG1_TMST_DELTA_EN    (0x01 << ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_DELTA_EN)

/*
 * tmst_fsync_en
 * Time Stamp register Fsync Enable . When set to 1, the contents of the Timestamp feature of  FSYNC  is enabled. The  user also needs to select  fifo_tmst_fsync_en in order to propagate the timestamp value to the  FIFO
 */
#define ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_FSYNC_EN     0x01
#define ICM42670P_MREG1_MASK_TMST_CONFIG1_TMST_FSYNC_EN    (0x01 << ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_FSYNC_EN)

/*
 * tmst_en
 * Time Stamp register Enable
 */
#define ICM42670P_MREG1_POS_TMST_CONFIG1_TMST_EN     0x00
#define ICM42670P_MREG1_MASK_TMST_CONFIG1_TMST_EN    0x01



/*
 * FIFO_CONFIG5
 * Register Name : FIFO_CONFIG5
 */

/*
 * fifo_wm_gt_th
 * 1: trigger FIFO-Watermark interrupt on every ODR(DMA Write)  if FIFO_COUNT: =FIFO_WM
 *
 * 0: Trigger FIFO-Watermark interrupt when FIFO_COUNT == FIFO_WM
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_WM_GT_TH     0x05
#define ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_WM_GT_TH    (0x01 << ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_WM_GT_TH)

/*
 * fifo_resume_partial_rd
 * 0: FIFO is read in packets. If a partial packet is read, then the subsequent read will start from the beginning of the un-read packet.
 * 1: FIFO can be read partially. When read is resumed, FIFO bytes will  continue  from last read point. The  SW driver is responsible for cascading previous read and present read and maintain frame boundaries.
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_RESUME_PARTIAL_RD     0x04
#define ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_RESUME_PARTIAL_RD    (0x01 << ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_RESUME_PARTIAL_RD)

/*
 * fifo_hires_en
 * Allows 20 bit resolution in the  FIFO packet readout
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_HIRES_EN     0x03
#define ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_HIRES_EN    (0x01 << ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_HIRES_EN)

/*
 * fifo_tmst_fsync_en
 * Allows the TMST in the FIFO to be replaced by the FSYNC timestamp
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_TMST_FSYNC_EN     0x02
#define ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_TMST_FSYNC_EN    (0x01 << ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_TMST_FSYNC_EN)

/*
 * fifo_gyro_en
 * Enables Gyro Packets to go to FIFO
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_GYRO_EN     0x01
#define ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_GYRO_EN    (0x01 << ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_GYRO_EN)

/*
 * fifo_accel_en
 * Enable Accel Packets to go to FIFO
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG5_FIFO_ACCEL_EN     0x00
#define ICM42670P_MREG1_MASK_FIFO_CONFIG5_FIFO_ACCEL_EN    0x01



/*
 * FIFO_CONFIG6
 * Register Name : FIFO_CONFIG6
 */

/*
 * fifo_empty_indicator_dis
 * 0: xFF is sent out as FIFO data when FIFO is empty.
 * 1: The last FIFO data is sent out when FIFO is empty.
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG6_FIFO_EMPTY_INDICATOR_DIS     0x04
#define ICM42670P_MREG1_MASK_FIFO_CONFIG6_FIFO_EMPTY_INDICATOR_DIS    (0x01 << ICM42670P_MREG1_POS_FIFO_CONFIG6_FIFO_EMPTY_INDICATOR_DIS)

/*
 * rcosc_req_on_fifo_ths_dis
 * 0: When the FIFO is operating in ALP+WUOSC mode and the watermark (WM) interrupt is enabled, the FIFO wakes up the system oscillator (RCOSC) as soon as the watermark level is reached. The system oscillator remains enabled until a Host FIFO read operation happens. This will temporarily cause a small increase in the power consumption due to the enabling of the system oscillator.
 * 1: The system oscillator is not automatically woken-up by the FIFO/INT when the WM interrupt is triggered. The side effect is that the host can receive invalid packets until the system oscillator is off after it has been turned on for other reasons not related to a WM interrupt.
 *
 * The recommended setting of this bit is ‘1’ before entering and during all power modes excluding ALP with WUOSC. This is in order to avoid having to do a FIFO access/flush before entering sleep mode. During ALP with WUOSC it is recommended to set this bit to ‘0’. It is recommended to reset this bit back to ‘1’ before exiting ALP+WUOSC with a wait time of 1 ODR or higher.
 */
#define ICM42670P_MREG1_POS_FIFO_CONFIG6_RCOSC_REQ_ON_FIFO_THS_DIS     0x00
#define ICM42670P_MREG1_MASK_FIFO_CONFIG6_RCOSC_REQ_ON_FIFO_THS_DIS    0x01



/*
 * FSYNC_CONFIG
 * Register Name : FSYNC_CONFIG
 */

/*
 * fsync_ui_sel
 * this register was called (ext_sync_sel)
 * 0 Do not tag Fsync flag
 * 1 Tag Fsync flag to TEMP_OUT’s LSB
 * 2 Tag Fsync flag to GYRO_XOUT’s LSB
 * 3 Tag Fsync flag to GYRO_YOUT’s LSB
 * 4 Tag Fsync flag to GYRO_ZOUT’s LSB
 * 5 Tag Fsync flag to ACCEL_XOUT’s LSB
 * 6 Tag Fsync flag to ACCEL_YOUT’s LSB
 * 7 Tag Fsync flag to ACCEL_ZOUT’s LSB
 */
#define ICM42670P_MREG1_POS_FSYNC_CONFIG_FSYNC_UI_SEL     0x04
#define ICM42670P_MREG1_MASK_FSYNC_CONFIG_FSYNC_UI_SEL    (0x07 << ICM42670P_MREG1_POS_FSYNC_CONFIG_FSYNC_UI_SEL)

/*
 * fsync_ui_flag_clear_sel
 * 0 means the FSYNC flag is cleared when UI sensor reg is updated
 * 1 means the FSYNC flag is cleared when UI interface  reads the sensor register LSB of FSYNC tagged axis
 */
#define ICM42670P_MREG1_POS_FSYNC_CONFIG_FSYNC_UI_FLAG_CLEAR_SEL     0x01
#define ICM42670P_MREG1_MASK_FSYNC_CONFIG_FSYNC_UI_FLAG_CLEAR_SEL    (0x01 << ICM42670P_MREG1_POS_FSYNC_CONFIG_FSYNC_UI_FLAG_CLEAR_SEL)

/*
 * fsync_polarity
 * 0: Start from Rising edge of FSYNC pulse to measure FSYNC interval
 * 1: Start from Falling edge of FSYNC pulse to measure FSYNC interval
 */
#define ICM42670P_MREG1_POS_FSYNC_CONFIG_FSYNC_POLARITY     0x00
#define ICM42670P_MREG1_MASK_FSYNC_CONFIG_FSYNC_POLARITY    0x01



/*
 * INT_CONFIG0
 * Register Name : INT_CONFIG0
 */

/*
 * ui_drdy_int_clear
 * Data Ready Interrupt Clear Option (latched mode)
 * 00: Clear on Status Bit Read
 * 01: Clear on Status Bit Read
 * 10: Clear on Sensor Register Read
 * 11: Clear on Status Bit Read OR on Sensor Register read
 */
#define ICM42670P_MREG1_POS_INT_CONFIG0_UI_DRDY_INT_CLEAR     0x04
#define ICM42670P_MREG1_MASK_INT_CONFIG0_UI_DRDY_INT_CLEAR    (0x03 << ICM42670P_MREG1_POS_INT_CONFIG0_UI_DRDY_INT_CLEAR)

/*
 * fifo_ths_int_clear
 * FIFO Threshold Interrupt Clear Option (latched mode)
 * 00: Clear on Status Bit Read
 * 01: Clear on Status Bit Read
 * 10: Clear on FIFO data 1Byte Read
 * 11: Clear on Status Bit Read OR on FIFO data 1 byte read
 */
#define ICM42670P_MREG1_POS_INT_CONFIG0_FIFO_THS_INT_CLEAR     0x02
#define ICM42670P_MREG1_MASK_INT_CONFIG0_FIFO_THS_INT_CLEAR    (0x03 << ICM42670P_MREG1_POS_INT_CONFIG0_FIFO_THS_INT_CLEAR)

/*
 * fifo_full_int_clear
 * FIFO Full Interrupt Clear Option (latched mode)
 * 00: Clear on Status Bit Read
 * 01: Clear on Status Bit Read
 * 10: Clear on FIFO data 1Byte Read
 * 11: Clear on Status Bit Read OR on FIFO data 1 byte read
 */
#define ICM42670P_MREG1_POS_INT_CONFIG0_FIFO_FULL_INT_CLEAR     0x00
#define ICM42670P_MREG1_MASK_INT_CONFIG0_FIFO_FULL_INT_CLEAR    0x03



/*
 * INT_CONFIG1
 * Register Name : INT_CONFIG1
 */

/*
 * int_tpulse_duration
 * 0 - (Default) Interrupt pulse duration is 100us
 * 1- Interrupt pulse duration is  8 us
 */
#define ICM42670P_MREG1_POS_INT_CONFIG1_INT_TPULSE_DURATION     0x06
#define ICM42670P_MREG1_MASK_INT_CONFIG1_INT_TPULSE_DURATION    (0x01 << ICM42670P_MREG1_POS_INT_CONFIG1_INT_TPULSE_DURATION)

/*
 * int_async_reset
 * 0: The interrupt pulse is reset as soon as the interrupt status register is read if the pulse is still active.
 * 1: The interrupt pulse remains high for the intended duration independent of when the interrupt status register is read. This is the default and recommended setting. In this case, when in ALP with the WUOSC clock, the clearing of the interrupt status register requires up to one ODR period after reading.
 */
#define ICM42670P_MREG1_POS_INT_CONFIG1_INT_ASYNC_RESET     0x04
#define ICM42670P_MREG1_MASK_INT_CONFIG1_INT_ASYNC_RESET    (0x01 << ICM42670P_MREG1_POS_INT_CONFIG1_INT_ASYNC_RESET)



/*
 * SENSOR_CONFIG3
 * Register Name : SENSOR_CONFIG3
 */

/*
 * apex_disable
 * 1: Disable APEX features to extend FIFO size to 2.25 Kbytes
 */
#define ICM42670P_MREG1_POS_SENSOR_CONFIG3_APEX_DISABLE     0x06
#define ICM42670P_MREG1_MASK_SENSOR_CONFIG3_APEX_DISABLE    (0x01 << ICM42670P_MREG1_POS_SENSOR_CONFIG3_APEX_DISABLE)

/*
 * ST_CONFIG
 * Register Name : ST_CONFIG
 */

/*
 * accel_st_reg
 * User must set this bit to 1 when enabling accelerometer self-test and clear it to 0 when self-test procedure has completed.
 */
#define ICM42670P_MREG1_POS_ST_CONFIG_ACCEL_ST_REG     0x07
#define ICM42670P_MREG1_MASK_ST_CONFIG_ACCEL_ST_REG    (0x01 << ICM42670P_MREG1_POS_ST_CONFIG_ACCEL_ST_REG)

/*
 * st_number_sample
 * This bit selects the number of sensor samples that should be used to process self-test
 * 0: 16 samples
 * 1: 200 samples
 */
#define ICM42670P_MREG1_POS_ST_CONFIG_ST_NUMBER_SAMPLE     0x06
#define ICM42670P_MREG1_MASK_ST_CONFIG_ST_NUMBER_SAMPLE    (0x01 << ICM42670P_MREG1_POS_ST_CONFIG_ST_NUMBER_SAMPLE)

/*
 * accel_st_lim
 * These bits control the tolerated ratio between self-test processed values and reference (fused) ones for accelerometer.
 * 0 : 5%
 * 1: 10%
 * 2: 15%
 * 3: 20%
 * 4: 25%
 * 5: 30%
 * 6: 40%
 * 7: 50%
 */
#define ICM42670P_MREG1_POS_ST_CONFIG_ACCEL_ST_LIM     0x03
#define ICM42670P_MREG1_MASK_ST_CONFIG_ACCEL_ST_LIM    (0x07 << ICM42670P_MREG1_POS_ST_CONFIG_ACCEL_ST_LIM)

/*
 * gyro_st_lim
 * These bits control the tolerated ratio between self-test processed values and reference (fused) ones for gyro.
 * 0 : 5%
 * 1: 10%
 * 2: 15%
 * 3: 20%
 * 4: 25%
 * 5: 30%
 * 6: 40%
 * 7: 50%
 */
#define ICM42670P_MREG1_POS_ST_CONFIG_GYRO_ST_LIM     0x00
#define ICM42670P_MREG1_MASK_ST_CONFIG_GYRO_ST_LIM    0x07



/*
 * SELFTEST
 * Register Name : SELFTEST
 */

/*
 * gyro_st_en
 * 1: enable gyro self test operation. Host needs to program this bit to 0 to move chip out of self test mode. If host programs this bit to 0 while st_busy = 1 and st_done =0, the current running self-test operation is terminated by host.
 */
#define ICM42670P_MREG1_POS_SELFTEST_GYRO_ST_EN     0x07
#define ICM42670P_MREG1_MASK_SELFTEST_GYRO_ST_EN    (0x01 << ICM42670P_MREG1_POS_SELFTEST_GYRO_ST_EN)

/*
 * accel_st_en
 * 1: enable accel self test operation. Host needs to program this bit to 0 to move chip out of self test mode. If host programs this bit to 0 while st_busy = 1 and st_done =0, the current running self-test operation is terminated by host.
 */
#define ICM42670P_MREG1_POS_SELFTEST_ACCEL_ST_EN     0x06
#define ICM42670P_MREG1_MASK_SELFTEST_ACCEL_ST_EN    (0x01 << ICM42670P_MREG1_POS_SELFTEST_ACCEL_ST_EN)

/*
 * en_gz_st
 * Enable Gyro Z-axis self test
 */
#define ICM42670P_MREG1_POS_SELFTEST_EN_GZ_ST     0x05
#define ICM42670P_MREG1_MASK_SELFTEST_EN_GZ_ST    (0x01 << ICM42670P_MREG1_POS_SELFTEST_EN_GZ_ST)

/*
 * en_gy_st
 * Enable Gyro Y-axis self test
 */
#define ICM42670P_MREG1_POS_SELFTEST_EN_GY_ST     0x04
#define ICM42670P_MREG1_MASK_SELFTEST_EN_GY_ST    (0x01 << ICM42670P_MREG1_POS_SELFTEST_EN_GY_ST)

/*
 * en_gx_st
 * Enable Gyro X-axis self test
 */
#define ICM42670P_MREG1_POS_SELFTEST_EN_GX_ST     0x03
#define ICM42670P_MREG1_MASK_SELFTEST_EN_GX_ST    (0x01 << ICM42670P_MREG1_POS_SELFTEST_EN_GX_ST)

/*
 * en_az_st
 * Enable Accel Z-axis self test
 */
#define ICM42670P_MREG1_POS_SELFTEST_EN_AZ_ST     0x02
#define ICM42670P_MREG1_MASK_SELFTEST_EN_AZ_ST    (0x01 << ICM42670P_MREG1_POS_SELFTEST_EN_AZ_ST)

/*
 * en_ay_st
 * Enable Accel Y-axis self test
 */
#define ICM42670P_MREG1_POS_SELFTEST_EN_AY_ST     0x01
#define ICM42670P_MREG1_MASK_SELFTEST_EN_AY_ST    (0x01 << ICM42670P_MREG1_POS_SELFTEST_EN_AY_ST)

/*
 * en_ax_st
 * Enable Accel X-axis self test
 */
#define ICM42670P_MREG1_POS_SELFTEST_EN_AX_ST     0x00
#define ICM42670P_MREG1_MASK_SELFTEST_EN_AX_ST    0x01



/*
 * INTF_CONFIG6
 * Register Name : INTF_CONFIG6
 */

/*
 * i3c_timeout_en
 * Value of 1 to enable i2c/i3c timeout function
 */
#define ICM42670P_MREG1_POS_INTF_CONFIG6_I3C_TIMEOUT_EN     0x04
#define ICM42670P_MREG1_MASK_INTF_CONFIG6_I3C_TIMEOUT_EN    (0x01 << ICM42670P_MREG1_POS_INTF_CONFIG6_I3C_TIMEOUT_EN)

/*
 * i3c_ibi_byte_en
 * I3C Enable IBI-payload function.
 */
#define ICM42670P_MREG1_POS_INTF_CONFIG6_I3C_IBI_BYTE_EN     0x03
#define ICM42670P_MREG1_MASK_INTF_CONFIG6_I3C_IBI_BYTE_EN    (0x01 << ICM42670P_MREG1_POS_INTF_CONFIG6_I3C_IBI_BYTE_EN)

/*
 * i3c_ibi_en
 * I3C Enable IBI function.
 */
#define ICM42670P_MREG1_POS_INTF_CONFIG6_I3C_IBI_EN     0x02
#define ICM42670P_MREG1_MASK_INTF_CONFIG6_I3C_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INTF_CONFIG6_I3C_IBI_EN)



/*
 * INTF_CONFIG10
 * Register Name : INTF_CONFIG10
 */

/*
 * asynctime0_dis
 * 1: Disable asynchronous timing control mode 0 operation.
 */
#define ICM42670P_MREG1_POS_INTF_CONFIG10_ASYNCTIME0_DIS     0x07
#define ICM42670P_MREG1_MASK_INTF_CONFIG10_ASYNCTIME0_DIS    (0x01 << ICM42670P_MREG1_POS_INTF_CONFIG10_ASYNCTIME0_DIS)

/*
 * INTF_CONFIG7
 * Register Name : INTF_CONFIG7
 */

/*
 * i3c_ddr_wr_mode
 * This bit controls how I3C slave treats the 1st 2-byte data from
 *     host in a DDR write operation.
 *
 *     0: (a) The 1st-byte in DDR-WR configures the starting register
 *            address where the write operation should occur.
 *        (b) The 2nd-byte in DDR-WR is ignored and dropped.
 *        (c) The 3rd-byte in DDR-WR will be written into the register
 *            with address specified by the 1st-byte.
 *            Or, the next DDR-RD will be starting from the address
 *            specified by the 1st-byte of previous DDR-WR.
 *
 *     1:  (a) The 1st-byte in DDR-WR configures the starting register
 *             address where the write operation should occur.
 *         (b) The 2nd-byte in DDR-WR will be written into the register
 *             with address specified by the 1st-byte.
 */
#define ICM42670P_MREG1_POS_INTF_CONFIG7_I3C_DDR_WR_MODE     0x03
#define ICM42670P_MREG1_MASK_INTF_CONFIG7_I3C_DDR_WR_MODE    (0x01 << ICM42670P_MREG1_POS_INTF_CONFIG7_I3C_DDR_WR_MODE)

/*
 * OTP_CONFIG
 * Register Name : OTP_CONFIG
 */

/*
 * otp_copy_mode
 * 00: Reserved
 * 01: Enable copying OTP block to SRAM
 * 10: Reserved
 * 11: Enable copying self-test data from OTP memory to SRAM
 */
#define ICM42670P_MREG1_POS_OTP_CONFIG_OTP_COPY_MODE     0x02
#define ICM42670P_MREG1_MASK_OTP_CONFIG_OTP_COPY_MODE    (0x03 << ICM42670P_MREG1_POS_OTP_CONFIG_OTP_COPY_MODE)

/*
 * INT_SOURCE6
 * Register Name : INT_SOURCE6
 */

/*
 * ff_int1_en
 * 0: Freefall interrupt not routed to INT1
 * 1: Freefall interrupt routed to INT1
 */
#define ICM42670P_MREG1_POS_INT_SOURCE6_FF_INT1_EN     0x07
#define ICM42670P_MREG1_MASK_INT_SOURCE6_FF_INT1_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE6_FF_INT1_EN)

/*
 * lowg_int1_en
 * 0: Low-g interrupt not routed to INT1
 * 1: Low-g interrupt routed to INT1
 */
#define ICM42670P_MREG1_POS_INT_SOURCE6_LOWG_INT1_EN     0x06
#define ICM42670P_MREG1_MASK_INT_SOURCE6_LOWG_INT1_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE6_LOWG_INT1_EN)

/*
 * step_det_int1_en
 * 0: Step detect interrupt not routed to INT1
 * 1: Step detect interrupt routed to INT1
 */
#define ICM42670P_MREG1_POS_INT_SOURCE6_STEP_DET_INT1_EN     0x05
#define ICM42670P_MREG1_MASK_INT_SOURCE6_STEP_DET_INT1_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE6_STEP_DET_INT1_EN)

/*
 * step_cnt_ofl_int1_en
 * 0: Step count overflow interrupt not routed to INT1
 * 1: Step count overflow interrupt routed to INT1
 */
#define ICM42670P_MREG1_POS_INT_SOURCE6_STEP_CNT_OFL_INT1_EN     0x04
#define ICM42670P_MREG1_MASK_INT_SOURCE6_STEP_CNT_OFL_INT1_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE6_STEP_CNT_OFL_INT1_EN)

/*
 * tilt_det_int1_en
 * 0: Tilt detect interrupt not routed to INT1
 * 1: Tile detect interrupt routed to INT1
 */
#define ICM42670P_MREG1_POS_INT_SOURCE6_TILT_DET_INT1_EN     0x03
#define ICM42670P_MREG1_MASK_INT_SOURCE6_TILT_DET_INT1_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE6_TILT_DET_INT1_EN)



/*
 * INT_SOURCE7
 * Register Name : INT_SOURCE7
 */

/*
 * ff_int2_en
 * 0: Freefall interrupt not routed to INT2
 * 1: Freefall interrupt routed to INT2
 */
#define ICM42670P_MREG1_POS_INT_SOURCE7_FF_INT2_EN     0x07
#define ICM42670P_MREG1_MASK_INT_SOURCE7_FF_INT2_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE7_FF_INT2_EN)

/*
 * lowg_int2_en
 * 0: Low-g interrupt not routed to INT2
 * 1: Low-g interrupt routed to INT2
 */
#define ICM42670P_MREG1_POS_INT_SOURCE7_LOWG_INT2_EN     0x06
#define ICM42670P_MREG1_MASK_INT_SOURCE7_LOWG_INT2_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE7_LOWG_INT2_EN)

/*
 * step_det_int2_en
 * 0: Step detect interrupt not routed to INT2
 * 1: Step detect interrupt routed to INT2
 */
#define ICM42670P_MREG1_POS_INT_SOURCE7_STEP_DET_INT2_EN     0x05
#define ICM42670P_MREG1_MASK_INT_SOURCE7_STEP_DET_INT2_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE7_STEP_DET_INT2_EN)

/*
 * step_cnt_ofl_int2_en
 * 0: Step count overflow interrupt not routed to INT2
 * 1: Step count overflow interrupt routed to INT2
 */
#define ICM42670P_MREG1_POS_INT_SOURCE7_STEP_CNT_OFL_INT2_EN     0x04
#define ICM42670P_MREG1_MASK_INT_SOURCE7_STEP_CNT_OFL_INT2_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE7_STEP_CNT_OFL_INT2_EN)

/*
 * tilt_det_int2_en
 * 0: Tilt detect interrupt not routed to INT2
 * 1: Tile detect interrupt routed to INT2
 */
#define ICM42670P_MREG1_POS_INT_SOURCE7_TILT_DET_INT2_EN     0x03
#define ICM42670P_MREG1_MASK_INT_SOURCE7_TILT_DET_INT2_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE7_TILT_DET_INT2_EN)



/*
 * INT_SOURCE8
 * Register Name : INT_SOURCE8
 */

/*
 * fsync_ibi_en
 * 0: FSYNC interrupt not routed to IBI
 * 1: FSYNC interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE8_FSYNC_IBI_EN     0x05
#define ICM42670P_MREG1_MASK_INT_SOURCE8_FSYNC_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE8_FSYNC_IBI_EN)

/*
 * pll_rdy_ibi_en
 * 0: PLL ready interrupt not routed to IBI
 * 1: PLL ready interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE8_PLL_RDY_IBI_EN     0x04
#define ICM42670P_MREG1_MASK_INT_SOURCE8_PLL_RDY_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE8_PLL_RDY_IBI_EN)

/*
 * ui_drdy_ibi_en
 * 0: UI data ready interrupt not routed to IBI
 * 1: UI data ready interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE8_UI_DRDY_IBI_EN     0x03
#define ICM42670P_MREG1_MASK_INT_SOURCE8_UI_DRDY_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE8_UI_DRDY_IBI_EN)

/*
 * fifo_ths_ibi_en
 * 0: FIFO threshold interrupt not routed to IBI
 * 1: FIFO threshold interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE8_FIFO_THS_IBI_EN     0x02
#define ICM42670P_MREG1_MASK_INT_SOURCE8_FIFO_THS_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE8_FIFO_THS_IBI_EN)

/*
 * fifo_full_ibi_en
 * 0: FIFO full interrupt not routed to IBI
 * 1: FIFO full interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE8_FIFO_FULL_IBI_EN     0x01
#define ICM42670P_MREG1_MASK_INT_SOURCE8_FIFO_FULL_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE8_FIFO_FULL_IBI_EN)

/*
 * agc_rdy_ibi_en
 * 0: AGC ready interrupt not routed to IBI
 * 1: AGC ready interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE8_AGC_RDY_IBI_EN     0x00
#define ICM42670P_MREG1_MASK_INT_SOURCE8_AGC_RDY_IBI_EN    0x01



/*
 * INT_SOURCE9
 * Register Name : INT_SOURCE9
 */

/*
 * i3c_protocol_error_ibi_en
 * 0: I3CSM protocol error interrupt not routed to IBI
 * 1: I3CSM protocol error interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_I3C_PROTOCOL_ERROR_IBI_EN     0x07
#define ICM42670P_MREG1_MASK_INT_SOURCE9_I3C_PROTOCOL_ERROR_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_I3C_PROTOCOL_ERROR_IBI_EN)

/*
 * ff_ibi_en
 * 0: Freefall interrupt not routed to IBI
 * 1: Freefall interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_FF_IBI_EN     0x06
#define ICM42670P_MREG1_MASK_INT_SOURCE9_FF_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_FF_IBI_EN)

/*
 * lowg_ibi_en
 * 0: Low-g interrupt not routed to IBI
 * 1: Low-g interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_LOWG_IBI_EN     0x05
#define ICM42670P_MREG1_MASK_INT_SOURCE9_LOWG_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_LOWG_IBI_EN)

/*
 * smd_ibi_en
 * 0: SMD interrupt not routed to IBI
 * 1: SMD interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_SMD_IBI_EN     0x04
#define ICM42670P_MREG1_MASK_INT_SOURCE9_SMD_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_SMD_IBI_EN)

/*
 * wom_z_ibi_en
 * 0: Z-axis WOM interrupt not routed to IBI
 * 1: Z-axis WOM interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_WOM_Z_IBI_EN     0x03
#define ICM42670P_MREG1_MASK_INT_SOURCE9_WOM_Z_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_WOM_Z_IBI_EN)

/*
 * wom_y_ibi_en
 * 0: Y-axis WOM interrupt not routed to IBI
 * 1: Y-axis WOM interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_WOM_Y_IBI_EN     0x02
#define ICM42670P_MREG1_MASK_INT_SOURCE9_WOM_Y_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_WOM_Y_IBI_EN)

/*
 * wom_x_ibi_en
 * 0: X-axis WOM interrupt not routed to IBI
 * 1: X-axis WOM interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_WOM_X_IBI_EN     0x01
#define ICM42670P_MREG1_MASK_INT_SOURCE9_WOM_X_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE9_WOM_X_IBI_EN)

/*
 * st_done_ibi_en
 * 0: Self-test done interrupt not routed to IBI
 * 1: Self-test done interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE9_ST_DONE_IBI_EN     0x00
#define ICM42670P_MREG1_MASK_INT_SOURCE9_ST_DONE_IBI_EN    0x01



/*
 * INT_SOURCE10
 * Register Name : INT_SOURCE10
 */

/*
 * step_det_ibi_en
 * 0: Step detect interrupt not routed to IBI
 * 1: Step detect interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE10_STEP_DET_IBI_EN     0x05
#define ICM42670P_MREG1_MASK_INT_SOURCE10_STEP_DET_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE10_STEP_DET_IBI_EN)

/*
 * step_cnt_ofl_ibi_en
 * 0: Step count overflow interrupt not routed to IBI
 * 1: Step count overflow interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE10_STEP_CNT_OFL_IBI_EN     0x04
#define ICM42670P_MREG1_MASK_INT_SOURCE10_STEP_CNT_OFL_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE10_STEP_CNT_OFL_IBI_EN)

/*
 * tilt_det_ibi_en
 * 0: Tilt detect interrupt not routed to IBI
 * 1: Tile detect interrupt routed to IBI
 */
#define ICM42670P_MREG1_POS_INT_SOURCE10_TILT_DET_IBI_EN     0x03
#define ICM42670P_MREG1_MASK_INT_SOURCE10_TILT_DET_IBI_EN    (0x01 << ICM42670P_MREG1_POS_INT_SOURCE10_TILT_DET_IBI_EN)



/*
 * APEX_CONFIG2
 * Register Name : APEX_CONFIG2
 */

/*
 * low_energy_amp_th_sel
 * Threshold to select a valid step. Used to increase step detection for slow walk use case.
 *
 * 0000: 30 mg
 * 0001: 35 mg
 * 0010: 40 mg
 * 0011: 45 mg
 * 0100: 50 mg
 * 0101: 55 mg
 * 0110: 60 mg
 * 0111: 65 mg
 * 1000: 70 mg
 * 1001: 75 mg
 * 1010: 80 mg (default)
 * 1011: 85 mg
 * 1100: 90 mg
 * 1101: 95 mg
 * 1110: 100 mg
 * 1111: 105 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL     0x04
#define ICM42670P_MREG1_MASK_APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL    (0x0f << ICM42670P_MREG1_POS_APEX_CONFIG2_LOW_ENERGY_AMP_TH_SEL)

/*
 * dmp_power_save_time_sel
 * Duration of the period while the DMP stays awake after receiving a WOM event.
 *
 * 0000: 0 seconds
 * 0001: 4 seconds
 * 0010: 8 seconds (default)
 * 0011: 12 seconds
 * 0100: 16 seconds
 * 0101: 20 seconds
 * 0110: 24 seconds
 * 0111: 28 seconds
 * 1000: 32 seconds
 * 1001: 36 seconds
 * 1010: 40 seconds
 * 1011: 44 seconds
 * 1100: 48 seconds
 * 1101: 52 seconds
 * 1110: 56 seconds
 * 1111: 60 seconds
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG2_DMP_POWER_SAVE_TIME_SEL    0x0f



/*
 * APEX_CONFIG3
 * Register Name : APEX_CONFIG3
 */

/*
 * ped_amp_th_sel
 * Threshold of step detection sensitivity.
 *
 * Low values increase detection sensitivity: reduce miss-detection.
 * High values reduce detection sensitivity: reduce false-positive.
 *
 * 0000: 30 mg
 * 0001: 34 mg
 * 0010: 38 mg
 * 0011: 42 mg
 * 0100: 46 mg
 * 0101: 50 mg
 * 0110: 54 mg
 * 0111: 58 mg
 * 1000: 62 mg (default)
 * 1001: 66 mg
 * 1010: 70 mg
 * 1011: 74 mg
 * 1100: 78 mg
 * 1101: 82 mg
 * 1110: 86 mg
 * 1111: 90 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG3_PED_AMP_TH_SEL     0x04
#define ICM42670P_MREG1_MASK_APEX_CONFIG3_PED_AMP_TH_SEL    (0x0f << ICM42670P_MREG1_POS_APEX_CONFIG3_PED_AMP_TH_SEL)

/*
 * ped_step_cnt_th_sel
 * Minimum number of steps that must be detected before step count is incremented.
 *
 * Low values reduce latency but increase false positives.
 * High values increase step count accuracy but increase latency.
 *
 * 0000: 0 steps
 * 0001: 1 step
 * 0010: 2 steps
 * 0011: 3 steps
 * 0100: 4 steps
 * 0101: 5 steps (default)
 * 0110: 6 steps
 * 0111: 7 steps
 * 1000: 8 steps
 * 1001: 9 steps
 * 1010: 10 steps
 * 1011: 11 steps
 * 1100: 12 steps
 * 1101: 13 steps
 * 1110: 14 steps
 * 1111: 15 steps
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG3_PED_STEP_CNT_TH_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG3_PED_STEP_CNT_TH_SEL    0x0f



/*
 * APEX_CONFIG4
 * Register Name : APEX_CONFIG4
 */

/*
 * ped_step_det_th_sel
 * Minimum number of steps that must be detected before step event is signaled.
 *
 * Low values reduce latency but increase false positives.
 * High values increase step event validity but increase latency.
 *
 * 000: 0 steps
 * 001: 1 step
 * 010: 2 steps (default)
 * 011: 3 steps
 * 100: 4 steps
 * 101: 5 steps
 * 110: 6 steps
 * 111: 7 steps
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG4_PED_STEP_DET_TH_SEL     0x05
#define ICM42670P_MREG1_MASK_APEX_CONFIG4_PED_STEP_DET_TH_SEL    (0x07 << ICM42670P_MREG1_POS_APEX_CONFIG4_PED_STEP_DET_TH_SEL)

/*
 * ped_sb_timer_th_sel
 * Duration before algorithm considers that user has stopped taking steps.
 *
 * 000: 50 samples
 * 001: 75 sample
 * 010: 100 samples
 * 011: 125 samples
 * 100: 150 samples (default)
 * 101: 175 samples
 * 110: 200 samples
 * 111: 225 samples
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG4_PED_SB_TIMER_TH_SEL     0x02
#define ICM42670P_MREG1_MASK_APEX_CONFIG4_PED_SB_TIMER_TH_SEL    (0x07 << ICM42670P_MREG1_POS_APEX_CONFIG4_PED_SB_TIMER_TH_SEL)

/*
 * ped_hi_en_th_sel
 * Threshold to classify acceleration signal as motion not due to steps.
 *
 * High values improve vibration rejection.
 * Low values improve detection.
 *
 * 00: 87.89 mg
 * 01: 104.49 mg (default)
 * 10: 132.81 mg
 * 11: 155.27 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG4_PED_HI_EN_TH_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG4_PED_HI_EN_TH_SEL    0x03



/*
 * APEX_CONFIG5
 * Register Name : APEX_CONFIG5
 */

/*
 * tilt_wait_time_sel
 * Minimum duration for which the device should be tilted before signaling event.
 *
 * 00: 0s
 * 01: 2s
 * 10: 4s (default)
 * 11: 6s
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG5_TILT_WAIT_TIME_SEL     0x06
#define ICM42670P_MREG1_MASK_APEX_CONFIG5_TILT_WAIT_TIME_SEL    (0x03 << ICM42670P_MREG1_POS_APEX_CONFIG5_TILT_WAIT_TIME_SEL)

/*
 * lowg_peak_th_hyst_sel
 * Hysteresis value added to the low-g threshold after exceeding it.
 *
 * 000: 31 mg (default)
 * 001: 63 mg
 * 010: 94 mg
 * 011: 125 mg
 * 100: 156 mg
 * 101: 188 mg
 * 110: 219 mg
 * 111: 250 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG5_LOWG_PEAK_TH_HYST_SEL     0x03
#define ICM42670P_MREG1_MASK_APEX_CONFIG5_LOWG_PEAK_TH_HYST_SEL    (0x07 << ICM42670P_MREG1_POS_APEX_CONFIG5_LOWG_PEAK_TH_HYST_SEL)

/*
 * highg_peak_th_hyst_sel
 * Hysteresis value subtracted from the high-g threshold after exceeding it.
 *
 * 000: 31 mg (default)
 * 001: 63 mg
 * 010: 94 mg
 * 011: 125 mg
 * 100: 156 mg
 * 101: 188 mg
 * 110: 219 mg
 * 111: 250 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG5_HIGHG_PEAK_TH_HYST_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG5_HIGHG_PEAK_TH_HYST_SEL    0x07



/*
 * APEX_CONFIG9
 * Register Name : APEX_CONFIG9
 */

/*
 * ff_debounce_duration_sel
 * Period after a freefall is signaled during which a new freefall will not be detected. Prevents false detection due to bounces.
 *
 * 0000: 0 ms
 * 0001: 1250 ms
 * 0010: 1375 ms
 * 0011: 1500 ms
 * 0100: 1625 ms
 * 0101: 1750 ms
 * 0110: 1875 ms
 * 0111: 2000 ms
 * 1000: 2125 ms (default)
 * 1001: 2250 ms
 * 1010: 2375 ms
 * 1011: 2500 ms
 * 1100: 2625 ms
 * 1101: 2750 ms
 * 1110: 2875 ms
 * 1111: 3000 ms
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG9_FF_DEBOUNCE_DURATION_SEL     0x04
#define ICM42670P_MREG1_MASK_APEX_CONFIG9_FF_DEBOUNCE_DURATION_SEL    (0x0f << ICM42670P_MREG1_POS_APEX_CONFIG9_FF_DEBOUNCE_DURATION_SEL)

/*
 * smd_sensitivity_sel
 * Parameter to tune SMD algorithm robustness to rejection, ranging from 0 to 4 (values higher than 4 are reserved).
 *
 * Low values increase detection rate but increase false positives.
 * High values reduce false positives but reduce detection rate (especially for transport use cases).
 *
 * Default value is 0.
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG9_SMD_SENSITIVITY_SEL     0x01
#define ICM42670P_MREG1_MASK_APEX_CONFIG9_SMD_SENSITIVITY_SEL    (0x07 << ICM42670P_MREG1_POS_APEX_CONFIG9_SMD_SENSITIVITY_SEL)

/*
 * sensitivity_mode
 * Pedometer sensitivity mode
 * 0: Normal (default)
 * 1: Slow walk
 *
 * Slow walk mode improves slow walk detection (<1Hz) but the number of false positives may increase.
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG9_SENSITIVITY_MODE     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG9_SENSITIVITY_MODE    0x01



/*
 * APEX_CONFIG10
 * Register Name : APEX_CONFIG10
 */

/*
 * lowg_peak_th_sel
 * Threshold for accel values below which low-g state is detected.
 *
 * 00000: 31 mg (default)
 * 00001: 63 mg
 * 00010: 94 mg
 * 00011: 125 mg
 * 00100: 156 mg
 * 00101: 188 mg
 * 00110: 219 mg
 * 00111: 250 mg
 * 01000: 281 mg
 * 01001: 313 mg
 * 01010: 344 mg
 * 01011: 375 mg
 * 01100: 406 mg
 * 01101: 438 mg
 * 01110: 469 mg
 * 01111: 500 mg
 * 10000: 531 mg
 * 10001: 563 mg
 * 10010: 594 mg
 * 10011: 625 mg
 * 10100: 656 mg
 * 10101: 688 mg
 * 10110: 719 mg
 * 10111: 750 mg
 * 11000: 781 mg
 * 11001: 813 mg
 * 11010: 844 mg
 * 11011: 875 mg
 * 11100: 906 mg
 * 11101: 938 mg
 * 11110: 969 mg
 * 11111: 1000 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG10_LOWG_PEAK_TH_SEL     0x03
#define ICM42670P_MREG1_MASK_APEX_CONFIG10_LOWG_PEAK_TH_SEL    (0x1f << ICM42670P_MREG1_POS_APEX_CONFIG10_LOWG_PEAK_TH_SEL)

/*
 * lowg_time_th_sel
 * Number of samples required to enter low-g state.
 *
 * 000: 1 sample (default)
 * 001: 2 samples
 * 010: 3 samples
 * 011: 4 samples
 * 100: 5 samples
 * 101: 6 samples
 * 110: 7 samples
 * 111: 8 samples
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG10_LOWG_TIME_TH_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG10_LOWG_TIME_TH_SEL    0x07



/*
 * APEX_CONFIG11
 * Register Name : APEX_CONFIG11
 */

/*
 * highg_peak_th_sel
 * Threshold for accel values above which high-g state is detected.
 *
 * 00000: 250 mg (default)
 * 00001: 500 mg
 * 00010: 750 mg
 * 00011: 1000 mg
 * 00100: 1250 mg
 * 00101: 1500 mg
 * 00110: 1750 mg
 * 00111: 2000 mg
 * 01000: 2250 mg
 * 01001: 2500 mg
 * 01010: 2750 mg
 * 01011: 3000 mg
 * 01100: 3250 mg
 * 01101: 3500 mg
 * 01110: 3750 mg
 * 01111: 4000 mg
 * 10000: 4250 mg
 * 10001: 4500 mg
 * 10010: 4750 mg
 * 10011: 5000 mg
 * 10100: 5250 mg
 * 10101: 5500 mg
 * 10110: 5750 mg
 * 10111: 6000 mg
 * 11000: 6250 mg
 * 11001: 6500 mg
 * 11010: 6750 mg
 * 11011: 7000 mg
 * 11100: 7250 mg
 * 11101: 7500 mg
 * 11110: 7750 mg
 * 11111: 8000 mg
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG11_HIGHG_PEAK_TH_SEL     0x03
#define ICM42670P_MREG1_MASK_APEX_CONFIG11_HIGHG_PEAK_TH_SEL    (0x1f << ICM42670P_MREG1_POS_APEX_CONFIG11_HIGHG_PEAK_TH_SEL)

/*
 * highg_time_th_sel
 * Number of samples required to enter high-g state.
 *
 * 000: 1 sample (default)
 * 001: 2 samples
 * 010: 3 samples
 * 011: 4 samples
 * 100: 5 samples
 * 101: 6 samples
 * 110: 7 samples
 * 111: 8 samples
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG11_HIGHG_TIME_TH_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG11_HIGHG_TIME_TH_SEL    0x07



/*
 * ACCEL_WOM_X_THR
 * Register Name : ACCEL_WOM_X_THR
 */

/*
 * wom_x_th
 * Threshold value for the Wake on Motion Interrupt for X-axis accelerometer
 * WoM thresholds are expressed in fixed “mg” independent of the selected Range [0g : 1g]; Resolution 1g/256=~3.9mg
 */
#define ICM42670P_MREG1_POS_ACCEL_WOM_X_THR_WOM_X_TH     0x00
#define ICM42670P_MREG1_MASK_ACCEL_WOM_X_THR_WOM_X_TH    0xff



/*
 * ACCEL_WOM_Y_THR
 * Register Name : ACCEL_WOM_Y_THR
 */

/*
 * wom_y_th
 * Threshold value for the Wake on Motion Interrupt for Y-axis accelerometer
 * WoM thresholds are expressed in fixed “mg” independent of the selected Range [0g : 1g]; Resolution 1g/256=~3.9mg
 */
#define ICM42670P_MREG1_POS_ACCEL_WOM_Y_THR_WOM_Y_TH     0x00
#define ICM42670P_MREG1_MASK_ACCEL_WOM_Y_THR_WOM_Y_TH    0xff



/*
 * ACCEL_WOM_Z_THR
 * Register Name : ACCEL_WOM_Z_THR
 */

/*
 * wom_z_th
 * Threshold value for the Wake on Motion Interrupt for Z-axis accelerometer
 * WoM thresholds are expressed in fixed “mg” independent of the selected Range [0g : 1g]; Resolution 1g/256=~3.9mg
 */
#define ICM42670P_MREG1_POS_ACCEL_WOM_Z_THR_WOM_Z_TH     0x00
#define ICM42670P_MREG1_MASK_ACCEL_WOM_Z_THR_WOM_Z_TH    0xff



/*
 * OFFSET_USER0
 * Register Name : OFFSET_USER0
 */

/*
 * gyro_x_offuser
 * Gyro offset programmed by user. Max value is +/-64 dps, resolution is 1/32 dps
 */
#define ICM42670P_MREG1_POS_OFFSET_USER0_GYRO_X_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER0_GYRO_X_OFFUSER    0xff



/*
 * OFFSET_USER1
 * Register Name : OFFSET_USER1
 */

/*
 * gyro_x_offuser
 * Gyro offset programmed by user. Max value is +/-64 dps, resolution is 1/32 dps
 */
#define ICM42670P_MREG1_POS_OFFSET_USER1_GYRO_X_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER1_GYRO_X_OFFUSER    0x0f

/*
 * gyro_y_offuser
 * Gyro offset programmed by user. Max value is +/-64 dps, resolution is 1/32 dps
 */
#define ICM42670P_MREG1_POS_OFFSET_USER1_GYRO_Y_OFFUSER     0x04
#define ICM42670P_MREG1_MASK_OFFSET_USER1_GYRO_Y_OFFUSER    (0x0f << ICM42670P_MREG1_POS_OFFSET_USER1_GYRO_Y_OFFUSER)



/*
 * OFFSET_USER2
 * Register Name : OFFSET_USER2
 */

/*
 * gyro_y_offuser
 * Gyro offset programmed by user. Max value is +/-64 dps, resolution is 1/32 dps
 */
#define ICM42670P_MREG1_POS_OFFSET_USER2_GYRO_Y_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER2_GYRO_Y_OFFUSER    0xff



/*
 * OFFSET_USER3
 * Register Name : OFFSET_USER3
 */

/*
 * gyro_z_offuser
 * Gyro offset programmed by user. Max value is +/-64 dps, resolution is 1/32 dps
 */
#define ICM42670P_MREG1_POS_OFFSET_USER3_GYRO_Z_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER3_GYRO_Z_OFFUSER    0xff



/*
 * OFFSET_USER4
 * Register Name : OFFSET_USER4
 */

/*
 * gyro_z_offuser
 * Gyro offset programmed by user. Max value is +/-64 dps, resolution is 1/32 dps
 */
#define ICM42670P_MREG1_POS_OFFSET_USER4_GYRO_Z_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER4_GYRO_Z_OFFUSER    0x0f

/*
 * accel_x_offuser
 * Accel offset programmed by user. Max value is +/-1 gee, resolution is 0.5 mgee
 */
#define ICM42670P_MREG1_POS_OFFSET_USER4_ACCEL_X_OFFUSER     0x04
#define ICM42670P_MREG1_MASK_OFFSET_USER4_ACCEL_X_OFFUSER    (0x0f << ICM42670P_MREG1_POS_OFFSET_USER4_ACCEL_X_OFFUSER)



/*
 * OFFSET_USER5
 * Register Name : OFFSET_USER5
 */

/*
 * accel_x_offuser
 * Accel offset programmed by user. Max value is +/-1 gee, resolution is 0.5 mgee
 */
#define ICM42670P_MREG1_POS_OFFSET_USER5_ACCEL_X_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER5_ACCEL_X_OFFUSER    0xff



/*
 * OFFSET_USER6
 * Register Name : OFFSET_USER6
 */

/*
 * accel_y_offuser
 * Accel offset programmed by user. Max value is +/-1 gee, resolution is 0.5 mgee
 */
#define ICM42670P_MREG1_POS_OFFSET_USER6_ACCEL_Y_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER6_ACCEL_Y_OFFUSER    0xff



/*
 * OFFSET_USER7
 * Register Name : OFFSET_USER7
 */

/*
 * accel_y_offuser
 * Accel offset programmed by user. Max value is +/-1 gee, resolution is 0.5 mgee
 */
#define ICM42670P_MREG1_POS_OFFSET_USER7_ACCEL_Y_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER7_ACCEL_Y_OFFUSER    0x0f

/*
 * accel_z_offuser
 * Accel offset programmed by user. Max value is +/-1 gee, resolution is 0.5 mgee
 */
#define ICM42670P_MREG1_POS_OFFSET_USER7_ACCEL_Z_OFFUSER     0x04
#define ICM42670P_MREG1_MASK_OFFSET_USER7_ACCEL_Z_OFFUSER    (0x0f << ICM42670P_MREG1_POS_OFFSET_USER7_ACCEL_Z_OFFUSER)



/*
 * OFFSET_USER8
 * Register Name : OFFSET_USER8
 */

/*
 * accel_z_offuser
 * Accel offset programmed by user. Max value is +/-1 gee, resolution is 0.5 mgee
 */
#define ICM42670P_MREG1_POS_OFFSET_USER8_ACCEL_Z_OFFUSER     0x00
#define ICM42670P_MREG1_MASK_OFFSET_USER8_ACCEL_Z_OFFUSER    0xff



/*
 * ST_STATUS1
 * Register Name : ST_STATUS1
 */

/*
 * accel_st_pass
 * 1: Accel self-test passed for all the 3 axes
 */
#define ICM42670P_MREG1_POS_ST_STATUS1_ACCEL_ST_PASS     0x05
#define ICM42670P_MREG1_MASK_ST_STATUS1_ACCEL_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS1_ACCEL_ST_PASS)

/*
 * accel_st_done
 * 1: Accel self-test done for all the 3 axes
 */
#define ICM42670P_MREG1_POS_ST_STATUS1_ACCEL_ST_DONE     0x04
#define ICM42670P_MREG1_MASK_ST_STATUS1_ACCEL_ST_DONE    (0x01 << ICM42670P_MREG1_POS_ST_STATUS1_ACCEL_ST_DONE)

/*
 * az_st_pass
 * 1: Accel Z-axis self-test passed
 */
#define ICM42670P_MREG1_POS_ST_STATUS1_AZ_ST_PASS     0x03
#define ICM42670P_MREG1_MASK_ST_STATUS1_AZ_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS1_AZ_ST_PASS)

/*
 * ay_st_pass
 * 1: Accel Y-axis self-test passed
 */
#define ICM42670P_MREG1_POS_ST_STATUS1_AY_ST_PASS     0x02
#define ICM42670P_MREG1_MASK_ST_STATUS1_AY_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS1_AY_ST_PASS)

/*
 * ax_st_pass
 * 1: Accel X-axis self-test passed
 */
#define ICM42670P_MREG1_POS_ST_STATUS1_AX_ST_PASS     0x01
#define ICM42670P_MREG1_MASK_ST_STATUS1_AX_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS1_AX_ST_PASS)



/*
 * ST_STATUS2
 * Register Name : ST_STATUS2
 */

/*
 * st_incomplete
 * 1: Self-test is incomplete.
 * This bit is set to 1 if the self-test was aborted.
 * One possible cause of aborting the self-test may be the detection of significant movement in the gyro when the self-test for gyro and/or accel is being executed.
 */
#define ICM42670P_MREG1_POS_ST_STATUS2_ST_INCOMPLETE     0x06
#define ICM42670P_MREG1_MASK_ST_STATUS2_ST_INCOMPLETE    (0x01 << ICM42670P_MREG1_POS_ST_STATUS2_ST_INCOMPLETE)

/*
 * gyro_st_pass
 * 1: Gyro self-test passed for all the 3 axes
 */
#define ICM42670P_MREG1_POS_ST_STATUS2_GYRO_ST_PASS     0x05
#define ICM42670P_MREG1_MASK_ST_STATUS2_GYRO_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS2_GYRO_ST_PASS)

/*
 * gyro_st_done
 * 1: Gyro self-test done for all the 3 axes
 */
#define ICM42670P_MREG1_POS_ST_STATUS2_GYRO_ST_DONE     0x04
#define ICM42670P_MREG1_MASK_ST_STATUS2_GYRO_ST_DONE    (0x01 << ICM42670P_MREG1_POS_ST_STATUS2_GYRO_ST_DONE)

/*
 * gz_st_pass
 * 1: Gyro Z-axis self-test passed
 */
#define ICM42670P_MREG1_POS_ST_STATUS2_GZ_ST_PASS     0x03
#define ICM42670P_MREG1_MASK_ST_STATUS2_GZ_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS2_GZ_ST_PASS)

/*
 * gy_st_pass
 * 1: Gyro Y-axis self-test passed
 */
#define ICM42670P_MREG1_POS_ST_STATUS2_GY_ST_PASS     0x02
#define ICM42670P_MREG1_MASK_ST_STATUS2_GY_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS2_GY_ST_PASS)

/*
 * gx_st_pass
 * 1: Gyro X-axis self-test passed
 */
#define ICM42670P_MREG1_POS_ST_STATUS2_GX_ST_PASS     0x01
#define ICM42670P_MREG1_MASK_ST_STATUS2_GX_ST_PASS    (0x01 << ICM42670P_MREG1_POS_ST_STATUS2_GX_ST_PASS)



/*
 * FDR_CONFIG
 * Register Name : FDR_CONFIG
 */

/*
 * fdr_sel
 * [7:4] Reserved
 * [3:0] FIFO packet rate decimation factor.  Sets the number of discarded FIFO packets.  Valid range is 0 to 127.  User must disable sensors when initializing FDR_SEL value or making changes to it.
 *
 * 0xxx:  Decimation is disabled, all packets are sent to FIFO
 * 1000: 1 packet out of 2 is sent to FIFO
 * 1001: 1 packet out of 4 is sent to FIFO
 * 1010: 1 packet out of 8 is sent to FIFO
 * 1011: 1 packet out of 16 is sent to FIFO
 * 1100: 1 packet out of 32 is sent to FIFO
 * 1101: 1 packet out of 64 is sent to FIFO
 * 1110: 1 packet out of 128 is sent to FIFO
 * 1111: 1 packet out of 256 is sent to FIFO
 */
#define ICM42670P_MREG1_POS_FDR_CONFIG_FDR_SEL     0x00
#define ICM42670P_MREG1_MASK_FDR_CONFIG_FDR_SEL    0xff



/*
 * APEX_CONFIG12
 * Register Name : APEX_CONFIG12
 */

/*
 * ff_max_duration_sel
 * Maximum freefall length. Longer freefalls are ignored.
 *
 * 0000: 102 cm (default)
 * 0001: 120 cm
 * 0010: 139 cm
 * 0011: 159 cm
 * 0100: 181 cm
 * 0101: 204 cm
 * 0110: 228 cm
 * 0111: 254 cm
 * 1000: 281 cm
 * 1001: 310 cm
 * 1010: 339 cm
 * 1011: 371 cm
 * 1100: 403 cm
 * 1101: 438 cm
 * 1110: 473 cm
 * 1111: 510 cm
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG12_FF_MAX_DURATION_SEL     0x04
#define ICM42670P_MREG1_MASK_APEX_CONFIG12_FF_MAX_DURATION_SEL    (0x0f << ICM42670P_MREG1_POS_APEX_CONFIG12_FF_MAX_DURATION_SEL)

/*
 * ff_min_duration_sel
 * Minimum freefall length. Shorter freefalls are ignored.
 *
 * 0000: 10 cm (default)
 * 0001: 12 cm
 * 0010: 13 cm
 * 0011: 16 cm
 * 0100: 18 cm
 * 0101: 20 cm
 * 0110: 23 cm
 * 0111: 25 cm
 * 1000: 28 cm
 * 1001: 31 cm
 * 1010: 34 cm
 * 1011: 38 cm
 * 1100: 41 cm
 * 1101: 45 cm
 * 1110: 48 cm
 * 1111: 52 cm
 */
#define ICM42670P_MREG1_POS_APEX_CONFIG12_FF_MIN_DURATION_SEL     0x00
#define ICM42670P_MREG1_MASK_APEX_CONFIG12_FF_MIN_DURATION_SEL    0x0f



/****************************************/
//	MREG2 Bits Position
/****************************************/

/*
 * OTP_CTRL7
 * Register Name : OTP_CTRL7
 */

/*
 * otp_reload
 * 1: to trigger OTP copy operation. This bit is cleared to 0 after OTP copy is done.
 *
 * With otp_copy_mode[1:0] = 2'b01, it takes 280us to complete the OTP reloading operation.
 * With otp_copy_mode[1:0] = 2'b11, it takes 20us to complete the OTP reloading operation.
 */
#define ICM42670P_MREG2_POS_OTP_CTRL7_OTP_RELOAD     0x03
#define ICM42670P_MREG2_MASK_OTP_CTRL7_OTP_RELOAD    (0x01 << ICM42670P_MREG2_POS_OTP_CTRL7_OTP_RELOAD)

/*
 * otp_pwr_down
 * 0: Power up OTP to copy from OTP to SRAM
 * 1: Power down OTP
 *
 * This bit is automatically set to 1 when OTP copy operation is complete.
 */
#define ICM42670P_MREG2_POS_OTP_CTRL7_OTP_PWR_DOWN     0x01
#define ICM42670P_MREG2_MASK_OTP_CTRL7_OTP_PWR_DOWN    (0x01 << ICM42670P_MREG2_POS_OTP_CTRL7_OTP_PWR_DOWN)



/****************************************/
//	MREG3 Bits Position
/****************************************/

/*
 * XA_ST_DATA
 * Register Name : XA_ST_DATA
 */

/*
 * xa_st_data
 * Accel X-axis self test data converted to 8 bit code.
 */
#define ICM42670P_MREG3_POS_XA_ST_DATA_XA_ST_DATA     0x00
#define ICM42670P_MREG3_MASK_XA_ST_DATA_XA_ST_DATA    0xff



/*
 * YA_ST_DATA
 * Register Name : YA_ST_DATA
 */

/*
 * ya_st_data
 * Accel Y-axis self test data converted to 8 bit code.
 */
#define ICM42670P_MREG3_POS_YA_ST_DATA_YA_ST_DATA     0x00
#define ICM42670P_MREG3_MASK_YA_ST_DATA_YA_ST_DATA    0xff



/*
 * ZA_ST_DATA
 * Register Name : ZA_ST_DATA
 */

/*
 * za_st_data
 * Accel Z-axis self test data converted to 8 bit code.
 */
#define ICM42670P_MREG3_POS_ZA_ST_DATA_ZA_ST_DATA     0x00
#define ICM42670P_MREG3_MASK_ZA_ST_DATA_ZA_ST_DATA    0xff



/*
 * XG_ST_DATA
 * Register Name : XG_ST_DATA
 */

/*
 * xg_st_data
 * Gyro X-axis self test data converted to 8 bit code.
 */
#define ICM42670P_MREG3_POS_XG_ST_DATA_XG_ST_DATA     0x00
#define ICM42670P_MREG3_MASK_XG_ST_DATA_XG_ST_DATA    0xff



/*
 * YG_ST_DATA
 * Register Name : YG_ST_DATA
 */

/*
 * yg_st_data
 * Gyro Y-axis self test data converted to 8 bit code.
 */
#define ICM42670P_MREG3_POS_YG_ST_DATA_YG_ST_DATA     0x00
#define ICM42670P_MREG3_MASK_YG_ST_DATA_YG_ST_DATA    0xff



/*
 * ZG_ST_DATA
 * Register Name : ZG_ST_DATA
 */

/*
 * zg_st_data
 * Gyro Z-axis self test data converted to 8 bit code.
 */
#define ICM42670P_MREG3_POS_ZG_ST_DATA_ZG_ST_DATA     0x00
#define ICM42670P_MREG3_MASK_ZG_ST_DATA_ZG_ST_DATA    0xff



#ifdef __cplusplus
}
#endif

#endif


