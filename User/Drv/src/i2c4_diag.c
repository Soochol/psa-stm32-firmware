#include "main.h"
#include "i2c4_diag.h"
#include "SEGGER_RTT.h"
#include <stdio.h>

extern I2C_HandleTypeDef hi2c4;

#define IR_DIAG_CAPACITY 32

typedef struct {
    uint32_t tick, power_age, good_age, frames;
    uint32_t state, error, isr, cr1, cr2;
    uint16_t reg, bytes, address;
    uint8_t event, operation, scl, sda, gpio_mode, power;
    int result;
} x_IR_DIAG_t;

static x_IR_DIAG_t records[IR_DIAG_CAPACITY];
static unsigned rd, wr, count, dropped;
static bool boot = true, started, draining, have_power, have_good;
static uint32_t power_tick, good_tick, frames, print_tick;
static char operation = '-';
static uint16_t address, reg, bytes;
static uint8_t previous_lines = 0xff;
static x_IR_DIAG_t before;

// Keep the first incident available in RAM even after RTT has drained the queue.
static volatile x_IR_DIAG_t first_fault;
static bool have_first_fault;

static x_IR_DIAG_t snapshot(e_IR_DIAG_EVENT_t event, int result) {
    x_IR_DIAG_t r = {0};
    r.tick = HAL_GetTick();
    r.power_age = have_power ? r.tick - power_tick : UINT32_MAX;
    r.good_age = have_good ? r.tick - good_tick : UINT32_MAX;
    r.frames = frames;
    r.event = event;
    r.operation = operation;
    r.address = address;
    r.reg = reg;
    r.bytes = bytes;
    r.result = result;
    r.state = hi2c4.State;
    r.error = hi2c4.ErrorCode;
    if(__HAL_RCC_I2C4_IS_CLK_ENABLED() && hi2c4.Instance != NULL) {
        r.isr = hi2c4.Instance->ISR;
        r.cr1 = hi2c4.Instance->CR1;
        r.cr2 = hi2c4.Instance->CR2;
    }
    if(__HAL_RCC_GPIOB_IS_CLK_ENABLED()) {
        uint32_t pins = GPIOB->IDR;
        r.scl = !!(pins & I2C4_SCL_ACT_Pin);
        r.sda = !!(pins & I2C4_SDA_ACT_Pin);
        r.gpio_mode = (GPIOB->MODER >> 16) & 0xf; // PB8/PB9: A=AF, 5=output
    }
    if(__HAL_RCC_GPIOC_IS_CLK_ENABLED()) {
        // Enable signal only; this is NOT a measurement of the sensor's VDD.
        r.power = !!(GPIOC->ODR & DO_12VA_EN_Pin);
    }
    return r;
}

// Caller holds the interrupt mask. Never overwrite unread records.
static void append(x_IR_DIAG_t r) {
    if(count == IR_DIAG_CAPACITY) {dropped++; return;}
    records[wr] = r;
    wr = (wr + 1) % IR_DIAG_CAPACITY;
    count++;
}

static void fault(x_IR_DIAG_t r) {
    if(!have_first_fault) {first_fault = r; have_first_fault = true;}
    // The last transaction's BEFORE state is needed when HAL has cleared ISR flags.
    if(started && !boot) {append(before);}
    append(r);
    draining = true;
}

void v_I2C4_Diag_Event(e_IR_DIAG_EVENT_t event, int result) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if(event == IRD_POWER_ON) {
        power_tick = HAL_GetTick();
        have_power = true;
    }
    x_IR_DIAG_t r = snapshot(event, result);
    if(event == IRD_IRQ_ERROR || event == IRD_TIMEOUT ||
       event == IRD_BAD_AUX || event == IRD_BAD_FRAME || event == IRD_INIT_FAIL) {
        fault(r);
    } else {
        append(r);
    }
    if(event == IRD_INIT_DONE || event == IRD_INIT_FAIL) {
        boot = false;
        draining = true;
    }
    __set_PRIMASK(mask);
}

void v_I2C4_Diag_Begin(char op, uint16_t addr, uint16_t memory, uint16_t length) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    operation = op; address = addr; reg = memory; bytes = length;
    started = true;
    before = snapshot(IRD_BEFORE, -1);
    if(boot) {append(before);}
    __set_PRIMASK(mask);
}

void v_I2C4_Diag_End(int result) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    x_IR_DIAG_t r = snapshot(IRD_AFTER, result);
    if(result != HAL_OK) {fault(r);}
    else if(boot) {append(r);}
    __set_PRIMASK(mask);
}

void v_I2C4_Diag_FrameGood(void) {
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    good_tick = HAL_GetTick(); have_good = true; frames++;
    __set_PRIMASK(mask);
}

void v_I2C4_Diag_Poll(void) {
    // Watch the otherwise idle lines between rail enable and the first probe.
    // No printing before sensor initialization: preserve the baseline start order.
    if(have_power && !started && __HAL_RCC_GPIOB_IS_CLK_ENABLED()) {
        uint8_t lines = (GPIOB->IDR >> 8) & 3;
        if(lines != previous_lines) {
            previous_lines = lines;
            v_I2C4_Diag_Event(IRD_LINES, 0);
        }
    }
    if(!draining || HAL_GetTick() - print_tick < 10) {return;}
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    bool available = count != 0;
    x_IR_DIAG_t r = {0};
    if(available) {r = records[rd];}
    unsigned lost = dropped;
    __set_PRIMASK(mask);
    if(!available) {return;}
    print_tick = HAL_GetTick(); // Also rate-limit retries when RTT is full.
    static const char* const names[] = {
        "PWR_OFF", "PWR_ON", "LINES", "BEFORE", "AFTER", "IRQ_ERR",
        "RATE", "MODE", "EEPROM", "PARAMS", "INIT_OK", "INIT_FAIL",
        "TIMEOUT", "RECOVERED", "BAD_AUX", "BAD_FRAME"
    };
    char line[256];
    int n = snprintf(line, sizeof(line),
        "[IRDIAG] t=%lu %s op=%c a=%02X reg=%04X n=%u ret=%d "
        "S=%02lX E=%08lX ISR=%08lX C1=%08lX C2=%08lX "
        "SCL=%u SDA=%u gm=%X en12=%u pAge=%lu goodAge=%lu frames=%lu lost=%u\r\n",
        (unsigned long)r.tick, names[r.event], r.operation,
        (unsigned)r.address, (unsigned)r.reg, (unsigned)r.bytes, r.result,
        (unsigned long)r.state, (unsigned long)r.error, (unsigned long)r.isr,
        (unsigned long)r.cr1, (unsigned long)r.cr2,
        (unsigned)r.scl, (unsigned)r.sda, (unsigned)r.gpio_mode, (unsigned)r.power,
        (unsigned long)r.power_age, (unsigned long)r.good_age,
        (unsigned long)r.frames, lost);
    // Leave a record queued when the existing 1 KB RTT buffer is full.
    if(n > 0 && n < (int)sizeof(line) && SEGGER_RTT_Write(0, line, (unsigned)n) == (unsigned)n) {
        mask = __get_PRIMASK(); __disable_irq();
        rd = (rd + 1) % IR_DIAG_CAPACITY; count--;
        __set_PRIMASK(mask);
    }
}
