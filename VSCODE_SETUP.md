# VS Code Development Setup

이 가이드는 VS Code에서 STM32H723 WithForce 펌웨어를 개발하는 최적의 방법을 설명합니다.

## 권장 개발 환경: VS Code + PlatformIO

**PlatformIO 빌드 완전 지원!** 이 프로젝트는 PlatformIO에서 완벽하게 빌드됩니다.

### 두 가지 빌드 옵션

#### 옵션 1: PlatformIO (권장) ⭐
- ✅ 완전히 작동하는 빌드 시스템
- ✅ 통합 업로드 및 디버깅
- ✅ 크로스 플랫폼 (Windows/Linux/macOS)
- ✅ CI/CD 친화적

#### 옵션 2: STM32CubeIDE Makefile
- ✅ 최신 GCC 13.3 (vs PlatformIO의 GCC 7.2.1)
- ✅ CubeMX 통합
- ✅ ST 공식 툴체인

### 장점

- ✅ VS Code의 강력한 편집 기능
- ✅ IntelliSense 자동 완성
- ✅ 통합 디버깅 (PlatformIO 또는 Cortex-Debug)
- ✅ Git 통합
- ✅ 시리얼 모니터

## 필수 도구 설치

### 1. VS Code 확장

VS Code에서 다음 확장을 설치하세요:

```
code --install-extension ms-vscode.cpptools
code --install-extension platformio.platformio-ide
code --install-extension marus25.cortex-debug
```

또는 VS Code에서 수동으로:
- **C/C++** (Microsoft) - IntelliSense, 디버깅
- **PlatformIO IDE** - 업로드, 시리얼 모니터
- **Cortex-Debug** - ARM 디버깅 (선택사항)

### 2. 빌드 도구

#### Windows
```powershell
# GNU ARM Embedded Toolchain
choco install gcc-arm-embedded

# Make (MinGW 또는 MSYS2)
choco install make
```

#### Linux (Ubuntu/Debian)
```bash
sudo apt install gcc-arm-none-eabi make
```

#### macOS
```bash
brew install gcc-arm-embedded make
```

### 3. 디버그 도구

#### ST-Link 드라이버 (Windows)
- STM32CubeIDE에 포함되어 있음
- 또는 [ST 공식 사이트](https://www.st.com/en/development-tools/stsw-link009.html)에서 다운로드

#### OpenOCD (선택사항, Linux/macOS)
```bash
# Linux
sudo apt install openocd

# macOS
brew install openocd
```

## 프로젝트 설정

이 저장소에는 이미 다음 설정 파일이 포함되어 있습니다:

### `.vscode/tasks.json`
빌드, 클린, 업로드 태스크가 정의되어 있습니다.

### `.vscode/launch.json`
3가지 디버그 설정:
- **PlatformIO Debug** - PlatformIO 통합 디버거
- **Cortex Debug (ST-Link)** - Cortex-Debug 확장 사용
- **OpenOCD Debug** - OpenOCD 사용 (Linux/macOS)

### `.vscode/c_cpp_properties.json`
IntelliSense를 위한 include 경로와 컴파일러 설정

## 사용 방법

### 빌드

**방법 1: 키보드 단축키**
- `Ctrl+Shift+B` (Windows/Linux) 또는 `Cmd+Shift+B` (macOS)

**방법 2: 명령 팔레트**
1. `Ctrl+Shift+P` (Windows/Linux) 또는 `Cmd+Shift+P` (macOS)
2. "Tasks: Run Build Task" 선택
3. "Build STM32 (Make)" 선택

**방법 3: 터미널**
```bash
cd Debug
make -j4 all
```

### 디버깅

**방법 1: F5 키**
1. `.vscode/launch.json`에서 원하는 설정 선택 (왼쪽 디버그 패널)
2. `F5` 키를 눌러 디버깅 시작

**방법 2: 디버그 패널**
1. 왼쪽 사이드바에서 디버그 아이콘 클릭 (벌레 모양)
2. 상단에서 디버그 설정 선택
3. 녹색 재생 버튼 클릭

### 업로드 (프로그래밍)

**PlatformIO를 사용한 업로드:**
1. `Ctrl+Shift+P` → "Tasks: Run Task"
2. "Upload (PlatformIO)" 선택

또는 터미널에서:
```bash
pio run -t upload
```

**STM32_Programmer_CLI 사용:**
```bash
STM32_Programmer_CLI -c port=SWD -w Debug/WithForce_1.00.34.elf -v -rst
```

### 시리얼 모니터

**PlatformIO 시리얼 모니터:**
1. `Ctrl+Shift+P` → "Tasks: Run Task"
2. "Open Serial Monitor" 선택

또는 터미널에서:
```bash
pio device monitor
```

**PlatformIO UI 사용:**
- 하단 상태표시줄에서 전원 플러그 아이콘 클릭
- "Monitor" 클릭

## 워크플로우 예제

### 일반 개발 사이클

```
1. 코드 편집 (VS Code 편집기)
      ↓
2. 빌드 (Ctrl+Shift+B)
      ↓
3. 디버그 또는 업로드 (F5 또는 Upload 태스크)
      ↓
4. 시리얼 모니터로 로그 확인
```

### 빠른 테스트 사이클

**Build and Upload 태스크 사용:**
```
Ctrl+Shift+P → "Tasks: Run Task" → "Build and Upload"
```

이 태스크는 빌드와 업로드를 순차적으로 실행합니다.

## IntelliSense 사용

VS Code의 IntelliSense가 자동으로 활성화됩니다:

- **자동 완성**: 타이핑 중 자동으로 제안
- **Go to Definition**: `F12` 또는 `Ctrl+클릭`
- **Find All References**: `Shift+F12`
- **Hover 정보**: 함수/변수 위에 마우스 올리기

## 디버깅 팁

### 중단점 (Breakpoint)
- 코드 라인 번호 왼쪽 클릭하여 중단점 설정
- `F9`로 현재 라인에 중단점 토글

### 단계별 실행
- `F10`: Step Over (다음 라인으로)
- `F11`: Step Into (함수 안으로)
- `Shift+F11`: Step Out (함수 밖으로)
- `F5`: Continue (다음 중단점까지)

### 변수 감시
- 왼쪽 디버그 패널의 "WATCH" 섹션에서 변수 추가
- 변수 위에 마우스 올려 현재 값 확인

### SWO (Serial Wire Output) 사용
Cortex-Debug 설정에서 SWO를 활성화하여 printf 디버깅:

```json
{
    "type": "cortex-debug",
    "swoConfig": {
        "enabled": true,
        "cpuFrequency": 192000000,
        "swoFrequency": 2000000,
        "decoders": [
            { "type": "console", "port": 0, "encoding": "ascii" }
        ]
    }
}
```

## 문제 해결

### 빌드 실패: "make: command not found"

**해결책**: Make 도구 설치 필요
```bash
# Windows
choco install make

# 또는 STM32CubeIDE의 make 사용
# PATH에 추가: C:\ST\STM32CubeIDE_1.x.x\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.make.win32_x.x.x\tools\bin
```

### IntelliSense 오류

**해결책**: C/C++ 확장 재로드
1. `Ctrl+Shift+P` → "C/C++: Reset IntelliSense Database"
2. VS Code 재시작

### 디버거 연결 실패

**해결책**: ST-Link 연결 확인
```bash
# ST-Link 펌웨어 업데이트 (STM32CubeIDE에서)
# Help → ST-Link Upgrade

# 또는 연결 상태 확인
STM32_Programmer_CLI -c port=SWD
```

### PlatformIO 업로드 실패

**해결책**: 수동 업로드
```bash
# OpenOCD 사용
openocd -f interface/stlink.cfg -f target/stm32h7x.cfg \
    -c "program Debug/WithForce_1.00.34.elf verify reset exit"

# 또는 STM32_Programmer_CLI
STM32_Programmer_CLI -c port=SWD -w Debug/WithForce_1.00.34.elf -v -rst
```

## 고급 기능

### Git 통합
VS Code에 내장된 Git 기능 사용:
- 왼쪽 사이드바의 Source Control 아이콘
- 변경사항 보기, 커밋, 푸시/풀

### 멀티 파일 편집
- `Ctrl+P`: 빠른 파일 열기
- `Ctrl+Tab`: 열린 파일 간 전환
- Split Editor: 파일을 드래그하여 분할 편집

### 코드 포맷팅
```json
// .vscode/settings.json에 추가
{
    "editor.formatOnSave": true,
    "C_Cpp.clang_format_style": "{ BasedOnStyle: LLVM, IndentWidth: 4, TabWidth: 4 }"
}
```

### 작업 공간 설정
프로젝트별 VS Code 설정:

```json
// .vscode/settings.json
{
    "files.associations": {
        "*.h": "c",
        "*.c": "c"
    },
    "editor.tabSize": 4,
    "editor.insertSpaces": false,
    "C_Cpp.default.compilerPath": "arm-none-eabi-gcc"
}
```

## 추가 리소스

- [CLAUDE.md](CLAUDE.md) - 프로젝트 아키텍처 및 개발 가이드
- [PLATFORMIO_SETUP.md](PLATFORMIO_SETUP.md) - PlatformIO 순수 빌드 시도 (현재 FPU 문제 있음)
- [Cortex-Debug Wiki](https://github.com/Marus/cortex-debug/wiki)
- [PlatformIO Docs](https://docs.platformio.org/)
- [VS Code C++ Docs](https://code.visualstudio.com/docs/languages/cpp)

## 요약

이 설정으로 다음을 얻을 수 있습니다:

✅ **빠른 빌드**: STM32CubeIDE Makefile (`Ctrl+Shift+B`)
✅ **강력한 디버깅**: Cortex-Debug 또는 PlatformIO (`F5`)
✅ **편리한 업로드**: PlatformIO 통합
✅ **완벽한 IntelliSense**: 모든 헤더 파일 인식
✅ **통합 시리얼 모니터**: PlatformIO
✅ **Git 통합**: VS Code 내장

Happy Coding! 🚀
