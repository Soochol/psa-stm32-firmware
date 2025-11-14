# GPS I2C Initialization - Final Fix Summary

## 날짜
2025-01-14

## 문제 분석

### 원본 에러 로그
```
GPS: I2C3 State after HAL_I2C_Init=32
GPS: CFG-PRT send failed (status=1, ErrorCode=0x00000004)
GPS: CFG-SAVE send failed (status=1, ErrorCode=0x00000004)
GPS_Bus: BUSY (e_gps_comm=2, returning -1)
```

### 근본 원인
1. **I2C State 오해**: `State=32` (0x20) = `HAL_I2C_STATE_READY` (정상)
2. **GPS NACK 에러**: `ErrorCode=0x00000004` = `HAL_I2C_ERROR_AF` (Acknowledge Failure)
3. **핵심 문제**: GPS 모듈이 부팅 후 **계속 NMEA 데이터를 출력**하여 TX 버퍼가 차있음
4. **Write 실패**: 버퍼가 차있는 동안 I2C Write 시도 → GPS가 NACK 응답

## SparkFun Arduino 라이브러리 분석

### 참고한 핵심 코드
```cpp
// SparkFun 예제 (Example1_NAV_PVT.ino)
myGNSS.begin()  // 내부적으로 isConnected() 최대 3회 재시도
myGNSS.setI2COutput(COM_TYPE_UBX)  // NMEA 끄고 UBX만 출력
```

### SparkFun 라이브러리의 핵심 전략
1. **isConnected() 재시도**: NEO-M8U가 가끔 명령을 무시하는 문제 대응
2. **I2C ACK 확인**: `beginTransmission()` + `endTransmission()`
3. **Port 설정 읽기**: 실제 통신 가능 여부 검증

## 최종 구현 (v2025-01-14 + SPARKFUN FIX)

### 1. 데이터 드레인 함수 추가
```c
static int i_GPS_DrainPendingData(I2C_HandleTypeDef* hi2c, uint8_t addr)
```
- 0xFD/0xFE 레지스터에서 Available Bytes 읽기
- 0 또는 0xFFFF 될 때까지 반복
- 128바이트 단위로 데이터 읽고 버림
- 최대 10회 시도, Watchdog refresh 포함

### 2. 개선된 초기화 시퀀스

```
┌─────────────────────────────────────────┐
│ 1. I2C3 하드웨어 리셋                    │
│    - GPIO 리셋 (v_I2C3_Pin_Deinit)      │
│    - HAL DeInit                         │
│    - RCC Force Reset                    │
│    - HAL ReInit                         │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│ 2. GPS 부팅 대기 (1500ms)               │
│    - 500ms씩 3번 나눠서 대기            │
│    - 각 단계마다 Watchdog refresh       │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│ 3. CFG-PRT 전송 (최대 3회 재시도)       │
│    ┌─────────────────────────────────┐  │
│    │ Drain → Write (즉시) → 성공?   │  │
│    └─────────────────────────────────┘  │
│             ↓ 실패                       │
│    ┌─────────────────────────────────┐  │
│    │ 300ms 대기 → Drain → Write     │  │
│    └─────────────────────────────────┘  │
│             ↓ 실패                       │
│    ┌─────────────────────────────────┐  │
│    │ 300ms 대기 → Drain → Write     │  │
│    └─────────────────────────────────┘  │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│ 4. CFG-SAVE 전송 (최대 3회 재시도)      │
│    - CFG-PRT과 동일한 패턴              │
│    - 재시도마다 Drain 먼저 실행         │
└─────────────────────────────────────────┘
```

### 3. 핵심 개선사항

#### A. Drain-Before-Retry 패턴 ⭐
**문제점**:
```
Drain() → 버퍼 비움
   ↓
HAL_Delay(500ms) → GPS가 NMEA 출력 → 버퍼 다시 참!
   ↓
Write CFG-PRT → NACK (버퍼 full)
```

**해결책**:
```c
while(cfg_attempts < 3) {
    // 각 시도마다 먼저 드레인
    i_GPS_DrainPendingData(&hi2c3, ADDR_GPS);

    // 드레인 직후 바로 전송 (버퍼 refill 시간 최소화)
    cfg_ret = HAL_I2C_Mem_Write(...);

    if(cfg_ret == HAL_OK) break;
    HAL_Delay(300);  // 재시도 전 짧은 대기
}
```

#### B. Watchdog 타임아웃 방지
- **문제**: 1500ms 대기 + 드레인 시간 > 2초 → Mode OFF 전환
- **해결**:
  - 1500ms를 500ms x 3회로 분할, 각 단계마다 refresh
  - 드레인 함수 내부에서도 매 루프마다 refresh
  - CFG-PRT/SAVE 재시도 루프에서도 refresh

#### C. 개선된 로깅
- I2C State를 hex로 표시 (`0x20=READY`)
- 각 재시도 attempt 번호 표시
- ErrorCode를 0x%08lX 형식으로 출력

## 예상 결과

### 성공 시 로그
```
*** GPS INIT START (FW v2025-01-14 + SPARKFUN FIX) ***
GPS: I2C3 State before reset=0x20 (0x20=READY)
GPS: I2C3 RCC force reset complete
GPS: HAL_I2C_Init returned 0 (0=OK)
GPS: I2C3 State after init=0x20 (0x20=READY)
GPS: Driver initialized (I2C3, addr=0x42)
GPS: Waiting 1500ms for module startup...
GPS: CFG-PRT attempt 1 - draining first...
GPS: Draining pending data (SparkFun method)...
GPS: Drain - 87 bytes available, reading...
GPS: Drain - Read 87 bytes OK
GPS: Drain complete - no data available (avail=0x0000)
GPS: CFG-PRT sent OK (attempt 1)
GPS: CFG-SAVE attempt 1 - draining first...
GPS: Drain complete - no data available (avail=0x0000)
GPS: CFG-SAVE sent OK - Config saved! (attempt 1)
GPS: Init complete - handler will start polling
[GPS-0001] NO FIX | Sats=3 FixType=0  ← 위성 감지됨!
[GPS-0002] NO FIX | Sats=5 FixType=0
[GPS-0003] 2D FIX | Sats=7 FixType=2  ← Fix 획득!
```

### 재시도 시 로그
```
GPS: CFG-PRT attempt 1 - draining first...
GPS: Drain complete - no data available (avail=0x0000)
GPS: CFG-PRT failed (status=1, ErrorCode=0x00000004, attempt 1)
GPS: CFG-PRT attempt 2 - draining first...
GPS: Drain - 43 bytes available, reading...
GPS: Drain - Read 43 bytes OK
GPS: Drain complete - no data available (avail=0x0000)
GPS: CFG-PRT sent OK (attempt 2)  ← 2번째 시도에서 성공
```

## 빌드 정보
- **빌드 성공**: 2025-01-14
- **Flash 사용**: 166,240 bytes (15.9%)
- **RAM 사용**: 79,928 bytes (24.4%)
- **증가분**: +152 bytes (드레인 함수 추가)

## 테스트 방법

1. **펌웨어 플래시**
   ```bash
   pio run -e debug -t upload
   ```

2. **시리얼 모니터**
   ```bash
   pio device monitor
   ```

3. **성공 확인 지표**
   - `GPS: Drain complete` 메시지 확인
   - `GPS: CFG-PRT sent OK` 메시지 확인
   - `Sats > 0` (실외에서 60초 이내)
   - Mode OFF로 전환되지 않음 (Watchdog OK)

## 참고 자료

- SparkFun u-blox GNSS Arduino Library
  - https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library
  - `src/SparkFun_u-blox_GNSS_Arduino_Library.cpp` - begin() 및 isConnected()
  - `examples/Example1_NAV_PVT/Example1_NAV_PVT.ino`

- u-blox Protocol Specification
  - I2C DDC (Display Data Channel) 프로토콜
  - Register 0xFD/0xFE: Available Bytes (big-endian)
  - Register 0xFF: Data Stream

## 핵심 교훈

1. **연속 출력 문제**: GPS는 부팅 후 NMEA를 **계속** 출력하므로, 한 번 드레인으로 끝나지 않음
2. **Drain 타이밍**: Write 직전에 드레인해야 버퍼 refill 최소화
3. **재시도 필요성**: SparkFun도 3회 재시도 권장 (하드웨어 타이밍 이슈)
4. **Watchdog 관리**: 긴 초기화 시퀀스는 반드시 중간중간 refresh 필요
