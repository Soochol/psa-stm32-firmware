/*
 * gps_test.c
 *
 * GPS I2C Communication Test Functions
 * Created: 2025-01-11
 */

#include "gps_test.h"
#include "lib_log.h"
#include "main.h"
#include "i2c.h"
#include "uart.h"
#include "tim.h"
#include "sam_m10q_platform.h"
#include "sam_m10q_driver.h"

// Test state
static uint32_t test_count = 0;

/*
 * Test 1: Basic I2C3 Ping Test
 * Attempts to write 0 bytes to GPS address to check ACK
 */
void v_GPS_Test_I2C_Ping(void) {
    LOG_INFO("GPS_TEST", "=== GPS I2C3 Ping Test ===");
    LOG_INFO("GPS_TEST", "GPS Address: 0x%02X (7-bit: 0x%02X)", ADDR_GPS, ADDR_GPS >> 1);

    // Simple write test - GPS should ACK if present
    extern I2C_HandleTypeDef hi2c3;
    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&hi2c3, ADDR_GPS, 3, 100);

    if(status == HAL_OK) {
        LOG_INFO("GPS_TEST", "[PASS] GPS device responded on I2C3!");
    } else if(status == HAL_BUSY) {
        LOG_INFO("GPS_TEST", "[FAIL] I2C3 bus is BUSY");
    } else if(status == HAL_TIMEOUT) {
        LOG_INFO("GPS_TEST", "[FAIL] GPS device timeout (no ACK)");
    } else {
        LOG_INFO("GPS_TEST", "[FAIL] I2C3 error (status=%d)", status);
    }

    LOG_INFO("GPS_TEST", "=========================");
}

/*
 * Test 2: Read Available Bytes
 * GPS stores available data count in registers 0xFD (MSB) and 0xFE (LSB)
 */
void v_GPS_Test_Read_AvailableBytes(void) {
    LOG_INFO("GPS_TEST", "=== GPS Available Bytes Test ===");

    extern I2C_HandleTypeDef hi2c3;
    uint8_t avail_bytes[2] = {0};

    // Read 2 bytes from register 0xFD
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c3, ADDR_GPS, 0xFD, I2C_MEMADD_SIZE_8BIT,
                                                avail_bytes, 2, 100);

    if(status == HAL_OK) {
        uint16_t available = (avail_bytes[0] << 8) | avail_bytes[1];
        LOG_INFO("GPS_TEST", "[PASS] Available bytes: %d (0x%02X%02X)",
                      available, avail_bytes[0], avail_bytes[1]);

        if(available > 0) {
            LOG_INFO("GPS_TEST", "       GPS has data ready to read!");
        } else {
            LOG_INFO("GPS_TEST", "       No GPS data available yet");
        }
    } else {
        LOG_INFO("GPS_TEST", "[FAIL] Could not read available bytes (status=%d)", status);
    }

    LOG_INFO("GPS_TEST", "================================");
}

/*
 * Test 3: Read GPS Data Stream
 * Attempts to read actual GPS data (UBX protocol)
 */
void v_GPS_Test_Read_Data(void) {
    LOG_INFO("GPS_TEST", "=== GPS Data Read Test ===");

    extern I2C_HandleTypeDef hi2c3;
    uint8_t data_buf[32] = {0};

    // Read from data stream register (0xFF)
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c3, ADDR_GPS, 0xFF, I2C_MEMADD_SIZE_8BIT,
                                                data_buf, 32, 100);

    if(status == HAL_OK) {
        LOG_INFO("GPS_TEST", "[PASS] Read 32 bytes from GPS:");
        v_printf_poll("       ");
        for(int i = 0; i < 32; i++) {
            v_printf_poll("%02X ", data_buf[i]);
            if((i + 1) % 16 == 0 && i < 31) {
                v_printf_poll("\r\n       ");
            }
        }
        v_printf_poll("\r\n");

        // Check for UBX header (0xB5 0x62)
        bool found_ubx = false;
        for(int i = 0; i < 31; i++) {
            if(data_buf[i] == 0xB5 && data_buf[i+1] == 0x62) {
                LOG_INFO("GPS_TEST", "       Found UBX header at offset %d!", i);
                found_ubx = true;
                break;
            }
        }
        if(!found_ubx) {
            LOG_INFO("GPS_TEST", "       No UBX header found (GPS may be starting up)");
        }
    } else {
        LOG_INFO("GPS_TEST", "[FAIL] Could not read GPS data (status=%d)", status);
    }

    LOG_INFO("GPS_TEST", "==========================");
}

/*
 * Test 4: Full Diagnostic
 * Runs all tests in sequence
 */
void v_GPS_Test_Full_Diagnostic(void) {
    LOG_INFO("GPS_TEST", " ");
    LOG_INFO("GPS_TEST", "========================================");
    LOG_INFO("GPS_TEST", "   GPS I2C3 Full Diagnostic Test");
    LOG_INFO("GPS_TEST", "========================================");

    // Test 1: Ping
    v_GPS_Test_I2C_Ping();
    HAL_Delay(100);

    // Test 2: Available bytes
    v_GPS_Test_Read_AvailableBytes();
    HAL_Delay(100);

    // Test 3: Data read
    v_GPS_Test_Read_Data();
    HAL_Delay(100);

    // Test 4: Driver status
    LOG_INFO("GPS_TEST", " ");
    LOG_INFO("GPS_TEST", "=== GPS Driver Status ===");
    LOG_INFO("GPS_TEST", "Fix Type:     %d", u8_GPS_GetFixType());
    LOG_INFO("GPS_TEST", "Satellites:   %d", u8_GPS_GetNumSatellites());
    LOG_INFO("GPS_TEST", "Latitude:     %.6f°", f_GPS_GetLatitude());
    LOG_INFO("GPS_TEST", "Longitude:    %.6f°", f_GPS_GetLongitude());
    LOG_INFO("GPS_TEST", "Altitude:     %.1f m", f_GPS_GetAltitude());
    LOG_INFO("GPS_TEST", "Speed:        %.2f m/s", f_GPS_GetSpeed());
    LOG_INFO("GPS_TEST", "=========================");

    LOG_INFO("GPS_TEST", "Diagnostic complete!");
}

/*
 * Test 5: Continuous Monitoring
 * Call this periodically (e.g., every 2 seconds)
 */
void v_GPS_Test_Monitor(void) {
    static uint32_t timRef = 0;

    if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, 2000)) {
        timRef = u32_Tim_1msGet();
        test_count++;

#if IWDG_USED
        // CRITICAL: Refresh watchdog before UART printf operations
        // v_printf_poll() can take significant time with long strings
        extern IWDG_HandleTypeDef hiwdg1;
        HAL_IWDG_Refresh(&hiwdg1);
#endif

        LOG_INFO("GPS_TEST", " ");
        LOG_INFO("GPS_TEST", "========== GPS STATUS [%04lu] ==========", test_count);

        // Check if GPS has fix
        if(b_GPS_HasFix()) {
            LOG_INFO("GPS_TEST", "Status: FIX ACQUIRED");
            LOG_INFO("GPS_TEST", "-------------------------------------------");
            LOG_INFO("GPS_TEST", "Latitude:    %11.7f°", f_GPS_GetLatitude());
            LOG_INFO("GPS_TEST", "Longitude:   %11.7f°", f_GPS_GetLongitude());
            LOG_INFO("GPS_TEST", "Altitude:    %8.2f m (MSL)", f_GPS_GetAltitude());
            LOG_INFO("GPS_TEST", "Speed:       %7.2f m/s", f_GPS_GetSpeed());
            LOG_INFO("GPS_TEST", "Satellites:  %2d", u8_GPS_GetNumSatellites());
            LOG_INFO("GPS_TEST", "Fix Type:    %d (3=3D Fix)", u8_GPS_GetFixType());

            // Get raw PVT data for additional info
            _x_GPS_PVT_t* pvt = px_GPS_GetPVT();
            if(pvt) {
                LOG_INFO("GPS_TEST", "Accuracy:    H=%lu.%03lum, V=%lu.%03lum",
                              (unsigned long)(pvt->hAcc / 1000),
                              (unsigned long)(pvt->hAcc % 1000),
                              (unsigned long)(pvt->vAcc / 1000),
                              (unsigned long)(pvt->vAcc % 1000));
                LOG_INFO("GPS_TEST", "PDOP:        %d.%02d",
                              pvt->pDOP / 100, pvt->pDOP % 100);
                LOG_INFO("GPS_TEST", "Time (UTC):  %04d-%02d-%02d %02d:%02d:%02d",
                              pvt->year, pvt->month, pvt->day,
                              pvt->hour, pvt->min, pvt->sec);
                LOG_INFO("GPS_TEST", "GPS TOW:     %lu ms",
                              (unsigned long)pvt->iTOW);

                // Heading (if moving)
                int32_t head_deg = pvt->headMot / 100000;
                int32_t head_frac = pvt->headMot % 100000;
                if(head_frac < 0) head_frac = -head_frac;
                (void)head_deg; (void)head_frac;  // Suppress warnings when logs disabled
                LOG_INFO("GPS_TEST", "Heading:     %ld.%05ld°",
                              head_deg, head_frac);
            }
        } else {
            LOG_INFO("GPS_TEST", "Status: NO FIX");
            LOG_INFO("GPS_TEST", "-------------------------------------------");
            LOG_INFO("GPS_TEST", "Fix Type:    %d (0=No Fix, 2=2D, 3=3D)", u8_GPS_GetFixType());
            LOG_INFO("GPS_TEST", "Satellites:  %2d (Need 4+ for 3D fix)", u8_GPS_GetNumSatellites());
            LOG_INFO("GPS_TEST", "Status:      Waiting for satellite lock...");

            // Show raw data if available
            _x_GPS_PVT_t* pvt = px_GPS_GetPVT();
            if(pvt && pvt->numSV > 0) {
                LOG_INFO("GPS_TEST", "Info:        %d satellites visible",
                              pvt->numSV);
            }
        }

        LOG_INFO("GPS_TEST", "===========================================");
    }
}
