# UART-only OFF / wake integration

## Scope

Both STM32 and ESP32 images must be deployed together. ESP32 target is ESP32-S3,
with STM32 connected to UART2 RX GPIO16 / TX GPIO15 at 115200 baud. No new wire
is required. LTE and server communication stop during OFF, as requested.

Cold STM32 boot starts in product OFF: it leaves 12V disabled, clears the LED
buffer and skips SD mount/log scan. A button ON still resets into normal boot
and performs those initializations. ESP32 cold boot initially starts normally
until it receives the STM32 OFF request.

OFF sends a sleep request, stops sampling/outputs, closes the STM32 SD log,
and suppresses ordinary UART traffic. ESP32 restarts into a minimal OFF service
before application tasks, BLE or Wi-Fi start. It drains old modem traffic until
500 ms quiet (bounded at 2 seconds), sends TYPE1SC `AT+CFUN=0`, and verifies
`AT+CFUN?` returned `+CFUN: 0` followed by OK. A bare OK is not RF confirmation. It
configures the existing RX GPIO as a LOW-level light-sleep wake source, and
acknowledges readiness. After 100 ms of UART silence it enters light sleep.
This is not deep sleep, and CFUN=0 disables modem RF rather than its power rail.

The physical button wakes STM32. On a valid power-on, STM32 sends disposable
zero bytes, waits for TX completion plus 20 ms, then sends a wake request,
retrying after 500 ms without an ACK. ESP32 ignores
partial/noisy frames and restarts into the normal application after a valid
wake request. Its normal UART task acknowledges subsequent retries. That ACK
means UART ready, not LTE registration complete. Existing normal modem startup
resets/re-enables the modem. BLE connections and unsent RAM telemetry do not
survive these software restarts; persistent configuration is retained.

## Protocol

Existing framed XOR-checksum protocol, new event command `0x85`:

| Direction | Data |
| --- | --- |
| STM32 event (`0x20`) | operation, token low byte, token high byte |
| ESP32 ACK (`0x02`) | operation, token low byte, token high byte, status |

Operation 0 requests OFF, 1 requests wake. Status 0 is ready, 1 failure,
2 accepted but busy (OTA or modem/wake preparation). STM32 keeps retrying busy
requests within its original 15-second budget. Token/op/length must match;
legacy empty ACKs and stale ACKs cannot complete the handshake. Checksum starts
at `0xA5` and XORs all bytes including STX, excluding checksum and ETX.

Sleep example token `0x1234`: `02 07 20 85 00 34 12 23 03`.
Ready response: `02 08 02 85 00 34 12 00 0E 03`.

## Failure behavior

- Handshake timeout is 15 seconds; STM32 still enters STOP without ESP32 success.
  Legacy/missing ESP32 firmware therefore does not prevent STM32 shutdown, but
  ESP32 low consumption is NOT guaranteed in this fallback.
- Stalled STM32 TX is aborted after 16 seconds from OFF entry instead of keeping
  STM32 awake forever. Button-held delay remains independent.
- Each modem command has a 5-second timeout. Errors, silence or unconfirmed RF
  state trigger retries with 1, 2, 4, 8, 16, then 30-second capped backoff. There
  is no permanent failure latch: later modem recovery can still reach sleep
  after STM32 has timed out and entered STOP. An ongoing hardware/modem fault
  still prevents verified low-power operation. The UART wake parser remains
  responsive during every modem wait/backoff. Wake-source setup retries at 1s.
- OTA busy retains the latest OFF request in RTC memory, including across an
  OTA software reset. The normal UART task polls it independently of blocking
  LTE setup. Once transfer and image validation are no longer pending, ESP32
  enters the OFF service automatically. A valid ON request cancels the intent.
  Cold power removal clears it. OTA that never finishes can still delay sleep.
- Short button presses re-arm STOP wake on every attempt. A pending interrupt
  that prevents actual STOP entry no longer leaves the software tick suspended.
- Low-power debug retention is disabled by default. `KEEP_DEBUG_IN_STOP=1`
  explicitly enables it for debugging and invalidates low-current measurements.

## Reproducible checks

In the STM32 worktree:

```sh
python3 tests/test_power.py
python3 tests/test_power_cycle.py
pio run -e stm32_release
```

In the sibling `psa-esp32-firmware` worktree:

```sh
g++ -std=c++17 -Wall -Wextra -Itests/power_stubs -Iinclude tests/low_power_test.cpp -o /tmp/psa-low-power-test
/tmp/psa-low-power-test
pio run
```

Host tests exercise production function bodies/state machines with hardware
stubs. They cover wire bytes, stale/empty ACKs, retries, wraparound, timeout,
STOP re-entry, skipped-STOP tick recovery, the button entry race, stalled TX,
cold boot, retained OFF state, OTA guards, noisy/truncated wake frames and
modem/wake-configuration failures. Additional cases cover real 5s silence,
RF-still-on query responses, automatic transient-failure recovery, DMA busy,
enqueue failure, deferred OTA completion/cancellation and ON during modem wait.
The wire-cycle harness connects both production power state machines for two
OFF/ON cycles and OTA cancellation; normal UART dispatch and hardware resets
are simulated. It does NOT execute the full application/HAL/RTOS or prove
electrical wake timing, peripheral recovery or battery-side current.

## Required board validation (not yet performed)

1. Flash both images. Disconnect debugger/USB for battery-current measurements.
2. Verify OFF ACK on a logic analyzer, BLE disappearance, CFUN=0 response,
   and stable battery-side current after STM32 STOP and ESP32 light sleep.
3. Repeat at least 100 OFF/ON cycles; verify sensors, LED, BLE, LTE/server and
   logging resume. Include immediate ON during the OFF handshake.
4. Test short/repeated button presses; verify return to STOP instead of staying
   awake. Test button pressed immediately before WFI.
5. Inject truncated/bad-checksum UART traffic and lose the initial wake bytes;
   verify retry recovery. Measure RX wake timing at 115200 baud on the board.
6. Test absent/legacy ESP32, missing ACK, stalled TX, LTE ERROR/timeout, OTA busy,
   and cold power removal while OFF. Verify cold STM32 starts OFF without a
   12V/green-LED pulse; ESP32 briefly starts normally until the OFF request.
7. Measure ESP32, modem and battery-rail currents separately. Use the measured
   OFF current and actual usable battery capacity for runtime estimates; no
   battery-life or achieved-current claim follows from these software tests.

Reference: [ESP-IDF ESP32-S3 sleep modes](https://docs.espressif.com/projects/esp-idf/en/v4.4.2/esp32s3/api-reference/system/sleep_modes.html).
