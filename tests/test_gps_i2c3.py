"""Host tests for GPS init/runtime over I2C3 with a missing or flaky module.

Run: python3 tests/test_gps_i2c3.py. No target board required.

The production sources run unmodified: User/Drv/src/i2c.c (included whole),
SAM_M10Q/platform/src/sam_m10q_platform.c and the SAM_M10Q core driver. Only the
STM32 HAL is replaced by a stub I2C3 bus. The stub records every HAL_I2C_Mem_*_IT
call and answers it through the same callbacks the HAL ISR uses: completion, NACK
error, or silence. Time is the stub u32_Tim_1msGet(), advanced 1 ms per main-loop
pass (v_GPS_Handler then v_GPS_Tout_Handler, as in Core/Src/main.c).
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]

MAIN_H = r'''
#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
typedef enum {HAL_OK=0,HAL_ERROR,HAL_BUSY,HAL_TIMEOUT} HAL_StatusTypeDef;
typedef enum {HAL_I2C_STATE_RESET=0,HAL_I2C_STATE_READY=0x20,HAL_I2C_STATE_BUSY_TX=0x21,
              HAL_I2C_STATE_BUSY_RX=0x22,HAL_I2C_STATE_ABORT=0x60} HAL_I2C_StateTypeDef;
typedef struct {uint32_t Timing;} I2C_InitTypeDef;
typedef struct {HAL_I2C_StateTypeDef State; I2C_InitTypeDef Init;} I2C_HandleTypeDef;
typedef struct {uint32_t Pin, Mode, Pull, Speed;} GPIO_InitTypeDef;
#define I2C3_EV_IRQn 1
#define I2C3_ER_IRQn 2
#define I2C_MEMADD_SIZE_8BIT 1u
#define I2C_MEMADD_SIZE_16BIT 2u
#define HAL_MAX_DELAY 0xFFFFFFFFu
#define I2C_ANALOGFILTER_ENABLE 0u
#define GPIO_MODE_OUTPUT_OD 1u
#define GPIO_MODE_OUTPUT_PP 2u
#define GPIO_NOPULL 0u
#define GPIO_SPEED_FREQ_LOW 0u
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
#define __HAL_RCC_GPIOC_CLK_ENABLE() ((void)0)
#define I2C1_SCL_ACT_Pin 1u
#define I2C1_SDA_ACT_Pin 2u
#define I2C2_SCL_SUB_Pin 4u
#define I2C2_SDA_SUB_Pin 8u
#define I2C4_SCL_ACT_Pin 16u
#define I2C4_SDA_ACT_Pin 32u
#define I2C5_SCL_AUDIO_Pin 64u
#define I2C5_SDA_AUDIO_Pin 128u
#define I2C1_SCL_ACT_GPIO_Port ((void*)1)
#define I2C1_SDA_ACT_GPIO_Port ((void*)1)
#define I2C2_SCL_SUB_GPIO_Port ((void*)1)
#define I2C2_SDA_SUB_GPIO_Port ((void*)1)
#define I2C4_SCL_ACT_GPIO_Port ((void*)1)
#define I2C4_SDA_ACT_GPIO_Port ((void*)1)
#define I2C5_SCL_AUDIO_GPIO_Port ((void*)1)
#define I2C5_SDA_AUDIO_GPIO_Port ((void*)1)
void HAL_NVIC_SetPriority(int,uint32_t,uint32_t);
void HAL_NVIC_EnableIRQ(int);
void HAL_GPIO_Init(void*,GPIO_InitTypeDef*);
void HAL_GPIO_WritePin(void*,uint32_t,int);
int HAL_GPIO_ReadPin(void*,uint32_t);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef*);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef*);
HAL_StatusTypeDef HAL_I2CEx_ConfigAnalogFilter(I2C_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_I2CEx_ConfigDigitalFilter(I2C_HandleTypeDef*,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read_DMA(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write_DMA(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef*,uint16_t,uint16_t,uint16_t,uint8_t*,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef*,uint16_t);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef*);
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef*);
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef*);
void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef*);
#endif
'''

HARNESS = r'''
#include "main.h"
#include "i2c.h"
#include "i2c4_diag.h"
#include "sam_m10q_platform.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

uint32_t now = 1000;
uint32_t u32_Tim_1msGet(void) { return now; }
int SEGGER_RTT_printf(unsigned ch, const char* fmt, ...) { (void)ch; (void)fmt; return 0; }
void HAL_NVIC_SetPriority(int a, uint32_t b, uint32_t c) { (void)a; (void)b; (void)c; }
void HAL_NVIC_EnableIRQ(int a) { (void)a; }
void HAL_GPIO_Init(void* a, GPIO_InitTypeDef* b) { (void)a; (void)b; }
void HAL_GPIO_WritePin(void* a, uint32_t b, int c) { (void)a; (void)b; (void)c; }
int HAL_GPIO_ReadPin(void* a, uint32_t b) { (void)a; (void)b; return 0; }
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef* h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef* h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_I2CEx_ConfigAnalogFilter(I2C_HandleTypeDef* h, uint32_t f) { (void)h; (void)f; return HAL_OK; }
HAL_StatusTypeDef HAL_I2CEx_ConfigDigitalFilter(I2C_HandleTypeDef* h, uint32_t f) { (void)h; (void)f; return HAL_OK; }
HAL_StatusTypeDef HAL_I2C_Mem_Read_DMA(I2C_HandleTypeDef* h, uint16_t a, uint16_t r, uint16_t s, uint8_t* p, uint16_t n) { (void)h; (void)a; (void)r; (void)s; (void)p; (void)n; return HAL_ERROR; }
HAL_StatusTypeDef HAL_I2C_Mem_Write_DMA(I2C_HandleTypeDef* h, uint16_t a, uint16_t r, uint16_t s, uint8_t* p, uint16_t n) { (void)h; (void)a; (void)r; (void)s; (void)p; (void)n; return HAL_ERROR; }
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* h, uint16_t a, uint16_t r, uint16_t s, uint8_t* p, uint16_t n, uint32_t t) { (void)h; (void)a; (void)r; (void)s; (void)p; (void)n; (void)t; return HAL_ERROR; }
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* h, uint16_t a, uint16_t r, uint16_t s, uint8_t* p, uint16_t n, uint32_t t) { (void)h; (void)a; (void)r; (void)s; (void)p; (void)n; (void)t; return HAL_ERROR; }

/* Sensor callbacks that i2c.c dispatches to for the other buses. Unused here. */
void v_AS6221_Write_DoneHandler(uint8_t a) { (void)a; }
void v_AS6221_Read_DoneHandler(uint8_t a, uint8_t* p, uint16_t n) { (void)a; (void)p; (void)n; }
void v_ADS111X_WrDone(uint8_t a) { (void)a; }
void v_ADS111X_RdDone(uint8_t a, uint8_t* p, uint16_t n) { (void)a; (void)p; (void)n; }
void v_Temp_IR_WR_Done(void) {}
void v_Temp_IR_RD_Done(uint8_t* p, uint16_t n) { (void)p; (void)n; }
void v_Codec_WrDone(void) {}
void v_Codec_RdDone(uint8_t* p, uint16_t n) { (void)p; (void)n; }
void v_I2C4_Diag_Event(e_IR_DIAG_EVENT_t e, int r) { (void)e; (void)r; }
void v_I2C4_Diag_Begin(char o, uint16_t a, uint16_t r, uint16_t b) { (void)o; (void)a; (void)r; (void)b; }
void v_I2C4_Diag_End(int r) { (void)r; }
void v_I2C4_Diag_FrameGood(void) {}
void v_I2C4_Diag_Poll(void) {}
void delay_us(uint32_t us) { (void)us; }

#include "i2c.c"

I2C_HandleTypeDef hi2c1 = {HAL_I2C_STATE_READY}, hi2c2 = {HAL_I2C_STATE_READY},
                  hi2c3 = {HAL_I2C_STATE_READY}, hi2c4 = {HAL_I2C_STATE_READY},
                  hi2c5 = {HAL_I2C_STATE_READY};

/* ---- I2C3 bus stub ---- */
typedef enum {O_OK, O_NACK, O_HANG, O_HANG_BUSY} outcome_t;
typedef struct {bool read; uint16_t reg, len; uint32_t t;} call_t;
static call_t calls[16384];
static unsigned ncalls;              /* every HAL_I2C_Mem_*_IT call, accepted or not */
static unsigned naccepted;           /* transfers the stub bus actually started */
static unsigned reject_next;         /* return HAL_BUSY for this many start calls */
static bool sync_delivery;           /* deliver the result inside the start call */
static uint32_t latency = 1;
static outcome_t (*policy)(unsigned idx);
static unsigned nacks;
static uint32_t t_last_nack;

static struct {bool active; bool read; uint16_t reg, len; uint8_t* buf; uint32_t at; outcome_t o;} pend;
static bool abort_pending; static uint32_t abort_at;

static uint8_t fifo[512]; static unsigned fifo_n;   /* bytes the module offers on 0xFF */

static void dev_push(const uint8_t* p, unsigned n) { memcpy(fifo + fifo_n, p, n); fifo_n += n; }
static void dev_push_frame(uint8_t cls, uint8_t id, const uint8_t* payload, uint16_t len) {
    uint8_t f[200]; f[0] = 0xB5; f[1] = 0x62; f[2] = cls; f[3] = id; f[4] = len & 0xff; f[5] = len >> 8;
    memcpy(f + 6, payload, len);
    v_UBX_CalcChecksum(f + 2, 4 + len, &f[6 + len], &f[7 + len]);
    dev_push(f, 8 + len);
}
static void dev_push_pvt(int32_t lat) {
    _x_UBX_NAV_PVT_PAYLOAD_t p; memset(&p, 0, sizeof p);
    p.fixType = 3; p.numSV = 9; p.lat = lat; p.lon = lat + 5;
    dev_push_frame(UBX_CLASS_NAV, UBX_NAV_PVT, (uint8_t*)&p, sizeof p);
}
static void dev_write(const uint8_t* d, uint16_t n) {
    if(n >= 4 && d[2] == UBX_CLASS_CFG && d[3] == UBX_CFG_VALSET) {
        const uint8_t ack[2] = {UBX_CLASS_CFG, UBX_CFG_VALSET};
        dev_push_frame(UBX_CLASS_ACK, UBX_ACK_ACK, ack, 2);
    }
}
static void dev_read(uint16_t reg, uint8_t* buf, uint16_t n) {
    if(reg == SAM_M10Q_REG_AVAIL_MSB && n == 2) { buf[0] = fifo_n >> 8; buf[1] = fifo_n & 0xff; return; }
    unsigned take = n < fifo_n ? n : fifo_n;
    memcpy(buf, fifo, take); memset(buf + take, 0xFF, n - take);
    memmove(fifo, fifo + take, fifo_n - take); fifo_n -= take;
}

static void complete(void) {
    pend.active = false;
    switch(pend.o) {
    case O_OK:
        if(pend.read) dev_read(pend.reg, pend.buf, pend.len);
        hi2c3.State = HAL_I2C_STATE_READY;
        if(pend.read) HAL_I2C_MemRxCpltCallback(&hi2c3); else HAL_I2C_MemTxCpltCallback(&hi2c3);
        break;
    case O_NACK:
        nacks++; t_last_nack = now;
        hi2c3.State = HAL_I2C_STATE_READY;
        HAL_I2C_ErrorCallback(&hi2c3);
        break;
    case O_HANG: hi2c3.State = HAL_I2C_STATE_READY; break;  /* callback never arrives */
    case O_HANG_BUSY: break;                                /* HAL handle stays mid-transfer */
    }
}
static HAL_StatusTypeDef start(bool read, uint16_t reg, uint8_t* buf, uint16_t n) {
    assert(ncalls < sizeof calls / sizeof calls[0]);
    calls[ncalls++] = (call_t){read, reg, n, now};
    if(reject_next) { reject_next--; return HAL_BUSY; }
    if(hi2c3.State != HAL_I2C_STATE_READY) return HAL_BUSY;
    pend.active = true; pend.read = read; pend.reg = reg; pend.len = n; pend.buf = buf;
    pend.at = now + latency; pend.o = policy(naccepted++);
    if(!read && pend.o == O_OK) dev_write(buf, n);
    hi2c3.State = read ? HAL_I2C_STATE_BUSY_RX : HAL_I2C_STATE_BUSY_TX;
    if(sync_delivery) complete();
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef* h, uint16_t a, uint16_t r, uint16_t s, uint8_t* p, uint16_t n) {
    assert(h == &hi2c3 && a == ADDR_GPS && s == I2C_MEMADD_SIZE_8BIT); return start(true, r, p, n);
}
HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef* h, uint16_t a, uint16_t r, uint16_t s, uint8_t* p, uint16_t n) {
    assert(h == &hi2c3 && a == ADDR_GPS && s == I2C_MEMADD_SIZE_8BIT); return start(false, r, p, n);
}
HAL_StatusTypeDef HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef* h, uint16_t a) {
    assert(h == &hi2c3 && a == ADDR_GPS);
    if(h->State == HAL_I2C_STATE_BUSY_RX || h->State == HAL_I2C_STATE_BUSY_TX) {
        h->State = HAL_I2C_STATE_ABORT; abort_pending = true; abort_at = now + 1; pend.active = false;
        return HAL_OK;
    }
    return HAL_ERROR;   /* HAL: Mode is NONE after an error/completion */
}

#include "sam_m10q_platform.c"

static void bus_tick(void) {
    if(pend.active && now >= pend.at) complete();
    if(abort_pending && now >= abort_at) {
        abort_pending = false; hi2c3.State = HAL_I2C_STATE_READY; HAL_I2C_AbortCpltCallback(&hi2c3);
    }
}
static void tick(void) { now++; bus_tick(); v_GPS_Handler(); v_GPS_Tout_Handler(); }
static void run(uint32_t ms) { while(ms--) tick(); }
static uint32_t t_init;

static bool ready(void) { return e_GPS_Ready() == COMM_STAT_DONE; }
static bool lat_is(int32_t lat) { return px_GPS_GetPVT()->lat == lat; }
static bool never_stop_hook(void) { return false; }
/* Run until cond() or the limit; returns elapsed ms or -1. */
#define RUN_UNTIL(cond, limit) ({ int _r = -1; uint32_t _t0 = now; \
    for(uint32_t _i = 0; _i < (limit); _i++) { if(cond) { _r = (int)(now - _t0); break; } tick(); } _r; })

static outcome_t all_ok(unsigned i) { (void)i; return O_OK; }
static outcome_t all_nack(unsigned i) { (void)i; return O_NACK; }
static outcome_t all_hang(unsigned i) { (void)i; return O_HANG; }
static outcome_t first_nack(unsigned i) { return i == 0 ? O_NACK : O_OK; }
static outcome_t first_hang(unsigned i) { return i == 0 ? O_HANG : O_OK; }
static outcome_t first_hang_busy(unsigned i) { return i == 0 ? O_HANG_BUSY : O_OK; }
static unsigned nack_budget;
static outcome_t nack_budgeted(unsigned i) { (void)i; if(nack_budget) { nack_budget--; return O_NACK; } return O_OK; }

static void begin(outcome_t (*p)(unsigned)) { policy = p; t_init = now; v_GPS_Init(); }

/* Init finished AND the runtime poller is alive: a PVT sent by the module is stored. */
static void expect_runtime_alive(int32_t lat) {
    dev_push_pvt(lat);
    assert(RUN_UNTIL(lat_is(lat), 3000) >= 0);
}

static void sc_absent(void) {
    begin(all_nack);
    int ms = RUN_UNTIL(ready(), 3000);
    assert(ms >= 0);
    printf("  init ended %d ms after v_GPS_Init\n", (int)(now - t_init));
    unsigned before = ncalls;
    run(60000);
    assert(ncalls == before);
}
static void sc_present(void) {
    begin(all_ok);
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    printf("  init ended %d ms after v_GPS_Init\n", (int)(now - t_init));
    expect_runtime_alive(375000000);
    expect_runtime_alive(376000000);
}
static void sc_init_transient(void) {
    begin(first_nack);
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    assert(nacks == 1);
    expect_runtime_alive(375000000);   /* DONE, not FAIL: FAIL would never poll */
}
static void sc_init_hang_recovers(void) {
    begin(first_hang);
    assert(RUN_UNTIL(ncalls >= 1, 3000) >= 0);
    assert(RUN_UNTIL(ncalls >= 2, 2000) >= 0);   /* next transfer reaches the HAL, not the queue */
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    expect_runtime_alive(375000000);
}
static void sc_init_hang_persists(void) {
    begin(all_hang);
    assert(RUN_UNTIL(ready(), 12000) >= 0);
    printf("  init ended %d ms after v_GPS_Init\n", (int)(now - t_init));
    assert(ncalls == 4);                         /* every attempt went to the HAL */
    run(10000);
    assert(ncalls == 4);                         /* FAIL: no more I2C3 traffic */
}
static void sc_init_hang_busy(void) {
    begin(first_hang_busy);                      /* HAL handle stays mid-transfer */
    assert(RUN_UNTIL(ready(), 8000) >= 0);
    expect_runtime_alive(375000000);
}
static void sc_start_fail(void) {
    reject_next = 1;
    begin(all_ok);
    assert(RUN_UNTIL(ncalls >= 1, 3000) >= 0);
    assert(RUN_UNTIL(ncalls >= 2, 500) >= 0);    /* retried through the HAL, not queued */
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    expect_runtime_alive(375000000);
}
static void sc_isr_race(void) {
    sync_delivery = true;                        /* NACK callback runs before the start call returns */
    begin(first_nack);
    assert(RUN_UNTIL(ncalls >= 1, 3000) >= 0);
    assert(RUN_UNTIL(ncalls >= 2, 300) >= 0);
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    expect_runtime_alive(375000000);
}
static void sc_runtime_transient(void) {
    begin(nack_budgeted);
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    expect_runtime_alive(375000000);
    nack_budget = 1; unsigned n0 = nacks;
    dev_push_pvt(376000000);
    assert(RUN_UNTIL(lat_is(376000000), 3000) >= 0);
    assert(nacks == n0 + 1);
    assert(now - t_last_nack <= 2000);
}
static void sc_runtime_persistent(void) {
    begin(nack_budgeted);
    assert(RUN_UNTIL(ready(), 5000) >= 0);
    expect_runtime_alive(375000000);
    nack_budget = 1000000;
    unsigned before = ncalls;
    run(10000);
    printf("  I2C3 starts in 10 s of NACKs: %u\n", ncalls - before);
    assert(ncalls - before <= 12);
    assert(ncalls - before >= 1);
    nack_budget = 0;
    expect_runtime_alive(376000000);
}

typedef struct {const char* name; void (*fn)(void);} scenario_t;
static const scenario_t scenarios[] = {
    {"absent", sc_absent}, {"present", sc_present}, {"init-transient", sc_init_transient},
    {"init-hang", NULL}, {"start-fail", sc_start_fail},
    {"runtime-transient", sc_runtime_transient}, {"runtime-persistent", sc_runtime_persistent},
    {"isr-race", sc_isr_race}, {"init-hang-busy", sc_init_hang_busy},
};
/* init-hang has two outcomes that need a fresh process image each: run both in children. */
static void child(void (*fn)(void)) {
    fflush(stdout);
    pid_t pid = fork();
    assert(pid >= 0);
    if(pid == 0) { fn(); fflush(stdout); _exit(0); }
    int st = 0; waitpid(pid, &st, 0);
    if(!(WIFEXITED(st) && WEXITSTATUS(st) == 0)) { fprintf(stderr, "FAIL init-hang sub-case\n"); exit(1); }
}
int main(int argc, char** argv) {
    assert(argc == 2); (void)never_stop_hook;
    for(unsigned i = 0; i < sizeof scenarios / sizeof scenarios[0]; i++) {
        if(strcmp(argv[1], scenarios[i].name)) continue;
        if(scenarios[i].fn) scenarios[i].fn();
        else { child(sc_init_hang_recovers); child(sc_init_hang_persists); }
        printf("PASS %s\n", argv[1]);
        return 0;
    }
    assert(!"unknown scenario");
}
'''

STUB_HEADERS = {
    'main.h': MAIN_H,
    'SEGGER_RTT.h': 'int SEGGER_RTT_printf(unsigned, const char*, ...);\n',
    'mode.h': '#include "main.h"\n',
    'uart.h': '#include "main.h"\n',
    'as6221_platform.h': '#include "main.h"\nvoid v_AS6221_Write_DoneHandler(uint8_t);\n'
                         'void v_AS6221_Read_DoneHandler(uint8_t, uint8_t*, uint16_t);\n',
    'ads111x_platform.h': '#include "main.h"\nvoid v_ADS111X_WrDone(uint8_t);\n'
                          'void v_ADS111X_RdDone(uint8_t, uint8_t*, uint16_t);\n',
    'mlx90640_platform.h': '#include "main.h"\nvoid v_Temp_IR_WR_Done(void);\n'
                           'void v_Temp_IR_RD_Done(uint8_t*, uint16_t);\n',
    'es8388_platform.h': '#include "main.h"\nvoid v_Codec_WrDone(void);\n'
                         'void v_Codec_RdDone(uint8_t*, uint16_t);\n',
}

SCENARIOS = ('absent', 'present', 'init-transient', 'init-hang', 'start-fail',
             'runtime-transient', 'runtime-persistent', 'isr-race', 'init-hang-busy')

with tempfile.TemporaryDirectory(prefix='psa-gps-i2c3-test-') as d:
    temp = Path(d)
    for name, text in STUB_HEADERS.items():
        (temp / name).write_text(text)
    (temp / 'test.c').write_text(HARNESS)
    exe = str(temp / 'test')
    includes = ['-I' + str(temp)] + ['-I' + str(ROOT / p) for p in (
        'SAM_M10Q/core/inc', 'SAM_M10Q/platform/inc', 'User/Drv/inc', 'User/Lib/inc',
        'User/Drv/src', 'SAM_M10Q/platform/src')]
    subprocess.run(['cc', '-std=gnu11', '-Wall', '-Wextra', '-Wno-unused-parameter',
                    '-Wno-unused-function', '-Wno-missing-field-initializers', '-DLOG_LEVEL=4'] + includes + [
                    str(temp / 'test.c'),
                    str(ROOT / 'SAM_M10Q/core/src/sam_m10q_driver.c'),
                    str(ROOT / 'SAM_M10Q/core/src/sam_m10q_ubx.c'),
                    str(ROOT / 'User/Lib/src/lib_tim.c'),
                    '-o', exe], check=True)
    failed = []
    for scenario in SCENARIOS:
        result = subprocess.run([exe, scenario], capture_output=True, text=True, timeout=120)
        print(result.stdout, end='')
        if result.returncode:
            print('FAIL %s' % scenario)
            print(result.stderr.strip())
            failed.append(scenario)
    if failed:
        raise SystemExit('failed: ' + ', '.join(failed))
