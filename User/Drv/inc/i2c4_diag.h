#ifndef I2C4_DIAG_H
#define I2C4_DIAG_H

#include "lib_def.h"

typedef enum {
    IRD_POWER_OFF, IRD_POWER_ON, IRD_LINES,
    IRD_BEFORE, IRD_AFTER, IRD_IRQ_ERROR,
    IRD_INIT_RATE, IRD_INIT_MODE, IRD_INIT_EEPROM, IRD_INIT_PARAMS,
    IRD_INIT_DONE, IRD_INIT_FAIL, IRD_TIMEOUT, IRD_RECOVERED,
    IRD_BAD_AUX, IRD_BAD_FRAME
} e_IR_DIAG_EVENT_t;

// RAM snapshots only. No I2C, GPIO writes, logging, delays or recovery here.
void v_I2C4_Diag_Event(e_IR_DIAG_EVENT_t event, int result);
void v_I2C4_Diag_Begin(char operation, uint16_t address, uint16_t reg, uint16_t bytes);
void v_I2C4_Diag_End(int hal_result);
void v_I2C4_Diag_FrameGood(void);
// Main-loop-only, nonblocking RTT drain. Starts after init finishes/fails or a fault.
void v_I2C4_Diag_Poll(void);

#endif
