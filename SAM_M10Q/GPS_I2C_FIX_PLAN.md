# GPS I2C Communication Fix Plan

## 문제 분석

### 1. 콜백과 드라이버 간 상태 불일치
**위치**: [i2c.c:814](../User/Drv/src/i2c.c#L814), [i2c.c:907](../User/Drv/src/i2c.c#L907)

**문제**:
```c
// i2c.c - TX 완료 콜백
else if(hi2c == p_i2c3){
    e_comm_i2c3 = COMM_STAT_READY;  // ❌ 즉시 READY로 전환
    if(u8_i2c3_addr == ADDR_GPS){
        v_GPS_Write_DoneHandler(u8_i2c3_addr);  // GPS 핸들러 호출
    }
}

// GPS platform - 핸들러
void v_GPS_Write_DoneHandler(uint8_t u8_addr) {
    if(u8_addr == ADDR_GPS) {
        e_gps_comm = COMM_STAT_DONE;  // ✅ DONE으로 설정
    }
}

// GPS driver - bus 체크
static int i_GPS_Bus(void) {
    if(e_gps_comm == COMM_STAT_DONE) {
        e_gps_comm = COMM_STAT_READY;
        return 0;  // ✅ DONE → READY 전환
    }
    if(e_gps_comm == COMM_STAT_READY) {
        return 0;  // ✅ 버스 준비됨
    }
    return -1;  // ❌ 버스 사용 중
}
```

**결과**: i2c.c가 `READY`로 먼저 설정하므로 GPS 핸들러가 `DONE`으로 변경해도 드라이버 state machine이 제대로 동작하지 않음.

### 2. I2C3 상태와 GPS 상태의 분리 문제
**위치**: [sam_m10q_platform.c:293-316](../SAM_M10Q/platform/src/sam_m10q_platform.c#L293-L316)

**문제**:
- `e_comm_i2c3` (I2C3 하드웨어 상태): FIFO 큐 관리용
- `e_gps_comm` (GPS 애플리케이션 상태): GPS 프로토콜 상태

이 두 상태가 독립적으로 관리되어야 하는데, `i_GPS_Bus()`가 `e_gps_comm`만 체크함.

### 3. 읽기 데이터 처리 흐름 문제
**위치**: [sam_m10q_platform.c:200-228](../SAM_M10Q/platform/src/sam_m10q_platform.c#L200-L228)

**문제**:
```c
void v_GPS_Read_DoneHandler(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_len) {
    if(u8_addr == ADDR_GPS) {
        if(u16_len == 2) {
            // Available bytes 읽기
            px_gps->u16_availBytes = (pu8_arr[0] << 8) | pu8_arr[1];
        } else {
            // 실제 GPS 데이터
            memcpy(px_gps->u8_rxBuf, pu8_arr, u16_len);  // ✅ 복사
            px_gps->u16_rxLen = u16_len;
        }
        e_gps_comm = COMM_STAT_DONE;  // ✅ 완료 신호
    }
}
```

드라이버 state machine이 `WAIT_DATA` → `PARSE`로 전환될 때 데이터가 이미 준비되어 있어야 하는데, 타이밍 이슈 가능성.

## 🔧 해결 방법

### Fix 1: GPS 전용 통신 상태 플래그 사용 (권장)

현재 구조를 유지하되, GPS가 I2C3의 유일한 장치라는 점을 활용.

**수정 위치**: [sam_m10q_platform.c:293-316](../SAM_M10Q/platform/src/sam_m10q_platform.c#L293-L316)

```c
static int i_GPS_Bus(void) {
    // 디버그 로그 제거 또는 주기 증가 (5초 → 30초)
    static uint32_t last_log = 0;
    uint32_t now = u32_Tim_1msGet();
    if((now - last_log) > 30000) {  // 30초마다 로그
        v_printf_poll("DEBUG: GPS bus - e_gps_comm=%d\r\n", e_gps_comm);
        last_log = now;
    }

    // FIX: GPS 상태만 체크 (I2C3 하드웨어 상태는 i2c.c가 관리)
    if(e_gps_comm == COMM_STAT_DONE) {
        e_gps_comm = COMM_STAT_READY;
        return 0;  // 작업 완료, 다음 작업 가능
    }

    if(e_gps_comm == COMM_STAT_READY) {
        return 0;  // 버스 준비됨
    }

    return -1;  // 버스 사용 중
}
```

**설명**:
- `e_gps_comm`은 GPS 프로토콜 레벨 상태 (드라이버 전용)
- `e_comm_i2c3`는 I2C3 하드웨어 상태 (FIFO 큐 관리)
- 두 상태는 독립적으로 동작

### Fix 2: i2c.c 콜백 순서 수정

**수정 위치**: [i2c.c:813-818](../User/Drv/src/i2c.c#L813-L818)

```c
else if(hi2c == p_i2c3){
    // FIX: GPS 핸들러를 먼저 호출한 후 READY로 전환
    if(u8_i2c3_addr == ADDR_GPS){
        v_GPS_Write_DoneHandler(u8_i2c3_addr);  // GPS 상태를 DONE으로 설정
    }
    e_comm_i2c3 = COMM_STAT_READY;  // I2C3 하드웨어 상태 READY
}
```

**수정 위치**: [i2c.c:906-913](../User/Drv/src/i2c.c#L906-L913)

```c
else if(hi2c == p_i2c3){
    // FIX: GPS 핸들러를 먼저 호출한 후 READY로 전환
    if(u8_i2c3_addr == ADDR_GPS){
        v_GPS_Read_DoneHandler(u8_i2c3_addr, u8_i2c3_rdArr, u16_i2c3_rdCnt);
    }
    e_comm_i2c3 = COMM_STAT_READY;
    u8_i2c3_addr = 0;
    u16_i2c3_rdCnt = 0;
}
```

### Fix 3: 드라이버 State Machine 타이밍 개선

**수정 위치**: [sam_m10q_driver.c:75-80](../SAM_M10Q/core/src/sam_m10q_driver.c#L75-L80)

```c
case SAM_M10Q_STATE_WAIT_AVAIL:
    // Wait for I2C read callback to complete
    if(px_drv->tr.i_bus() == 0) {  // Callback finished, bus is ready
        // FIX: 데이터 처리 확인 후 전환
        if(px_drv->u16_availBytes > 0) {
            v_printf_poll("GPS: Available bytes detected: %d\r\n", px_drv->u16_availBytes);
        }
        px_drv->e_state = SAM_M10Q_STATE_READ_DATA;
    }
    break;
```

**수정 위치**: [sam_m10q_driver.c:100-105](../SAM_M10Q/core/src/sam_m10q_driver.c#L100-L105)

```c
case SAM_M10Q_STATE_WAIT_DATA:
    // Wait for data stream read callback
    if(px_drv->tr.i_bus() == 0) {  // Callback finished
        // FIX: 데이터 수신 확인
        if(px_drv->u16_rxLen > 0) {
            v_printf_poll("GPS: Data received: %d bytes\r\n", px_drv->u16_rxLen);
            px_drv->e_state = SAM_M10Q_STATE_PARSE;
        } else {
            // 데이터가 없으면 IDLE로 복귀
            px_drv->e_state = SAM_M10Q_STATE_IDLE;
        }
    }
    break;
```

### Fix 4: 콜백 데이터 복사 확인

**수정 위치**: [sam_m10q_platform.c:200-228](../SAM_M10Q/platform/src/sam_m10q_platform.c#L200-L228)

```c
void v_GPS_Read_DoneHandler(uint8_t u8_addr, uint8_t* pu8_arr, uint16_t u16_len) {
    if(u8_addr == ADDR_GPS) {
        // Check if this was reading available bytes or actual data
        if(u16_len == 2) {
            // Available bytes count (registers 0xFD, 0xFE) - big-endian
            px_gps->u16_availBytes = (pu8_arr[0] << 8) | pu8_arr[1];
            v_printf_poll("GPS: Available bytes=%d\r\n", px_gps->u16_availBytes);
        } else {
            // FIX: 버퍼 오버플로우 방지
            if(u16_len <= sizeof(px_gps->u8_rxBuf)) {
                memcpy(px_gps->u8_rxBuf, pu8_arr, u16_len);
                px_gps->u16_rxLen = u16_len;

                v_printf_poll("GPS: Received %d bytes\r\n", u16_len);
                // Hex dump 간소화 (첫 16바이트만)
                v_printf_poll("GPS: Data = ");
                uint16_t dump_len = (u16_len > 16) ? 16 : u16_len;
                for(uint16_t i = 0; i < dump_len; i++) {
                    v_printf_poll("%02X ", pu8_arr[i]);
                }
                v_printf_poll("%s\r\n", (u16_len > 16) ? "..." : "");
            } else {
                v_printf_poll("GPS: ERROR - Buffer overflow! len=%d, bufSize=%d\r\n",
                              u16_len, sizeof(px_gps->u8_rxBuf));
                px_gps->u16_rxLen = 0;  // 데이터 무효화
            }
        }
        e_gps_comm = COMM_STAT_DONE;
    }
}
```

## 🧪 테스트 계획

### 1. I2C 하드웨어 테스트
```c
// GPS 초기화 후 실행
void v_GPS_I2C_Hardware_Test(void) {
    extern I2C_HandleTypeDef hi2c3;

    // 1. I2C Device Ready 테스트
    v_printf_poll("TEST: Checking GPS I2C presence...\r\n");
    HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(&hi2c3, ADDR_GPS, 3, 100);
    v_printf_poll("TEST: IsDeviceReady = %d (0=OK, 1=ERROR)\r\n", ret);

    // 2. Available Bytes 읽기 테스트 (동기식)
    uint8_t avail_buf[2];
    ret = HAL_I2C_Mem_Read(&hi2c3, ADDR_GPS, SAM_M10Q_REG_AVAIL_MSB,
                           I2C_MEMADD_SIZE_8BIT, avail_buf, 2, 200);
    if(ret == HAL_OK) {
        uint16_t avail = (avail_buf[0] << 8) | avail_buf[1];
        v_printf_poll("TEST: Available bytes (blocking) = %d\r\n", avail);
    } else {
        v_printf_poll("TEST: Failed to read available bytes (ret=%d)\r\n", ret);
    }

    // 3. 데이터 읽기 테스트 (동기식)
    if(ret == HAL_OK && avail > 0) {
        uint8_t data_buf[64];
        uint16_t read_len = (avail > 64) ? 64 : avail;
        ret = HAL_I2C_Mem_Read(&hi2c3, ADDR_GPS, SAM_M10Q_REG_STREAM,
                               I2C_MEMADD_SIZE_8BIT, data_buf, read_len, 500);
        if(ret == HAL_OK) {
            v_printf_poll("TEST: Data read OK (%d bytes)\r\n", read_len);
            v_printf_poll("TEST: Data = ");
            for(int i = 0; i < 16 && i < read_len; i++) {
                v_printf_poll("%02X ", data_buf[i]);
            }
            v_printf_poll("\r\n");
        } else {
            v_printf_poll("TEST: Data read FAILED (ret=%d)\r\n", ret);
        }
    }
}
```

### 2. State Machine 디버그
드라이버의 상태 전환을 모니터링:
```c
// sam_m10q_driver.c - 각 상태 진입 시 로그 추가
v_printf_poll("GPS State: %s → %s\r\n",
              state_names[prev_state],
              state_names[px_drv->e_state]);
```

### 3. 타이밍 검증
```c
// 각 I2C 작업의 시간 측정
static uint32_t start_time = 0;
case SAM_M10Q_STATE_CHECK_AVAIL:
    start_time = u32_Tim_1msGet();
    // ... I2C 작업
    break;

case SAM_M10Q_STATE_WAIT_AVAIL:
    if(bus_ready) {
        uint32_t elapsed = u32_Tim_1msGet() - start_time;
        v_printf_poll("GPS: I2C read took %lu ms\r\n", elapsed);
    }
    break;
```

## 📝 수정 우선순위

1. **우선순위 1** (필수): Fix 1 + Fix 2 - 콜백 순서 수정
2. **우선순위 2** (권장): Fix 3 - State machine 타이밍 개선
3. **우선순위 3** (선택): Fix 4 - 데이터 검증 강화
4. **우선순위 4** (디버그): 테스트 코드 추가

## 🔍 예상 결과

수정 후 정상 동작 시나리오:
```
GPS: Checking I2C presence at 0x42...
GPS: I2C device ACK OK - GPS is in I2C mode
GPS State: IDLE → CHECK_AVAIL
GPS State: CHECK_AVAIL → WAIT_AVAIL
GPS: Available bytes=98
GPS State: WAIT_AVAIL → READ_DATA
GPS State: READ_DATA → WAIT_DATA
GPS: Received 98 bytes
GPS: Data = B5 62 01 07 5C 00 ...
GPS State: WAIT_DATA → PARSE
GPS: Lat=37.123456 Lon=127.123456 Alt=50.0m Sats=8 Fix=3
```

## ⚠️ 주의사항

1. **IWDG 리프레시**: GPS 처리 중 Watchdog refresh 필수
2. **버퍼 크기**: I2C3_RD_SIZE (128 bytes)가 UBX-NAV-PVT (100 bytes)보다 큼 확인
3. **타이밍**: GPS 폴링 주기 (1000ms)와 I2C 타임아웃 (2000ms) 조화
4. **GPS 모드**: GPS가 I2C 모드여야 함 (UART 모드 시 NAK 발생)