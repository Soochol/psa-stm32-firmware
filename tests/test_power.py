"""Host regression tests execute production functions with hardware stubs.

Run: python3 tests/test_power.py. No target board required. This verifies control
flow/wire protocol, not physical STOP current or GPIO wake timing.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

def run(source):
    with tempfile.TemporaryDirectory(prefix='psa-power-') as directory:
        exe = str(Path(directory) / 'test')
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wno-unused-parameter',
                        '-x', 'c', '-', '-o', exe], input=source, text=True, check=True)
        subprocess.run([exe], check=True)

comm = (ROOT / 'User/Edit/src/comm_esp.c').read_text()
globals_code = comm[comm.index('static bool power_active'):comm.index('void v_ESP_Handler(){')]
run(r'''
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define ESP_DIR_REQ 0x20
#define ESP_CMD_EVT_POWER 0x85
#define ESP_CMD_STAT 0x70
#define ESP_TX_FMT_BUF_SIZE 96
#define ESP_FMT_SIZE_MIN 6
#define ESP_FMT_CHK_INIT 0xa5
#define ESP_FMT_STX 2
#define ESP_FMT_LEN_MIN 4
#define ESP_FMT_ETX 3
static uint32_t now;
static unsigned sends;
static uint8_t wire[96];
static uint16_t wire_len;
static bool tx_idle=true, enqueue_ok=true;
static bool b_Uart_ESP_TxIdle(void) { return tx_idle; }
static uint32_t u32_Tim_1msGet(void) { return now; }
static void v_ESP_StatProc(uint8_t c, uint8_t *d, uint8_t n) {}
static bool b_Uart_ESP_Out(uint8_t *p, uint16_t n) {
    if(!enqueue_ok) return false;
    ++sends; wire_len=n; memcpy(wire,p,n); return true;
}
static bool b_ESP_Transmit(uint8_t, uint8_t, uint8_t *, uint16_t);
'''
    + globals_code
    + function(comm, 'static bool b_ESP_Transmit(uint8_t u8_dir, uint8_t u8_cmd, uint8_t* pu8_data, uint16_t u16_len){')
    + function(comm, 'static void v_ESP_RxAck(uint8_t u8_cmd, uint8_t* pu8_data, uint8_t u8_len){')
    + r'''
int main(void) {
    v_ESP_PowerBegin(true); v_ESP_PowerHandler();
    const uint8_t expected[]={2,7,0x20,0x85,0,1,0,4,3};
    assert(wire_len==9 && memcmp(wire,expected,9)==0 && sends==1);
    assert(!b_ESP_Transmit(0x20,0x82,NULL,0));
    uint8_t ack[]={0,0,0,0};
    v_ESP_RxAck(0x85,ack,4); v_ESP_PowerHandler();
    assert(i_ESP_PowerResult()==0); // stale token
    ack[1]=1;
    v_ESP_RxAck(0x85,ack,0); assert(!power_acked); // legacy empty ACK
    v_ESP_RxAck(0x85,ack,4); v_ESP_PowerHandler();
    assert(i_ESP_PowerResult()==1 && power_active); // stay silent in OFF
    now=UINT32_MAX-100;
    v_ESP_PowerBegin(false); v_ESP_PowerHandler();
    assert(sends==2 && wire_len==16); // pulse alone, no premature frame
    tx_idle=false;
    now+=40; v_ESP_PowerHandler(); assert(sends==2); // DMA still busy
    tx_idle=true;
    v_ESP_PowerHandler(); // pulse drained; start 20 ms recovery interval
    now+=19; v_ESP_PowerHandler(); assert(sends==2);
    now+=1; v_ESP_PowerHandler();
    assert(sends==3); // disposable pulse + valid request
    now+=499; v_ESP_PowerHandler(); assert(sends==3);
    now+=1; v_ESP_PowerHandler(); assert(sends==4); // wrap-safe retry pulse
    v_ESP_PowerHandler(); now+=20; v_ESP_PowerHandler(); assert(sends==5);
    now+=14500; v_ESP_PowerHandler();
    assert(i_ESP_PowerResult()==-1 && !power_active); // bounded legacy fallback
    v_ESP_PowerBegin(false);
    uint8_t wake[]={1,3,0,0};
    v_ESP_RxAck(0x85,wake,4); v_ESP_PowerHandler();
    assert(i_ESP_PowerResult()==1 && !power_active);
    v_ESP_PowerBegin(true);
    uint8_t busy[]={0,4,0,2};
    v_ESP_RxAck(0x85,busy,4);
    assert(i_ESP_PowerResult()==0 && power_active); // BUSY is deferred, not failure
    now+=500; v_ESP_PowerHandler();
    busy[3]=0; v_ESP_RxAck(0x85,busy,4); v_ESP_PowerHandler();
    assert(i_ESP_PowerResult()==1 && power_active);

    v_ESP_PowerBegin(true); // OFF -> ON while OFF is still preparing
    v_ESP_PowerBegin(false);
    uint8_t oldAck[]={0,5,0,0};
    v_ESP_RxAck(0x85,oldAck,4); assert(!power_acked);
    unsigned before=sends;
    enqueue_ok=false; v_ESP_PowerHandler();
    assert(sends==before && power_pulse==POWER_PULSE_IDLE);
    enqueue_ok=true; v_ESP_PowerHandler();
    assert(sends==before+1 && power_pulse==POWER_PULSE_DRAIN);
    uint8_t onAck[]={1,6,0,0};
    v_ESP_RxAck(0x85,onAck,4); v_ESP_PowerHandler();
    assert(i_ESP_PowerResult()==1 && !power_active);
    puts("PASS: STM wire, stale/empty ACK, wake burst/retry, wraparound, timeout, ACK/refusal");
}
''')

mode = (ROOT / 'User/Edit/src/mode.c').read_text()
run(r'''
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
typedef int e_modeID_t;
typedef struct { struct { int e_curr; } guide;
 struct { struct { int b1_upd,b1_on,b1_off; } bit; } cr; } x_modeWORK_t;
typedef struct { uint32_t u32_timToutRef,u32_timLedRef; } x_modePUB_t;
static uint32_t now, mask;
static int i_mode_off, armed, suspended, stops, clock_restores, real_stop=1, button=1;
static int scb; struct { uint32_t ICSR; } registers;
#define SCB (&registers)
#define SCB_ICSR_PENDSTCLR_Msk 1
#define MODE_POWEROFF_DELAY 100
#define MODE_LOWPWR_ENTRY_DELAY 1000
#define _keyPRESSED 1
#define GPIO_PIN_RESET 0
#define PWR_LOWPOWERREGULATOR_ON 0
#define PWR_STOPENTRY_WFI 0
static uint32_t u32_Tim_1msGet(void) { return now; }
#define _b_Tim_Is_OVR(n,r,d) ((uint32_t)((n)-(r)) > (d))
#define NOOP(...) ((void)0)
#define v_Mode_Clear_PwrSW NOOP
#define v_RGB_Disable_Duty NOOP
#define v_RGB_Clear NOOP
#define v_TIM2_Ch1_Out NOOP
#define v_TIM2_Ch2_Out NOOP
#define v_TIM2_Ch3_Out NOOP
#define v_TIM2_Ch4_Out NOOP
#define i_MP3_ForceStop NOOP
#define v_ESP_Send_EvtModeChange NOOP
#define v_SD_Log_Close NOOP
#define v_SD_Log_Backfill_Abort NOOP
#define v_ESP_PowerBegin NOOP
#define v_IO_Disable_12V NOOP
#define HAL_ADC_Stop_DMA NOOP
#define v_I2C_Deinit NOOP
#define v_AUDIO_Init NOOP
#define v_I2C1_Pin_Deinit NOOP
#define v_I2C2_Pin_Deinit NOOP
#define v_I2C3_Pin_Deinit NOOP
#define v_I2C4_Pin_Deinit NOOP
#define v_I2C5_Pin_Deinit NOOP
#define HAL_GPIO_WritePin NOOP
#define HAL_TIM_Base_Stop_IT NOOP
#define HAL_TIM_PWM_Stop_DMA NOOP
#define HAL_NVIC_ClearPendingIRQ NOOP
#define HAL_TIM_Base_Start_IT NOOP
#define v_Key_Power_Init NOOP
#define v_Mode_MoveNext NOOP
#define e_Key_Read_PWR() 0
#define i_ESP_PowerResult() 1
#define b_Uart_ESP_TxIdle() tx_idle
static int tx_idle=1, aborts=0;
#define v_Uart_ESP_AbortTx() (++aborts)
#define HAL_GPIO_ReadPin(...) button
#define __get_PRIMASK() mask
#define __disable_irq() (mask=1)
#define __set_PRIMASK(p) (mask=(p))
#define v_IO_PWR_WakeUp_Enable() (armed=1)
#define v_IO_PWR_WakeUp_Disable() (armed=0)
#define HAL_SuspendTick() (suspended=1)
#define HAL_ResumeTick() (suspended=0)
#define __HAL_PWR_CLEAR_FLAG(...) (scb=0)
#define __HAL_PWR_GET_FLAG(...) scb
#define v_WakeUp_Clock_Config() (++clock_restores)
static void HAL_PWR_EnterSTOPMode(int r,int e) {
    assert(armed && mask && suspended); ++stops; scb=real_stop;
}
''' + function(mode, 'void v_Mode_Off(e_modeID_t e_id, x_modeWORK_t* px_work, x_modePUB_t* px_pub){') + r'''
int main(void) {
    x_modeWORK_t work={0}; x_modePUB_t pub={0};
    work.cr.bit.b1_upd=work.cr.bit.b1_on=1;
    v_Mode_Off(0,&work,&pub);
    now=101; v_Mode_Off(0,&work,&pub);
    now=1101; v_Mode_Off(0,&work,&pub);
    assert(stops==1 && !armed && !suspended && !mask && clock_restores==1);
    now+=1100; v_Mode_Off(0,&work,&pub); // second STOP must re-arm
    assert(stops==2 && clock_restores==2);
    real_stop=0;
    now+=1100; v_Mode_Off(0,&work,&pub); // WFI returns without STOPF
    assert(stops==3 && !suspended && !mask && clock_restores==2);
    button=0;
    now+=1100; v_Mode_Off(0,&work,&pub); // raw press wins entry race
    assert(stops==3 && !mask);
    button=1; tx_idle=0;
    now+=1100; v_Mode_Off(0,&work,&pub);
    assert(stops==3 && aborts==0);
    now=17000; v_Mode_Off(0,&work,&pub);
    assert(stops==4 && aborts==1);
    puts("PASS: STOP re-entry, tick recovery, button race, bounded TX drain");
}
''')
