# ESP32 수정안 — FORCE_DOWN 미발화(deep-ToF 릴리즈 교착) 해소

> **대상 파일**: `psa-esp32-firmware/src/uartMaster.cpp`, `include/globals.h`
> **연계 보고**: STM32 보고 #70 (`ESP-STM32_SD_Logging_STM32_Progress_70_forceDownStall.md`)
> **상태**: 초안 — ESP32 팀 검토·채택 대기

---

## 0. 한 줄 요약

deep-ToF 경로로 진입한 FU 사이클은 릴리즈가 `tof > 180mm`뿐인데, 가열-연장된
SMA가 ToF를 계속 낮게 붙들면 사용자가 일어서도 릴리즈가 영원히 관측되지 않아
FORCE_DOWN이 전송되지 않는다. 8a5a096이 hybrid 릴리즈에서 ToF를 뺀 바로 그
기전("SMA can get stuck in extended state, leaving ToF artificially low even
after the user released weight")이 deep 릴리즈에 다시 들어가 있는 구조다.

아래 4개 수정 중 **P1·P3은 필수**, P2는 안전판, P4는 선택이다.

---

## 1. [P1 · 필수] 사이클 중 deep→hybrid 재분류

### 문제

빠른 숙임에서는 STM32 accel-tilt의 적응형 LPF(동적 가속 시 α=0.001, 사실상
동결 — `icm42670p_platform.c:1965-2016`) 때문에 각도가 늦게 반응한다. 그래서
평범한 허리 굽힘도 ToF가 160mm를 먼저 뚫으면 deep 경로로 발동된다
(`uartMaster.cpp:1343-1345`). 이후 각도가 35°를 넘어 "허리 굽힘이었다"는 것이
분명해져도 `fuViaDeepTof`는 그대로라 릴리즈는 ToF 기준에 묶인다.

### 수정

FD 블록(`uartMaster.cpp:1450`) 직전에 추가:

```cpp
// Deep-ToF로 진입한 사이클이라도 이후 허리 각도가 FU 임계(35°)를 넘었다면
// 실제로는 허리 굽힘 사이클이다 — 릴리즈를 각도 기준으로 되돌린다.
// (STM32 accel-tilt LPF 지연으로 ToF가 각도보다 먼저 임계를 넘는
//  진입 캡처 레이스의 사후 보정. 사용자가 굽힘을 유지하는 수 초 동안
//  지연된 각도가 따라잡으므로 재분류는 사이클 초반에 완료된다.)
if (fuViaDeepTof && forceUpTimestamp > 0)
{
    int16_t reclassTilt = computeWaistTilt();
    if (reclassTilt > FU_ANGLE_THRESH &&
        abs((int32_t)reclassTilt) < WAIST_TILT_SANE_MAX)
    {
        fuViaDeepTof = false;
        ESP_LOGI(TAG, "[FU-RECLASS] deep→hybrid (tilt=%+.2f° crossed FU thresh)",
                 reclassTilt / 100.0f);
    }
}
```

### 효과

현장에서 관측된 사고 유형(평범한 숙임이 deep에 잡힌 경우)이 그대로 해소된다.
각도가 35°를 실증적으로 넘은 사이클에만 작동하므로 무릎 스쿼트 사이클의
릴리즈 의미는 건드리지 않는다.

---

## 2. [P2 · 안전판] 진짜 deep 사이클의 각도 폴백 릴리즈

### 문제

각도가 끝까지 35°를 못 넘는 진짜 무릎 스쿼트 사이클은 P1로 구제되지 않는다.
이 경우에도 가열-연장 SMA가 `tof < 180`을 유지하면 같은 교착에 빠지고,
장치는 forceOn 타임아웃 60초(`uartMaster.cpp:273`) 동안 52 °C 히터를 등에
붙이고 있게 된다.

### 수정

`include/globals.h`:

```cpp
#define DEEP_RELEASE_FALLBACK_MS   10000  // deep 릴리즈(ToF) 무관측 시 각도 폴백 개시
#define DEEP_FALLBACK_DEBOUNCE_MS  2000   // 폴백 각도 릴리즈의 강화 디바운스
```

FD 블록의 released 계산(`uartMaster.cpp:1456-1467`)을:

```cpp
bool usedDeepFallback = false;
bool released;
if (fuViaDeepTof)
{
    released = (fdTof > 0) && (fdTof > TOF_FORCEUP_THRESHOLD);

    // 폴백: 가열-연장 SMA가 ToF를 계속 낮게 붙들면 위 조건은 오지 않는다.
    // FU 후 충분히 지났고 허리가 명백히 직립이면 릴리즈로 인정한다.
    // 하중 하 오수축 위험을 줄이려고 디바운스를 2 s로 강화한다.
    if (!released && elapsed >= DEEP_RELEASE_FALLBACK_MS)
    {
        released = (fdWaistTilt < FD_ANGLE_THRESH) &&
                   (abs((int32_t)fdWaistTilt) < WAIST_TILT_SANE_MAX);
        usedDeepFallback = released;
    }
}
else
{
    released = (fdWaistTilt < FD_ANGLE_THRESH) &&
               (abs((int32_t)fdWaistTilt) < WAIST_TILT_SANE_MAX);
}
```

디바운스 판정(`uartMaster.cpp:1469-1493`)은 요구 시간을 경로별로:

```cpp
uint32_t requiredDebounce = usedDeepFallback ? DEEP_FALLBACK_DEBOUNCE_MS
                                             : FD_ANGLE_DEBOUNCE_MS;
...
else if ((millis() - fdReleaseStart) >= requiredDebounce)
```

### 트레이드오프 — ESP32 팀 결정 요청

잔존 오발화 창은 "상체 직립(<15°) 무릎 스쿼트를 하중 실은 채 10 s + 2 s 이상
유지"하는 자세다. 비용 비대칭을 고려하면(오수축: 지지 조기 해제 1회 vs 현행:
사용자 의사에 반해 52 °C 히터 60 s 유지 + 배터리 소모) 폴백 쪽이 낫다고
판단하나, **SMA 하중-수축의 기계적 위험 평가는 그쪽 몫**이다. 10 s / 2 s
값도 함께 검토 바란다.

---

## 3. [P3 · 필수] `setDeviceMode()` 반환값 미확인 — 실패 시 사이클 소실

### 문제

FD 발화 지점(`uartMaster.cpp:1478-1483`)이 전송 성공 여부와 무관하게
`forceUpTimestamp = 0`으로 사이클을 닫는다. `sendCommandAsync`가 슬롯 고갈
등으로 false를 반환하면(`uartMaster.cpp:2582-2588`) FORCE_DOWN은 재시도 없이
소실되고, ACK-타임아웃 재시도 경로(슬롯이 잡혀야 동작)도 타지 못한다.

### 수정

실패 원인을 둘로 구분해야 한다. **일시적 전송 실패**(슬롯 고갈 등)는 재시도가
맞지만, **정책 차단**(batWarnActive — `uartMaster.cpp:3610-3618`)은 재시도해도
계속 거부되므로 사이클을 닫아야 한다. 구분 없이 재시도하면 배터리 저전압 중
FD 조건 성립 시 10 Hz로 "blocked" 경고가 forceOn 타임아웃까지 최대 60 s
반복된다. (!stm32Ready·TESTING은 FD 블록 게이트인 `canControlSTM`이 먼저
막으므로 여기까지 오는 정책 차단은 batWarnActive뿐이다.)

```cpp
else if ((millis() - fdReleaseStart) >= requiredDebounce)
{
    if (setDeviceMode(DeviceMode::FORCE_DOWN) || batWarnActive)
    {
        // 성공, 또는 정책 차단(배터리 저전압 — STM32가 이미 액추에이터를
        // 중지했고 재시도해도 계속 거부됨)이면 사이클을 닫는다.
        forceUpTimestamp = 0;
        fdReleaseStart = 0;
    }
    // 일시적 전송 실패만 상태 유지 — 다음 STAT 샘플(100 ms 뒤)에서 자연 재시도.
}
```

FU 발화 지점(`uartMaster.cpp:1355-1367`)도 동일하게: 성공 시에만
`forceUpTimestamp = millis()` / `fuArmed = false` / 카운터 갱신을 수행
(FU는 실패 시 그냥 미발화 상태 유지라 batWarnActive 분기 불필요).
healing FU/FD 지점(`uartMaster.cpp:1276-1322`)도 같은 패턴 점검 권장.

---

## 4. [P4 · 선택] deep 진입 게이트 강화 — 레이스 자체를 축소

deep 진입을 "허리가 명백히 직립일 때"로 제한:

```cpp
bool deepTofOnly = !hybridTrigger &&
                   (fuTof > 0) &&
                   (fuTof < TOF_FORCEUP_DEEP_THRESHOLD) &&
                   (waistTilt < FD_ANGLE_THRESH);   // 15° 미만 = 무릎 주도 자세만
```

굽힘 진행 중(각도 15–35° 통과 구간)에는 deep이 발동하지 않고, 다음 샘플에서
각도가 35°를 넘으면 hybrid로 정상 발동한다. LPF 동결로 각도가 15° 미만에
머무는 극단적 급숙임은 여전히 deep에 잡힐 수 있으나 P1이 사후 재분류한다.
P1·P2와 겹치는 방어라 선택 사항.

---

## 5. STM32 쪽 병행 수정 (우리 몫 — 참고)

IMU I2C2 3회 복구 실패 후 `e_imu_config=READY`로 되돌리지만 세션 중
`e_IMU_Ready()`를 다시 돌리는 주체가 없어(BOOTING 전용) 이후 각도가 0으로
고정 스트리밍된다(`icm42670p_platform.c:179-186`). 이 상태에서는 hybrid
FU(>35° 필요)가 불가능해 **모든 사이클이 deep 경로로 강제**되므로 본 결함의
증폭기다. 세션 내 IMU 재초기화 경로는 STM32 저장소에서 별도 처리 예정.

## 6. 테스트 계획

1. **재현 확인(수정 전)**: 빠른 숙임 반복 → ESP 로그 `FORCE UP [deep]` 발생
   사이클에서 기립 후 `FORCE DOWN` 미출력 + STM32가 60 s 뒤 SLEEP 확인.
2. **P1 검증**: 같은 시나리오에서 `[FU-RECLASS]` 로그 후 기립 시 FD가
   디바운스 500 ms 내 발화.
3. **P2 검증**: 무릎 스쿼트(각도 <15°) 유지 상태로 deep FU → ToF가 180mm를
   못 넘게 유지 → 기립 후 10–12.5 s 내 FD 발화.
4. **P3 검증**: `MAX_PENDING_COMMANDS` 포화 상태를 인위적으로 만들고 FD
   조건 성립 → 다음 샘플에서 재시도로 FD 발화.
5. **회귀**: 정상 hybrid 사이클(굽힘→기립)의 FD 타이밍이 기존과 동일한지,
   wear-check·rearm 로직에 부작용 없는지 확인.

## 7. 참조

| 항목 | 위치 |
|------|------|
| FU 이중 경로 진입 | `src/uartMaster.cpp:1330-1352` |
| FD 릴리즈 판정 | `src/uartMaster.cpp:1450-1495` |
| rearm 블록 | `src/uartMaster.cpp:1522-1563` |
| STATE-SYNC 리셋 | `src/uartMaster.cpp:1671-1694` |
| 임계값 정의 | `include/globals.h:181-215` |
| STM 타임아웃 설정 (forceOn=60 s) | `src/uartMaster.cpp:273` |
| STM32 accel-tilt 적응형 LPF | `psa-stm32-firmware/ICM42670P/platform/src/icm42670p_platform.c:1965-2016` |
| ToF-저값 고착 근거 커밋 | 8a5a096 (FD 블록 주석) |
