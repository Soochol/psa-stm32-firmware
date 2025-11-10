#ifndef __JH_LIB_BUF_H
#define __JH_LIB_BUF_H

#include "lib_def.h"


typedef struct {
  uint8_t* pu8_arr;
  uint16_t u16_cnt;
  uint16_t u16_max;
} _x_BUF8_t;

typedef struct {
  _x_BUF8_t*  p_buf;
  uint16_t    u16_max;
  uint16_t    u16_cnt;
  uint16_t    u16_in;
  uint16_t    u16_out;
} _x_BUF8Q_t;


typedef struct {
  uint16_t* pu16_arr;
  uint16_t  u16_cnt;
  uint16_t  u16_max;
} _x_BUF16_t;

typedef struct {
  _x_BUF16_t* p_buf;
  uint16_t    u16_max;
  uint16_t    u16_cnt;
  uint16_t    u16_in;
  uint16_t    u16_out;
} _x_BUF16Q_t;


typedef struct {
  uint32_t* pu32_arr;
  uint16_t  u16_cnt;
  uint16_t  u16_max;
} _x_BUF32_t; 

typedef struct {
  _x_BUF32_t* p_buf;
  uint16_t    u16_max;
  uint16_t    u16_cnt;
  uint16_t    u16_in;
  uint16_t    u16_out;
} _x_BUF32Q_t;




void _v_BUF8_Init(_x_BUF8_t* px_buf, uint8_t* pu8_arr, uint16_t u16_arr_len);
bool _b_BUF8Q_Init(_x_BUF8Q_t* px_q, uint8_t* pu8_arr, uint16_t u16_arr_len, uint16_t u16_q_cnt, uint16_t* pu16_heap);
bool _b_BUF8Q_Deinit(_x_BUF8Q_t* px_q, uint16_t* pu16_heap);
bool _b_BUF8Q_Put(_x_BUF8Q_t* px_q, uint8_t* pu8_arr, uint16_t u16_arr_len);
bool _b_BUF8Q_Get(_x_BUF8Q_t* px_q, uint8_t* pu8_arr, uint16_t* pu16_arr_len, uint16_t u16_arr_max);



void _v_BUF16_Init(_x_BUF16_t* px_buf, uint16_t* pu16_arr, uint16_t u16_arr_len);
bool _b_BUF16Q_Init(_x_BUF16Q_t* px_q, uint16_t* pu16_arr, uint16_t u16_arr_len, uint16_t u16_q_cnt, uint16_t* pu16_heap);
bool _b_BUF16Q_Deinit(_x_BUF16Q_t* px_q, uint16_t* pu16_heap);
bool _b_BUF16Q_Put(_x_BUF16Q_t* px_q, uint16_t* pu16_arr, uint16_t u16_arr_len);
bool _b_BUF16Q_Get(_x_BUF16Q_t* px_q, uint16_t* pu16_arr, uint16_t* pu16_arr_len, uint16_t u16_arr_max);




void _v_BUF32_Init(_x_BUF32_t* px_buf, uint32_t* pu32_arr, uint16_t u16_arr_len);
bool _b_BUF32Q_Init(_x_BUF32Q_t* px_q, uint32_t* pu32_arr, uint16_t u16_arr_len, uint16_t u16_q_cnt, uint16_t* pu16_heap);
bool _b_BUF32Q_Deinit(_x_BUF32Q_t* px_q, uint16_t* pu16_heap);
bool _b_BUF32Q_Put(_x_BUF32Q_t* px_q, uint32_t* pu32_arr, uint16_t u16_arr_len);
bool _b_BUF32Q_Get(_x_BUF32Q_t* px_q, uint32_t* pu32_arr, uint16_t* pu16_arr_len, uint16_t u16_arr_max);





#endif

