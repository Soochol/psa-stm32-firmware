# ESP32-STM32 SD 로깅 확장 사양서 v1.0 — STM32 팀 회신

> **대상 문서**: `ESP-STM32_SD_Logging_Extension_v1.0.md` (제안, ESP32 팀)
> **회신 일자**: 2026-08-11
> **근거 리비전**: `psa-stm32-firmware` @ `7def8e7`
> **판정**: **채택 가능**. 단 §13 Q1의 전제 정정 및 아래 7건 반영 필요

---

## 📋 목차

0. [요약](#0-요약)
1. [🔴 Q1 정정 — 현행 SD 로깅은 동작하지 않습니다](#1--q1-정정--현행-sd-로깅은-동작하지-않습니다)
2. [✅ 검증 완료 — 문서가 정확한 부분](#2--검증-완료--문서가-정확한-부분)
3. [⚠️ 문서 정정 요청 8건](#3-️-문서-정정-요청-8건)
4. [🔧 STM32 측 구현 장애물 — 파서 화이트리스트](#4--stm32-측-구현-장애물--파서-화이트리스트)
5. [§13 결정 요청 항목 답변 (Q1~Q9)](#5-13-결정-요청-항목-답변-q1q9)
6. [정정 요청 요약표](#6-정정-요청-요약표)
7. [STM32 측 작업 항목 및 다음 단계](#7-stm32-측-작업-항목-및-다음-단계)
8. [부록 — 근거 코드 위치 색인](#8-부록--근거-코드-위치-색인)

---

## 0. 요약

문서의 아키텍처 판단(Tier 1 = SD 원본, BLE = 관측 수단), §5 앵커 방식, 고정 80 B + CRC 레코드,
병합 키 `(deviceId, bootId, seq)` 는 모두 타당하며 STM32 하드웨어 제약과도 정합합니다.
특히 **§3 정오 사항(체크섬 0xA5 / LEN = 4 + dataLen / 커맨드 점유 현황)은 STM32 코드와 정확히 일치**하므로
그대로 진실로 삼으셔도 됩니다.

다만 회신에서 가장 중요한 세 가지는 다음과 같습니다.

| # | 항목 | 영향 |
|---|------|------|
| **A** | **§13 Q1의 전제가 틀렸습니다.** 현행 펌웨어는 SD에 **0바이트**를 씁니다 (부팅 8.2초 후 로깅 영구 정지) | "이미 충족되어 제거 가능한 항목"은 **없음**. 문서 요구사항 전체가 신규 구현 대상 |
| **B** | **§11 / §12 게이팅 대상이 SLEEP이 아니라 ERROR입니다.** SLEEP에서는 샘플링이 계속됩니다 | 경보 게이팅 조건 및 수용 기준 문구 수정 필요 |
| **C** | **§10.1 에러 비트값이 한 칸씩 밀렸습니다.** SD는 0x0200, MP3는 0x0400 | 그대로 구현 시 ESP 헬스 리포트가 **FSR 고장을 SD 고장으로 오보** |

---

## 1. 🔴 Q1 정정 — 현행 SD 로깅은 동작하지 않습니다

제안서 §13 Q1은 *"현재 이미 SD에 데이터를 기록 중인 것으로 알고 있습니다"* 를 전제하고 있으나,
코드 확인 결과 **현행 펌웨어는 부팅 약 8.2초 후 로깅이 영구 정지**하며 `sensor.bin` 은 0바이트로 남습니다.

### 1.1 원인 — 선언 레코드 크기와 실제 레코드 크기의 불일치

`User/Drv/src/sd.c:336-339`

```c
#define SD_LOG_RECORD_SIZE   56                     // 4B timestamp + 52B sensor data
#define SD_LOG_FLUSH_ITV     10000                  // 10s
#define SD_LOG_BUF_MAX       100                    // 10s / 100ms
#define SD_LOG_BUF_SIZE      (56 * 100)             // = 5600 B
```

`SD_LOG_RECORD_SIZE` 는 STAT 페이로드가 52 B이던 시절의 값입니다.
이후 GPS 10 B (`User/Edit/src/comm_esp.c:655-690`) 와 각도 12 B (`:692-704`) 가 추가되어
**STAT DATA는 64 B로 확장**되었으나, 위 상수는 갱신되지 않았습니다.

실제 레코드 = `4 B (tickMs, LE) + 64 B (STAT payload)` = **68 B**

| 항목 | 값 |
|------|-----|
| 버퍼 크기 | 5,600 B |
| 실제 레코드 크기 | 68 B |
| 수용 가능 레코드 | **82개 = 8.2초분** |
| flush 주기 | **10초** |

버퍼가 flush 시점보다 **1.8초 먼저 가득 찹니다.**

### 1.2 왜 "지연"이 아니라 "영구 정지"인가

가드 조건이 flush 검사보다 **앞에** 있는 것이 결정적입니다.

`User/Drv/src/sd.c:421-440`

```c
void v_SD_Log_Write(uint8_t* pu8_data, uint16_t u16_len){
    if(!b_logOpen) return;
    if((u16_logBufIdx + 4 + u16_len) > SD_LOG_BUF_SIZE) return;   // ← [sd.c:423] 조기 반환

    /* ... timestamp + payload 적재 ... */

    if(_b_Tim_Is_OVR(u32_Tim_1msGet(), u32_logFlushRef, SD_LOG_FLUSH_ITV)){
        v_SD_Log_Flush();                                          // ← [sd.c:437] 도달 불가
    }
}
```

진행 순서:

1. t = 0 ~ 8.2 s → 레코드 82개 적재, `u16_logBufIdx = 5,576`
2. t = 8.2 s (83번째) → `5576 + 68 = 5644 > 5600` → **`sd.c:423` 조기 반환**
3. 조기 반환이므로 `sd.c:437` flush 검사에 **도달하지 못함** → `u32_logFlushRef` 갱신 불가
4. 이후 모든 호출이 2번에서 반환 → **`v_SD_Log_Flush()` 는 단 한 번도 호출되지 않음**

`v_SD_Log_Flush()` 가 유일한 `f_write()` 경로이므로(`sd.c:452`),
**첫 flush(t = 10 s)가 오기 전에 쓰기 경로가 스스로 막히는 데드락**입니다.
`b_SD_Log_Open()` 이 `f_open` 으로 파일을 생성만 하고(`Core/Src/main.c:362`), 내용은 영원히 비어 있습니다.

### 1.3 부수 문제 — 카드에 기록되는 포맷 설명서가 구버전입니다

`User/Drv/src/sd.c:347-380` 의 `c_sensorFmt` 는 카드에 `sensor_fmt.txt` 로 저장되어
병합 도구가 참조하도록 의도된 문서인데, **56 B / GPS까지만 있는 구버전 레이아웃**을 기술하고 있습니다.
각도 12 B가 누락되어 있어 그대로 신뢰하면 파싱이 어긋납니다.

### 1.4 Q1의 정직한 답

- **현재 SD에 남는 데이터는 없습니다.**
- 따라서 본 제안서에서 "이미 충족되어 삭제할 항목"은 없으며, **요구사항 전체가 신규 구현 대상**입니다.
- 다만 기반은 존재합니다 — SDMMC2 4비트 39.2 MHz 마운트 경로(`sd.c:270-309`)와 FatFs 연동은 검증되어 있습니다.

### 1.5 재발 방지 (STM32 측 설계 반영 사항)

이 사고의 본질은 **선언된 레코드 크기와 실제 기록 크기 사이에 강제 수단이 없었다는 것**입니다.
제안서 §7이 `statPayload` 64 B를 그대로 복사하도록 설계한 이상 동일한 사고가 재발할 수 있으므로,
STM32 구현 시 다음을 넣겠습니다.

- SD 레코드 조립 시점에 `u16_len != 64` 이면 레코드를 드롭하고 `writeErrorCount` 를 증가
- 컴파일 타임 `_Static_assert(sizeof(x_sd_record_t) == 80, ...)`
  (같은 리포의 `User/Edit/src/flash_cfg.c:30` 에 동일 패턴 선례가 있습니다)

> 제안서 §7의 `formatVersion` 은 **의도적 포맷 변경**은 다루지만
> **실수로 어긋난 길이**는 잡지 못합니다. 위 런타임 계약을 함께 두는 것을 권장합니다.

---

## 2. ✅ 검증 완료 — 문서가 정확한 부분

제안서 §3 정오 사항은 **전부 STM32 코드와 일치**합니다. 수정 없이 진행하셔도 됩니다.

| 제안서 주장 | STM32 코드 근거 | 판정 |
|-------------|-----------------|------|
| 체크섬 초기값 `0xA5`, `STX`·`LEN`·`DIR`·`CMD`·`DATA` 전체 XOR | `comm_esp.c:19` (`ESP_FMT_CHK_INIT`), `:138-157` | ✅ 일치 |
| `LEN = 4 + dataLen` | `comm_esp.c:18` (`ESP_FMT_LEN_MIN = 6 - 2 = 4`), `:145` | ✅ 일치 |
| `0x54` = `ctrlSpkPlay` (MP3 트랙 1~33) | `comm_esp.c:62`, `:536-540` | ✅ 일치 |
| `0x83` = `evtWarn` (LEN=1, payload 0 = 배터리 저전압) | `comm_esp.c:70`, `comm_esp.h:37-39` | ✅ 일치 |
| `0x22` = `initWearableFanPWM` | `comm_esp.c:42` (`ESP_CMD_INIT_PWM_BLOWFAN`) | ✅ 일치 |
| STAT(0x70) DATA = 64 B이며 포화 상태 | `comm_esp.c:586-706` 합산: 12+12+4+6+4+2+2+10+12 = **64** | ✅ 정확 |
| 샘플링 10 Hz | `mode.h:342` (`MODE_SENSING_SEND_ITV 100`) | ✅ 일치 |
| 신규 코드 `0x23/0x43/0x56/0x84` 미점유 | `comm_esp.c:29-73` enum 전수 확인 | ✅ 충돌 없음 |

### 2.1 프레임 예제 검산

제안서에 실린 신규 커맨드 프레임 예제의 체크섬을 STM32 알고리즘으로 재계산했습니다.

| 커맨드 | 프레임 | 문서 체크섬 | 검산 | 판정 |
|--------|--------|-------------|------|------|
| `initLogIdentity` 요청 | `02 0A 20 23 A1 B2 C3 D4 E5 F6 ..` | `B9` | `B9` | ✅ |
| `initLogIdentity` 응답 | `02 04 02 23 ..` | `82` | `82` | ✅ |
| `reqLogStatus` 요청 | `02 04 20 43 ..` | `C0` | `C0` | ✅ |
| `ctrlLogEnable(0)` | `02 05 20 56 00 ..` | `D4` | `D4` | ✅ |
| §3.2 정답 예제 | `02 06 20 10 19 37 ..` | `BF` | `BF` | ✅ |

---

## 3. ⚠️ 문서 정정 요청 8건

### 3.1 [정정 ①] §10.1 — 에러 비트값이 한 칸씩 밀렸습니다 🔴

`User/Edit/inc/mode.h:170-185` 의 실제 정의:

```c
typedef enum {
    modeERR_TEMP_IR     = (1<<0),   // 0x0001
    modeERR_TEMP_OUT    = (1<<1),   // 0x0002
    modeERR_TEMP_IN     = (1<<2),   // 0x0004
    modeERR_IMU         = (1<<3),   // 0x0008
    modeERR_BLOW_FAN    = (1<<4),   // 0x0010
    modeERR_COOL_FAN    = (1<<5),   // 0x0020
    modeERR_TOF         = (1<<6),   // 0x0040
    modeERR_AUDIO       = (1<<7),   // 0x0080
    modeERR_FSR         = (1<<8),   // 0x0100  ← 문서가 ERR_SD_CARD 라고 기재
    modeERR_SD_MOUNT    = (1<<9),   // 0x0200  ← 실제 SD
    modeERR_MP3_FILE    = (1<<10),  // 0x0400  ← 실제 MP3
    modeERR_HEATER_CURR = (1<<11),  // 0x0800  ← 문서 미기재
    modeERR_ESP_COMM    = (1<<12),  // 0x1000  ← 문서 미기재
} e_modeERR_t;
```

`errInit`(0x90) 페이로드는 `User/Edit/src/mode.c:2334` 에서
`v_ESP_Send_Error((uint16_t)e_Mode_Get_Error())` 로 이 비트마스크를 **가공 없이 그대로** 전송합니다.
따라서 위 값이 진실입니다.

| | 문서 §10.1 | **실제** |
|---|-----------|----------|
| `ERR_SD_CARD` | 0x0100 | **0x0200** (`modeERR_SD_MOUNT`) |
| `ERR_MP3_FILE` | 0x0200 | **0x0400** (`modeERR_MP3_FILE`) |
| 0x0100의 의미 | — | **`modeERR_FSR`** (압력 센서) |

> ⚠️ 이대로 구현하면 **FSR 고장이 SD 고장으로, SD 고장이 MP3 오류로 보고**됩니다.
> §10.1이 "ERR_MP3_FILE과 SD를 혼동하지 말 것"을 강조하는 절이므로 특히 중요합니다.

### 3.2 [정정 ②] §11 / §12 — 게이팅 대상은 SLEEP이 아니라 ERROR 🔴

제안서 §11 및 §13 Q9는 *"SLEEP 모드에서 샘플링이 멈추면 `lastSeq` 가 정체된다"* 를 가정하지만,
**실제는 반대**입니다.

`User/Edit/src/mode.c:2025-2060` 의 `v_Mode_Sleep()` 은 `v_Mode_Sensing_Handler()` 를 **호출합니다**
(`mode.c:2055`). SLEEP 중에도 seq는 10 Hz로 정상 증가합니다.

센싱을 수행하는 모드와 그렇지 않은 모드는 다음과 같습니다.

| 샘플링 **수행** | 샘플링 **정지** |
|-----------------|-----------------|
| Healing (`mode.c:1667`) | **ERROR** (`mode.c:2274` — 센싱 호출 없음) |
| Waiting (`:1738`) | BOOTING (`:1343`) |
| ForceUp (`:1829`) | WAKE_UP (`:2163`) |
| ForceOn (`:1907`) | OFF (`:2074`, `i_mode_off = 1`) |
| ForceDown (`:2006`) | |
| **Sleep** (`:2055`) | |
| TEST (`:2594`) | |

**요청 사항**

- §11의 `lastSeq` 증가율 경보는 `evtMode(0x82) == ERROR(6)` 으로 게이팅
  (`ESP_EVT_MODE_ERROR = 6`, `comm_esp.h:34`)
- §12 시나리오 1의 합격 기준을 **"비-ERROR 구간 연속성 100%"** 로 수정
- §13 Q9의 "멈추는 경우" 분기는 삭제 — SLEEP은 "계속되는 경우"로 확정

### 3.3 [정정 ③] §6 — ESP→STM 방향 DATA 상한은 64 B가 아니라 32 B

제안서 §6 서두는 *"모든 DATA는 64 B 제약을 준수합니다"* 라고 기술하지만,
STM32 수신 파서에는 **32 B 하드 리밋**이 있습니다.

`User/Edit/src/comm_esp.c:204, 232-239`

```c
uint8_t data[32];
...
if(data_len > 32){          // ← 초과 시 프레임 폐기 + 링버퍼 1바이트 재동기화
    v_Uart_ESP_DisableIT();
    espRx->fn.b_Jmp(espRx, 1);
    v_Uart_ESP_EnableIT();
    len = 0;
    return;
}
```

방향별 실제 상한:

| 방향 | 상한 | 근거 |
|------|------|------|
| ESP → STM (수신) | **32 B** | `comm_esp.c:233` |
| STM → ESP (송신) | **90 B** | `comm_esp.c:132` (`ESP_TX_FMT_BUF_SIZE(96) - 6`) |

신규 커맨드 요청 페이로드는 6 B / 0 B / 1 B 이므로 **당장은 문제없습니다.**
다만 향후 확장 사고 방지를 위해 문서에 방향별 상한을 명시해 주십시오.

### 3.4 [정정 ④] §8.2 — LFN이 활성화되어 있으므로 기본 명명 그대로 사용 가능

`FATFS/Target/ffconf.h:112-113`

```c
#define _USE_LFN     1      /* 0 to 3 */
#define _MAX_LFN     255
```

→ §8.2 기본 명명 `/LOG/A1B2C3D4E5F6/0000002A_0003.psa` 를 **그대로 채택**합니다.

**8.3 호환 대체안(`/LOG/002A0003.PSA`)과 그에 딸린 조건부 문구는 삭제해 주십시오.**
§13 Q4의 조건 분기도 함께 정리 대상입니다.

### 3.5 [정정 ⑤] §8.2 — 카드는 FAT32 포맷 필수 (exFAT 미지원)

`FATFS/Target/ffconf.h:212`

```c
#define _FS_EXFAT    0
```

FatFs 리비전은 **R0.12c** (`Middlewares/Third_Party/FatFs/src/ff.h:22`, `_FATFS 68300`) 이며 exFAT은 비활성입니다.

- 32 GB 이하 (SDHC / FAT32) → 정상
- **64 GB 이상 SDXC를 공장 출하 exFAT 상태로 삽입하면 마운트 실패**

제안서 §1은 32 GB를 기준으로 하고 있어 실무상 문제는 없으나,
운영 지침에 **"카드는 FAT32로 포맷할 것"** 을 명시해 주십시오.
(현장에서 64 GB 카드를 그대로 꽂으면 `evtLogError(0)` 만 반복하고 데이터가 하나도 남지 않습니다.)

### 3.6 [정정 ⑥] §3.3 — STM32 파서의 카테고리 상한은 ESP 맵보다 좁습니다

제안서 §3.3은 카테고리 범위를 `INIT 0x10-0x29 / REQ 0x30-0x49 / CTRL 0x50-0x69 / EVT 0x80-0x89` 로
기술했으나, 이는 **ESP32 측 맵**입니다. STM32 파서의 실제 허용 상한은 더 좁습니다 (상세는 §4).

문서에 "STM32 측은 `0x22 / 0x42 / 0x55 / 0x83` 상한의 화이트리스트이며,
신규 코드 사용 시 STM32 파서 확장이 선행되어야 함"을 명시해 주십시오.
**이 항목이 누락되면 ESP 측 구현 완료 후에도 STM32가 프레임을 전부 폐기하여
"통신이 안 되는" 증상으로 나타납니다.**

### 3.7 [정정 ⑦] §13 Q1 — 전제 삭제

§1 참조. "이미 기록 중" 전제와 "이미 충족된 항목을 제거하여 작업량을 줄일 수 있습니다" 문구를
삭제해 주십시오. 현행 기록물은 없습니다.

### 3.8 [정정 ⑧] §7 `deviceMode` — 값 범위가 부족하고, 두 개의 번호 체계가 섞여 있습니다 🔴

제안서 §7은 레코드 오프셋 72의 `deviceMode` 를 **"0=SLEEP ~ 4=FORCE_DOWN"** 으로 정의했습니다.
이 범위로는 실제로 기록될 값을 담을 수 없습니다.

#### (1) 두 개의 서로 다른 열거형이 존재합니다

STM32 내부 모드 ID와 프로토콜 모드 코드는 **번호 체계가 완전히 다릅니다.**

| 값 | `e_modeID_t` (STM32 내부, `mode.h:130-140`) | `e_ESP_EVT_MODE_t` (프로토콜, `comm_esp.h:27-35`) |
|---|---|---|
| 0 | `modeBOOTING` | `SLEEP` |
| 1 | `modeHEALING` | `WAITING` |
| 2 | `modeWAITING` | `FORCE_UP` |
| 3 | `modeFORCE_UP` | `FORCE_ON` |
| 4 | `modeFORCE_ON` | `FORCE_DOWN` |
| 5 | `modeFORCE_DOWN` | `TEST` |
| 6 | `modeSLEEP` | `ERROR` |
| 7 | `modeOFF` | — |
| 8 | `modeERROR` | — |
| 9 | `modeTEST` | — |
| 10 | `modeWAKE_UP` | — |

§7은 "0=SLEEP"이라 했으므로 **프로토콜 열거형(`e_ESP_EVT_MODE_t`) 기준**으로 읽힙니다.
STM32는 기록 시 반드시 내부 ID → 프로토콜 코드로 **변환**해야 하며, 이를 문서에 명시해 주십시오.
(변환을 빠뜨리면 `modeWAITING(2)` 가 `FORCE_UP(2)` 으로 기록되는 등 조용히 어긋납니다.)

#### (2) 상한이 4가 아니라 5입니다 — TEST가 기록됩니다

§3.2의 샘플링 표 참조. **TEST 모드는 샘플링을 수행**하므로(`mode.c:2594`)
`deviceMode = 5 (TEST)` 인 레코드가 CRC 정상 상태로 카드에 남습니다.
"0~4" 기준으로 만든 병합 도구는 이 레코드를 해석하지 못합니다.

- `TEST(5)` → **기록됨**. 범위에 포함해 주십시오
- `ERROR(6)` → 기록되지 않음 (`mode.c:2274` 센싱 정지). 다만 값 자체는 문서에 남겨 두는 편이 안전합니다

#### (3) HEALING은 프로토콜에 대응 값이 아예 없습니다 ⚠️

`modeHEALING` 은 부팅 직후 `BOOTING → HEALING → WAITING` 경로의 **약 1초짜리 전이 상태**입니다
(`mode.c:1554` 진입, `mode.h:345` `MODE_HEALING_INITIAL_TOUT 1000`, `mode.c:1659-1660` 에서 WAITING 전이).

문제는 다음 두 가지가 겹친다는 점입니다.

- `v_Mode_Healing()` 은 **샘플링을 수행합니다** (`mode.c:1667`) → 매 부팅 약 10개 레코드 발생
- 그런데 `v_Mode_Healing()` 은 **`v_ESP_Send_EvtModeChange()` 를 호출하지 않습니다**
  (evtMode 송신 지점은 `mode.c:1698 / 1783 / 1864 / 1951 / 2035 / 2333 / 2455` 7곳뿐이며 Healing은 없음)

즉 `e_ESP_EVT_MODE_t` 에 **HEALING에 해당하는 값이 존재하지 않습니다.**

**요청**: §7 `deviceMode` 표에 HEALING 값을 신설해 주십시오. 다음 중 택일이면 됩니다.

| 안 | 내용 | 비고 |
|----|------|------|
| **(a)** | `e_ESP_EVT_MODE_t` 에 `HEALING = 7` 추가 (권장) | evtMode(0x82)와 SD 레코드가 같은 코드계를 유지 |
| (b) | SD 레코드에서만 `0xFF = UNKNOWN` 으로 기록 | 프로토콜 변경 없음. 단 1초 구간의 모드 식별 불가 |

(a)를 택하시면 STM32 측에서 `v_Mode_Healing()` 에 `evtMode` 송신도 함께 추가하겠습니다.

> **정리**: §7 `deviceMode` 를 **"프로토콜 모드 코드 (`e_ESP_EVT_MODE_t`), 0=SLEEP ~ 6=ERROR"** 로
> 확장하고, HEALING 값을 신설. STM32 내부 `e_modeID_t` 와는 별개 체계임을 주석으로 명시.

---

## 4. 🔧 STM32 측 구현 장애물 — 파서 화이트리스트

신규 커맨드 4종은 **코드가 비어 있어서 바로 쓸 수 있는 것이 아닙니다.**
STM32 수신 파서는 **화이트리스트 방식**이라, 등록되지 않은 커맨드는 CHK 검증 이전에 폐기됩니다.

`User/Edit/src/comm_esp.c:172-184`

```c
bool b_ESP_CmdCompare(uint8_t u8_cmd){
    if(u8_cmd == ESP_CMD_STAT                                              // 0x70
    ||(u8_cmd >= ESP_CMD_INIT_TEMP_SLEEP && u8_cmd <= ESP_CMD_INIT_PWM_BLOWFAN)  // 0x10~0x22
    ||(u8_cmd >= ESP_CMD_REQ_TEMP_SLEEP  && u8_cmd <= ESP_CMD_REQ_PWM_BLOWFAN)   // 0x30~0x42
    ||(u8_cmd >= ESP_CMD_CTRL_RST        && u8_cmd <= ESP_CMD_CTRL_COOLFAN_ON)   // 0x50~0x55
    ||(u8_cmd >= ESP_CMD_EVT_INIT_START  && u8_cmd <= ESP_CMD_EVT_WARN)          // 0x80~0x83
    ||(u8_cmd == ESP_CMD_ERR)){                                            // 0x90
        return true;
    }
    return false;
}
```

| 신규 코드 | 해당 카테고리 상한 | 현재 결과 |
|-----------|--------------------|-----------|
| `0x23` `initLogIdentity` | INIT 상한 `0x22` | ❌ 폐기 |
| `0x43` `reqLogStatus` | REQ 상한 `0x42` | ❌ 폐기 |
| `0x56` `ctrlLogEnable` | CTRL 상한 `0x55` | ❌ 폐기 |
| `0x84` `evtLogError` 의 **ACK** | EVT 상한 `0x83` | ❌ 폐기 |

### STM32 측 수정 대상 3곳

1. **`b_ESP_CmdCompare()`** (`comm_esp.c:172`) — 카테고리 상한 확장
2. **`v_ESP_RxProc()`** (`comm_esp.c:299-312`) — INIT / REQ / CTRL 디스패치 범위도 동일하게 확장
   (여기가 누락되면 CHK는 통과하지만 아무 동작도 하지 않습니다)
3. **`v_ESP_RxAck()`** (`comm_esp.c:314-318`) — 현재 `ESP_CMD_STAT` 만 처리.
   `evtLogError(0x84)` 에 대한 ESP측 ACK 수신 분기 추가 필요

> 3번을 빠뜨리면 ACK가 올 때마다 링버퍼가 `b_Jmp(1)` 로 **1바이트 단위 재동기화**에 들어갑니다
> (`comm_esp.c:246-252`). 기능은 동작하지만 수신 지연과 CPU 낭비가 발생합니다.

---

## 5. §13 결정 요청 항목 답변 (Q1~Q9)

### Q1. 현행 SD 로깅 현황 → §1 참조

| 항목 | 현황 |
|------|------|
| 레코드 포맷 | 바이너리 `4 B tickMs(**LE**) + STAT 64 B` = 68 B 고정 |
| 기록 주기 | 10 Hz (`mode.h:342`) |
| 파일 명명 | `sensor.bin` 고정. **로테이션 없음**, 매 부팅 `FA_OPEN_APPEND` |
| flush 주기 | 명목상 10초 — **실제로는 한 번도 실행되지 않음** |
| seq / CRC / 파일 헤더 | **전부 없음** |
| 전원 차단 시 동작 | **`v_SD_Log_Close()` 가 어디서도 호출되지 않음** (아래 참조) |
| **실제 기록량** | **0 바이트** |

전원 차단 경로에 안전 종료가 없습니다. `v_Mode_Off()` (`mode.c:2074-2130`) 는
`i_mode_off = 1` 로 센싱만 중단시키고 `MODE_POWEROFF_DELAY` 후 주변장치를 내린 뒤 STOP 모드로 진입하며,
`v_SD_Log_Close()` / `b_UnMountSD()` 를 호출하지 않습니다.

> 참고: 엔디안이 제안서 §7(`tickMs` = uint32 **BE**)과 다릅니다.
> 현행은 LE(`sd.c:427-430`)지만 신규 포맷에서 BE로 통일하겠습니다.

### Q2. bootId 소스 → **내장 플래시 카운터** (RTC 백업 레지스터는 사용 불가)

**RTC 백업 레지스터는 요구사항을 만족할 수 없습니다.** 이 보드에는 코인셀이 없습니다.

`Core/Src/main.c:1818`
> `// RTC backup is wiped when VBAT loses power (no coin cell on this board).`

실제로 펌웨어는 이 소실을 **콜드부팅 판정에 이용**하고 있습니다 (`mode.c:2621-2628`):

```c
if(u32_RTC_Read_BKUP() == 0xA5A5){ v_Mode_SetInit(modeBOOTING); }   // 웜부팅
else                             { v_Mode_SetInit(modeOFF); }       // 콜드부팅
```

즉 "전원 완전 차단 후에도 유지"라는 bootId 요구사항과 **정면으로 배치**됩니다.

**채택안**: `User/Edit/src/flash_cfg.c` 의 내장 플래시 섹터 7
(`0x080E0000`, 128 KB, 32 B 플래시워드 append 방식) 을 확장하여
`bootId` 와 `deviceId` 를 함께 보관합니다. 기존 스피커 볼륨 저장과 동일한 마모 분산 구조를 재사용합니다.

> ⚠️ **설계 시 주의 (STM32 측 자체 리스크)**
> 32 B × 4,096 슬롯이므로 **4,096 부팅마다 섹터 소거**가 발생합니다.
> STM32H723VG는 단일 뱅크이고 128 KB 섹터 소거는 1~4초가 걸리며 그동안 XIP가 정지합니다.
> IWDG는 **2초** 설정이므로(`main.c:949-953`: LSI 32 kHz ÷ 32 = 1 kHz, Reload 1999),
> 그대로 두면 4,096번째 부팅에서 워치독 리셋 루프에 빠집니다.
> → 소거는 **부팅 초기 단계에서만**, IWDG 리프레시를 끼워 수행하도록 구현하겠습니다.
> 운용 중(로깅 중)에는 절대 소거하지 않습니다.

### Q3. seq의 기준 → **맞습니다**

문서의 해석("STM32 센싱 루프가 생성한 샘플 순번, UART 송신 성공 여부와 무관")이 정확합니다.

`User/Edit/src/mode.c:1206-1210` 의 `v_Mode_Sensing_toESP()` 가 100 ms 틱에서 샘플을 생성하고,
`User/Edit/src/comm_esp.c:706-709` 에서 UART 송신과 SD 기록이 나란히 일어납니다.

```c
v_ESP_Transmit(ESP_DIR_REQ, ESP_CMD_STAT, data, cnt);   // UART 송신
v_SD_Log_Write(data, cnt);                              // SD 기록 (송신 결과와 무관)
```

UART가 단선되어도 seq는 계속 증가하며, SD **기록 호출 경로**도 그대로 진행됩니다
(§1의 버그로 인해 현재는 실제 바이트가 남지 않을 뿐, 구조상 UART와 독립입니다).
→ §7 레코드의 `flags` bit0 (STAT UART 송신 성공 여부) 도 구현 가능합니다.

### Q4. SD 카드 및 파일시스템 사양

| 항목 | 값 |
|------|-----|
| 라이브러리 | FatFs **R0.12c** (`ff.h:22`, `_FATFS 68300`) |
| **`_USE_LFN`** | **1 (활성)**, `_MAX_LFN 255` — §8.2 기본 명명 사용 가능 |
| `_FS_EXFAT` | **0 (비활성)** — **FAT32 전용**, 64 GB+ SDXC는 재포맷 필요 |
| `_VOLUMES` | 1 |
| `_MIN_SS / _MAX_SS` | 512 / 4096 |
| `_CODE_PAGE` | 850 |
| 인터페이스 | SDMMC2, **4비트**, 마운트 시 400 kHz → 이후 **39.2 MHz** 재설정 (`sd.c:294-301`) |
| DMA | 읽기 경로에 DMA 사용 (`sd.c:487-501`), D-Cache clean 처리 포함 |

→ **§8.2 기본 명명 확정, 8.3 대체안 폐기** (정정 ④·⑤ 참조)

### Q5. 100 Hz 제어 루프 부하 → **네, 실질적 우려입니다**

세 가지 근거를 제시합니다.

1. **동기 블로킹 쓰기**: `f_write()` 가 메인 루프 컨텍스트에서 동기 실행됩니다 (`sd.c:452`).
   현행 설계는 10초마다 5.6 KB를 일괄 기록하는 구조라 스파이크가 큽니다.
2. **IWDG 2초**: 워치독 리프레시는 메인 루프에서만 일어납니다 (`main.c:377`).
   SD 쓰기가 길게 물리면 제어 지터를 넘어 **워치독 리셋**으로 이어집니다.
3. **제안서 §9.1이 오히려 유리합니다**: 2초 / 20레코드 flush면 회당 1,600 B(3~4 섹터)로,
   현행 5,600 B 일괄 쓰기 대비 **피크가 1/3 이하**로 낮아집니다.

**→ 제안서 §9.1의 2초 flush 정책을 지지합니다.**
DMA 쓰기 및 더블 버퍼링은 1차 구현 후 지터를 실측하고 필요 시 도입하겠습니다.
§12 시나리오 8(제어 루프 영향도)을 수용 기준에 넣으신 것은 적절합니다.

### Q6. 용량 소진 시 정책 → **(a) 로깅 중지** 권장

Tier 1이 system of record인데 미회수 데이터를 장치가 스스로 삭제하면 설계 원칙이 무너집니다.
제안서 §1 기준 32 GB에 약 1,100시간(140 교대)이 들어가므로,
(b) 순환 삭제가 필요한 상황은 **회수 운영이 이미 실패한 경우**뿐이고
그것은 §11 헬스 리포트로 사전에 잡아야 할 문제입니다.

`freeSpaceMB < 500` 경고를 §11에 두신 것이 정확한 대응입니다.

### Q7. 카드 회수·교체 운영 방식 → **STM32 측에서 답할 수 없습니다**

코드로 판단 가능한 항목이 아니며, 현장 운영 결정 사항입니다.
**ESP32 팀 또는 운영 담당의 회신이 필요합니다.**

다만 어느 방식을 택하든 공통으로 필요한 사항을 알려드립니다.

- 현재 전원 OFF 경로에 `v_SD_Log_Close()` 호출이 없으므로(Q1 참조),
  **안전 종료 훅은 어느 쪽이든 신규 구현 대상**입니다.
- 교대마다 카드를 뽑는 방식이면 `ctrlLogEnable(0)` 을 실제로 쓰게 되므로 우선순위가 올라갑니다.
- 장치 고정 + 덤프 방식이면 전원 버튼 종료 경로에 flush + close를 심는 것으로 충분합니다.

### Q8. 커맨드 코드 충돌 → **없습니다** (단, 파서 확장 필요)

`comm_esp.c:29-73` enum 전수 확인 결과 `0x23 / 0x43 / 0x56 / 0x84` 는 미점유입니다.
다만 §4에 기술한 대로 화이트리스트 3곳 확장이 선행되어야 실제로 수신됩니다.

### Q9. SLEEP 모드에서의 샘플링 → **계속됩니다**

`User/Edit/src/mode.c:2055` — `v_Mode_Sleep()` 이 `v_Mode_Sensing_Handler()` 를 호출합니다.
SLEEP 진입 시 flush / close도 하지 않습니다(애초에 close 경로 자체가 없음).

**단, 정정 ②에 기술한 대로 `modeERROR` 에서는 샘플링이 정지합니다.**
따라서 게이팅 대상을 SLEEP → ERROR로 바꿔 주셔야 합니다.

신규 구현 시 SLEEP 처리 방침(권장):
- SLEEP 진입 시 **flush 수행** (전원이 오래 유지되지 않을 수 있으므로)
- 파일은 **닫지 않고 같은 파일에 이어쓰기** (로테이션 조건은 §8.3 그대로 적용)
- `deviceMode` 필드(§7 오프셋 72)에 SLEEP이 기록되므로 병합 도구가 구간 판별 가능

---

## 6. 정정 요청 요약표

| # | 위치 | 요청 내용 | 우선도 |
|---|------|-----------|--------|
| ① | §10.1 | 에러 비트 정정: SD = **0x0200**, MP3 = **0x0400**, 0x0100은 **FSR**. 0x0800/0x1000 추가 | 🔴 높음 |
| ② | §11, §12-1, §13 Q9 | 게이팅·합격기준을 SLEEP → **ERROR** 기준으로 변경 | 🔴 높음 |
| ③ | §6 서두 | "모든 DATA 64 B" → "ESP→STM **32 B** / STM→ESP **90 B**" | 🟡 중간 |
| ④ | §8.2, §13 Q4 | LFN 활성 확정 → 8.3 대체안 및 조건부 문구 **삭제** | 🟡 중간 |
| ⑤ | §8.2 | "카드는 **FAT32** 포맷 필수 (exFAT 미지원)" 추가 | 🟡 중간 |
| ⑥ | §3.3 | STM32 파서 상한(`0x22/0x42/0x55/0x83`)이 ESP 맵과 다름을 명시 | 🟡 중간 |
| ⑦ | §13 Q1 | "이미 기록 중" 전제 및 "작업량 축소 가능" 문구 **삭제** | 🟢 낮음 |
| ⑧ | §7 | `deviceMode` 범위 **0~4 → 0~6** 확장 (TEST=5 기록됨), **HEALING 값 신설**, `e_modeID_t` 와 별개 체계임 명시 | 🔴 높음 |

---

## 7. STM32 측 작업 항목 및 다음 단계

### 7.1 즉시 처리 (본 사양서와 무관하게 현재 데이터가 남지 않고 있음)

| # | 작업 | 파일 |
|---|------|------|
| 1 | `SD_LOG_RECORD_SIZE` 불일치 및 flush 데드락 수정 | `User/Drv/src/sd.c:336-440` |
| 2 | `c_sensorFmt` 를 실제 68 B 레이아웃으로 갱신 (또는 신규 포맷 전환 시 제거) | `User/Drv/src/sd.c:347-380` |
| 3 | 전원 OFF 경로에 `v_SD_Log_Close()` 추가 | `User/Edit/src/mode.c:2074` |

### 7.2 사양서 구현 (정정 반영 후 착수)

| 단계 | 작업 |
|------|------|
| 1 | 파서 화이트리스트 3곳 확장 (§4) — 신규 커맨드 수신 기반 확보 |
| 2 | `bootId` 플래시 카운터 구현 (Q2, 섹터 소거 IWDG 대책 포함) |
| 3 | 80 B 고정 레코드 + CRC-16/CCITT-FALSE + 512 B 파일 헤더 (§7, §8.1) |
| 4 | 파일 명명 / 로테이션 (§8.2, §8.3) — LFN 기본안 |
| 5 | 2초 flush 정책 및 안전 종료 (§9.1) |
| 6 | `evtLogError(0x84)` 이벤트 및 매체 이상 복구 (§9.3) |
| 7 | `reqLogStatus(0x43)` 응답 20 B (§6.2) |
| 8 | §12 수용 기준 검증 (시나리오 1~8) |

### 7.3 회신 대기 항목

- **Q7 (카드 회수 운영 방식)** — ESP32 팀 / 운영 담당 회신 필요
- **정정 ⑧ HEALING 값 처리** — (a) `HEALING = 7` 신설 / (b) SD 레코드에서만 `0xFF` 중 택일 회신 필요
- 정정 ①~⑧ 반영된 **사양서 v1.1** 수령 후 7.2 착수

---

## 8. 부록 — 근거 코드 위치 색인

리비전 `7def8e7` 기준입니다.

### 프로토콜

| 내용 | 위치 |
|------|------|
| 체크섬 초기값 `0xA5` | `User/Edit/src/comm_esp.c:19` |
| 프레임 조립 (`LEN = 4 + dataLen`) | `User/Edit/src/comm_esp.c:128-164` |
| 커맨드 enum 전체 | `User/Edit/src/comm_esp.c:29-73` |
| 커맨드 화이트리스트 | `User/Edit/src/comm_esp.c:172-184` |
| 수신 파서 / 32 B 상한 | `User/Edit/src/comm_esp.c:193-290` (상한 `:233`) |
| REQ 디스패치 | `User/Edit/src/comm_esp.c:299-312` |
| ACK 디스패치 | `User/Edit/src/comm_esp.c:314-318` |
| STAT 페이로드 조립 (64 B) | `User/Edit/src/comm_esp.c:580-715` |
| 에러 전송 (`errInit` 0x90) | `User/Edit/src/comm_esp.c:731-736`, `User/Edit/src/mode.c:2334` |

### SD / FatFs

| 내용 | 위치 |
|------|------|
| SD 마운트 (4비트 39.2 MHz) | `User/Drv/src/sd.c:270-309` |
| 로그 상수 (불일치 지점) | `User/Drv/src/sd.c:336-339` |
| 포맷 설명서 (구버전) | `User/Drv/src/sd.c:347-380` |
| `v_SD_Log_Write` (데드락) | `User/Drv/src/sd.c:421-440` |
| `v_SD_Log_Flush` | `User/Drv/src/sd.c:448-460` |
| `v_SD_Log_Close` (미호출) | `User/Drv/src/sd.c:468-477` |
| 로그 오픈 호출 | `Core/Src/main.c:362` |
| FatFs 설정 (LFN / exFAT) | `FATFS/Target/ffconf.h:112-113, 212` |
| FatFs 리비전 | `Middlewares/Third_Party/FatFs/src/ff.h:22` |

### 모드 / 전원 / 저장

| 내용 | 위치 |
|------|------|
| 샘플링 주기 100 ms | `User/Edit/inc/mode.h:342` |
| 에러 비트 정의 | `User/Edit/inc/mode.h:170-185` |
| 센싱 디스패치 | `User/Edit/src/mode.c:1206-1245` |
| SLEEP 센싱 호출 | `User/Edit/src/mode.c:2055` |
| ERROR 핸들러 (센싱 없음) | `User/Edit/src/mode.c:2274` |
| 전원 OFF 경로 | `User/Edit/src/mode.c:2074-2130` |
| 콜드/웜 부팅 판정 | `User/Edit/src/mode.c:2621-2628` |
| RTC BKUP — 코인셀 없음 | `Core/Src/main.c:1818-1825` |
| IWDG 2초 설정 | `Core/Src/main.c:949-953` |
| 내장 플래시 설정 저장 | `User/Edit/src/flash_cfg.c` (섹터 7, `:13-31`) |

---

## 📝 회신 정보

| 항목 | 값 |
|------|-----|
| 회신 버전 | v1.0 |
| 대상 문서 | `ESP-STM32_SD_Logging_Extension_v1.0.md` |
| 근거 리비전 | `psa-stm32-firmware` @ `7def8e7` |
| 종합 판정 | **채택 가능** — 정정 ①~⑧ 반영 후 구현 착수 |
| ESP32 팀 회신 대기 | **Q7 (카드 회수 운영 방식)**, **정정 ⑧ HEALING 값 처리**, 사양서 v1.1 |
