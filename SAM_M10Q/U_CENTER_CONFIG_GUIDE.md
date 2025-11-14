# GPS I2C Configuration Guide - u-center Method

**목적**: UART로 연결 불가능한 경우, PC를 통해 GPS를 I2C 모드로 설정
**상태**: GPS가 I2C NAK 응답 (ErrorCode=0x00000004)
**해결**: u-center를 통한 수동 설정

---

## 사전 준비물

### 필수 하드웨어
1. **SAM-M10Q GPS 모듈** (STM32에서 분리)
2. **USB-to-UART 어댑터** (CP2102, FT232, CH340 등)
   - 3.3V 레벨 필수! (5V는 GPS 손상 가능)
3. **점퍼 와이어** 4개

### 필수 소프트웨어
- **u-blox u-center** (Windows 전용)
  - 다운로드: https://www.u-blox.com/en/product/u-center
  - 최신 버전: u-center 2 또는 u-center 21.02 이상

---

## 1단계: GPS를 PC에 연결

### 하드웨어 연결 (STM32에서 GPS 분리 후)

```
GPS 모듈        USB-UART 어댑터
─────────      ───────────────
TX    ──────→  RX
RX    ──────→  TX
GND   ──────→  GND
3.3V  ──────→  3.3V (또는 5V → 3.3V 레귤레이터)
```

**중요 사항**:
- GPS TX → 어댑터 RX (교차 연결)
- GPS RX → 어댑터 TX (교차 연결)
- 전원은 반드시 3.3V! (5V 직결 금지)
- GND는 반드시 연결

### 보드율 확인

SAM-M10Q 공장 기본 보드율:
- **9600 baud** (가장 일반적)
- 38400 baud (일부 펌웨어)
- 115200 baud (드물게)

**모르는 경우**: u-center에서 자동 감지 사용

---

## 2단계: u-center 실행 및 연결

### 2-1. u-center 실행

1. u-center 프로그램 실행
2. 메뉴: **Receiver → Port** 선택
3. USB-UART 어댑터의 COM 포트 선택 (예: COM3, COM7 등)
4. **Connect** 클릭

### 2-2. 보드율 자동 감지 (연결 실패 시)

1. 메뉴: **Receiver → Baudrate** 선택
2. **Autobaudrate** 클릭
3. u-center가 자동으로 보드율 찾음

### 2-3. 연결 확인

화면 우측 하단 "Packet Console"에 메시지가 나타나면 연결 성공:
```
UBX-NAV-PVT (Position, Velocity, Time)
NMEA-GGA (GPS Fix Data)
NMEA-GSA (Satellite Status)
```

만약 아무 메시지도 안 나오면:
- TX/RX 핀 교차 확인
- 보드율 다시 설정
- 전원 연결 확인

---

## 3단계: I2C 모드 설정

### 방법 A: Configuration File 로드 (권장)

1. **파일 준비**: `GPS_I2C_CONFIG.txt` 파일 (같은 폴더에 있음)

2. **u-center에서 로드**:
   - 메뉴: **Tools → Receiver Configuration**
   - **File** 탭 선택
   - **Load Configuration** 클릭
   - `GPS_I2C_CONFIG.txt` 선택
   - **OK** 클릭

3. **적용 확인**:
   - "Configuration loaded successfully" 메시지 확인
   - GPS가 자동으로 재부팅됨

---

### 방법 B: 수동 설정 (Configuration View 사용)

더 세밀한 제어가 필요한 경우:

#### Step 1: I2C 포트 활성화

1. 메뉴: **View → Configuration View** (또는 F9)
2. 좌측 목록에서 **PRT (Ports)** 선택
3. 설정:
   - **Target**: "0 - DDC (I2C)" 선택
   - **Protocol in**: "0+1 - UBX+NMEA" 또는 "1 - UBX"
   - **Protocol out**: "1 - UBX" (권장) 또는 "0+1 - UBX+NMEA"
   - **I2C Slave Address**: **0x42** (decimal 66)
4. **Send** 버튼 클릭 (화면 하단)

#### Step 2: UART 포트 비활성화 (선택사항)

1. **Target**: "1 - UART" 선택
2. 설정:
   - **Protocol in**: "3 - NONE"
   - **Protocol out**: "3 - NONE"
3. **Send** 버튼 클릭

#### Step 3: NAV-PVT 메시지 활성화 (테스트용)

1. 좌측 목록에서 **MSG (Messages)** 선택
2. 설정:
   - **Message**: "01 07 - NAV-PVT"
   - **Rate**: "1" (every solution)
3. **Send** 버튼 클릭

#### Step 4: 업데이트 주기 설정

1. 좌측 목록에서 **RATE (Rates)** 선택
2. 설정:
   - **Measurement Period**: "1000" ms (1 Hz)
   - **Navigation Rate**: "1" (every measurement)
3. **Send** 버튼 클릭

#### Step 5: 설정 저장 (중요!)

1. 좌측 목록에서 **CFG (Configuration)** 선택
2. 설정:
   - **Save current configuration** 선택
   - **Devices**: 모두 체크 (BBR, Flash, EEPROM)
3. **Send** 버튼 클릭

"Configuration saved" 메시지 확인!

---

## 4단계: GPS 재부팅 및 확인

### 재부팅

1. 메뉴: **Receiver → Action → Cold Start**
2. 또는 GPS 전원 재인가 (USB 분리 후 재연결)

### I2C 설정 확인

1. u-center에서 **View → Configuration View**
2. **PRT (Ports)** → Target "0 - DDC (I2C)" 선택
3. **Poll** 버튼 클릭
4. 확인 사항:
   - I2C Slave Address = 0x42
   - Protocol in = UBX (또는 UBX+NMEA)
   - Protocol out = UBX

### 설정 영구 저장 확인

1. **CFG (Configuration)** 선택
2. **Poll** 버튼 클릭
3. "Configuration saved to Flash" 확인

---

## 5단계: STM32에 재연결 및 테스트

### GPS를 STM32 I2C3에 연결

```
GPS 모듈        STM32H723
─────────      ───────────
SDA   ──────→  I2C3_SDA (예: PH8)
SCL   ──────→  I2C3_SCL (예: PH7)
GND   ──────→  GND
3.3V  ──────→  3.3V
```

### STM32 펌웨어 실행

1. STM32 펌웨어 플래시:
   ```bash
   pio run -t upload
   ```

2. 시리얼 모니터 실행:
   ```bash
   pio device monitor -b 115200
   ```

### 예상 로그 (성공 시)

```
*** GPS INIT START (FW v2025-01-14 + FIX v3) ***
GPS: I2C3 State before reset=32 (0x20=READY)
GPS: I2C3 State after reset=32
GPS: I2C3 reset complete - FIFO cleared
GPS: Driver initialized (I2C3, addr=0x42)
GPS: Sending CFG-PRT (28 bytes, NMEA=OFF)
GPS: I2C3 State before CFG-PRT=32 (0x20=READY)
GPS: CFG-PRT sent OK (UBX-only mode configured)  ← I2C 통신 성공!
GPS: Sending CFG-SAVE (21 bytes, mask=0x1F)
GPS: CFG-SAVE sent OK - Config saved to Flash!
GPS: Init complete - handler will start polling

GPS State: IDLE
GPS State: CHECK_AVAIL
GPS State: WAIT_AVAIL
GPS: Available bytes=98
GPS State: READ_DATA
GPS: Reading 98 bytes from stream...
GPS State: WAIT_DATA
GPS: Received 98 bytes: B5 62 01 07 5C 00 ...
GPS State: PARSE
GPS: NAV-PVT parsed (Fix=3, Sats=8)
GPS: Lat=37.123456 Lon=127.123456 Alt=50.0m Sats=8 Fix=3
GPS State: IDLE
```

**핵심 확인 사항**:
- ✅ `CFG-PRT sent OK` (더 이상 ACK Failure 없음!)
- ✅ `Available bytes=98` (I2C 데이터 읽기 성공)
- ✅ `NAV-PVT parsed` (GPS 데이터 파싱 성공)

---

## 문제 해결

### 문제 1: u-center가 GPS와 연결 안됨

**증상**: Packet Console에 아무 메시지도 없음

**해결**:
1. TX/RX 핀 교차 확인 (GPS TX → 어댑터 RX)
2. 보드율 자동 감지 (Autobaudrate)
3. 다른 COM 포트 시도
4. GPS 전원 확인 (3.3V, GND 연결)
5. 어댑터 드라이버 재설치

### 문제 2: "Configuration failed" 에러

**원인**: 일부 명령이 GPS에서 거부됨

**해결**:
1. GPS 재부팅 (전원 재인가)
2. 명령을 하나씩 개별 전송
3. 각 명령 후 2-3초 대기
4. ACK 수신 확인 후 다음 명령 전송

### 문제 3: 설정 후에도 STM32에서 NAK 발생

**원인**: 설정이 Flash에 저장 안됨

**해결**:
1. u-center에서 **CFG-CFG → Save** 다시 실행
2. **Devices: Flash** 체크 확인
3. GPS Cold Start 실행
4. STM32 재부팅

### 문제 4: I2C 주소 0x42에서 응답 없음

**원인**: UART 모드로 돌아감

**해결**:
1. u-center 재연결
2. **PRT (Ports)** Poll로 현재 설정 확인
3. I2C 모드 다시 설정 + Save

---

## 설정 백업 (선택사항)

### 현재 설정 저장

u-center에서 설정을 파일로 저장:
1. 메뉴: **Tools → Receiver Configuration → File**
2. **Store current configuration** 선택
3. 파일명: `SAM_M10Q_I2C_Backup.txt`
4. 저장 위치 지정
5. **OK** 클릭

### 백업 파일 복원

1. u-center 연결
2. **Tools → Receiver Configuration → File**
3. **Load Configuration** 선택
4. `SAM_M10Q_I2C_Backup.txt` 선택
5. **OK** 클릭

---

## 설정 내용 요약

| 설정 항목 | 값 | 설명 |
|----------|-----|------|
| **I2C 포트** | Enabled | Port 0 (DDC) 활성화 |
| **I2C 주소** | 0x42 (7-bit) | STM32에서 0x84로 읽음 (8-bit) |
| **프로토콜 입력** | UBX only | NMEA 비활성화 (선택) |
| **프로토콜 출력** | UBX only | 바이너리 데이터만 출력 |
| **UART 포트** | Disabled (선택) | I2C만 사용 시 UART 끔 |
| **업데이트 주기** | 1000ms (1 Hz) | 초당 1회 위치 업데이트 |
| **저장 위치** | Flash + BBR | 전원 꺼도 유지 |

---

## 검증 체크리스트

### u-center 설정 단계
- [ ] USB-UART 어댑터 연결 완료
- [ ] u-center COM 포트 연결 성공
- [ ] Packet Console에 NMEA/UBX 메시지 확인
- [ ] I2C 포트 (Port 0) 설정 완료
- [ ] I2C 주소 0x42 설정 확인
- [ ] CFG-CFG Save 실행 완료
- [ ] GPS Cold Start 실행
- [ ] Poll로 설정 영구 저장 확인

### STM32 테스트 단계
- [ ] GPS를 STM32 I2C3에 재연결
- [ ] 펌웨어 플래시 완료
- [ ] 시리얼 모니터 연결
- [ ] "CFG-PRT sent OK" 로그 확인
- [ ] "Available bytes" > 0 확인
- [ ] "NAV-PVT parsed" 로그 확인
- [ ] 위도/경도 데이터 수신 확인
- [ ] GPS Fix 획득 (Fix=2 or 3)

---

## 참고 자료

### u-blox 공식 문서
- **u-center User Guide**: https://www.u-blox.com/en/docs/UBX-13005250
- **SAM-M10Q Interface Description**: https://www.u-blox.com/en/docs/UBX-21035062
- **u-blox Protocol Specification**: https://www.u-blox.com/en/docs/UBX-18010854

### 프로젝트 문서
- [GPS_I2C_MODE_ISSUE.md](GPS_I2C_MODE_ISSUE.md) - 원래 NAK 문제 분석
- [GPS_I2C_FIX_IMPLEMENTED.md](GPS_I2C_FIX_IMPLEMENTED.md) - I2C3 최적화 내용
- [GPS_I2C_CONFIG.txt](GPS_I2C_CONFIG.txt) - u-center 로드용 설정 파일

---

## 다음 단계 (설정 완료 후)

### 1. GPS 기능 테스트
- 실외에서 GPS Fix 획득 (최대 30초 소요)
- 위치 정확도 확인 (±10m 이내)
- 지속적인 데이터 수신 확인 (1 Hz)

### 2. 통합 테스트
- 다른 I2C 센서와 동시 동작 확인
- 장시간 안정성 테스트 (1시간 이상)
- 전원 재인가 후 I2C 모드 유지 확인

### 3. 코드 최적화 (선택)
- 불필요한 I2C3 reset 코드 제거
- GPS 타임아웃 recovery 코드는 유지 (방어 코드)
- 로그 레벨 조정 (상용화 시 로그 축소)

---

**상태**: ✅ u-center 설정 가이드 완료
**다음**: GPS를 PC에 연결하여 u-center로 I2C 모드 설정!

**중요**: 설정은 GPS 플래시에 영구 저장되므로, **한 번만 실행하면 됨**!
