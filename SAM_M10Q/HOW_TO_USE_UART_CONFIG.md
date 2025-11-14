# GPS UART Configuration - 사용 가이드

**목적**: GPS 모듈을 UART를 통해 I2C 모드로 설정
**상황**: GPS가 I2C NAK 응답 (ErrorCode=0x00000004)
**해결**: UART로 CFG-PRT 명령 전송

---

## 1단계: GPS UART 연결 확인

### 하드웨어 연결 필요

GPS 모듈의 UART 핀이 STM32의 어떤 UART에 연결되어 있는지 확인:

```
GPS TX  →  STM32 UART RX  (예: PC11 = UART4_RX)
GPS RX  →  STM32 UART TX  (예: PC10 = UART4_TX)
GND     →  GND
```

### UART 보드율 확인

u-blox GPS 기본 보드율:
- **9600** (factory default)
- 38400 (일부 모델)
- 115200 (펌웨어에 따라)

---

## 2단계: 코드에 추가

### sam_m10q_platform.c에 추가

```c
#include "sam_m10q_uart_config.h"

void v_GPS_Init(void) {
    v_printf_poll("\r\n*** GPS INIT START ***\r\n");

    // STEP 0: UART를 통한 I2C 모드 활성화 (최초 1회만 필요)
    #ifdef GPS_UART_CONFIG_ENABLE
    extern UART_HandleTypeDef huart4;  // GPS가 연결된 UART (확인 필요!)

    v_printf_poll("GPS: Attempting UART configuration...\r\n");

    int uart_ret = i_GPS_ConfigureI2C_ViaUART(&huart4, 0x42, true);
    if(uart_ret == 0) {
        v_printf_poll("GPS: UART config SUCCESS - I2C mode enabled!\r\n");
        HAL_Delay(1000);  // GPS 재부팅 대기
    } else {
        v_printf_poll("GPS: UART config FAILED - GPS may already be in I2C mode\r\n");
        v_printf_poll("GPS: Continuing with I2C init...\r\n");
    }
    #endif

    // ... 기존 I2C 초기화 코드 ...
    extern I2C_HandleTypeDef hi2c3;
    // ... I2C3 reset sequence ...
}
```

### 컴파일 플래그 추가

**platformio.ini**:
```ini
[env:debug]
build_flags =
    -D GPS_UART_CONFIG_ENABLE  ; UART 설정 활성화 (최초 1회)
```

또는 **코드에서 직접**:
```c
#define GPS_UART_CONFIG_ENABLE  // 주석 처리하면 비활성화
```

---

## 3단계: 테스트

### 예상 로그 (성공 시)

```
*** GPS INIT START ***
GPS: Attempting UART configuration...

=== GPS UART CONFIGURATION START ===
GPS: Configuring I2C mode (addr=0x42) via UART
GPS: Testing UART connection...
GPS: UART connection OK ✓
GPS: Sending CFG-PRT (enable I2C, disable NMEA)
GPS UART: Sending UBX message (24 bytes)
GPS UART: TX = B5 62 06 00 14 00 00 00 00 00 42 00 00 00 01 00 ...
GPS UART: Waiting for ACK/NAK...
GPS UART: RX = B5 62 05 01 02 00 06 00 0F 38
GPS UART: ACK received ✓
GPS: CFG-PRT succeeded ✓
GPS: Sending CFG-SAVE (save to flash)
GPS UART: Sending UBX message (21 bytes)
GPS UART: TX = B5 62 06 09 0D 00 00 00 00 00 1F 00 00 00 00 00 ...
GPS UART: Waiting for ACK/NAK...
GPS UART: RX = B5 62 05 01 02 00 06 09 17 65
GPS UART: ACK received ✓
GPS: CFG-SAVE succeeded ✓
GPS: Configuration saved to flash (permanent)
GPS: I2C mode enabled successfully!
=== GPS UART CONFIGURATION COMPLETE ===

GPS: Please wait 1 second for GPS to reboot...

GPS: I2C3 State before reset=32 (0x20=READY)
... I2C initialization ...
GPS: CFG-PRT sent OK  ← I2C 이제 작동!
GPS: Available bytes=98
GPS: NAV-PVT parsed (Fix=0, Sats=0)
```

### 예상 로그 (UART 연결 안됨)

```
GPS: Testing UART connection...
GPS: WARNING - No response on UART
GPS: Check UART connection and baud rate
GPS UART: Transmit failed (ret=1)
GPS: CFG-PRT failed - GPS may already be in I2C mode
GPS: Continuing with I2C init...
```

### 예상 로그 (GPS 이미 I2C 모드)

```
GPS UART: NAK received ✗
GPS: CFG-PRT failed - GPS may already be in I2C mode
```
→ 이 경우 GPS는 이미 I2C 모드이므로 설정 불필요

---

## 4단계: 설정 완료 후

### 한번만 실행하면 됨!

GPS 플래시에 저장되므로, 다음부터는 UART 설정 불필요:

```c
// UART 설정 완료 후 주석 처리 또는 플래그 제거
// #define GPS_UART_CONFIG_ENABLE

void v_GPS_Init(void) {
    // UART 설정 코드 스킵
    // 바로 I2C 초기화
    extern I2C_HandleTypeDef hi2c3;
    // ...
}
```

---

## 문제 해결

### 1. UART Transmit failed
**원인**: UART가 초기화되지 않았거나 잘못된 UART 선택
**해결**:
- `MX_UART4_Init()`가 호출되었는지 확인
- 올바른 UART 핸들 사용 (`huart3`, `huart4`, `huart5` 등)

### 2. No response on UART
**원인**: 보드율 불일치 또는 TX/RX 핀 교차
**해결**:
- 보드율 변경: 9600, 38400, 115200 시도
- TX/RX 핀 확인 (GPS TX → MCU RX, GPS RX → MCU TX)

### 3. GPS UART: NAK received
**원인**: GPS가 명령을 거부 (이미 I2C 모드일 수 있음)
**해결**: I2C 통신 직접 테스트:
```c
HAL_StatusTypeDef ret = HAL_I2C_IsDeviceReady(&hi2c3, 0x42<<1, 3, 100);
if(ret == HAL_OK) {
    v_printf_poll("GPS already in I2C mode!\r\n");
}
```

### 4. CFG-SAVE failed
**원인**: 플래시 쓰기 실패 (드물게 발생)
**해결**:
- GPS 전원 재인가
- RAM 전용으로 설정 (save_to_flash = false)
- 다시 시도

---

## u-center를 통한 수동 설정 (대안)

UART가 STM32에 연결되지 않은 경우:

### 준비물
- GPS 모듈
- USB-to-UART 어댑터 (CP2102, FT232 등)
- u-blox u-center 소프트웨어

### 절차

1. **GPS를 PC에 연결**:
   ```
   GPS TX  →  UART Adapter RX
   GPS RX  →  UART Adapter TX
   GND     →  GND
   3.3V    →  3.3V (전원)
   ```

2. **u-center 실행**:
   - Receiver → Port 선택 → Connect

3. **I2C 설정**:
   - View → Configuration View
   - PRT (Ports) 선택
   - PortID: 0 (I2C/DDC)
   - Address: 0x42 (dec 66)
   - In/Out Protocol: 1 (UBX only)
   - Send 클릭

4. **플래시 저장**:
   - CFG (Configuration) 선택
   - Save current configuration 클릭
   - Devices: BBR, Flash, EEPROM 모두 체크
   - Send 클릭

5. **GPS 재부팅**:
   - Receiver → Reset → Cold Start
   - 또는 전원 재인가

6. **STM32 연결**:
   - PC에서 분리
   - STM32 I2C3에 연결
   - v_GPS_Init() 실행

---

## 요약

| 방법 | 장점 | 단점 |
|-----|------|------|
| **STM32 UART** | 자동화 가능, 펌웨어에 내장 | UART 연결 필요 |
| **u-center** | 한번만 설정, 쉬움 | 수동 작업, PC 필요 |

**권장**: u-center로 1회 설정 → 플래시 저장 → STM32 I2C 사용

---

## 최종 체크리스트

- [ ] GPS UART 핀 확인 (STM32 어느 UART?)
- [ ] UART 초기화 확인 (`MX_UARTx_Init()`)
- [ ] 보드율 확인 (9600/38400/115200)
- [ ] `sam_m10q_uart_config.h/c` 파일 추가
- [ ] `v_GPS_Init()`에 UART 설정 코드 추가
- [ ] 컴파일 및 플래시
- [ ] 시리얼 로그 확인 (ACK 수신?)
- [ ] I2C 통신 테스트 (CFG-PRT 성공?)
- [ ] 설정 완료 후 UART 코드 제거

---

**상태**: ✅ 구현 완료, 테스트 준비됨
**다음**: GPS UART 연결 확인 후 테스트 실행!
