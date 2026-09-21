"""Wire-level integration of both production power state machines.

Uses the sibling ESP32 worktree by default, or pass its path as argv[1].
Models software resets/sleep and transports bytes, not the full HAL/RTOS,
normal application parser, physical buttons or electrical timing.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
esp = Path(sys.argv[1]) if len(sys.argv) > 1 else root.parent / 'psa-esp32-firmware'
comm = (root / 'User/Edit/src/comm_esp.c').read_text()

def function(signature):
    start = comm.index(signature)
    end = comm.index('{', start) + 1
    depth = 1
    while depth:
        depth += (comm[end] == '{') - (comm[end] == '}')
        end += 1
    return comm[start:end]

source = r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include "lowPower.cpp"
namespace stm {
#define ESP_DIR_REQ 0x20
#define ESP_CMD_EVT_POWER 0x85
#define ESP_CMD_STAT 0x70
#define ESP_TX_FMT_BUF_SIZE 96
#define ESP_FMT_SIZE_MIN 6
#define ESP_FMT_CHK_INIT 0xa5
#define ESP_FMT_STX 2
#define ESP_FMT_LEN_MIN 4
#define ESP_FMT_ETX 3
static uint32_t u32_Tim_1msGet() { return ticks; }
static bool b_Uart_ESP_TxIdle() { return true; }
static bool b_Uart_ESP_Out(uint8_t *p, uint16_t n) {
    STMSerial.rx.insert(STMSerial.rx.end(), p, p+n); return true;
}
static void v_ESP_StatProc(uint8_t, uint8_t *, uint8_t) {}
static bool b_ESP_Transmit(uint8_t, uint8_t, uint8_t *, uint16_t);
''' + comm[comm.index('static bool power_active'):comm.index('void v_ESP_Handler(){')] + function(
    'static bool b_ESP_Transmit(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len){'
) + function(
    'static void v_ESP_RxAck(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){'
) + r'''
}
static void receiveAcks() {
    while(STMSerial.tx.size() >= 10) {
        auto &v = STMSerial.tx;
        uint8_t checksum = 0xa5;
        for(unsigned i=0;i<8;++i) checksum ^= v[i];
        assert(v[0]==2 && v[1]==8 && v[2]==2 && v[3]==0x85 &&
               v[8]==checksum && v[9]==3);
        stm::v_ESP_RxAck(v[3], v.data()+4, 4);
        v.erase(v.begin(),v.begin()+10);
    }
    stm::v_ESP_PowerHandler();
}
// Normal UART dispatch surrogate. The minimal OFF parser runs unmodified.
static void normalReceive() {
    while(STMSerial.rx.size() >= 9) {
        if(STMSerial.rx.front()!=2) { STMSerial.rx.pop_front(); continue; }
        uint8_t f[9];
        for(auto &b:f) { b=STMSerial.rx.front(); STMSerial.rx.pop_front(); }
        uint8_t checksum=0xa5;
        for(unsigned i=0;i<7;++i) checksum^=f[i];
        assert(f[1]==7 && f[2]==0x20 && f[3]==0x85 && f[7]==checksum && f[8]==3);
        lowPowerRequest(f+4);
    }
}
static void normalWake() {
    // ESP's software reset restores normal boot; STM keeps retrying ON.
    lowPowerBoot();
    for(unsigned i=0;i<1000 && stm::i_ESP_PowerResult()==0;++i) {
        ++ticks;
        stm::v_ESP_PowerHandler(); normalReceive(); receiveAcks();
    }
    assert(stm::i_ESP_PowerResult()==1 && !stm::power_active);
}
int main() {
    resetReason=0; lowPowerBoot(); // cold battery insertion
    resetReason=ESP_RST_SW;
    deadline=100000;
    onDelay=[] { receiveAcks(); };
    onSleep=[] {
        receiveAcks();
        assert(stm::i_ESP_PowerResult()==1 && stm::power_active);
        stm::v_ESP_PowerBegin(false); // user ON releases simulated STOP
        stm::v_ESP_PowerHandler(); // disposable pulse wakes ESP
    };
    for(unsigned cycle=0;cycle<2;++cycle) {
        stm::v_ESP_PowerBegin(true); stm::v_ESP_PowerHandler();
        try { normalReceive(); assert(false); } catch(Restart &) {}
        assert(retained(OFF_MAGIC));
        try { lowPowerBoot(); assert(false); } catch(Restart &) {}
        normalWake();
    }
    assert(sleeps==2); // cold OFF -> ON -> OFF -> ON

    otaBusy=true;
    stm::v_ESP_PowerBegin(true); stm::v_ESP_PowerHandler();
    normalReceive(); receiveAcks();
    assert(stm::i_ESP_PowerResult()==0 && retained(DEFER_MAGIC));
    stm::v_ESP_PowerBegin(false); // cancel deferred OFF with ON
    normalWake();
    otaBusy=false; lowPowerPoll();
    assert(!offMagic);
    puts("PASS: two-MCU power wire cycle, software resets, deferred OFF cancelled by ON");
}
'''

with tempfile.TemporaryDirectory(prefix='psa-power-cycle-') as directory:
    exe = str(Path(directory) / 'cycle')
    subprocess.run(['g++', '-std=c++17', '-Wall', '-Wextra', '-Wno-narrowing',
                    '-I' + str(esp / 'tests/power_stubs'), '-I' + str(esp / 'include'),
                    '-I' + str(esp / 'src'), '-x', 'c++', '-', '-o', exe],
                   input=source, text=True, check=True)
    subprocess.run([exe], check=True)
