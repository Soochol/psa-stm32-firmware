
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define MODE_MP3_PLAYING 1
#define MODE_BOOTING_TOUT 20000
#define MODE_TILT_CENTER_TIMEOUT 10000
#define modeERR_TEMP_IR 1
#define modeERR_TOF 64
#define modeERR_AUDIO 128
#define modeERR_FSR 256
#define modeBOOTING 1
#define modeHEALING 2
#define modeERROR 3
#define ESP_EVT_MODE_BOOTING 1
#define MP3_DONE 1
#define MP3_ERR 2
#define MP3_IDLE 0
#define ADDR_IMU_LEFT 0xd0
#define ADDR_IMU_RIGHT 0xd2
#define ADDR_FSR_LEFT 0x90
#define ADDR_FSR_RIGHT 0x92
#define ADDR_TEMP_INDOOR 0x90
#define ADDR_TEMP_OUTDOOR 0x92
#define ADDR_TOF1 0x52
#define ADDR_IR_TEMP 0x66
#define LOG_ERROR(...) ((void)0)
#define LOG_WARN(...) ((void)0)
#define SEGGER_RTT_printf(...) ((void)0)
typedef enum {COMM_STAT_READY,COMM_STAT_OK,COMM_STAT_BUSY,COMM_STAT_ERR,COMM_STAT_DONE} e_COMM_STAT_t;
typedef int e_modeID_t;
typedef int e_modeERR_t;
typedef int I2C_HandleTypeDef;
I2C_HandleTypeDef hi2c1, hi2c2, hi2c4;
typedef struct {struct {int e_curr;} guide; struct {struct {int b1_upd,b1_on,b1_off;} bit;} cr;} x_modeWORK_t;
typedef struct {uint32_t u32_timToutRef;} x_modePUB_t;
static uint32_t now=1;
static int error_bits, next_mode, ir_calls;
static uint32_t u32_Tim_1msGet(void) {return now;}
static bool _b_Tim_Is_OVR(uint32_t t,uint32_t ref,uint32_t interval) {return t-ref>=interval;}
static void v_Mode_Set_Error(int e) {error_bits|=e;}
static int e_Mode_Get_Error(void) {return error_bits;}
static void v_Mode_SetNext(int mode) {next_mode=mode;}
static int ir_result = COMM_STAT_DONE, tof_result = COMM_STAT_BUSY, imu_result = COMM_STAT_DONE;
static int e_IR_Temp_Ready(void) {ir_calls++;return ir_result;}
static int i_I2C_ProbeDevice(void *h,int bus,int addr,const char *name) {
 (void)h;(void)bus;(void)addr;(void)name;return 0;
}
#define v_RGB_Clear() ((void)0)
#define v_Mode_Booting_Led() ((void)0)
#define v_ESP_PowerBegin(off) ((void)0)
#define i_ESP_PowerResult() 1
#define v_ESP_Send_InitStart() ((void)0)
#define v_ESP_Send_EvtModeChange(m) ((void)0)
#define v_GPS_Init() ((void)0)
#define i_Mode_Get_MP3_Play() 0
#define e_IMU_Ready() imu_result
#define e_FSR_Ready() COMM_STAT_DONE
#define e_Codec_Ready() COMM_STAT_DONE
#define e_Temp_InOut_Ready() COMM_STAT_DONE
#define e_TOF_Ready() tof_result
#define e_GPS_Ready() COMM_STAT_DONE
#define v_IMU_Tilt_Center_Enable() ((void)0)
#define i_MP3_Begin(n) ((void)0)
#define i_MP3_Is_Ready() 1
#define i_IMU_Is_Available() 0
#define i_IMU_Tilt_Is_Center() 0
#define v_I2C_DiagDump() ((void)0)
#define v_IMU_Handler() ((void)0)
#define v_IMU_Tilt_Center_Disable() ((void)0)
#define i_MP3_Get_Stat() MP3_DONE
#define i_MP3_ForceStop() ((void)0)
#define v_Mode_MoveNext(p) ((void)0)
#define v_ESP_Send_InitEnd() ((void)0)
#define v_DBG_BootComplete() ((void)0)

#include "boot_function.h"
int main(int argc, char** argv) {
 assert(argc == 2);
 const char* scenario = argv[1];
 x_modeWORK_t work={.guide={modeBOOTING},.cr={.bit={1,1,0}}};
 x_modePUB_t pub={0};
 if(!strcmp(scenario, "slow-imu")) {imu_result=COMM_STAT_BUSY; tof_result=COMM_STAT_DONE;}
 if(!strcmp(scenario, "ir-retry")) {tof_result=COMM_STAT_DONE; ir_result=COMM_STAT_BUSY;}
 if(!strcmp(scenario, "ir-failure")) {tof_result=COMM_STAT_DONE; ir_result=COMM_STAT_ERR;}
 if(!strcmp(scenario, "ir-retry")) {
  for(int i=0;i<3;i++) {v_Mode_Booting(modeBOOTING,&work,&pub);now+=100;assert(!error_bits);}
  ir_result=COMM_STAT_DONE;
  for(int i=0;i<5;i++) {v_Mode_Booting(modeBOOTING,&work,&pub);now+=100;}
  assert(!error_bits && next_mode==modeHEALING);
 } else {
  for(int i=0;i<22 && next_mode!=modeERROR;i++) {
   v_Mode_Booting(modeBOOTING,&work,&pub); now+=1000;
  }
  assert(ir_calls>=1);
  if(!strcmp(scenario, "slow-tof")) assert(error_bits==modeERR_TOF);
  if(!strcmp(scenario, "slow-imu")) {
   assert(!(error_bits & modeERR_TEMP_IR));
#if MODE_IMU_USED
   // FSR is gated by IMU, so its separate timeout is expected when enabled.
   assert(error_bits==(MODE_FSR_USED ? modeERR_FSR : 0));
#else
   assert(!error_bits);
#endif
  }
  if(!strcmp(scenario, "ir-failure")) assert(error_bits==modeERR_TEMP_IR && next_mode==modeERROR);
 }
 printf("PASS boot (IMU=%d, FSR=%d): %s\n",MODE_IMU_USED,MODE_FSR_USED,scenario);
 return 0;
}
