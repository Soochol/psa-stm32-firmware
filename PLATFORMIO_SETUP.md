# PlatformIO Setup Guide

이 문서는 STM32H723 WithForce 펌웨어 프로젝트를 PlatformIO에서 개발하는 방법을 설명합니다.

## ✅ 빌드 성공!

**PlatformIO 빌드가 완전히 작동합니다!** 초기 FPU 호환성 문제가 해결되었습니다.

이 프로젝트는 **hard float ABI (FPv5-D16)**를 사용하며, PlatformIO와 STM32CubeIDE 모두에서 정상적으로 빌드됩니다.

## PlatformIO 설정 파일

기본 설정 파일 `platformio.ini`가 포함되어 있습니다. 이 파일은 다음을 포함합니다:

- STM32H723VGTx 타겟 설정
- 모든 인클루드 경로
- 컴파일러 플래그
- 링커 스크립트 설정

## 빠른 시작

### 빌드

```bash
# Debug 빌드
pio run -e debug

# Release 빌드
pio run -e release

# 클린 빌드
pio run -t clean && pio run
```

### 업로드

```bash
# 빌드 및 업로드
pio run -t upload

# 특정 환경으로 업로드
pio run -e debug -t upload
```

### 디버깅

VS Code에서 `F5`를 누르면 자동으로 빌드하고 디버깅을 시작합니다.

### 시리얼 모니터

```bash
pio device monitor

# 또는 특정 포트
pio device monitor --port COM3
```

## 작동 원리

### 해결된 문제들

이 프로젝트는 다음 방법으로 PlatformIO 호환성을 달성했습니다:

1. **프레임워크 비활성화**: `framework = stm32cube` 주석 처리로 사전 컴파일 라이브러리 회피
2. **로컬 소스 빌드**: `platformio_extra.py` 스크립트로 모든 HAL 소스를 직접 빌드
3. **링커 플래그 수정**: `platformio_fix_linker.py`로 FPU 플래그를 링커에 명시적 전달
4. **GCC 7.x 호환**: 링커 스크립트에서 `READONLY` 키워드 제거

## 권장 개발 환경

프로젝트는 두 가지 빌드 시스템을 모두 지원합니다:

1. **PlatformIO** - VS Code 통합, 크로스 플랫폼, CI/CD
2. **STM32CubeIDE** - 최신 GCC (13.3), CubeMX 통합, ST 공식 툴체인

## VS Code 설정 (STM32CubeIDE Makefile 사용)

VS Code에서 개발하면서 STM32CubeIDE의 빌드 시스템을 사용하는 방법:

###  1. 설치 필요 확장:
- C/C++ (Microsoft)
- Cortex-Debug
- STM32 VS Code Extension (선택사항)

### 2. tasks.json 생성

`.vscode/tasks.json` 파일:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build STM32",
            "type": "shell",
            "command": "make",
            "args": ["-j4"],
            "options": {
                "cwd": "${workspaceFolder}/Debug"
            },
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Clean STM32",
            "type": "shell",
            "command": "make",
            "args": ["clean"],
            "options": {
                "cwd": "${workspaceFolder}/Debug"
            }
        }
    ]
}
```

### 3. launch.json 생성

`.vscode/launch.json` 파일 (ST-Link 디버깅):

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Cortex Debug",
            "cwd": "${workspaceFolder}",
            "executable": "./Debug/WithForce_1.00.34.elf",
            "request": "launch",
            "type": "cortex-debug",
            "runToEntryPoint": "main",
            "servertype": "stlink",
            "device": "STM32H723VGTx",
            "interface": "swd",
            "serialNumber": "",
            "svdFile": "./STM32H723.svd"
        }
    ]
}
```

### 4. c_cpp_properties.json

`.vscode/c_cpp_properties.json` 파일:

```json
{
    "configurations": [
        {
            "name": "STM32",
            "includePath": [
                "${workspaceFolder}/**",
                "${workspaceFolder}/Core/Inc",
                "${workspaceFolder}/Drivers/STM32H7xx_HAL_Driver/Inc",
                "${workspaceFolder}/Drivers/CMSIS/Device/ST/STM32H7xx/Include",
                "${workspaceFolder}/Drivers/CMSIS/Include"
            ],
            "defines": [
                "USE_HAL_DRIVER",
                "STM32H723xx",
                "DEBUG"
            ],
            "compilerPath": "arm-none-eabi-gcc",
            "cStandard": "c11",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-arm"
        }
    ],
    "version": 4
}
```

### 5. 빌드 및 디버그

- **빌드**: `Ctrl+Shift+B` 또는 Terminal → Run Build Task
- **디버그**: `F5` (Cortex-Debug 확장 필요)
- **정리**: `Ctrl+Shift+P` → Tasks: Run Task → Clean STM32

## 빌드 명령어

### STM32CubeIDE Makefile 사용

```bash
# Debug 빌드
cd Debug
make all

# 병렬 빌드 (더 빠름)
make -j4

# 클린 빌드
make clean
make all

# 빌드 후 프로그래밍 (openocd 필요)
make all
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg -c "program WithForce_1.00.34.elf verify reset exit"
```

### STM32_Programmer_CLI 사용

```bash
# ELF 파일 플래싱
STM32_Programmer_CLI -c port=SWD -w Debug/WithForce_1.00.34.elf -v -rst

# HEX 파일 플래싱
STM32_Programmer_CLI -c port=SWD -w Debug/WithForce_1.00.34.hex 0x08000000 -v -rst
```

## 추가 정보

- 프로젝트 아키텍처 및 개발 가이드: [CLAUDE.md](CLAUDE.md)
- STM32CubeIDE 설정: `.cproject` 및 `.project` 파일 참조
- 링커 스크립트: `STM32H723VGTX_FLASH.ld`

## 문제 해결

### 문제: arm-none-eabi-gcc를 찾을 수 없음

**해결책**: GNU ARM Embedded Toolchain 설치 및 PATH 추가
```bash
# Windows (Chocolatey)
choco install gcc-arm-embedded

# Linux (Ubuntu/Debian)
sudo apt install gcc-arm-none-eabi

# macOS (Homebrew)
brew install gcc-arm-embedded
```

### 문제: ST-Link 드라이버 인식 안됨

**해결책**:
- Windows: ST-Link USB 드라이버 설치 (STM32CubeIDE 포함)
- Linux: udev 규칙 설정 필요

### 문제: OpenOCD 연결 실패

**해결책**: ST-Link 펌웨어 업데이트
```bash
# STM32CubeIDE에서
# Help → ST-Link Upgrade
```

## 기여

PlatformIO 빌드 문제를 해결한 경우, PR을 환영합니다!
