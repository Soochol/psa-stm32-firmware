# CLAUDE.md

PSA 장치의 STM32H723 펌웨어. 빌드·플래싱: `pio run -e stm32_debug` (업로드는 `-t upload`).

## Versioning

- 펌웨어 버전은 `User/Edit/inc/version.h`의 `STM_FW_VERSION` 한 곳이며, `reqDeviceVersion(0x46)`
  응답으로 와이어에 그대로 나간다 (16 B ASCII, 뒤쪽 0x00 패딩).
- **릴리스할 때만 올린다** (수동 semver). 커밋마다 올리면 커밋 카운터가 되어 버전이 의미를
  잃는다. ESP32 저장소 CLAUDE.md의 릴리스-only 규칙과 짝을 이루는 합의다 (보고 #67·#68).
- **개발 중에는 다음 버전에 `-dev`를 붙인다** — 릴리스 `1.0.0` 이후 펌웨어에 영향 주는 첫
  변경과 함께 `1.0.1-dev`로 바꾸고, 릴리스 시점에 `-dev`를 뗀다. 이유: ESP32와 달리 STM32
  응답에는 빌드 시각 필드가 없어서, 표식이 없으면 서로 다른 개발 빌드가 와이어에서 같은
  버전으로 보인다. ESP32·PC 뷰어는 문자열을 해석하지 않고 그대로 표시하므로(`S:1.0.1-dev`)
  이 표식에 상대 쪽 수정은 필요 없다.
- 16 B 초과는 version.h의 `_Static_assert`가 컴파일 단계에서 막는다. 빈 문자열·all-zero 16 B는
  프로토콜상 "미상" 예약이므로 금지.

## 팀 간 문서 교환

ESP32 팀과는 `docs/`의 번호 매긴 보고서로 소통한다. 받은 문서는 `docs/received/`에 원문
그대로 보관한다. 새 번호를 쓰기 전 양쪽 저장소(워크트리 포함)에서 번호 충돌을 확인하고,
파일명에 스레드 접미사를 붙인다 (예: `..._Progress_66_reqDeviceVersion.md`). 정식 전달은
상대 저장소 `docs/received-stm32/`에 커밋해서 push까지 해야 성립한다.
