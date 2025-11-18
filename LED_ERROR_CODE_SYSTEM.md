# LED 에러 코드 시스템

STM32H723 펌웨어는 5비트 LED 패턴을 사용하여 기기 에러를 시각적으로 표시합니다.

## LED 배치

```
┌─────────────────────────────────────┐
│     [5] [4] [3] [2] [1]  (상단)    │  ← 5비트 패턴
│                                     │
│         기기 본체                    │
│                                     │
│     [5] [4] [3] [2] [1]  (하단)    │  ← 미러 패턴
└─────────────────────────────────────┘
```

- **LED 총 개수**: 10개 (상단 5개 + 하단 5개 미러)
- **깜빡임 주기**: 500ms (0.5초) ON/OFF
- **에러 색상**: 빨강-오렌지 (RGB: 255, 77, 79)
- **비트 순서**: LED1=bit0(LSB), LED5=bit4(MSB)

## 에러 코드 매핑표

| 에러 이름 | 16진수 코드 | 5비트 패턴 | LED 시각화 [5][4][3][2][1] | 켜지는 LED | 한글 설명 | 부품 |
|----------|------------|-----------|---------------------------|-----------|---------|------|
| `modeERR_TEMP_IR` | 0x0001 | 0b00001 | ○○○○● | LED1 | 열화상 카메라 | MLX90640 |
| `modeERR_TEMP_OUT` | 0x0002 | 0b00010 | ○○○●○ | LED2 | 온도 센서 | AS6221 |
| `modeERR_TOF` | 0x0040 | 0b00011 | ○○○●● | LED1,2 | 거리 센서 | VL53L0X |
| `modeERR_IMU` | 0x0008 | 0b00100 | ○○●○○ | LED3 | 자이로 센서 | ICM42670P |
| `modeERR_FSR` | 0x0100 | 0b00101 | ○○●○● | LED1,3 | 압력 센서 | ADS111x |
| `modeERR_AUDIO` | 0x0080 | 0b00110 | ○○●●○ | LED2,3 | 오디오 모듈 | ES8388 |
| `modeERR_MP3_FILE` | 0x0400 | 0b00111 | ○○●●● | LED1,2,3 | MP3 파일 없음 | SD/FatFS |
| `modeERR_SD_MOUNT` | 0x0200 | 0b01000 | ○●○○○ | LED4 | SD 카드 오류 | SDMMC2 |
| `modeERR_BLOW_FAN` | 0x0010 | 0b01001 | ○●○○● | LED1,4 | 송풍팬 고장 | TIM2_CH2 |
| `modeERR_COOL_FAN` | 0x0020 | 0b01010 | ○●○●○ | LED2,4 | 쿨링팬 고장 | TIM2_CH4 |
| `modeERR_ESP_COMM` | 0x1000 | 0b01011 | ○●○●● | LED1,2,4 | WiFi 통신 끊김 | ESP32 |
| `modeERR_HEATER_CURR` | 0x0800 | 0b11111 | ●●●●● | **전체 5개** | **히터 과전류 (긴급!)** | TIM2_CH1 |

**범례:**
- ● = LED 켜짐 (빨강-오렌지 깜빡임)
- ○ = LED 꺼짐 (항상 꺼짐)

## 시각적 예시

### 예시 1: 온도 센서 에러 (AS6221)

**에러 코드**: `modeERR_TEMP_OUT` (0x0002)

**LED 패턴**: 0b00010

```
┌─────────────────────────────────────┐
│     [○] [○] [○] [●] [○]  (상단)    │
│                                     │
│         기기 본체                    │
│                                     │
│     [○] [○] [○] [●] [○]  (하단)    │
└─────────────────────────────────────┘

깜빡임: 500ms 켜짐 → 500ms 꺼짐 → 반복
색상: 빨강-오렌지 (RGB: 255, 77, 79)
→ LED2만 깜빡임 = 온도 센서 문제
```

### 예시 2: 자이로 센서 에러 (ICM42670P)

**에러 코드**: `modeERR_IMU` (0x0008)

**LED 패턴**: 0b00100

```
┌─────────────────────────────────────┐
│     [○] [○] [●] [○] [○]  (상단)    │
│                                     │
│         기기 본체                    │
│                                     │
│     [○] [○] [●] [○] [○]  (하단)    │
└─────────────────────────────────────┘

깜빡임: 500ms 켜짐 → 500ms 꺼짐 → 반복
색상: 빨강-오렌지 (RGB: 255, 77, 79)
→ LED3만 깜빡임 = IMU 센서 문제
```

### 예시 3: 히터 과전류 (긴급!)

**에러 코드**: `modeERR_HEATER_CURR` (0x0800)

**LED 패턴**: 0b11111

```
┌─────────────────────────────────────┐
│     [●] [●] [●] [●] [●]  (상단)    │
│                                     │
│         기기 본체                    │
│                                     │
│     [●] [●] [●] [●] [●]  (하단)    │
└─────────────────────────────────────┘

깜빡임: 500ms 켜짐 → 500ms 꺼짐 → 반복
색상: 빨강-오렌지 (RGB: 255, 77, 79)
→ 전체 LED 깜빡임 = 긴급 상황! 즉시 전원 차단 필요
```

## 현장 진단 가이드

### 빠른 참조표

| LED 패턴 | 2진수 | 가능한 원인 | 첫 번째 확인사항 |
|---------|------|----------|---------------|
| ○○○○● | 00001 | MLX90640 열화상 카메라 | I2C1 버스, 카메라 전원 |
| ○○○●○ | 00010 | AS6221 온도 센서 | I2C1 버스, 센서 연결 |
| ○○●○○ | 00100 | ICM42670P IMU/자이로 | I2C2 버스, IMU 전원 |
| ○●○○○ | 01000 | SD 카드 마운트 실패 | SD 카드 삽입? FAT32 포맷? |
| ○●○●● | 01011 | ESP32 WiFi 연결 끊김 | ESP32 전원, UART 연결 |
| ●●●●● | 11111 | **히터 과전류** | **위험! 히터 회로 점검** |

### 진단 절차

1. **LED 패턴 관찰**: 기기 상단 또는 하단의 LED 확인
2. **켜진 LED 세기**: 오른쪽부터 왼쪽으로 (LED1, LED2, LED3, LED4, LED5)
3. **매핑표 참조**: 위 표에서 패턴 검색
4. **부품 점검**: 해당 부품 확인
5. **UART 연결**: 상세 로그 확인 (115200 baud)

### 시리얼 콘솔 진단

에러 모드 진입 시 출력 예시:

```
[ERROR] MODE: *** Actuator shutdown initiated ***
[INFO]  MODE:   Heater: OFF (PWM=0, PID reset)
[INFO]  MODE:   Heat Pad: OFF (PWM=0)
[INFO]  MODE:   Blow Fan: OFF (PWM=0)
[INFO]  MODE:   DO_PAD_EN: RESET (GPIO=LOW, hardware cutoff)
[INFO]  MODE:   Cooling Fan: ON (residual heat removal)
[ERROR] MODE: *** Actuator shutdown complete ***
[INFO]  MODE: State change: 7 -> 8 (error=0x0002)
[ERROR] MODE: Entering ERROR mode - GPS handler disabled
```

## 에러 코드 정의

소스: `User/Edit/inc/mode.h` (lines 169-184)

```c
typedef enum {
    modeERR_TEMP_IR     = 0x0001,  // 0b0000000000001 - 열화상 카메라
    modeERR_TEMP_OUT    = 0x0002,  // 0b0000000000010 - 온도 센서
    modeERR_TOF         = 0x0004,  // 0b0000000000100 - 거리 센서
    modeERR_IMU         = 0x0008,  // 0b0000000001000 - 자이로 센서
    modeERR_BLOW_FAN    = 0x0010,  // 0b0000000010000 - 송풍팬
    modeERR_COOL_FAN    = 0x0020,  // 0b0000000100000 - 쿨링팬
    modeERR_AUDIO       = 0x0080,  // 0b0000010000000 - 오디오 모듈
    modeERR_FSR         = 0x0100,  // 0b0000100000000 - 압력 센서
    modeERR_SD_MOUNT    = 0x0200,  // 0b0001000000000 - SD 카드 오류
    modeERR_MP3_FILE    = 0x0400,  // 0b0010000000000 - MP3 파일 없음
    modeERR_HEATER_CURR = 0x0800,  // 0b0100000000000 - 히터 과전류
    modeERR_ESP_COMM    = 0x1000,  // 0b1000000000000 - WiFi 통신
} e_modeERR_t;
```

## LED 패턴 테이블 구현

소스: `User/Edit/src/mode.c` (lines 1906-1919)

```c
// 에러별 LED 5비트 패턴 정의
typedef struct {
    e_modeERR_t error_code;   // 에러 코드
    uint8_t led_pattern;      // 5비트 패턴 (0-31)
} s_ERROR_LED_CONFIG_t;

// 에러별 LED 패턴 테이블 (깜빡임 속도는 모두 500ms로 통일)
// LED 배치: [5][4][3][2][1] (왼쪽부터 오른쪽)
// 비트 순서: LED1=bit0(LSB), LED5=bit4(MSB)
static const s_ERROR_LED_CONFIG_t ERROR_LED_TABLE[] = {
    {modeERR_TEMP_IR,     0b00001},  // ○○○○● - 열화상 카메라
    {modeERR_TEMP_OUT,    0b00010},  // ○○○●○ - 온도 센서
    {modeERR_TOF,         0b00011},  // ○○○●● - 거리 센서
    {modeERR_IMU,         0b00100},  // ○○●○○ - 자이로 센서
    {modeERR_FSR,         0b00101},  // ○○●○● - 압력 센서
    {modeERR_AUDIO,       0b00110},  // ○○●●○ - 오디오 모듈
    {modeERR_MP3_FILE,    0b00111},  // ○○●●● - MP3 파일 없음
    {modeERR_SD_MOUNT,    0b01000},  // ○●○○○ - SD 카드 오류
    {modeERR_BLOW_FAN,    0b01001},  // ○●○○● - 송풍팬 고장
    {modeERR_COOL_FAN,    0b01010},  // ○●○●○ - 쿨링팬 고장
    {modeERR_ESP_COMM,    0b01011},  // ○●○●● - WiFi 통신 끊김
    {modeERR_HEATER_CURR, 0b11111},  // ●●●●● - 히터 과전류 (긴급!)
};
```

## 핵심 함수

### 패턴 검색 함수

소스: `User/Edit/src/mode.c` (lines 1923-1932)

```c
static uint8_t u8_Get_Error_LED_Pattern(e_modeERR_t err) {
    // 테이블에서 첫 번째 매칭되는 에러 찾기
    for(uint8_t i = 0; i < ERROR_LED_TABLE_SIZE; i++) {
        if(ERROR_LED_TABLE[i].error_code & err) {
            return ERROR_LED_TABLE[i].led_pattern;
        }
    }
    // 매칭 실패 시 기본 패턴 (모든 LED 켜짐)
    return 0b11111;
}
```

### LED 패턴 적용 함수

소스: `User/Edit/src/mode.c` (lines 1935-1957)

```c
static void v_Set_Error_LED_Pattern(uint8_t pattern, bool blink_state) {
    uint8_t R = blink_state ? MODE_ERROR_LED_R : 0;
    uint8_t G = blink_state ? MODE_ERROR_LED_G : 0;
    uint8_t B = blink_state ? MODE_ERROR_LED_B : 0;

    // 5개 LED를 비트마스크에 따라 개별 제어
    for(uint8_t i = 0; i < 5; i++) {
        bool led_on = (pattern & (1 << i)) ? true : false;

        if(led_on) {
            // 이 LED는 패턴에 포함 → 깜빡임 상태에 따라 ON/OFF
            b_RGB_Set_Color(RGB_TOP_1 + i, R, G, B);
            b_RGB_Set_Color(RGB_BOT_1 + i, R, G, B);  // 하단 미러
        } else {
            // 이 LED는 패턴에 없음 → 항상 OFF
            b_RGB_Set_Color(RGB_TOP_1 + i, 0, 0, 0);
            b_RGB_Set_Color(RGB_BOT_1 + i, 0, 0, 0);
        }
    }

    // CRITICAL: SK6812 하드웨어로 RGB PWM 출력
    v_RGB_Refresh_Enable();
}
```

### 에러 LED 핸들러

소스: `User/Edit/src/mode.c` (lines 1959-1969)

```c
static void v_Mode_Error_Led(uint16_t u16_toggle){
    // 0.5초마다 깜빡임 (5비트 패턴 사용)
    e_modeERR_t err = e_Mode_Get_Error();
    uint8_t pattern = u8_Get_Error_LED_Pattern(err);

    // 깜빡임 상태: 홀수=OFF, 짝수=ON
    bool blink_state = (u16_toggle & 1) ? false : true;

    // 패턴 적용
    v_Set_Error_LED_Pattern(pattern, blink_state);
}
```

## 테스트 함수

소스: `User/Edit/src/mode.c` (lines 2462-2483)

```c
void v_Mode_Error_LED_Test(e_modeERR_t test_error) {
    LOG_INFO("MODE", "Testing error pattern for code: 0x%04X", test_error);

    // 에러 강제 설정
    e_modeError = test_error;
    v_Mode_SetNext(modeERROR);

    // 예상 패턴 출력
    uint8_t pattern = u8_Get_Error_LED_Pattern(test_error);
    LOG_INFO("MODE", "  LED Pattern (5-bit): 0b%c%c%c%c%c",
           (pattern & 0b10000) ? '1' : '0',
           (pattern & 0b01000) ? '1' : '0',
           (pattern & 0b00100) ? '1' : '0',
           (pattern & 0b00010) ? '1' : '0',
           (pattern & 0b00001) ? '1' : '0');
}
```

### 사용 예시

```c
// UART 콘솔에서 특정 에러 패턴 테스트
v_Mode_Error_LED_Test(modeERR_TEMP_OUT);

// 출력:
// [INFO][MODE] Testing error pattern for code: 0x0002
// [INFO][MODE]   LED Pattern (5-bit): 0b00010
```

## I2C 버스 할당

| 버스 | 속도 | 센서 | 주소 | 에러 코드 |
|-----|------|------|------|----------|
| I2C1 | 400kHz | MLX90640 (IR 카메라) | 0x33 | `modeERR_TEMP_IR` |
| I2C1 | 400kHz | AS6221 (온도 센서) | 0x48 | `modeERR_TEMP_OUT` |
| I2C1 | 400kHz | ADS111x (압력 센서) | 0x48 | `modeERR_FSR` |
| I2C2 | 400kHz | ICM42670P (IMU) | 0x68 | `modeERR_IMU` |
| I2C4 | 400kHz | VL53L0X (거리 센서) | - | `modeERR_TOF` |
| I2C5 | 400kHz | VL53L0X (거리 센서) | - | `modeERR_TOF` |

## PWM 할당 (TIM2)

| 채널 | GPIO | 기능 | 에러 코드 | ERROR 모드 동작 |
|------|------|------|----------|---------------|
| TIM2_CH1 | PA0 | 히터 PWM | `modeERR_HEATER_CURR` | **정지** (PWM=0) |
| TIM2_CH2 | PA1 | 송풍팬 PWM | `modeERR_BLOW_FAN` | **정지** (PWM=0) |
| TIM2_CH3 | PA2 | 히트패드 PWM | - | **정지** (PWM=0) |
| TIM2_CH4 | PA3 | 쿨링팬 PWM | `modeERR_COOL_FAN` | **계속 작동** (냉각) |

## ERROR 모드 안전 셧다운

에러 모드 진입 시 이중 안전 셧다운 실행:

### 1단계: 소프트웨어 PWM 셧다운

```c
v_Mode_Heater_Off();        // 히터 PWM → 0, PID 리셋
v_Mode_HeatPad_Disable();   // 히트패드 PWM → 0
v_Mode_BlowFan_Disable();   // 송풍팬 PWM → 0
```

### 2단계: 하드웨어 GPIO 차단

```c
HAL_GPIO_WritePin(DO_PAD_EN_GPIO_Port, DO_PAD_EN_Pin, GPIO_PIN_RESET);
```

### 계속 동작하는 시스템

- ✅ 쿨링팬 (잔열 제거)
- ✅ LED 에러 패턴 표시
- ✅ ESP32 통신
- ✅ UART 디버그 출력

## 부록: 전체 에러 참조표

| 에러 | Hex | Bit | LED 패턴 | 부품 | 인터페이스 | 타임아웃 |
|------|-----|-----|---------|------|-----------|---------|
| TEMP_IR | 0x0001 | 0 | ○○○○● | MLX90640 | I2C1 (0x33) | 2s |
| TEMP_OUT | 0x0002 | 1 | ○○○●○ | AS6221 | I2C1 (0x48) | 2s |
| TOF | 0x0040 | 6 | ○○○●● | VL53L0X | I2C4/I2C5 | 2s |
| IMU | 0x0008 | 3 | ○○●○○ | ICM42670P | I2C2 (0x68) | 2s |
| FSR | 0x0100 | 8 | ○○●○● | ADS111x | I2C1 (0x48) | 2s |
| AUDIO | 0x0080 | 7 | ○○●●○ | ES8388 | SAI1 (I2S) | - |
| MP3_FILE | 0x0400 | 10 | ○○●●● | SD Card | SDMMC2 | - |
| SD_MOUNT | 0x0200 | 9 | ○●○○○ | FatFS | SDMMC2 | - |
| BLOW_FAN | 0x0010 | 4 | ○●○○● | TIM2_CH2 | PWM (PA1) | - |
| COOL_FAN | 0x0020 | 5 | ○●○●○ | TIM2_CH4 | PWM (PA3) | - |
| ESP_COMM | 0x1000 | 12 | ○●○●● | ESP32 | UART8 | - |
| HEATER_CURR | 0x0800 | 11 | ●●●●● | TIM2_CH1 | PWM (PA0) | 5s |

## 안전 및 규정 준수

### 의료기기 표준

**IEC 60601-1 준수**: ERROR 모드에서 이중 안전 셧다운
- **1단계**: 소프트웨어 PWM = 0
- **2단계**: 하드웨어 GPIO 차단 (DO_PAD_EN = LOW)

### 긴급 에러 처리

**히터 과전류** (`modeERR_HEATER_CURR`):
- **패턴**: 전체 5개 LED 깜빡임 (0b11111)
- **안전 조치**: 즉시 히터 PWM 차단 + GPIO 비활성화
- **우선순위**: 최고 (긴급 상황)
- **사용자 조치**: 기기 사용 중지, 서비스 센터 연락

---

**문서 버전**: 1.0
**최종 업데이트**: 2025-11-18
**펌웨어 버전**: WithForce 1.00.34
**타겟 MCU**: STM32H723VGTx
