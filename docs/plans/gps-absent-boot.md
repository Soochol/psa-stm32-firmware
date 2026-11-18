# 플랜: GPS 모듈 유무와 관계없이 부팅·운용 정상화

- 작성일: 2026-11-18
- 이 문서는 승인 시점의 기록이며 구현 중 바뀐 것은 반영하지 않는다.

## 사용자가 풀려는 문제

STM32에 GPS 모듈(SAM-M10Q, I2C3)을 달지 않으면 전원 ON 뒤 부팅이 한참 멈췄다가 진행된다.
GPS를 달면 지연이 없다. 사용자는 GPS가 있든 없든 정상 동작하기를 원한다.

- GPS가 없을 때: GPS 때문에 부팅이 늦어지지 않는다. 다른 센서가 준비되면 바로 HEALING으로 간다.
- GPS가 있을 때: 지금처럼 지연 없이 초기화되고, 운용 중에 PVT가 계속 갱신된다. 운용 중
  I2C3 에러가 잠깐 나도 회복한다.

## 원인 (코드 추적으로 확인. 보드 RTT 로그로는 아직 확인하지 않았다)

1. GPS가 없으면 I2C3 읽기가 NACK로 끝나고 HAL이 `HAL_I2C_ErrorCallback`을 부른다.
   그런데 `User/Drv/src/i2c.c`의 `HAL_I2C_ErrorCallback`에는 I2C1/2/4 분기만 있고 I2C3 분기가
   없다. 첫 커밋 f5d3749부터 그랬다.
2. 그 결과 두 가지가 고착된다.
   - `e_comm_i2c3`가 `COMM_STAT_OK`에 머문다. 이후 `i_I2C3_Read/Write`는 큐(`i2c3_buf`)에만
     쌓이고 실제로 전송되지 않는다. 큐는 완료 콜백에서만 빠지는데 완료 콜백이 오지 않는다.
   - GPS 계층(`SAM_M10Q/platform/src/sam_m10q_platform.c`)의 `e_gps_comm`은 통지를 받지 못해
     BUSY로 남는다.
3. 1.2초 뒤 `v_GPS_Tout_Handler`가 `HAL_I2C_Master_Abort_IT`를 부른다. 그러나 HAL은 에러 처리
   뒤 이미 Mode를 NONE으로 돌려 놓았으므로 `HAL_ERROR`만 반환하고, AbortCplt 콜백도 오지 않는다.
   이 핸들러는 `e_gps_comm`만 READY로 바꾼다.
4. 초기화 상태 머신의 WAIT 상태들(`GPS_INIT_DRAIN_AVAIL_WAIT` 등)은 `e_gps_comm == COMM_STAT_DONE`만
   기다린다. READY로 돌아온 경우를 처리하지 않아 영원히 멈춘다. 그래서 `e_GPS_Ready()`는 계속
   BUSY를 반환한다.
5. `User/Edit/src/mode.c`의 `v_Mode_Booting()`은 `ready_mask`에 `modeCONFIG_GPS`가 채워져야
   HEALING으로 간다. GPS 비트는 `MODE_BOOTING_TOUT`(20000 ms, `User/Edit/inc/mode.h`) 타임아웃
   분기에서야 non-fatal로 채워진다. 사용자가 느낀 약 20초 지연이 이것이다.
6. 코드 주석은 "GPS 미연결 시 `GPS_INIT_FAIL`로 graceful skip"이라고 하지만, 실제로 FAIL로 가는
   경로는 드라이버 init 실패와 CFG 메시지 생성 실패 두 가지뿐이다.
7. 이 문제는 972da74 "Refactor GPS init to non-blocking state machine" 이후에 드러났다. 그 전의
   blocking init은 시간 제한이 있는 루프에서 실패를 반환하고 넘어갔다.

부수 위험이 둘 있다. 같은 이유로 GPS가 연결된 상태에서도 운용 중 NACK가 한 번 나면 I2C3가 세션 내내
멈추고, PVT 갱신이 끊긴다. 또 `i_I2C3_Read/Write`에서 HAL 시작이 `HAL_BUSY`나 `HAL_ERROR`를 반환하면
`e_comm_i2c3`가 BUSY나 ERR로 남는다. 진행 중인 전송이 없으니 이를 READY로 되돌릴 콜백도 오지 않아,
같은 방식으로 고착된다.

## 확정된 결정 (요청에서 정해진 것)

- 버전 규칙(CLAUDE.md): `User/Edit/inc/version.h`의 `STM_FW_VERSION`은 이미 `"1.0.1-dev"`다.
  이 작업에서는 바꾸지 않는다.
- 검증의 최소선은 `pio run -e stm32_debug` 빌드 성공이다. 여기에 호스트 단위 시험(`tests/`의
  Python+C 하니스 패턴)으로 GPS 상태 머신과 에러 경로를 확인한다.
- 실제 보드에서 GPS 유무별 부팅 시간을 RTT 로그로 확인하는 일은 사용자 몫으로 남긴다.

## 플래너가 정한 결정과 이유

1. **I2C3 에러 콜백을 추가하고 GPS에 실패를 통지한다.** `HAL_I2C_ErrorCallback`에 I2C3 분기를 넣어
   `e_comm_i2c3`를 READY로 되돌린다. 주소가 `ADDR_GPS`이면 GPS 계층의 에러 핸들러를 부른다.
   핸들러 이름과 시그니처는 작업자가 정한다.
   이유: 근본 원인이 이 누락이다. 다른 모든 증상은 여기서 파생된다.
2. **init 중 에러는 짧게 재시도한 뒤 FAIL로 끝낸다.** 같은 단계를 최대 3회 다시 시도하고(간격은 약
   100 ms), 그래도 실패하면 `GPS_INIT_FAIL`로 간다. `e_GPS_Ready()`는 지금처럼 FAIL을 DONE으로
   보고한다.
   이유: GPS가 있을 때 2초 부팅 대기 직후에 일시적인 NACK가 나도 견딘다. GPS가 없으면 init 시작 뒤
   약 2.5초 안에 끝나므로 GPS가 있을 때의 경로(약 2.6~3초)보다 짧다. 그래서 부팅 시간이 GPS
   유무에 좌우되지 않는다.
3. **init WAIT 상태에서 DONE이 아닌 READY로 돌아오면 에러와 같이 처리한다.** 예를 들어 타임아웃
   핸들러가 강제로 READY로 바꾼 경우다. 이것은 콜백이 아예 오지 않는 전송(버스 hang 등)에 대한
   안전장치다. 타임아웃 뒤에는 다음 전송이 실제로 HAL까지 가도록 `e_comm_i2c3`도 READY로 돌린다.
   단, HAL 핸들이 아직 전송 중이 아닐 때만 그렇게 한다. 구체적인 방법은 작업자가 정한다.
4. **FAIL은 "이번 세션 동안 GPS 비활성"이다.** FAIL 뒤 `v_GPS_Handler`는 런타임 폴링
   (`i_SAM_M10Q_Handler`)을 돌리지 않는다. 전원을 켤 때마다 BOOTING 진입에서 `v_GPS_Init()`이 다시
   불리므로 다음 부팅에서 다시 감지한다. 세션 도중에 모듈을 꽂는 hot-plug는 지원하지 않는다.
   이유: 지금 코드는 FAIL 뒤에도 런타임 폴링을 돌리므로, 없는 장치에 1 Hz로 NACK를 계속 낸다.
   이 장치에서 GPS는 조립 때 결정되는 부품이고, IMU나 Temp의 "disabled for this session" 패턴과도
   맞는다. ESP로 가는 GPS 필드는 `fixType == GPS_FIX_NONE`이므로 지금처럼 0으로 나간다.
5. **런타임 에러는 다음 폴링 주기에 재시도하되, 폴링 주기(1 s)보다 자주 하지 않는다.** DONE 뒤에 나는
   에러는 `e_gps_comm`을 READY로 돌려 드라이버 상태 머신이 IDLE로 돌아가게 한다. 연속 에러(운용 중
   모듈 분리 등)에서도 I2C3 시작 횟수가 약 1회/초를 넘지 않게 한다. 모듈이 다시 응답하면 PVT 갱신이
   재개된다.
   이유: 일시적 에러에서는 회복해야 하고, 영구 에러에서도 메인 루프 매 회전마다 NACK를 쏟아 내면 안 된다.
6. **HAL 시작 실패로 `e_comm_i2c3`가 고착되지 않게 한다.** `i_I2C3_Read/Write`에서 HAL 시작이
   `HAL_BUSY`나 `HAL_ERROR`를 반환해 아무 전송도 진행되지 않는 경우, 다음 호출이 다시 HAL을 시도할 수
   있어야 한다. GPS 계층도 그 실패를 에러로 처리한다(지금은 `e_gps_comm`이 BUSY로 남는다).
   이유: 원인 1과 같은 부류의 고착이고, 운용 중 회복 요구에 포함된다.
7. **`mode.c`의 부팅 게이팅은 유지한다.** GPS를 `ready_mask`에서 빼지 않고 `MODE_BOOTING_TOUT`도
   바꾸지 않는다. 논리 변경 없이, 사실과 달라진 주석만 고칠 수 있다.
   이유: 결정 2로 GPS가 없을 때의 경로가 있을 때보다 짧아지므로 게이팅을 남겨도 지연이 생기지 않는다.
   최소 변경이 회귀 위험도 가장 작다.
8. **I2C1/2/4/5의 콜백 동작은 바꾸지 않는다.** 그 채널들의 에러 뒤 큐 처리 같은 비슷한 문제는
   범위 밖이다.

## 작업 나누기

작업은 하나(`gps-i2c3-recovery`, implementer, mode `poteto`)다.

- 한 PR로 묶은 이유: 고치는 곳은 `i2c.c`의 I2C3 부분, `sam_m10q_platform.c/.h`, 새 호스트 시험
  하나뿐이다. 콜백 통지(i2c.c)와 그 통지를 소비하는 상태 머신(platform)은 서로의 결과를 써야
  시험할 수 있다. 둘로 나누면 순차가 되고, 앞 PR만으로는 동작을 확인할 수 없다.
- mode를 `poteto`로 한 이유: ISR 콜백과 메인 루프가 공유하는 상태(`e_comm_i2c3`, `e_gps_comm`)를
  다루는 동시성 수정이고, 상태 머신의 실패 경로를 새로 설계한다.

## 범위 밖

- 세션 도중 GPS hot-plug 감지 (결정 4)
- I2C1/2/4/5 에러 콜백과 큐 처리 정비
- `MODE_BOOTING_TOUT` 값이나 다른 센서의 부팅 경로 변경
- GPS가 있을 때의 init 시간 단축(2초 부팅 대기 등)
- ESP32 프로토콜이나 GPS 데이터 필드 형식 변경
- 펌웨어 버전 변경(이미 `1.0.1-dev`)
- 보드 플래싱과 RTT 실측 (사용자 몫)

## 사용자가 정해야 하는 결정

1. **hot-plug 미지원(결정 4)으로 괜찮은지.** 기본값은 "FAIL 뒤 이번 세션은 GPS 비활성, 다음 전원
   ON에서 재감지"다. 세션 도중에 꽂은 GPS도 잡아야 한다면 FAIL 상태에서 저빈도(예: 10초마다)로
   다시 감지하는 별도 작업이 필요하다.
2. **보드 확인.** 이 플랜의 검증은 빌드와 호스트 시험까지다. 머지 뒤 GPS를 뗀 보드와 단 보드에서
   전원 ON부터 `[B]GPS OK` 로그와 HEALING 진입까지의 시간을 RTT로 확인해 주어야 한다.
   검증을 약하게 하는 항목(시험 삭제, 시간 제한 증가, 범위 축소)은 없다.
