"""Production GPS platform/core with asynchronous I2C and clock fault injection."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
headers = {
    'main.h': '''#pragma once
#include <stdint.h>
typedef struct { int state; } I2C_HandleTypeDef;
extern I2C_HandleTypeDef hi2c3;
#define HAL_I2C_STATE_READY 0
#define I2C3_EV_IRQn 0
#define I2C3_ER_IRQn 1
static inline int HAL_I2C_GetState(I2C_HandleTypeDef*h){return h->state;}
static inline void HAL_NVIC_SetPriority(int a,int b,int c){(void)a;(void)b;(void)c;}
static inline void HAL_NVIC_EnableIRQ(int a){(void)a;}
''',
    'i2c.h': '''#pragma once
#include "main.h"
#define ADDR_GPS 0x84
void v_I2C3_Set_Comm_Ready(void);
void v_I2C3_Cancel(void);
int i_I2C3_Read(uint8_t,uint16_t,uint16_t);
int i_I2C3_Write(uint8_t,uint16_t,uint8_t*,uint16_t);
''',
    'tim.h': '''#pragma once
#include <stdint.h>
uint32_t u32_Tim_1msGet(void);
static inline int _b_Tim_Is_OVR(uint32_t n,uint32_t s,uint32_t d){return (uint32_t)(n-s)>=d;}
''',
    'mode.h': '#pragma once\n#define IWDG_USED 0\n',
    'uart.h': '#pragma once\n',
    'lib_log.h': '''#pragma once
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define LOG_DEBUG(...) ((void)0)
''',
}
i2c_source = (root / 'User/Drv/src/i2c.c').read_text()
def function(signature):
    start = i2c_source.index(signature)
    end = i2c_source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (i2c_source[end] == '{') - (i2c_source[end] == '}')
        end += 1
    return i2c_source[start:end]

source = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "i2c.h"
#include "sam_m10q_platform.h"
#include "lib_def.h"
I2C_HandleTypeDef hi2c3;
static uint32_t now;
static unsigned reads,writes,cancels;
static int behavior,pending;
static bool ack_pending;
uint32_t u32_Tim_1msGet(void){return now;}
void v_I2C3_Set_Comm_Ready(void){}
static I2C_HandleTypeDef hi2c1,hi2c2,hi2c4;
static I2C_HandleTypeDef *p_i2c1=&hi2c1,*p_i2c2=&hi2c2,*p_i2c3=&hi2c3,*p_i2c4=&hi2c4;
static int e_comm_i2c1,e_comm_i2c2,e_comm_i2c3,e_comm_i2c4;
static bool b_i2c1_sync_done;
static uint32_t i2c_error,irqMask;
static uint8_t u8_i2c3_addr;
static uint16_t u16_i2c3_rdCnt;
static struct {uint16_t count;uint8_t bytes[128];} i2c3_buf;
uint32_t __get_PRIMASK(void){return irqMask;}
void __disable_irq(void){irqMask=1;}
void __set_PRIMASK(uint32_t p){irqMask=p;}
int HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef*h,int addr){assert(addr==0x84);++cancels;pending=0;h->state=0;return 0;}
int i_I2C3_Read(uint8_t a,uint16_t reg,uint16_t len){
 (void)a;(void)len;++reads;
 if(behavior==1)return COMM_STAT_ERR;
 pending=reg;hi2c3.state=1;return COMM_STAT_OK;
}
int i_I2C3_Write(uint8_t a,uint16_t reg,uint8_t *buf,uint16_t len){
 (void)a;(void)reg;(void)buf;(void)len;++writes;
 pending=-1;hi2c3.state=1;return COMM_STAT_OK;
}
''' + function('void v_I2C3_Cancel(void){') + function('void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c){') + '\n#include "' + str(root / 'SAM_M10Q/platform/src/sam_m10q_platform.c') + '"\n' + r'''
static void pump(void){
 if(behavior!=2||!pending)return;
 int job=pending;pending=0;hi2c3.state=0;
 if(job==-1){ack_pending=true;v_GPS_Write_DoneHandler(ADDR_GPS);}
 else if(job==SAM_M10Q_REG_AVAIL_MSB){
  uint8_t bytes[2]={0,ack_pending?10:0};v_GPS_Read_DoneHandler(ADDR_GPS,bytes,2);
 } else {
  uint8_t ack[]={0xb5,0x62,5,1,2,0,6,0x8a,0,0};
  v_UBX_CalcChecksum(ack+2,6,ack+8,ack+9);
  ack_pending=false;v_GPS_Read_DoneHandler(ADDR_GPS,ack,10);
 }
}
static void advance(unsigned duration){
 for(unsigned i=0;i<duration;++i){++now;pump();v_GPS_Handler();v_GPS_Tout_Handler();}
}
static void reset(void){
 v_GPS_Deinit();now=reads=writes=cancels=0;behavior=pending=0;ack_pending=false;
 u32_gps_retry_next=30000;hi2c3.state=0;
}
int main(void){
 memset(&i2c3_buf,0xff,sizeof(i2c3_buf));u8_i2c3_addr=0x84;u16_i2c3_rdCnt=20;
 irqMask=1;v_I2C3_Cancel();assert(irqMask==1&&u8_i2c3_addr==0&&u16_i2c3_rdCnt==0&&i2c3_buf.count==0);
 irqMask=0;
 reset();advance(10000);assert(reads==0); // OFF/IDLE never polls
 v_GPS_Init();advance(3300);
 assert(e_gps_init_state==GPS_INIT_FAIL&&reads==1&&!b_GPS_IsAvailable());
 unsigned failedAt=u32_gps_retry_start;
 advance(10000);assert(reads==1); // no normal polling while unavailable
 // A late callback from the timed-out read cannot populate old coordinates.
 uint8_t stale[2]={0x01,0x00};v_GPS_Read_DoneHandler(ADDR_GPS,stale,2);
 assert(e_gps_comm==COMM_STAT_READY&&px_gps->u16_availBytes==0);
 behavior=2;
 advance(30000-(now-failedAt)+3000);
 assert(b_GPS_IsAvailable()&&!b_GPS_HasFix()); // late module, no satellite fix needed
 assert(writes==1);
 assert(u16_gps_cfg_len>0&&u16_gps_cfg_len<sizeof(u8_gps_cfg_buf));
 // Runtime ISR error invalidates stale PVT and returns to optional retry.
 px_gps->b_pvtValid=true;px_gps->x_pvt.fixType=3;px_gps->x_pvt.lat=370000000;
 e_gps_comm=COMM_STAT_BUSY;HAL_I2C_ErrorCallback(&hi2c3);v_GPS_Handler();
 assert(!b_GPS_HasFix()&&px_GPS_GetPVT()->fixType==0&&e_gps_init_state==GPS_INIT_FAIL);
 v_GPS_Deinit();unsigned stopped=reads;advance(65000);assert(reads==stopped);

 reset();behavior=1;v_GPS_Init();advance(2100);
 assert(e_gps_init_state==GPS_INIT_FAIL&&reads==1&&writes==0); // immediate NACK
 // Abort must complete before a new attempt can touch the bus.
 failedAt=u32_gps_retry_start;hi2c3.state=1;advance(31000);assert(reads==1);
 hi2c3.state=0;behavior=2;advance(3000);assert(b_GPS_IsAvailable());

 reset();now=UINT32_MAX-1000;behavior=2;v_GPS_Init();advance(3000);
 assert(b_GPS_IsAvailable()); // all init deadlines survive tick wraparound

 reset();v_GPS_Init();advance(2002);assert(pending);
 v_GPS_ErrorHandler();v_GPS_Handler();assert(e_gps_init_state==GPS_INIT_FAIL);
 // Requests cannot remain active past the overall initialization budget.
 reset();v_GPS_Init();now=8000;v_GPS_Handler();assert(e_gps_init_state==GPS_INIT_FAIL);
 puts("PASS: actual GPS init/core, silent/NACK/error callbacks, late recovery, no-fix, OFF, abort wait, deadlines and rollover");
}
'''
with tempfile.TemporaryDirectory(prefix='psa-optional-gps-') as directory:
    folder = Path(directory)
    for name, content in headers.items():
        (folder / name).write_text(content)
    test = folder / 'gps.c'
    exe = folder / 'gps'
    test.write_text(source)
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wno-unused-parameter',
                    '-Wno-unused-variable', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-I' + str(folder),
                    '-I' + str(root / 'SAM_M10Q/platform/inc'),
                    '-I' + str(root / 'SAM_M10Q/core/inc'), '-I' + str(root / 'User/Lib/inc'),
                    str(test), str(root / 'SAM_M10Q/core/src/sam_m10q_driver.c'),
                    str(root / 'SAM_M10Q/core/src/sam_m10q_ubx.c'), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True, timeout=10)
