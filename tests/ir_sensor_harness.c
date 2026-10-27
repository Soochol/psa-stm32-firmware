#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "mlx90640_platform.c"

I2C_HandleTypeDef hi2c4;
static I2C_HandleTypeDef* p_i2c4 = &hi2c4;
static e_COMM_STAT_t e_comm_i2c4;
static I2C_HandleTypeDef bus1, bus2;
static I2C_HandleTypeDef* p_i2c1 = &bus1;
static I2C_HandleTypeDef* p_i2c2 = &bus2;
static e_COMM_STAT_t e_comm_i2c1, e_comm_i2c2;
static bool b_i2c1_sync_done;
static uint32_t i2c_error;
static uint32_t now = 1, last_read_timeout;
static int error_bits, next_mode, notifications, recoveries;
static int attempts, fail_stage, failures, dma_start_error, calculations;
static int pending, pending_write, sda_stuck, scl_lows, scl_highs, sda_writes;
static uint16_t pending_reg, pending_words;
static int hal_sequence;

#include "i2c4_functions.h"

uint32_t u32_Tim_1msGet(void) {return now;}
bool _b_Tim_Is_OVR(uint32_t t, uint32_t ref, uint32_t interval) {return (uint32_t)(t-ref) >= interval;}
void v_Mode_Set_Error(int err) {error_bits |= err;}
int e_Mode_Get_Error(void) {return error_bits;}
void v_Mode_SetNext(int mode) {next_mode = mode;}
void v_ESP_Send_Error(uint16_t err) {assert(err & modeERR_TEMP_IR); notifications++;}

HAL_StatusTypeDef HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef* h, uint16_t addr) {
    assert(h == &hi2c4 && addr == ADDR_IR_TEMP);
    hal_sequence = 1; pending = 0; return HAL_OK;
}
void HAL_GPIO_Init(int port, GPIO_InitTypeDef* config) {
    (void)port;
    assert(config->Mode == GPIO_MODE_OUTPUT_OD);
    assert(config->Pin == (I2C4_SCL_ACT_Pin | I2C4_SDA_ACT_Pin));
    if(hal_sequence == 1) hal_sequence = 2;
}
void HAL_GPIO_WritePin(int port, int pin, int state) {
    (void)port;
    if(pin == I2C4_SCL_ACT_Pin) {
        if(state == GPIO_PIN_RESET) scl_lows++; else scl_highs++;
    } else {
        assert(pin == I2C4_SDA_ACT_Pin);
        // Each STOP must drive SDA low, raise SCL, then release SDA.
        if(state == GPIO_PIN_SET) assert(scl_highs == scl_lows + sda_writes/2 + 1);
        sda_writes++;
    }
}
int HAL_GPIO_ReadPin(int port, int pin) {(void)port; assert(pin == I2C4_SDA_ACT_Pin); return !sda_stuck;}
void delay_us(uint32_t delay) {assert(delay == 2);}
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef* h) {
    assert(h == &hi2c4 && hal_sequence == 2); hal_sequence = 3; return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef* h) {
    assert(h == &hi2c4 && hal_sequence == 3); hal_sequence = 4; return HAL_OK;
}
void v_I2C4_Reset_CommState(void) {
    assert(hal_sequence == 4); hal_sequence = 0; recoveries++;
    // Every recovery must retain the original 9 clocks + STOP sequence.
    assert(scl_lows == 9 && scl_highs == 10 && sda_writes == 2);
    scl_lows = scl_highs = sda_writes = 0;
}
void v_I2C4_Set_Comm_Ready(void) {}
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* h, uint16_t addr, uint16_t reg,
                                 uint16_t addr_size, uint8_t* data, uint16_t bytes, uint32_t timeout) {
    (void)h; (void)addr; (void)reg; (void)addr_size; (void)bytes;
    last_read_timeout = timeout;
    if(failures) return HAL_TIMEOUT;
    data[0] = 0x12; data[1] = 0x34; return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* h, uint16_t addr, uint16_t reg,
                                  uint16_t addr_size, uint8_t* data, uint16_t bytes, uint32_t timeout) {
    (void)h; (void)addr; (void)reg; (void)addr_size; (void)data; (void)bytes; (void)timeout;
    return HAL_OK;
}
int i_I2C4_Read_DMA(uint8_t addr, uint16_t reg, uint16_t bytes) {
    assert(addr == ADDR_IR_TEMP);
    if(dma_start_error) return COMM_STAT_ERR;
    assert(!pending); pending = 1; pending_write = 0; pending_reg = reg; pending_words = bytes/2;
    return COMM_STAT_OK;
}
int i_I2C4_Write_DMA(uint8_t addr, uint16_t reg, uint8_t* data, uint16_t bytes) {
    (void)data; (void)bytes; assert(addr == ADDR_IR_TEMP);
    if(dma_start_error) return COMM_STAT_ERR;
    assert(!pending); pending = 1; pending_write = 1; pending_reg = reg;
    return COMM_STAT_OK;
}
static int fail(int stage) {
    if(stage == fail_stage && failures) {failures--; return -1;}
    return 0;
}
int MLX90640_SetRefreshRate(uint8_t addr, uint8_t rate) {
    (void)addr; (void)rate; attempts++; return fail(1);
}
int MLX90640_SetInterleavedMode(uint8_t addr) {(void)addr; return fail(2);}
int MLX90640_DumpEE(uint8_t addr, uint16_t* data) {(void)addr; (void)data; return fail(3);}
int MLX90640_ExtractParameters(uint16_t* data, paramsMLX90640* params) {(void)data; (void)params; return fail(4);}
float MLX90640_GetTa(uint16_t* frame, const paramsMLX90640* params) {(void)frame; (void)params; return 25;}
void MLX90640_CalculateTo(uint16_t* frame, const paramsMLX90640* params, float e, float tr, float* out) {
    (void)params; (void)e; (void)tr;
    // A missing control-register completion must never use the previous AUX word.
    assert(frame[832] == 0x1980);
    calculations++;
    for(int i = 0; i < 768; i++) out[i] = 30;
}
void MLX90640_BadPixelsCorrection(uint16_t* pixels, float* to, int mode, paramsMLX90640* params) {
    (void)pixels; (void)to; (void)mode; (void)params;
}
static void complete_pending(void) {
    assert(pending);
    pending = 0;
    if(pending_write) {v_Temp_IR_WR_Done(); return;}
    uint8_t data[200] = {0};
    for(int i = 0; i < pending_words; i++) {
        uint16_t value = pending_reg == MLX90640_STATUS_REG ? 9 :
                         pending_reg == MLX90640_CTRL_REG ? 0x1980 : 0x1234;
        data[2*i] = value >> 8; data[2*i+1] = value;
    }
    v_Temp_IR_RD_Done(data, pending_words*2);
}
static void run_frame(void) {
    int target = calculations + 1;
    for(int i = 0; i < 500 && calculations < target; i++) {
        v_Temp_IR_Data_Handler();
        if(pending) complete_pending();
        v_Temp_IR_Tout_Handler(); now++;
    }
    assert(calculations == target && !error_bits);
}
static void boot_ready(void) {v_TempIR_Deinit(); assert(e_IR_Temp_Ready() == COMM_STAT_DONE);}
static void lose_transfer(void) {
    assert(pending && e_tempIR_evt == COMM_STAT_BUSY);
    now += 2001; v_Temp_IR_Tout_Handler();
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const char* test = argv[1];
    if(!strcmp(test, "healthy")) {
        boot_ready(); run_frame(); run_frame(); assert(!recoveries);
    } else if(!strncmp(test, "init-", 5) && test[5] >= '1' && test[5] <= '4') {
        fail_stage = atoi(test+5); failures = 1;
        v_TempIR_Deinit();
        assert(e_IR_Temp_Ready() == COMM_STAT_BUSY && recoveries == 1);
        v_Temp_IR_Tout_Handler(); assert(recoveries == 1);
        now += 99; assert(e_IR_Temp_Ready() == COMM_STAT_BUSY && attempts == 1);
        now++; assert(e_IR_Temp_Ready() == COMM_STAT_DONE && attempts == 2);
        run_frame();
    } else if(!strcmp(test, "init-permanent")) {
        fail_stage = 1; failures = 10; v_TempIR_Deinit();
        for(int i = 0; i < 3; i++) {
            assert(e_IR_Temp_Ready() == COMM_STAT_BUSY); now += 100;
        }
        assert(e_IR_Temp_Ready() == COMM_STAT_ERR && recoveries == 3 && attempts == 4);
        assert(e_IR_Temp_Ready() == COMM_STAT_ERR && attempts == 4);
        failures = 0; boot_ready(); run_frame();
    } else if(!strcmp(test, "init-wrap")) {
        now = UINT32_MAX - 50; fail_stage = 1; failures = 1; v_TempIR_Deinit();
        assert(e_IR_Temp_Ready() == COMM_STAT_BUSY);
        now += 99; assert(e_IR_Temp_Ready() == COMM_STAT_BUSY);
        now++; assert(e_IR_Temp_Ready() == COMM_STAT_DONE);
    } else if(!strcmp(test, "runtime-permanent")) {
        boot_ready();
        for(int i = 0; i < 4; i++) {
            v_Temp_IR_Data_Handler(); lose_transfer(); now++;
            if(i < 3) assert(!error_bits && recoveries == i+1);
        }
        assert(error_bits == modeERR_TEMP_IR && next_mode == modeERROR && notifications == 1);
        assert(recoveries == 3 && e_tempIR_config == COMM_STAT_ERR);
        v_Temp_IR_Tout_Handler(); assert(notifications == 1);
    } else if(!strcmp(test, "dma-start") || !strcmp(test, "irq-error")) {
        boot_ready();
        dma_start_error = !strcmp(test, "dma-start");
        v_Temp_IR_Data_Handler();
        if(!dma_start_error) {pending = 0; HAL_I2C_ErrorCallback(&hi2c4);}
        assert(e_tempIR_evt == COMM_STAT_ERR);
        v_Temp_IR_Data_Handler(); assert(!calculations);
        v_Temp_IR_Tout_Handler(); assert(!recoveries);
        now += 1999; v_Temp_IR_Tout_Handler(); assert(!recoveries);
        now++; v_Temp_IR_Tout_Handler(); assert(recoveries == 1 && !error_bits);
        dma_start_error = 0; run_frame(); assert(u8_ir_i2c4_retry_cnt == 0);
    } else if(!strcmp(test, "invalid-frames")) {
        boot_ready(); u32_tempIR_ErrCnt = 11;
        v_Temp_IR_Tout_Handler(); assert(recoveries == 1 && !error_bits);
        run_frame(); assert(u32_tempIR_ErrCnt == 0 && u8_ir_i2c4_retry_cnt == 0);
    } else if(!strcmp(test, "read-bound")) {
        uint16_t data[832] = {0};
        assert(MLX90640_I2CRead(ADDR_IR_TEMP, MLX90640_EEPROM_START_ADDRESS, 832, data) == 0);
        assert(last_read_timeout > 200 && last_read_timeout < 2000 && data[0] == 0x1234);
        failures = 1; data[0] = 0x1234;
        assert(MLX90640_I2CRead(ADDR_IR_TEMP, MLX90640_EEPROM_START_ADDRESS, 832, data) == -1);
        assert(data[0] == 0x1234); // failed data must not be byte-swapped
    } else if(!strcmp(test, "gpio")) {
        v_I2C4_Bus_Recovery_FastMode();
        assert(scl_lows == 9 && scl_highs == 10 && sda_writes == 2);
        scl_lows = scl_highs = sda_writes = 0; sda_stuck = 1;
        v_I2C4_Bus_Recovery_FastMode();
        assert(scl_lows == 39 && scl_highs == 41 && sda_writes == 4);
    } else if(!strncmp(test, "lost-", 5)) {
        if(!strcmp(test, "lost-wrap")) now = UINT32_MAX - 100;
        boot_ready(); int injected = 0;
        for(int i = 0; i < 200 && !injected; i++) {
            v_Temp_IR_Data_Handler();
            if(pending) {
                bool target = (!strcmp(test, "lost-write") && pending_write) ||
                    (!pending_write && ((!strcmp(test, "lost-status") && pending_reg == 0x8000) ||
                    (!strcmp(test, "lost-pixel") && pending_reg == 0x0420) ||
                    (!strcmp(test, "lost-aux") && pending_reg == 0x0700) ||
                    ((!strcmp(test, "lost-control") || !strcmp(test, "lost-wrap")) && pending_reg == 0x800D)));
                if(target) {
                    injected = 1; lose_transfer();
                    assert(b_tempIR_handler_reset && recoveries == 1 && !calculations);
                    uint8_t late[2] = {0xff, 0xff};
                    v_Temp_IR_RD_Done(late, 2); v_Temp_IR_WR_Done();
                    assert(e_tempIR_evt == COMM_STAT_READY && u16_rdBuf[0] == 0);
                    now++; v_Temp_IR_Data_Handler();
                    assert(pending && !pending_write && pending_reg == MLX90640_STATUS_REG && !calculations);
                    complete_pending();
                } else complete_pending();
            }
            now++;
        }
        assert(injected); run_frame(); assert(u8_ir_i2c4_retry_cnt == 0);
        // Another unrelated fault after a good frame gets a fresh retry budget.
        u32_tempIR_ErrCnt = 11; v_Temp_IR_Tout_Handler();
        assert(u8_ir_i2c4_retry_cnt == 1 && !error_bits);
    } else {assert(!"unknown scenario");}
    printf("PASS IR: %s\n", test);
    return 0;
}
