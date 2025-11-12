/*
 * gps_test.h
 *
 * GPS I2C Communication Test Functions
 * Created: 2025-01-11
 */

#ifndef __GPS_TEST_H
#define __GPS_TEST_H

#include <stdint.h>
#include <stdbool.h>

/*
 * GPS I2C Communication Test Functions
 */

// Basic I2C3 connectivity test
void v_GPS_Test_I2C_Ping(void);

// Read GPS available bytes (registers 0xFD, 0xFE)
void v_GPS_Test_Read_AvailableBytes(void);

// Read GPS data stream
void v_GPS_Test_Read_Data(void);

// Complete GPS diagnostic test
void v_GPS_Test_Full_Diagnostic(void);

// Continuous GPS status monitoring (call periodically)
void v_GPS_Test_Monitor(void);

#endif /* __GPS_TEST_H */
