# 구현 플랜 — `reqDeviceVersion` (0x46) STM32 측

> **대상**: ESP32 팀 `SPEC_PROPOSAL_reqDeviceVersion` (Q79·Q80·Q81)
> **판정**: **세 질문 모두 수락 가능.** 파서 변경 없이 enum 1줄 + case 1개 + 버전 정의 1개
> **미결정**: 버전 문자열 초기값·체계 1건 (§6) — 그 외에는 막히는 것이 없습니다

---

## 0. 요약

| 회신 요청 | 판정 | 근거 |
|---|---|---|
| **Q79** 0x46 신설 | ✅ 수락 | 파서가 이미 REQUEST 범위 0x30~0x49를 통째로 수용 (`ESP_CMD_REQ_MAX`) — 라우팅 변경 없음 |
| **Q80** ASCII 16 B, 0x00 패딩, all-zero 예약 | ✅ 수락 | `strncpy` 한 줄로 복사+패딩. all-zero 예약은 비어 있지 않은 문자열이면 자동 충족 |
| **Q81** 0x81 직후 질의 | ✅ 문제없음 | 버전은 컴파일타임 상수(플래시 상주), 수신은 ISR→링버퍼라 0x81 직후 도착해도 유실 없음 |

작업은 코드 2파일 + 문서 2건입니다:

1. `User/Edit/inc/version.h` 신설 — `STM_FW_VERSION` 정의 (§2)
2. `User/Edit/src/comm_esp.c` — enum 1줄, `v_ESP_ReqProc` case 1개 (§3)
3. 제안서를 `docs/received/`로 아카이브 (§5)
4. STM32 보고 **#45** 회신 (§5)

---

## 1. 왜 파서는 손댈 것이 없는가

[comm_esp.c:95](../User/Edit/src/comm_esp.c#L95)의 카테고리 경계가 이미 일을 끝내 놓았습니다.

```c
#define ESP_CMD_REQ_MIN		(0x30)
#define ESP_CMD_REQ_MAX		(0x49)
```

SD 로깅 확장 때 "마지막 정의된 커맨드"가 아니라 **카테고리 전체**를 받도록
고친 것이 이 경계입니다. `v_ESP_RxProc`([comm_esp.c:432](../User/Edit/src/comm_esp.c#L432))는
0x46을 지금도 `v_ESP_ReqProc`로 넘기고, case가 없으니 `len=0`인 **빈 ACK**로
답합니다. 제안서 §4가 "빈 ACK로 답하거나"라고 쓴 그 동작이 현행 코드입니다.

따라서 이번 작업은 **case 하나를 채우는 것**이고, 프레임 파싱·체크섬·송신
경로는 전부 기존 그대로입니다.

### 1.1 구형 펌웨어의 실제 동작 — 두 층위 (보고 #45에 적을 것)

| 배포본 | 0x46을 받으면 |
|---|---|
| REQ 경계 확장 **이후** 빌드 (SD 로깅 파서 수정 포함) | **빈 ACK** (LEN 4, DATA 없음) |
| 그 **이전** 빌드 | **무응답** — 코드가 경계 밖이라 프레임을 버리고 바이트 단위 재동기 진입 |

둘 다 제안서 §4의 예상("빈 ACK 또는 무응답") 안입니다. 재동기는 부팅당
1프레임이라 실질 영향이 없지만, 사실관계는 회신에 명시합니다. ESP32 쪽은
**DATA 길이 ≥ 16을 유효 조건**으로 게이팅해야 합니다(빈 ACK와 구분) —
그쪽 §3.2의 길이-구분 원칙 그대로입니다.

---

## 2. Step 1 — 버전 정의: `User/Edit/inc/version.h` 신설

저장소 전체에 펌웨어 버전 정의가 **없습니다** (`FW_VERSION`·`VERSION` 계열
전수 검색 결과 없음). 새로 만듭니다.

```c
#ifndef __JH_VERSION_H
#define __JH_VERSION_H

// STM32 firmware version, reported verbatim in reqDeviceVersion(0x46).
// Wire format is 16 B ASCII zero-padded; all-zero is reserved for "unknown"
// by the spec, so this string must never be empty.
#define STM_FW_VERSION		"1.0.0"

// 16 chars max on the wire (sizeof includes the terminating NUL).
_Static_assert(sizeof(STM_FW_VERSION) <= 17, "STM_FW_VERSION exceeds 16 B wire field");

#endif
```

- **comm_esp.h가 아니라 별도 헤더인 이유**: 버전은 통신이 아니라 장치
  전체의 사실입니다. 부팅 로그·SD 로그 헤더 등이 나중에 같은 정의를 쓰게
  됩니다.
- `_Static_assert`로 16 B 초과를 **컴파일 에러**로 막습니다. 런타임 방어가
  필요 없어집니다.
- 초기값·체계는 §6에서 결정 필요.

## 3. Step 2 — comm_esp.c: enum 1줄 + case 1개

### 3.1 enum ([comm_esp.c:59](../User/Edit/src/comm_esp.c#L59) 부근, REQUEST 블록)

```c
	ESP_CMD_REQ_LOG_FILES		=0x45,	//SD logging spec 6.3
	ESP_CMD_REQ_DEVICE_VERSION	=0x46,	//SPEC_PROPOSAL_reqDeviceVersion
```

이웃 항목들의 사양-참조 주석 스타일을 따릅니다. ESP32 쪽 사양서
(`ESP-STM32_UART_Protocol_Specification`)에 절 번호가 잡히면 주석을 그
번호로 바꿉니다.

### 3.2 핸들러 (`v_ESP_ReqProc`, [comm_esp.c:563](../User/Edit/src/comm_esp.c#L563) switch)

```c
	case ESP_CMD_REQ_DEVICE_VERSION:
		// 16 B ASCII; strncpy zero-pads the remainder, which is exactly
		// the wire format. Length capped at build time in version.h.
		strncpy((char*)&data[len], STM_FW_VERSION, 16);
		len += 16;
		break;
```

- `strncpy`는 n까지 **복사 후 나머지를 0x00으로 채우므로** 복사와 패딩이 한
  줄입니다. "NUL 미보장" 경고는 여기서는 무의미합니다 — 와이어 포맷이 애초에
  NUL 종단이 아니라 고정 16 B입니다.
- 응답 프레임은 6+16 = **22 B**. `data[90]` 버퍼, TX 포맷 버퍼 96 B
  (`ESP_TX_FMT_BUF_SIZE`) 모두 여유가 큽니다.
- `#include "version.h"` 추가.

### 3.3 Q81이 성립하는 코드 근거

- 응답 내용이 **컴파일타임 상수**라 init 진행 상태와 무관합니다.
- 수신은 `HAL_UART_RxCpltCallback`([uart.c:133](../User/Drv/src/uart.c#L133))
  → `v_ESP_Recive` → 링버퍼 적재이고, 파싱은 메인루프 `v_ESP_Handler`에서
  합니다. 0x81 송신 직후 0x46이 도착해도 링버퍼에 쌓였다가 처리됩니다.
  **유실 경로가 없습니다.**

---

## 4. 검증

| 단계 | 내용 |
|---|---|
| 빌드 | 기존 빌드 경로 (STM32CubeIDE / PlatformIO — `docs/PLATFORMIO_SETUP.md`) 통과 확인. `_Static_assert`는 C11 — 빌드가 C99이면 음수 배열 크기 트릭으로 대체 |
| 프레임 예시 | 제안서 §3.3 두 예시의 체크섬을 **수기 검증 완료** — 요청 `C5`, 응답(`"1.2.3"`) `C7` 둘 다 맞습니다. 구현 후 같은 입력으로 대조 |
| 경계 케이스 | ① 16자 정확히 찬 버전(패딩 0) ② 짧은 버전 뒤쪽 0x00 패딩 ③ 구형 동작 회귀 — 0x47~0x49는 여전히 빈 ACK |
| 실기 | ESP32 측 질의 구현이 올라온 뒤 0x81→0x46 시퀀스 실측 (그쪽 §5 배포 일정과 독립) |

---

## 5. 문서 작업

1. **아카이브**: `SPEC_PROPOSAL_reqDeviceVersion.md`를 `docs/received/`로 복사
   — Q77 때의 관례(커밋 `dd2b2da`) 그대로.
2. **STM32 보고 #45** (`docs/ESP-STM32_SD_Logging_STM32_Reply_Progress_45.md`):
   - Q79 수락 (0x46 확정, 파서 무변경 근거 §1)
   - Q80 수락 + 우리 버전 체계 통보 (§6 결정값)
   - Q81 문제없음 (§3.3 근거)
   - 구형 두 층위 동작 (§1.1) + **길이 ≥ 16 게이팅** 권고
   - 관례대로 푸터에 펌웨어 커밋 해시 기재
3. **번호 확인 기록**: `Progress_45`·`Progress_46`은 양쪽 저장소 어디에도
   없음을 확인했습니다 (이 스레드의 #40~#42 재번호 사건 재발 방지).

---

## 6. 미결정 — 버전 문자열 초기값·체계 (유일한 오픈 결정)

Q80이 내용 체계를 우리 자율로 넘겼습니다. 제안:

- **수동 semver, 릴리스마다 수기 범프** — `"1.0.0"`부터. 도구 없이 확실하고,
  16 B 안에서 여유가 큽니다.
- 대안(나중 개선): 빌드 스크립트로 git 해시 주입 (`"1.0.0-9bd5679"`, 13자).
  빌드 체계 작업이 필요하므로 **이번 범위에서는 제외**, 회신에도 언급만.

초기값은 팀 결정 사항입니다. 현장 최초 식별이 목적이므로 "이 커맨드가 실린
첫 릴리스"를 `1.0.0`으로 삼는 것을 기본안으로 둡니다.

---

## 7. 범위 밖 발견 (이 플랜에 포함하지 않음)

ESP32 워크트리에 아직 우리 `docs/received/`로 전달되지 않은 문서들이
있습니다 — ESP32 보고 #38·#39·#40·**#44**, 사양서 **v1.8**,
`SPEC_PROPOSAL_ctrlLogDeleteAll`, `STM32_PowerOn_Init_Event_Spec`. 별도
스레드이므로 여기서는 다루지 않습니다.
