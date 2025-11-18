# PSA STM32 Firmware

STM32H723VGTx 기반 WithForce 제품 펌웨어 (v1.00.35)

## 📁 프로젝트 구조

```
├── docs/                    # 📚 프로젝트 문서
│   ├── CLAUDE.md           # Claude Code 작업 가이드
│   ├── LED_ERROR_CODE_SYSTEM.md  # LED 에러 코드 현장 매뉴얼
│   ├── PLATFORMIO_SETUP.md # PlatformIO 빌드 환경 설정
│   └── ...                 # 기타 기술 문서
├── build-tools/            # 🔧 빌드 스크립트
│   ├── platformio_extra.py
│   ├── platformio_fix_linker.py
│   └── upload.bat
├── Core/                   # STM32 HAL 초기화 및 메인 루프
├── Drivers/                # STM32 HAL 및 CMSIS 드라이버
├── User/                   # 애플리케이션 코드
│   ├── Drv/               # 하드웨어 드라이버 래퍼
│   ├── Edit/              # 모드 제어 및 통신 로직
│   └── Lib/               # 유틸리티 라이브러리
├── [Sensor Modules]/       # 센서 드라이버 (ADS111x, AS6221, etc.)
└── platformio.ini          # PlatformIO 빌드 설정
```

## 🚀 빌드 방법

### PlatformIO (권장)

```bash
# 디버그 빌드
pio run -e debug

# 빌드 및 업로드
pio run -t upload

# 시리얼 모니터
pio device monitor
```

### STM32CubeIDE

```bash
cd Debug
make -j4
```

## 📖 주요 문서

- **[docs/CLAUDE.md](docs/CLAUDE.md)** - Claude Code 작업 가이드
- **[docs/LED_ERROR_CODE_SYSTEM.md](docs/LED_ERROR_CODE_SYSTEM.md)** - LED 에러 코드 현장 매뉴얼
- **[docs/PLATFORMIO_SETUP.md](docs/PLATFORMIO_SETUP.md)** - PlatformIO 설정 가이드
- **[docs/UPLOAD_TROUBLESHOOTING.md](docs/UPLOAD_TROUBLESHOOTING.md)** - 업로드 문제 해결
- **[docs/GPS_RESTORATION.md](docs/GPS_RESTORATION.md)** - GPS 복원 가이드
- **[docs/ERROR_MODE_ACTUATOR_SAFETY.md](docs/ERROR_MODE_ACTUATOR_SAFETY.md)** - 에러 모드 안전 설계
- **[docs/TIMEOUT_HANDLER_STANDARDIZATION.md](docs/TIMEOUT_HANDLER_STANDARDIZATION.md)** - 타임아웃 핸들러 표준화
- **[docs/PROBE_FLOOD_FIX.md](docs/PROBE_FLOOD_FIX.md)** - 프로브 플러드 수정

## 🎯 하드웨어

- **MCU**: STM32H723VGTx (ARM Cortex-M7, 192 MHz)
- **FPU**: FPv5-D16 (하드 플로트)
- **Flash**: 1 MB (현재 사용량: ~156 KB, 14.9%)
- **RAM**: 320 KB (현재 사용량: ~79 KB, 24.2%)

## 📡 주요 기능

- 온도 제어 (히터/쿨러)
- 압력 센서 (FSR)
- GPS 위치 추적 (SAM-M10Q)
- 열화상 카메라 (MLX90640)
- 오디오 재생 (ES8388 + MP3)
- WiFi 통신 (ESP8266)
- LED 에러 코드 표시

## 🔧 개발 환경

- **Toolchain**: GNU ARM Embedded (arm-none-eabi-gcc)
- **IDE**: STM32CubeIDE / VS Code + PlatformIO
- **Debugger**: ST-Link v2/v3
- **OpenOCD**: 업로드 및 디버깅

## 📝 펌웨어 버전

**v1.00.35** (2025-11-18)
- ✅ 실내 온도 센서 에러 추가
- ✅ LED 에러 패턴 재정렬
- ✅ AS6221 드라이버 버그 수정
- ✅ 모든 로그 비활성화 (22KB 절약)

## 📄 라이선스

내부 프로젝트 - 비공개
