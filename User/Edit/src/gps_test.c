/*
 * gps_test.c
 *
 * GPS I2C Communication Test Functions
 * Created: 2025-01-11
 */

#include "gps_test.h"
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
    v_printf_poll("\r\n=== GPS I2C3 Ping Test ===\r\n");
    v_printf_poll("GPS Address: 0x%02X (7-bit: 0x%02X)\r\n", ADDR_GPS, ADDR_GPS >> 1);

    // Simple write test - GPS should ACK if present
    extern I2C_HandleTypeDef hi2c3;
    HAL_StatusTypeDef status = HAL_I2C_IsDeviceReady(&hi2c3, ADDR_GPS, 3, 100);

    if(status == HAL_OK) {
        v_printf_poll("[PASS] GPS device responded on I2C3!\r\n");
    } else if(status == HAL_BUSY) {
        v_printf_poll("[FAIL] I2C3 bus is BUSY\r\n");
    } else if(status == HAL_TIMEOUT) {
        v_printf_poll("[FAIL] GPS device timeout (no ACK)\r\n");
    } else {
        v_printf_poll("[FAIL] I2C3 error (status=%d)\r\n", status);
    }

    v_printf_poll("=========================\r\n\r\n");
}

/*
 * Test 2: Read Available Bytes
 * GPS stores available data count in registers 0xFD (MSB) and 0xFE (LSB)
 */
void v_GPS_Test_Read_AvailableBytes(void) {
    v_printf_poll("\r\n=== GPS Available Bytes Test ===\r\n");

    extern I2C_HandleTypeDef hi2c3;
    uint8_t avail_bytes[2] = {0};

    // Read 2 bytes from register 0xFD
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c3, ADDR_GPS, 0xFD, I2C_MEMADD_SIZE_8BIT,
                                                avail_bytes, 2, 100);

    if(status == HAL_OK) {
        uint16_t available = (avail_bytes[0] << 8) | avail_bytes[1];
        v_printf_poll("[PASS] Available bytes: %d (0x%02X%02X)\r\n",
                      available, avail_bytes[0], avail_bytes[1]);

        if(available > 0) {
            v_printf_poll("       GPS has data ready to read!\r\n");
        } else {
            v_printf_poll("       No GPS data available yet\r\n");
        }
    } else {
        v_printf_poll("[FAIL] Could not read available bytes (status=%d)\r\n", status);
    }

    v_printf_poll("================================\r\n\r\n");
}

/*
 * Test 3: Read GPS Data Stream
 * Attempts to read actual GPS data (UBX protocol)
 */
void v_GPS_Test_Read_Data(void) {
    v_printf_poll("\r\n=== GPS Data Read Test ===\r\n");

    extern I2C_HandleTypeDef hi2c3;
    uint8_t data_buf[32] = {0};

    // Read from data stream register (0xFF)
    HAL_StatusTypeDef status = HAL_I2C_Mem_Read(&hi2c3, ADDR_GPS, 0xFF, I2C_MEMADD_SIZE_8BIT,
                                                data_buf, 32, 100);

    if(status == HAL_OK) {
        v_printf_poll("[PASS] Read 32 bytes from GPS:\r\n");
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
                v_printf_poll("       Found UBX header at offset %d!\r\n", i);
                found_ubx = true;
                break;
            }
        }
        if(!found_ubx) {
            v_printf_poll("       No UBX header found (GPS may be starting up)\r\n");
        }
    } else {
        v_printf_poll("[FAIL] Could not read GPS data (status=%d)\r\n", status);
    }

    v_printf_poll("==========================\r\n\r\n");
}

/*
 * Test 4: Full Diagnostic
 * Runs all tests in sequence
 */
void v_GPS_Test_Full_Diagnostic(void) {
    v_printf_poll("\r\n");
    v_printf_poll("╔════════════════════════════════════════╗\r\n");
    v_printf_poll("║   GPS I2C3 Full Diagnostic Test       ║\r\n");
    v_printf_poll("╚════════════════════════════════════════╝\r\n");

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
    v_printf_poll("\r\n=== GPS Driver Status ===\r\n");
    v_printf_poll("Fix Type:     %d\r\n", u8_GPS_GetFixType());
    v_printf_poll("Satellites:   %d\r\n", u8_GPS_GetNumSatellites());
    v_printf_poll("Latitude:     %.6f°\r\n", f_GPS_GetLatitude());
    v_printf_poll("Longitude:    %.6f°\r\n", f_GPS_GetLongitude());
    v_printf_poll("Altitude:     %.1f m\r\n", f_GPS_GetAltitude());
    v_printf_poll("Speed:        %.2f m/s\r\n", f_GPS_GetSpeed());
    v_printf_poll("=========================\r\n\r\n");

    v_printf_poll("Diagnostic complete!\r\n\r\n");
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

        v_printf_poll("\r\n╔════════════════════ GPS STATUS [%04lu] ════════════════════╗\r\n", test_count);

        // Check if GPS has fix
        if(b_GPS_HasFix()) {
            v_printf_poll("║ STATUS: ✓ FIX ACQUIRED                                    ║\r\n");
            v_printf_poll("║ -------------------------------------------------------- ║\r\n");
            v_printf_poll("║ Latitude:    %11.7f°                           ║\r\n", f_GPS_GetLatitude());
            v_printf_poll("║ Longitude:   %11.7f°                           ║\r\n", f_GPS_GetLongitude());
            v_printf_poll("║ Altitude:    %8.2f m (MSL)                         ║\r\n", f_GPS_GetAltitude());
            v_printf_poll("║ Speed:       %7.2f m/s                               ║\r\n", f_GPS_GetSpeed());
            v_printf_poll("║ Satellites:  %2d                                         ║\r\n", u8_GPS_GetNumSatellites());
            v_printf_poll("║ Fix Type:    %d (3=3D Fix)                              ║\r\n", u8_GPS_GetFixType());

            // Get raw PVT data for additional info
            _x_GPS_PVT_t* pvt = px_GPS_GetPVT();
            if(pvt) {
                v_printf_poll("║ Accuracy:    H=%lu.%03lum, V=%lu.%03lum                      ║\r\n",
                              (unsigned long)(pvt->hAcc / 1000),
                              (unsigned long)(pvt->hAcc % 1000),
                              (unsigned long)(pvt->vAcc / 1000),
                              (unsigned long)(pvt->vAcc % 1000));
                v_printf_poll("║ PDOP:        %d.%02d                                     ║\r\n",
                              pvt->pDOP / 100, pvt->pDOP % 100);
                v_printf_poll("║ Time (UTC):  %04d-%02d-%02d %02d:%02d:%02d                    ║\r\n",
                              pvt->year, pvt->month, pvt->day,
                              pvt->hour, pvt->min, pvt->sec);
                v_printf_poll("║ GPS TOW:     %lu ms                                  ║\r\n",
                              (unsigned long)pvt->iTOW);

                // Heading (if moving)
                int32_t head_deg = pvt->headMot / 100000;
                int32_t head_frac = pvt->headMot % 100000;
                if(head_frac < 0) head_frac = -head_frac;
                v_printf_poll("║ Heading:     %ld.%05ld°                              ║\r\n",
                              head_deg, head_frac);
            }
        } else {
            v_printf_poll("║ STATUS: ✗ NO FIX                                          ║\r\n");
            v_printf_poll("║ -------------------------------------------------------- ║\r\n");
            v_printf_poll("║ Fix Type:    %d (0=No Fix, 2=2D, 3=3D)                  ║\r\n", u8_GPS_GetFixType());
            v_printf_poll("║ Satellites:  %2d (Need 4+ for 3D fix)                   ║\r\n", u8_GPS_GetNumSatellites());
            v_printf_poll("║ Status:      Waiting for satellite lock...              ║\r\n");

            // Show raw data if available
            _x_GPS_PVT_t* pvt = px_GPS_GetPVT();
            if(pvt && pvt->numSV > 0) {
                v_printf_poll("║ Info:        %d satellites visible                      ║\r\n",
                              pvt->numSV);
            }
        }

        v_printf_poll("╚═══════════════════════════════════════════════════════════╝\r\n\r\n");
    }
}
