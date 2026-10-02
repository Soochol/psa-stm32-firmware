"""Execute the production boot mode with GPS busy and required-sensor failures."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
source = (root / 'User/Edit/src/mode.c').read_text()
a = source.index('typedef enum {', source.index('//\tMode - Booting'))
b = source.index('//\tMODE - Healing', a)
boot = source[a:b]
stubs = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "lib_def.h"
#define LOG_INFO(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define LOG_ERROR(...) ((void)0)
#define SEGGER_RTT_printf(...) ((void)0)
#define MODE_BOOT_LED_R 0
#define MODE_BOOT_LED_G 0
#define MODE_BOOT_LED_B 0
#define MODE_BOOTING_TOUT 20000
#define MODE_TILT_CENTER_TIMEOUT 5000
#define MODE_MP3_PLAYING 0
#define ESP_EVT_MODE_BOOTING 8
#define ADDR_IMU_LEFT 1
#define ADDR_IMU_RIGHT 2
#define ADDR_FSR_LEFT 3
#define ADDR_FSR_RIGHT 4
#define ADDR_TEMP_INDOOR 5
#define ADDR_TEMP_OUTDOOR 6
#define ADDR_TOF1 7
#define ADDR_IR_TEMP 8
#define NOOP(...) ((void)0)
#define v_RGB_Set_Top NOOP
#define v_RGB_Set_Bot NOOP
#define v_RGB_Clear NOOP
#define v_ESP_Send_InitStart NOOP
#define v_ESP_Send_EvtModeChange NOOP
#define v_IMU_Tilt_Center_Enable NOOP
#define v_IMU_Tilt_Center_Disable NOOP
#define v_IMU_Handler NOOP
#define i_MP3_Begin NOOP
#define i_MP3_ForceStop NOOP
#define v_I2C_DiagDump NOOP
#define v_Mode_MoveNext NOOP
#define v_DBG_BootComplete NOOP
typedef int e_modeID_t,e_modeERR_t,I2C_HandleTypeDef;
enum {modeBOOTING=1,modeHEALING,modeERROR};
enum {modeERR_TEMP_IR=1,modeERR_TOF=2,modeERR_AUDIO=4,modeERR_FSR=8};
enum {MP3_DONE=1,MP3_ERR,MP3_IDLE};
typedef struct {struct {int e_curr;} guide;struct {struct {bool b1_upd,b1_on,b1_off;} bit;} cr;} x_modeWORK_t;
typedef struct {uint32_t u32_timToutRef;} x_modePUB_t;
I2C_HandleTypeDef hi2c1,hi2c2,hi2c4;
static uint32_t now;
static unsigned gpsStarts,gpsQueries,initEnds;
static int nextMode,errors,irStatus=COMM_STAT_DONE,tofStatus=COMM_STAT_DONE,imuStatus=COMM_STAT_DONE;
static unsigned fsrCalls,irCalls;
uint32_t u32_Tim_1msGet(void){return now;}
bool _b_Tim_Is_OVR(uint32_t n,uint32_t s,uint32_t d){return (uint32_t)(n-s)>=d;}
void v_ESP_PowerBegin(bool sleep){assert(!sleep);}
int i_ESP_PowerResult(void){return 1;}
void v_GPS_Init(void){++gpsStarts;}
e_COMM_STAT_t e_GPS_Ready(void){++gpsQueries;return COMM_STAT_BUSY;}
void v_ESP_Send_InitEnd(void){++initEnds;}
int i_Mode_Get_MP3_Play(void){return 0;}
int i_MP3_Is_Ready(void){return 1;}
int i_MP3_Playing(void){return MP3_DONE;}
int i_MP3_Get_Stat(void){return MP3_DONE;}
int i_I2C_ProbeDevice(I2C_HandleTypeDef*h,int b,int a,const char*n){(void)h;(void)b;(void)a;(void)n;return 0;}
e_COMM_STAT_t e_IMU_Ready(void){return imuStatus;}
e_COMM_STAT_t e_FSR_Ready(void){++fsrCalls;return COMM_STAT_DONE;}
e_COMM_STAT_t e_Codec_Ready(void){return COMM_STAT_DONE;}
e_COMM_STAT_t e_Temp_InOut_Ready(void){return COMM_STAT_DONE;}
e_COMM_STAT_t e_TOF_Ready(void){return tofStatus;}
e_COMM_STAT_t e_IR_Temp_Ready(void){++irCalls;return irStatus;}
int i_IMU_Is_Available(void){return 0;}
int i_IMU_Tilt_Is_Center(void){return 0;}
void v_Mode_Set_Error(int e){errors|=e;}
int e_Mode_Get_Error(void){return errors;}
void v_Mode_SetNext(int e){nextMode=e;}
'''
checks = r'''
static x_modeWORK_t work;
static x_modePUB_t pub;
static void start(void){
 work=(x_modeWORK_t){0};pub=(x_modePUB_t){0};work.guide.e_curr=modeBOOTING;
 work.cr.bit.b1_upd=work.cr.bit.b1_on=true;now=0;errors=nextMode=0;
}
int main(void){
 start();
 for(;now<20&&!nextMode;++now)v_Mode_Booting(modeBOOTING,&work,&pub);
 assert(nextMode==modeHEALING&&gpsStarts==1&&gpsQueries==0&&now<2000);
 work.cr.bit.b1_on=false;work.cr.bit.b1_off=true;
 v_Mode_Booting(modeBOOTING,&work,&pub);assert(initEnds==1);
 start();irStatus=COMM_STAT_ERR;
 for(;now<20&&!nextMode;++now)v_Mode_Booting(modeBOOTING,&work,&pub);
 assert(nextMode==modeERROR&&(errors&modeERR_TEMP_IR)); // required sensors still gate
 start();irStatus=COMM_STAT_DONE;tofStatus=COMM_STAT_BUSY;
 for(;now<20002&&!nextMode;++now)v_Mode_Booting(modeBOOTING,&work,&pub);
 assert(nextMode==modeERROR&&(errors&modeERR_TOF)&&now>=20000);
#if MODE_IMU_USED
 // Reproduce RTT ready_mask=0x1C: an unfinished IMU prevents dependent
 // initialization. The overall timeout then labels those sensors failed
 // even though their drivers were never called. This is an existing boot
 // sequencing issue, not proof that the optional-module change causes NACK.
 start();imuStatus=COMM_STAT_BUSY;tofStatus=COMM_STAT_DONE;
 irStatus=COMM_STAT_DONE;fsrCalls=irCalls=0;
 for(;now<20002&&!nextMode;++now)v_Mode_Booting(modeBOOTING,&work,&pub);
 assert(nextMode==modeERROR&&(errors&modeERR_TEMP_IR));
 assert(irCalls==0&&fsrCalls==0&&now>=20000);
#if MODE_FSR_USED
 assert(errors&modeERR_FSR);
#endif
 puts("REPRODUCED: blocked IMU produces dependent-sensor errors before their initialization");
#endif
 puts("PASS: actual boot completes with busy GPS, preserves required-sensor failures/deadline");
}
'''
with tempfile.TemporaryDirectory(prefix='psa-optional-boot-') as directory:
    cpp = Path(directory) / 'boot.c'
    cpp.write_text(stubs + boot + checks)
    for imu in [0, 1]:
        for fsr in [0, 1]:
            exe = Path(directory) / f'boot-{imu}-{fsr}'
            subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Wno-unused-variable',
                            '-Wno-unused-but-set-variable', '-fsanitize=address,undefined',
                            '-fno-sanitize-recover=all', f'-DMODE_IMU_USED={imu}',
                            f'-DMODE_FSR_USED={fsr}', '-I' + str(root / 'User/Lib/inc'),
                            str(cpp), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True, timeout=10)
