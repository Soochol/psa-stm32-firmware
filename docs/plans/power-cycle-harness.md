# 플랜: 두 MCU 전원 사이클 하니스(`tests/test_power_cycle.py`) 링크 실패 복구

- 작성일: 2026-11-18
- 이 문서는 승인 시점의 기록이며 구현 중 바뀐 것은 반영하지 않는다.

## 사용자가 풀려는 문제

`tests/test_power_cycle.py`는 STM32 `comm_esp.c`의 전원 핸드셰이크와 ESP32 `src/lowPower.cpp`를
한 프로세스에 묶어 UART 협조 OFF/wake 사이클을 바이트 단위로 돌리는 회귀 시험이다. 지금 ESP32
`origin/main`(5096fd0)을 인자로 주면 빌드 단계에서 멈춰 아무것도 검사하지 못한다.

```
undefined reference to `fallAlertsBeginOn()'
undefined reference to `fallAlertsPowerOff()'
```

STM32 `origin/main`(99034ef) 그대로에서 재현했다.

## 원인 (재현으로 확인)

- ESP32 `src/lowPower.cpp`가 `include/fallAlertGate.h`를 include하고 `lowPowerRequest()` 안에서
  `fallAlertsBeginOn()`(ON 요청), `fallAlertsPowerOff()`(OFF 요청)를 부르게 바뀌었다. 정의는
  `src/fallAlertGate.cpp`에 있다.
- 하니스는 `lowPower.cpp`를 `#include`해서 한 번역 단위로 빌드하므로 `fallAlertGate.cpp`의 정의가
  링크되지 않는다.
- ESP32 저장소는 자기 시험(`tests/falls/run.py`)에서 같은 조합을 이렇게 빌드한다:
  `tests/power_stubs` + `tests/falls/stubs`(FreeRTOS mutex 스텁) include 경로, `tests/low_power_test.cpp`와
  `src/fallAlertGate.cpp`를 함께 컴파일, 시험 쪽에서 `bool failGateAllocation = false;` 정의.
  ESP32 계획 문서 `docs/superpowers/plans/2026-10-19-esp32-sensor-only.md:184`도 이 링크 실패를
  "STM32 하니스는 이번 범위에서 수정하지 않는다"며 남겨 두었다.

플랜 작성 중 스크래치 사본에서 아래 3가지만 바꿔 ESP32 5096fd0 기준
`PASS: two-MCU power wire cycle, software resets, deferred OFF cancelled by ON`을 확인했다
(저장소 파일은 고치지 않았다).

1. `#include "lowPower.cpp"` 뒤에 `bool failGateAllocation = false;`
2. `main()` 첫 줄에 `assert(initFallAlertGate());`
3. g++ 명령에 `-I <esp>/tests/falls/stubs`와 `<esp>/src/fallAlertGate.cpp` 추가

## 확정된 결정 (요청에서)

- 통과 조건은 `python3 tests/test_power_cycle.py <ESP32 origin/main 체크아웃>`이다.
- 기존 검사와 단언은 지우거나 완화하지 않는다.
- 수정은 STM32 저장소의 `tests/test_power_cycle.py` 한 파일로 한정한다. 펌웨어 코드와 `version.h`는
  바꾸지 않는다.
- `tests/test_power.py`, `tests/test_i2c4_diag.py`는 계속 통과해야 한다.
- 사용자의 형제 ESP32 체크아웃(`../psa-esp32-firmware`, 다른 브랜치에 변경 있음)의 작업 트리는
  건드리지 않는다. 검증은 ESP32 `origin/main`을 따로 꺼낸 사본으로 한다.
- 열려 있는 PR #15(GPS I2C3 복구: `User/Drv/src/i2c.c`, `SAM_M10Q/platform/inc/sam_m10q_platform.h`,
  `SAM_M10Q/platform/src/sam_m10q_platform.c`, `tests/test_gps_i2c3.py`)와 파일이 겹치지 않는다.

## 스스로 정한 결정과 이유

1. **하니스 안 빈 스텁이 아니라 실제 `fallAlertGate.cpp`를 링크한다.**
   - ESP32 팀이 자기 러너(`tests/falls/run.py`)에서 이미 `power_stubs` + `falls/stubs` +
     `fallAlertGate.cpp` 조합을 유지한다. 이 조합이 깨지면 그쪽 시험이 먼저 깨지므로, 우리가 기대는
     빌드 방식이 상대 팀 시험으로 지켜진다.
   - 빈 스텁은 STM32 저장소에 ESP32 API 사본을 두는 것이다. `lowPowerRequest()`가 게이트 함수를
     하나 더 부르게 되면 오늘과 똑같이 다시 깨지고, 게이트 함수가 반환값을 갖게 되면 스텁 값이
     실제 동작과 어긋난 채 통과할 수 있다. 실제 구현은 그런 어긋남이 없다.
   - 대가: ESP32의 시험 내부 경로(`tests/falls/stubs`)와 외부 심볼 `failGateAllocation`에 의존한다.
     ESP32가 이것을 옮기면 하니스가 컴파일·링크 오류로 시끄럽게 실패한다(조용히 통과하지 않는다).
2. **`initFallAlertGate()`를 `main()` 처음에 호출한다.** 호출하지 않으면 mutex가 없어 게이트 함수가
   모두 아무 일도 안 하는 경로로 빠진다. 실제 펌웨어도 UART 작업 시작 전에 한 번 호출한다.
   실패 시 단언으로 멈춘다.
3. **fall alert 게이트 상태는 이 하니스의 관찰 대상으로 두지 않는다(새 단언을 더하지 않는다).**
   - 게이트 상태는 ESP32 내부 상태이고 STM32와의 와이어에 나타나지 않는다. 이 하니스의 목적은
     두 MCU 사이 바이트 계약이다.
   - 하니스는 `esp_restart()`를 예외로 흉내 낼 뿐 RAM을 지우지 않는다. 실제 장치는 소프트 리셋에서
     게이트가 `Unknown`으로 돌아가지만 하니스에서는 이전 상태가 남는다. 리셋을 건너는 게이트 단언은
     실제와 다른 것을 검사하게 된다.
   - OFF/ON 요청에서 게이트가 막히는지는 ESP32 `tests/low_power_test.cpp`가 이미 단언한다
     (`assert(!fallAlertsAllowed())` 등).
4. **옛 ESP32 체크아웃(`fallAlertGate.cpp`가 없던 시절) 호환 분기를 두지 않는다.** 대상은 ESP32
   `origin/main`이다. 파일이 없으면 g++가 실패하고 시험도 실패해야 한다. 파일 유무로 스텁에
   폴백하는 코드는 검사를 조용히 약하게 만든다.
5. **검증용 ESP32 사본은 `git archive origin/main`을 임시 폴더에 풀어 만든다.** detached worktree와
   결과가 같고, 사용자 ESP32 저장소의 작업 트리·브랜치·worktree 목록을 바꾸지 않는다(`fetch`로
   원격 추적 ref만 갱신).
6. **ESP32 저장소는 고치지 않는다.** 위 방식으로 STM32 하니스만 바꿔 통과하므로 팀 간 협의가 필요
   없다.

## 작업

| 작업 ID | 내용 | profile | mode | 순서 |
|---|---|---|---|---|
| `power-cycle-gate-link` | 하니스가 실제 `fallAlertGate.cpp`를 링크하도록 수정 | `minor-fix` | `basic` | 단독 |

- 방식: `basic` — 시험 파일 한 개만 고치고 펌웨어·상태 저장·동시성 코드를 건드리지 않는다.
- 작업이 하나뿐이라 병렬·순차 관계가 없다. PR #15와 파일이 겹치지 않으므로 동시에 진행해도 된다.

## 범위 밖

- 펌웨어 코드, `version.h`, 다른 시험 파일 수정.
- ESP32 저장소 수정, 그리고 이 링크 실패를 언급한 ESP32 계획 문서 갱신.
- 하니스를 CI나 PlatformIO 시험 목록에 넣는 일.
- fall alert 게이트를 두 MCU 사이클에서 검사하는 새 시나리오(결정 3).

## 사용자가 정해야 하는 결정

- **검증을 약하게 하는 작업: 없음.** 단언 삭제, 시간 제한 증가, 시나리오 축소가 없다.
- **ESP32 저장소 수정: 필요 없음.**
- (선택) ESP32 팀에 "STM32 하니스가 `tests/falls/stubs`와 `failGateAllocation`에 의존한다"고 알릴지.
  알리면 그쪽이 이 경로를 옮길 때 STM32 하니스도 함께 고려할 수 있다. 알리지 않아도 깨질 때 링크
  오류로 바로 드러난다. 알린다면 번호 매긴 보고서 절차로 따로 진행한다.
