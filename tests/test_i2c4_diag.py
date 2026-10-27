"""Host checks for passive I2C4 fault snapshots and nonblocking RTT output.

Run: python3 tests/test_i2c4_diag.py. No target or serial port is used.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
MAIN = r'''
#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
typedef struct {uint32_t ISR, CR1, CR2;} Registers;
typedef struct {uint32_t IDR, MODER, ODR;} GPIO;
typedef struct {Registers* Instance; uint32_t State, ErrorCode;} I2C_HandleTypeDef;
extern GPIO port_b, port_c;
extern bool clock_i2c, clock_b, clock_c;
extern uint32_t irq_mask, now;
#define GPIOB (&port_b)
#define GPIOC (&port_c)
#define I2C4_SCL_ACT_Pin (1u<<8)
#define I2C4_SDA_ACT_Pin (1u<<9)
#define DO_12VA_EN_Pin (1u<<13)
#define HAL_OK 0
#define __HAL_RCC_I2C4_IS_CLK_ENABLED() clock_i2c
#define __HAL_RCC_GPIOB_IS_CLK_ENABLED() clock_b
#define __HAL_RCC_GPIOC_IS_CLK_ENABLED() clock_c
#define __get_PRIMASK() irq_mask
#define __disable_irq() (irq_mask=1)
#define __set_PRIMASK(m) (irq_mask=(m))
uint32_t HAL_GetTick(void);
#endif
'''
HARNESS = r'''
#include "main.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
GPIO port_b, port_c;
bool clock_i2c, clock_b=true, clock_c=true;
uint32_t irq_mask, now=1;
I2C_HandleTypeDef hi2c4;
static Registers regs;
static unsigned writes, used;
static bool room=true;
static char output[20000];
uint32_t HAL_GetTick(void) {return now;}
#include "i2c4_diag.c"
unsigned SEGGER_RTT_Write(unsigned channel, const void* text, unsigned len) {
    assert(channel==0 && !irq_mask); writes++;
    if(!room) return 0;
    assert(used+len<sizeof(output));
    memcpy(output+used,text,len); used+=len; output[used]=0; return len;
}
static void setup(void) {
    hi2c4.Instance=&regs; hi2c4.State=0x20; clock_i2c=true;
    port_b.IDR=0x100; port_b.MODER=0xa<<16; port_c.ODR=1<<13;
    now=100; v_I2C4_Diag_Event(IRD_POWER_ON,0);
}
static void drain(void) {
    for(unsigned i=0;i<100 && count;i++){now+=10;v_I2C4_Diag_Poll();}
    assert(count==0);
}
int main(int argc,char** argv) {
    assert(argc==2);
    if(!strcmp(argv[1],"first-fault")) {
        setup(); now=110; v_I2C4_Diag_Poll();
        assert(!writes && records[0].scl==1 && records[0].sda==0 && records[0].power_age==0);
        v_I2C4_Diag_Begin('P',0x66,0,0); v_I2C4_Diag_End(0);
        now=120; v_I2C4_Diag_Event(IRD_INIT_RATE,0);
        v_I2C4_Diag_Begin('R',0x66,0x800d,2);
        assert(!writes);
        now=121; regs.ISR=0x201; hi2c4.ErrorCode=2; v_I2C4_Diag_End(1);
        assert(have_first_fault && first_fault.reg==0x800d && first_fault.operation=='R');
        assert(first_fault.error==2 && first_fault.isr==0x201 && first_fault.tick==121);
        // Production recovery may clear both values. The fault must survive it.
        regs.ISR=0; hi2c4.ErrorCode=0; v_I2C4_Diag_Event(IRD_INIT_FAIL,-1);
        Registers saved=regs; GPIO saved_b=port_b, saved_c=port_c;
        drain(); assert(first_fault.error==2 && first_fault.isr==0x201);
        assert(strstr(output,"reg=800D n=2 ret=1") && strstr(output,"E=00000002 ISR=00000201"));
        assert(strstr(output,"SCL=1 SDA=0 gm=A en12=1"));
        assert(!memcmp(&saved,&regs,sizeof(regs)) && !memcmp(&saved_b,&port_b,sizeof(port_b)) && !memcmp(&saved_c,&port_c,sizeof(port_c)));
    } else if(!strcmp(argv[1],"runtime-irq")) {
        setup(); v_I2C4_Diag_Event(IRD_INIT_DONE,0); drain();
        now=1000; v_I2C4_Diag_FrameGood();
        now=1010; v_I2C4_Diag_Begin('r',0x66,0x700,128); v_I2C4_Diag_End(0);
        assert(count==0); // Healthy DMA transfers do not flood RTT.
        unsigned previous_writes=writes;
        now=1021; hi2c4.ErrorCode=2; regs.ISR=0x201; irq_mask=1;
        v_I2C4_Diag_Event(IRD_IRQ_ERROR,1);
        assert(irq_mask==1 && writes==previous_writes && count==2);
        assert(first_fault.good_age==21 && first_fault.frames==1 && first_fault.reg==0x700);
        assert(records[rd].event==IRD_BEFORE && records[rd].good_age==10);
        irq_mask=0; drain(); assert(strstr(output,"IRQ_ERR") && strstr(output,"goodAge=21 frames=1"));
    } else if(!strcmp(argv[1],"backpressure")) {
        setup(); v_I2C4_Diag_Poll();
        v_I2C4_Diag_Event(IRD_INIT_DONE,0); unsigned queued=count;
        room=false; now+=10; v_I2C4_Diag_Poll(); assert(count==queued && writes==1);
        for(int i=0;i<50;i++) {v_I2C4_Diag_Poll();}
        assert(writes==1);
        room=true; now+=10; v_I2C4_Diag_Poll(); assert(count==queued-1); drain();
    } else if(!strcmp(argv[1],"clock-off")) {
        clock_i2c=clock_b=clock_c=false; hi2c4.Instance=NULL;
        v_I2C4_Diag_Event(IRD_POWER_OFF,0);
        assert(records[0].isr==0 && records[0].cr1==0 && records[0].gpio_mode==0 && !writes);
    } else if(!strcmp(argv[1],"overflow")) {
        setup(); for(int i=0;i<50;i++) v_I2C4_Diag_Event(IRD_LINES,0);
        assert(count==IR_DIAG_CAPACITY && dropped>0);
        hi2c4.ErrorCode=2; v_I2C4_Diag_Event(IRD_TIMEOUT,11);
        assert(have_first_fault && first_fault.error==2); drain(); assert(strstr(output,"lost="));
    } else if(!strcmp(argv[1],"wrap")) {
        setup(); now=UINT32_MAX-5; v_I2C4_Diag_Event(IRD_POWER_ON,0);
        now=UINT32_MAX-2; v_I2C4_Diag_FrameGood();
        now=4; v_I2C4_Diag_Begin('R',0x66,0x800d,2); v_I2C4_Diag_End(1);
        assert(first_fault.power_age==10 && first_fault.good_age==7);
    } else if(!strcmp(argv[1],"idle-lines")) {
        setup(); v_I2C4_Diag_Poll(); unsigned queued=count;
        port_b.IDR=0x300; v_I2C4_Diag_Poll(); assert(count==queued+1);
        assert(records[(wr+IR_DIAG_CAPACITY-1)%IR_DIAG_CAPACITY].sda==1);
        v_I2C4_Diag_Begin('P',0x66,0,0); queued=count;
        port_b.IDR=0x100; v_I2C4_Diag_Poll(); assert(count==queued && !writes);
    } else assert(!"unknown scenario");
    printf("PASS I2C4 diagnostics: %s\n",argv[1]);
}
'''
with tempfile.TemporaryDirectory(prefix='psa-ir-diag-test-') as d:
    temp=Path(d)
    (temp/'main.h').write_text(MAIN)
    (temp/'SEGGER_RTT.h').write_text('unsigned SEGGER_RTT_Write(unsigned, const void*, unsigned);\n')
    (temp/'test.c').write_text(HARNESS)
    exe=str(temp/'test')
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',
                    '-I'+str(temp),'-I'+str(ROOT/'User/Drv/inc'),
                    '-I'+str(ROOT/'User/Drv/src'),'-I'+str(ROOT/'User/Lib/inc'),
                    str(temp/'test.c'),'-o',exe],check=True)
    for scenario in ('first-fault','runtime-irq','backpressure','clock-off','overflow','wrap','idle-lines'):
        subprocess.run([exe,scenario],check=True)
