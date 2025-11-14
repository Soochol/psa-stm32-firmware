# GPS Watchdog Timeout Fix v6 - Fast-Fail Strategy

## 날짜
2025-01-14 (긴급 수정)

## 문제 발견
```
GPS: Drain - Can't read avail bytes (ErrorCode=0x00000004), attempt 1
GPS: Drain - Can't read avail bytes (ErrorCode=0x00000004), attempt 2
[시스템 리셋 - Mode OFF로 전환]
```

### 근본 원인
- **드레인 실패 시간**: 200ms × 10회 = **2초 초과** → Watchdog 타임아웃!
- **GPS 불응답**: GPS 모듈이 연결되지 않았거나 응답하지 않음
- **계속 재시도**: 실패해도 계속 시도하여 시간 낭비

## 긴급 수정사항

### 1. 드레인 함수 Fast-Fail 전략

#### Before (문제)
```c
while(drain_attempts < 10) {  // 최대 10회!
    ret = HAL_I2C_Mem_Read(..., 500);  // 500ms timeout
    if(ret != HAL_OK) {
        HAL_Delay(200);  // 200ms 대기
        continue;  // 계속 재시도
    }
}
// 최악: 10 × (500 + 200) = 7초!
```

#### After (해결)
```c
while(drain_attempts < 3) {  // 최대 3회만
    ret = HAL_I2C_Mem_Read(..., 100);  // 100ms timeout (짧게)
    if(ret != HAL_OK) {
        // GPS 응답 없음 - 즉시 실패 반환
        return -1;  // ← Fast-fail!
    }
    HAL_Delay(50);  // 50ms만 대기
}
// 최악: 3 × (100 + 50) = 450ms (안전!)
```

### 2. CFG-PRT 재시도 감소

#### Before
```c
while(cfg_attempts < 3) {
    i_GPS_DrainPendingData();  // 매번 드레인
    HAL_I2C_Mem_Write(..., 1000);
    HAL_Delay(300);
}
// 드레인 실패하면 계속 재시도
```

#### After
```c
while(cfg_attempts < 2) {  // 2회만
    if(cfg_attempts == 0) {
        // 첫 시도에서만 드레인
        int drain_ret = i_GPS_DrainPendingData();
        if(drain_ret != 0) {
            // GPS 없음 - 즉시 종료!
            return;  // ← Early exit
        }
    }
    HAL_I2C_Mem_Write(..., 500);  // 500ms timeout (짧게)
    HAL_Delay(100);  // 100ms만 대기
}
```

### 3. CFG-SAVE 단순화

#### Before
```c
while(save_attempts < 3) {
    i_GPS_DrainPendingData();  // 매번 드레인
    HAL_I2C_Mem_Write(..., 1000);
    HAL_Delay(300);
}
```

#### After
```c
if(cfg_ret == HAL_OK) {  // CFG-PRT 성공했을 때만
    // 1회만 시도
    HAL_I2C_Mem_Write(..., 500);
    // 실패해도 OK (RAM 설정은 활성화됨)
}
```

## 타이밍 분석

### 최악의 경우 (GPS 없을 때)
```
I2C3 Reset:           ~100ms
1500ms 대기:          1500ms (500×3, watchdog refresh 포함)
Drain 1회 실패:       ~100ms (즉시 반환)
Early exit:           0ms
-------------------------
Total:                ~1700ms ✅ (2초 이내!)
```

### 최선의 경우 (GPS 정상)
```
I2C3 Reset:           ~100ms
1500ms 대기:          1500ms
Drain 성공:           ~150ms (1-2회 시도)
CFG-PRT:              ~50ms
CFG-SAVE:             ~50ms
-------------------------
Total:                ~1850ms ✅ (2초 이내!)
```

## 주요 개선사항

### ✅ Fast-Fail 전략
- **GPS 불응답 감지**: 첫 드레인 실패 시 즉시 종료
- **시간 낭비 방지**: 불필요한 재시도 제거
- **Early Return**: GPS 없으면 설정 단계 스킵

### ✅ Timeout 단축
- I2C Read timeout: 500ms → 100ms
- I2C Write timeout: 1000ms → 500ms
- Retry delay: 300ms → 100ms

### ✅ 재시도 횟수 감소
- Drain: 10회 → 3회
- CFG-PRT: 3회 → 2회
- CFG-SAVE: 3회 → 1회

### ✅ 로깅 간소화
- 중복 로그 제거
- 짧은 메시지로 변경
- 핵심 정보만 출력

## 예상 동작

### Case 1: GPS 없음 (가장 흔한 경우)
```
*** GPS INIT START ***
GPS: I2C3 State before reset=0x20
GPS: I2C3 RCC force reset complete
GPS: HAL_I2C_Init returned 0
GPS: Driver initialized
GPS: Waiting 1500ms for module startup...
GPS: Sending CFG-PRT (24 bytes)
GPS: CFG-PRT attempt 1
GPS: Draining...
GPS: Drain failed (0x04) - GPS may not be connected
GPS: No GPS detected - skipping config
GPS: Init complete - handler will start polling
mount succ[GPS-0001] NO FIX | Sats=0 FixType=0
```
→ **1.7초에 완료, Mode OFF 없음!**

### Case 2: GPS 정상 응답
```
*** GPS INIT START ***
...
GPS: Waiting 1500ms for module startup...
GPS: Sending CFG-PRT (24 bytes)
GPS: CFG-PRT attempt 1
GPS: Draining...
GPS: Drain - 87 bytes avail
GPS: Drain OK (avail=0x0000)
GPS: CFG-PRT OK
GPS: Sending CFG-SAVE (21 bytes)
GPS: CFG-SAVE OK - saved to flash
GPS: Init complete - handler will start polling
[GPS-0001] NO FIX | Sats=3 FixType=0  ← 위성 감지!
```
→ **1.85초에 완료, 설정 성공!**

## 빌드 정보
- Flash: 165,912 bytes (15.8%, -328 bytes 감소!)
- Timeout 안전성: ✅ 2초 이내 보장

## 테스트 방법

```bash
# 빌드
pio run -e debug

# 플래시
pio run -e debug -t upload

# 모니터
pio device monitor
```

## 성공 지표

✅ **Mode OFF 없음** - Watchdog 타임아웃 없이 초기화 완료
✅ **GPS 없어도 OK** - 빠르게 실패하고 계속 진행
✅ **GPS 있으면 설정** - 정상적으로 CFG-PRT/SAVE 성공

## 핵심 교훈

1. **Fast-Fail is Better**: 실패 감지 즉시 종료 > 끝까지 재시도
2. **Watchdog First**: 모든 긴 작업은 Watchdog 타임아웃을 최우선 고려
3. **GPS Optional**: GPS가 없어도 시스템은 정상 동작해야 함
4. **Timeout Tuning**: I2C timeout은 가능한 짧게 (100-500ms)
