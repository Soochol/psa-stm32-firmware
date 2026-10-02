# LTE·GPS 선택적 부팅

STM32 기준 시작점: `6b3265b`. 함께 변경한 ESP32 기준 시작점: `b55c510`.

## GPS와 부팅

GPS를 부팅의 필수 센서 준비 마스크에서 제외한다. `v_Mode_Booting()`이 기존처럼 GPS 비동기 초기화를 시작하지만, 필수 센서·기존 기울기 보정·음성 조건이 준비되면 GPS 준비를 기다리지 않고 부팅을 완료한다. 필수 센서의 오류·20초 초기화 제한·ERROR 전환은 유지한다. 위성 fix는 부팅 완료 조건으로 사용하지 않는다.

GPS는 기존 2초 전원 안정화 대기와 UBX 설정을 유지한다. I2C 즉시 실패·오류 IRQ·1.2초 수신 완료 제한·8초 전체 초기화 제한이 발생하면 미사용 상태로 전환한다. 이전 PVT를 무효화하고 I2C3 대기열과 진행 중 통신을 취소한다. 정상 GPS 폴링과 반복 상태 출력은 미사용 상태에서 실행하지 않는다.

30초 후 다시 초기화를 시도하며 연속 실패 시 60초 간격으로 재확인한다. HAL이 READY 상태로 돌아온 뒤에만 새 시도를 시작하여 이전 abort·완료 콜백과 겹치지 않게 한다. OFF 진입에서는 GPS를 deinit하여 재시도를 멈춘다. 정상 응답이 돌아오면 재부팅 없이 기존 GPS 폴링과 좌표 전달로 복귀한다. ESP32로 보내는 GPS 10바이트 영역은 유지한다.

## 전원 프로토콜 `0x85`

기존 요청 `operation(0 OFF / 1 ON), token LE16`과 응답 `operation, token LE16, status` 형식을 유지한다.

| status | 의미 |
|---|---|
| 0 | ON: ESP UART 준비 완료. OFF: ESP 절전 준비 및 LTE RF OFF 확인 완료 |
| 2 | 요청 수락, 준비 중 또는 OTA로 보류 중 |
| 3 | OFF 전용: ESP 절전 준비 완료, LTE RF 상태는 확인하지 못함 |

status 3은 RF OFF 성공으로 기록하지 않는다. operation·token·프레임 검증을 통과한 OFF 응답에서만 핸드셰이크 완료로 받아들인다. ON에 대해서는 성공 응답으로 받아들이지 않는다. 기존 15초 제한·UART wake burst·OTA BUSY·OFF 취소 동작은 유지한다.

ESP32는 모뎀 완전 무응답 시 2초, 응답·오류·잡음이 있는 경우 총 12초 내에 확인을 끝내며 미확인일 때 status 3을 보낸다. 새 STM32는 이전 ESP32의 status 0/2도 처리한다. 이전 STM32의 status 3 처리는 실패 fallback이므로 STOP 동작의 실기기 호환 검증이 필요하다. 두 펌웨어를 함께 적용하는 것을 검증 기준으로 한다.

## 검증

```sh
pio run -e stm32_debug -e stm32_release
python3 tests/test_optional_gps.py
python3 tests/test_optional_boot.py
python3 tests/test_power.py
python3 tests/test_power_cycle.py ../psa-esp32-firmware
```

GPS 테스트는 실제 플랫폼·core·UBX 코드와 I2C 취소·오류 콜백을 실행한다. 무응답·NACK·오류·늦은 응답·fix 없음·타이머 wraparound·OFF 중 재시도 중단을 검증한다. 부팅 테스트는 IMU/FSR 네 설정 조합에서 GPS BUSY 상태의 부팅 완료와 필수 센서 오류·제한 유지 여부를 확인한다. 전원 통합 테스트는 양쪽 실제 전원 함수로 정상/무응답/RF 미확인 OFF→ON과 OTA 중 OFF 취소를 확인한다.

호스트 테스트와 debug/release 빌드는 통과했다. 실기기의 모듈 있음/없음 조합, 실제 필수 센서 초기화, GPS fix 및 재연결, OFF 전류, GPIO/UART 깨우기, 제조 인증서 설치, BLE OTA는 추가 검증 대상이다. 펌웨어 업로드는 수행하지 않았다.

## 2026-10-02 센서 오류 비교

기존 장치 RTT에서는 오른쪽 IMU의 I2C2 ACK 실패(`ErrorCode=0x00000004`, 7비트 주소 `0x69`)와 반복 타임아웃이 관측됐다. 해당 부팅은 정상 모드 진입 전 `ready_mask=0x1C/0x3F`에서 20초 제한에 도달하여 IR·FSR 오류 `0x0101`로 전환됐다. `tests/test_optional_boot.py`는 실제 부팅 함수를 실행하여 IMU가 BUSY 상태로 남으면 FSR·IR 초기화 함수가 호출되지 않은 채 두 오류가 발생하는 기존 경로를 재현한다. 이 브랜치는 해당 IMU 실패 처리 경로를 수정하지 않는다.

이후 사용자는 기존 장치에 ESP32·STM32 모두 이전 main을 업로드한 뒤에도 같은 오류가 재현됐으며, 새 장치에서는 양쪽 모두 이 작업 브랜치의 펌웨어로 정상 동작 중이라고 보고했다. 기존 장치의 하드웨어 문제 가능성이 높아졌지만 물리적 원인은 미확정이다. 새 장치 RTT 접속은 실패하여 정상 초기화 로그와 반복 전원 전환은 아직 검증하지 못했다. 업로드는 사용자가 수행했다.
