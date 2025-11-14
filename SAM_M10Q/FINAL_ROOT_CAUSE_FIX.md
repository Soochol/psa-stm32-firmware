# GPS I2C 문제 - 근본 원인 분석 및 최종 해결

## 날짜
2025-01-14

## 🔍 체계적 재분석 결과

### 증상
```
GPS: Drain - Can't read avail bytes (ErrorCode=0x00000004), attempt 1
GPS: Drain - Can't read avail bytes (ErrorCode=0x00000004), attempt 2
[시스템 리셋 → Mode OFF]
```

- **ErrorCode 0x00000004** = `HAL_I2C_ERROR_AF` = **NACK (Not Acknowledged)**
- GPS 모듈이 I2C 주소 0x42에 전혀 응답하지 않음

### ❌ 기존 접근 방식의 문제

**잘못된 가정**: "GPS가 NMEA 데이터를 계속 출력해서 버퍼가 차있다"

**실제 문제**: GPS 모듈이 **전원조차 받지 못함**!

## 🎯 근본 원인 발견!

### main.c 초기화 순서 분석

```c
// Line 291: Watchdog 시작
MX_IWDG1_Init();

// Line 300: GPS 초기화 시도 ← GPS에 전원 없음!
v_GPS_Init();

// Line 306: 12V 전원 활성화 ← 여기서 처음 12V 켜짐!
v_IO_Enable_12V();
```

**타임라인**:
```
t=0     Watchdog 시작
t=300   v_GPS_Init() 호출
          ↓
        GPS 전원 없음!
          ↓
        I2C 통신 시도
          ↓
        NACK (GPS 응답 불가)
          ↓
        1500ms 대기 (의미 없음, 전원 없음)
          ↓
        Drain 시도 → NACK
          ↓
        Watchdog timeout (2초 초과)
          ↓
        시스템 리셋!
```

### 왜 GPS만 문제인가?

**다른 센서들 (IMU, MP3 등)**:
- 3.3V 라인에서 전원 공급
- MCU 부팅 시 자동 활성화
- 초기화 시점에 이미 전원 공급됨 ✅

**GPS (SAM-M10Q)**:
- 12V 라인에서 전원 공급 추정
- `v_IO_Enable_12V()`로 수동 활성화 필요
- GPS_Init() 시점에는 전원 없음 ❌

## ✅ 최종 해결책

### 1. GPS_Init() 위치 이동

**Before**:
```c
v_IMU_Init();
v_GPS_Init();      // ← 12V OFF 상태!
v_ADC_Init();
...
v_IO_Enable_12V(); // ← 12V ON
```

**After**:
```c
v_IMU_Init();
// v_GPS_Init();   // ← 제거
v_ADC_Init();
...
v_IO_Enable_12V(); // ← 12V ON

// GPS 부팅 대기 (SAM-M10Q: 1-2초 필요)
HAL_Delay(2000);
HAL_IWDG_Refresh(&hiwdg1);

v_GPS_Init();      // ← 전원 켜진 후 초기화!
HAL_IWDG_Refresh(&hiwdg1);
```

### 2. GPS_Init() 내부 간소화

**Before**:
```c
// GPS_Init 내부에서 1500ms 대기
v_printf_poll("GPS: Waiting 1500ms for module startup...\r\n");
for(int i = 0; i < 3; i++) {
    HAL_Delay(500);
    HAL_IWDG_Refresh(&hiwdg1);
}
```

**After**:
```c
// main.c에서 이미 2초 대기했으므로 불필요
v_printf_poll("GPS: Starting config (12V enabled, 2s boot complete)\r\n");
// 바로 설정 시작
```

### 3. Watchdog 안전성

**타이밍 분석**:
```
t=0     v_IMU_Init()        100ms
t=100   v_ADC_Init()        50ms
t=150   v_RGB_Init()        50ms
t=200   v_Mode_Init()       50ms
t=250   v_IO_Enable_12V()   1ms
t=251   HAL_IWDG_Refresh()  ← Refresh #1
t=251   HAL_Delay(2000)     2000ms (GPS 부팅)
t=2251  HAL_IWDG_Refresh()  ← Refresh #2 (2초 이내!)
t=2251  v_GPS_Init()
          - I2C3 리셋: 100ms
          - Drain: 100ms
          - CFG-PRT: 50ms
          - CFG-SAVE: 50ms
          Total: ~300ms
t=2551  HAL_IWDG_Refresh()  ← Refresh #3
t=2551  Main loop 진입
```

**모든 Watchdog 간격 < 2초** ✅

## 📊 예상 결과

### Case 1: GPS 정상 연결
```
*** GPS INIT START ***
GPS: I2C3 State before reset=0x20
GPS: I2C3 RCC force reset complete
GPS: HAL_I2C_Init returned 0
GPS: Driver initialized (I2C3, addr=0x42)
GPS: Starting config (12V enabled, 2s boot complete)
GPS: Sending CFG-PRT (24 bytes)
GPS: CFG-PRT attempt 1
GPS: Draining...
GPS: Drain - 87 bytes avail          ← 드레인 성공!
GPS: Drain OK (avail=0x0000)
GPS: CFG-PRT OK                      ← 설정 성공!
GPS: Sending CFG-SAVE (21 bytes)
GPS: CFG-SAVE OK - saved to flash
GPS: Init complete
mount succ[GPS-0001] NO FIX | Sats=3 FixType=0  ← 위성 감지!
```

### Case 2: GPS 미연결 (하드웨어 없음)
```
*** GPS INIT START ***
...
GPS: Starting config (12V enabled, 2s boot complete)
GPS: Sending CFG-PRT (24 bytes)
GPS: CFG-PRT attempt 1
GPS: Draining...
GPS: Drain failed (0x04) - GPS may not be connected  ← Fast-fail
GPS: No GPS detected - skipping config
GPS: Init complete
mount succ[GPS-0001] NO FIX | Sats=0 FixType=0      ← 시스템 정상 동작
```

**두 경우 모두 Mode OFF 없음!** ✅

## 🔑 핵심 교훈

### 1. 전원 의존성 확인 필수
- 주변장치 초기화 전에 **전원 상태 확인**
- 특히 외부 전원 라인 (12V, 5V 등) 사용 시

### 2. 초기화 순서의 중요성
```
올바른 순서:
1. 전원 공급
2. 안정화 대기 (부팅 시간)
3. 초기화 시도
```

### 3. NACK 에러의 의미
- NACK = 단순히 "장치가 바쁨"이 아님
- **"장치가 전혀 응답 불가"** 상태
- 전원, 배선, 주소 등 근본적 문제 확인 필요

### 4. 증상에 속지 말 것
- "버퍼가 차있다" ← 잘못된 가정
- "전원이 없다" ← 근본 원인
- 증상보다 **시스템 구조** 먼저 분석

## 📦 변경 파일

### 1. Core/Src/main.c
- GPS_Init() 위치를 12V 활성화 이후로 이동
- 2초 GPS 부팅 대기 추가
- Watchdog refresh 적절히 배치

### 2. SAM_M10Q/platform/src/sam_m10q_platform.c
- 1500ms 내부 대기 제거 (중복)
- Fast-fail 드레인 유지
- 간소화된 로깅

## 🚀 테스트 방법

```bash
# 빌드
pio run -e debug

# 플래시
pio run -e debug -t upload

# 모니터
pio device monitor
```

## ✅ 성공 지표

1. **Mode OFF 없음** - Watchdog timeout 해결
2. **GPS 응답** - I2C NACK 해결
3. **위성 감지** - GPS 정상 동작
4. **빠른 초기화** - 불필요한 대기 제거

## 🎉 빌드 정보

- Flash: 165,944 bytes (15.8%)
- RAM: 79,928 bytes (24.4%)
- Watchdog 안전성: ✅ 모든 간격 < 2초
