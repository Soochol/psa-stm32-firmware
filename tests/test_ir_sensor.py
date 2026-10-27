"""Execute production IR initialization/recovery and boot functions on the host.

Run: python3 tests/test_ir_sensor.py. Hardware stubs inject failed transfers,
missing callbacks and slow boot prerequisites. GPIO tests check the existing
short-test pulse/STOP sequence, not physical signal integrity.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


MAIN_STUB = r'''
#ifndef TEST_MAIN_H
#define TEST_MAIN_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "lib_def.h"
#include "MLX90640_API.h"
typedef enum {HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct {int State; uint32_t ErrorCode;} I2C_HandleTypeDef;
#define HAL_I2C_STATE_RESET 0
#define HAL_I2C_ERROR_NONE 0
#define I2C_MEMADD_SIZE_16BIT 2
#define I2C4_CHG 0
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define GPIO_MODE_OUTPUT_OD 1
#define GPIO_NOPULL 0
#define GPIO_SPEED_FREQ_LOW 0
#define I2C4_SCL_ACT_Pin 1
#define I2C4_SDA_ACT_Pin 2
#define I2C4_SCL_ACT_GPIO_Port 0
#define I2C4_SDA_ACT_GPIO_Port 0
typedef struct {int Pin, Mode, Pull, Speed;} GPIO_InitTypeDef;
#define __HAL_RCC_GPIOB_CLK_ENABLE() ((void)0)
HAL_StatusTypeDef HAL_I2C_Master_Abort_IT(I2C_HandleTypeDef*, uint16_t);
HAL_StatusTypeDef HAL_I2C_DeInit(I2C_HandleTypeDef*);
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef*);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef*, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t, uint32_t);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef*, uint16_t, uint16_t, uint16_t, uint8_t*, uint16_t, uint32_t);
void HAL_GPIO_Init(int, GPIO_InitTypeDef*);
void HAL_GPIO_WritePin(int, int, int);
int HAL_GPIO_ReadPin(int, int);
void delay_us(uint32_t);
#endif
'''


def run():
    with tempfile.TemporaryDirectory(prefix='psa-ir-test-') as directory:
        temp = Path(directory)
        (temp / 'main.h').write_text(MAIN_STUB)
        (temp / 'tim.h').write_text('#include "main.h"\nuint32_t u32_Tim_1msGet(void);\nbool _b_Tim_Is_OVR(uint32_t, uint32_t, uint32_t);\n')
        (temp / 'mode.h').write_text('''#define modeERR_TEMP_IR 1
#define modeERROR 9
void v_Mode_Set_Error(int);
int e_Mode_Get_Error(void);
void v_Mode_SetNext(int);
''')
        (temp / 'comm_esp.h').write_text('void v_ESP_Send_Error(uint16_t);\n')
        (temp / 'lib_log.h').write_text('#define LOG_ERROR(...) ((void)0)\n#define LOG_WARN(...) ((void)0)\n')
        driver = (ROOT / 'User/Drv/src/i2c.c').read_text()
        (temp / 'i2c4_functions.h').write_text('\n'.join(function(driver, name) for name in [
            'void v_I2C4_Bus_Recovery_FastMode(void)',
            'int i_I2C4_Read(uint8_t', 'int i_I2C4_Write(uint8_t',
            'void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)',
        ]))
        includes = [temp, ROOT / 'MLX90640/platform/inc', ROOT / 'MLX90640/core/inc',
                    ROOT / 'MLX90640/platform/src', ROOT / 'User/Lib/inc', ROOT / 'User/Drv/inc']
        exe = str(temp / 'ir')
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-Wno-unused-function', '-Wno-unused-parameter',
                        '-ffunction-sections', '-fdata-sections',
                        *['-I' + str(p) for p in includes],
                        str(ROOT / 'tests/ir_sensor_harness.c'), '-Wl,--gc-sections', '-o', exe], check=True)
        scenarios = ['healthy', 'init-permanent', 'init-wrap', 'runtime-permanent', 'dma-start',
                     'irq-error', 'invalid-frames', 'read-bound', 'gpio']
        scenarios += [f'init-{i}' for i in range(1, 5)]
        scenarios += ['lost-status', 'lost-pixel', 'lost-write', 'lost-aux', 'lost-control', 'lost-wrap']
        for scenario in scenarios:
            subprocess.run([exe, scenario], check=True)

        mode = (ROOT / 'User/Edit/src/mode.c').read_text()
        start = mode.index('static void v_Mode_Booting(e_modeID_t')
        boot = mode[start:mode.index('/////////////////////////////////\n//\tMODE - Healing', start)]
        start = mode.rfind('typedef enum {', 0, mode.index('modeCONFIG_IMU'))
        config = mode[start:mode.index('} e_modeCONFIG_t;', start) + len('} e_modeCONFIG_t;')]
        (temp / 'boot_function.h').write_text(config + '\n' + boot)
        boot_exe = str(temp / 'boot')
        subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-DMODE_IMU_USED=1', '-DMODE_FSR_USED=1',
                        '-I' + str(temp), str(ROOT / 'tests/ir_boot_harness.c'),
                        '-o', boot_exe], check=True)
        for scenario in ('slow-tof', 'slow-imu', 'ir-retry', 'ir-failure'):
            subprocess.run([boot_exe, scenario], check=True)


if __name__ == '__main__':
    run()
