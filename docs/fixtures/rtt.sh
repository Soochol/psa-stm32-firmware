#!/usr/bin/env bash
# RTT viewer and probe diagnostics over OpenOCD, for the hardware sessions in
# CAPTURE.md and TESTPLAN.md.
#
#   ./rtt.sh check     probe and target voltage only, no attach
#   ./rtt.sh           attach and stream RTT
#   ./rtt.sh reset     same, but connect while holding nRST
#   ./rtt.sh dap       diagnose over DAP-direct instead of ST-Link HLA
#
# Uses the OpenOCD that PlatformIO already ships, so nothing extra is installed.
set -u

OCD_ROOT="${OCD_ROOT:-$HOME/.platformio/packages/tool-openocd}"
OCD="$OCD_ROOT/bin/openocd"
SCRIPTS="$OCD_ROOT/openocd/scripts"
PORT="${RTT_PORT:-9090}"

# RAM_D1 (see STM32H723VGTX_FLASH.ld). The RTT control block lives in .bss, and
# its address moves whenever the firmware is rebuilt, so scan the region rather
# than pinning an address that goes stale.
RAM_BASE=0x24000000
RAM_SIZE=0x50000

[ -x "$OCD" ] || { echo "openocd not found at $OCD"; echo "set OCD_ROOT to override"; exit 1; }

# A previous run left in the background still owns the probe, and OpenOCD's
# complaint about that is easy to miss among its start-up chatter.
if pgrep -x openocd >/dev/null 2>&1; then
    echo "!! openocd is already running and holds the probe:"
    pgrep -a openocd | sed 's/^/   /'
    echo "   kill it first:  pkill openocd"
    exit 1
fi

# DAP-direct, not the ST-Link HLA transport that stlink.cfg selects by default.
# On this board HLA reports "Cortex-M PARTNO 0x0" and never examines the target,
# while DAP-direct reads DPIDR 0x6ba02477 and detects the Cortex-M7 immediately.
# HLA leaves the SWD sequencing to the ST-Link firmware; DAP-direct drives it
# from OpenOCD, and only the latter gets this H723 to answer.
# Requires ST-Link V2 firmware J28+ (this probe is V2J46).
CFG=(-s "$SCRIPTS" -f interface/stlink-dap.cfg
     -c "transport select dapdirect_swd"
     -f target/stm32h7x.cfg)

case "${1:-attach}" in
dap)
    # hla_swd hands SWD to the ST-Link firmware, which reports failures as one
    # opaque status. DAP-direct drives SWD itself, so a bad line shows up as the
    # specific transaction that failed rather than "PARTNO 0x0".
    # Needs ST-Link V2 firmware J28 or newer; this probe reports V2J46.
    echo "== DAP-direct probe of the debug port =="
    "$OCD" "${CFG[@]}" -c "init" -c "dap info" -c "exit" 2>&1
    echo "(openocd exit: $?)"
    echo
    echo "IDCODE / DPIDR readable -> SWD wiring is good, the core is the problem"
    echo "nothing readable        -> SWDIO / SWCLK / GND"
    ;;
check)
    # Prints "Target voltage:" even when the connection fails, which is the one
    # reading that separates an unpowered board from a wiring or protocol fault.
    echo "== probe and target voltage =="
    "$OCD" "${CFG[@]}" -c "init" -c "exit" 2>&1 | grep -Ei "voltage|stlink|serial|error|clock" || true
    echo
    echo "0.0 V        -> target has no power (the probe's VCC pin only senses)"
    echo "3.2 - 3.4 V  -> powered; check SWDIO / SWCLK / GND, then try: $0 reset"
    ;;
attach|reset)
    EXTRA=()
    if [ "${1:-}" = "reset" ]; then
        # connect_assert_srst is the part that actually holds nRST through the
        # attach. srst_only alone leaves connect_deassert_srst in effect, which
        # releases reset first and lets the firmware run away before we arrive —
        # on this device that means straight back into STOP mode.
        EXTRA=(-c "reset_config srst_only srst_nogate connect_assert_srst"
               -c "adapter speed 480")
    fi

    echo "== streaming RTT on port $PORT (ctrl-c to stop) =="
    "$OCD" "${CFG[@]}" "${EXTRA[@]}" \
        -c "init" \
        -c "rtt setup $RAM_BASE $RAM_SIZE \"SEGGER RTT\"" \
        -c "rtt start" \
        -c "rtt server start $PORT 0" &
    OCD_PID=$!
    trap 'kill $OCD_PID 2>/dev/null' EXIT INT TERM

    # Give OpenOCD time to find the control block before the reader connects.
    sleep 2
    if command -v nc >/dev/null; then
        nc localhost "$PORT"
    else
        echo "nc not found; connect to localhost:$PORT yourself"
        wait $OCD_PID
    fi
    ;;
*)
    sed -n '2,9p' "$0" | sed 's/^# \{0,1\}//'
    exit 2
    ;;
esac
