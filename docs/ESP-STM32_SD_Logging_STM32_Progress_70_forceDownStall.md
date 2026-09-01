# STM32 보고 #70 — 간헐적 FORCE_DOWN 미발화 원인 규명 + ESP32 수정안 전달

> **대상**: 신규 스레드 (현장 증상 보고 → 원인 조사 결과)
> **판정**: 원인은 ESP32 `uartMaster.cpp`의 deep-ToF 릴리즈 조건 — STM32는 명령 미수신 상태로 정상 동작
> **회신 요청**: 있음 — 수정안 P1~P4 채택 여부 + SMA 기계 특성 1건 확인

---

## 1. 증상 (현장 보고)

- 사용자가 허리를 숙였다 일어서면 heat on/off(FU/FD)가 정상 동작하는데,
  **간헐적으로** 일어서도 FORCE_DOWN(cooling)이 발화하지 않고 상태가 그대로다.
- 그 상태로 있다가 **SLEEP 전환 후에는 다시 정상** 동작한다.
- 전원 스위치 2 s off/on으로도 회복된다.

## 2. 원인 — deep-ToF 사이클의 릴리즈 교착 (ESP32)

STM32는 부팅 시 AI를 ESP에 위임하고(`mode.c:2703`, `MODE_AI_ESP`), FU/FD
전이는 그쪽 0x51(ctrlMode) 단발 명령으로만 일어난다. 0x51을 받으면 STM32는
무조건 전이하므로(`comm_esp.c:760-768`) "상태 변화 없음"은 곧 **명령이
전송되지 않은 것**이다. 조사 결과:

1. **진입 캡처 레이스** — FU 진입이 hybrid(각도>35° AND ToF<180mm)와
   deep-ToF(ToF<160mm 단독)의 이중 경로인데(`uartMaster.cpp:1330-1352`),
   저희 accel-tilt는 동적 가속 중 LPF가 사실상 동결되어(α=0.001,
   `icm42670p_platform.c:1965-2016`) 각도가 늦게 반응한다. 빠른 숙임에서는
   ToF가 먼저 임계를 뚫어 **평범한 허리 굽힘이 deep 경로로 발동**될 수 있다.
   어느 쪽이 먼저냐가 자세·속도에 갈리므로 "간헐적"이 된다.

2. **릴리즈 교착** — deep 사이클의 릴리즈는 `tof > 180mm`뿐이다
   (`uartMaster.cpp:1456-1467`). 그런데 FU가 가열한 SMA는 연장 상태를
   유지하며 ToF를 낮게 붙들 수 있다 — 8a5a096에서 hybrid 릴리즈를 각도
   전용으로 바꾼 근거("SMA can get stuck in extended state, leaving ToF
   artificially low even after the user released weight")와 같은 기전이다.
   결과: 기립해도 released=false → FD 미전송 → STM32는 FORCE_ON에서 히터
   52 °C 유지.

3. **회복 타이밍이 증상과 일치** — 그쪽이 설정한 forceOn 타임아웃 60 s
   (`uartMaster.cpp:273`) 후 STM32가 SLEEP으로 넘어가며 evtMode(SLEEP)를
   보내면 STATE-SYNC(`uartMaster.cpp:1671-1694`)가 `forceUpTimestamp`·
   `fuViaDeepTof`를 리셋하고, rearm 기준이 각도로 돌아가 즉시 재무장된다.
   → "SLEEP 후 다시 정상". 전원 2 s off/on은 evtInitResult(0x81) →
   `armHealingCycle()` 전체 리셋이라 역시 회복된다.

경합 가설은 배제했다: 0x51 프레임 유실은 5 s×3회 재시도로 20 s 내 자가
회복하고, STM32 IMU 통신 정지는 ~8 s 내 각도 0 스트림으로 오히려 hybrid
릴리즈를 발화시키며, 배터리 ALERT 래치는 전원 재인가 전까지 안 풀려
"SLEEP 후 정상"과 모순된다.

## 3. 현장 검증 시그니처 (펌웨어 수정 없이 확인 가능)

- **ESP32 시리얼 로그**: 사고 사이클은 `██ FORCE UP [deep] ██`로 시작하고
  이후 `FORCE DOWN` 출력이 없어야 한다. 정상 사이클은 `[hybrid]`.
- **STM32 SD 로그(.psa)**: deviceMode=FORCE_ON이 약 60 s 유지되는 동안
  각도는 15° 아래인데 ToF는 180mm 미만에 머무는 구간.
- 이 확인은 "가열-연장 SMA가 기립 후에도 ToF<180을 유지한다"는, 코드만으로
  확증 못 한 유일한 기계적 가정도 함께 검증한다. 배포 펌웨어가 저장소
  코드와 다를 가능성도 로그 확인으로 커버된다.

## 4. 수정안 — 상세는 동봉 문서

`PROPOSAL_ESP32_forceDownStall_fix.md`에 diff 수준으로 정리했다. 요약:

| # | 우선순위 | 내용 |
|---|----------|------|
| P1 | 필수 | 사이클 중 각도가 35°를 넘으면 deep→hybrid 재분류 (릴리즈를 각도 기준으로 복귀) |
| P2 | 안전판 | 진짜 deep 사이클도 FU 후 10 s 경과 + 직립 각도(<15°, 디바운스 2 s)면 폴백 릴리즈 |
| P3 | 필수 | FD/FU 발화 지점의 `setDeviceMode()` 반환 미확인 수정 — 전송 실패 시 사이클을 닫지 말고 다음 샘플에서 재시도 |
| P4 | 선택 | deep 진입 게이트에 `waistTilt < 15°` 추가 — 굽힘 진행 중 오캡처 축소 |

STM32 쪽 병행 수정(우리 몫)도 확인했다: IMU 3회 복구 실패 후 세션 내
재초기화 주체가 없어 각도가 0으로 고정되면 모든 사이클이 deep으로 강제되는
증폭기(`icm42670p_platform.c:179-186`)로, 저희 저장소에서 별도 처리한다.

## 5. 회신 요청

1. **P1~P4 채택 여부** — 특히 P2의 폴백 시간(10 s)·강화 디바운스(2 s) 값과
   "하중 하 SMA 수축"의 기계적 위험 평가는 그쪽 판단이 필요하다.
2. **기계 특성 확인** — 가열-연장 SMA가 무하중 기립 상태에서 ToF를 180mm
   미만으로 붙드는지, 실기 로그(§3 시그니처)로 확인 부탁한다.

---

## 📝 보고 정보

| 항목 | 값 |
|------|-----|
| 대상 | 신규 스레드 (forceDownStall) |
| 판정 | 원인 = ESP32 deep-ToF 릴리즈 교착 + 진입 캡처 레이스 |
| 동봉 | `PROPOSAL_ESP32_forceDownStall_fix.md` (수정안 P1~P4) |
| 신규 회신 요청 | 있음 — §5 (채택 여부 + 기계 특성 확인) |
| STM32 펌웨어 | `1.0.0` (본 건 STM32 코드 변경 없음 — IMU 재초기화 건은 별도 진행) |
