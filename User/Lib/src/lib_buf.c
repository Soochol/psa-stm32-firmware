#include "lib_buf.h"
#include "string.h"
#include "stdlib.h"

void _v_BUF8_Init(_x_BUF8_t* px_buf, uint8_t* pu8_arr, uint16_t u16_arr_len){
  px_buf->u16_cnt = 0;
  px_buf->u16_max = u16_arr_len;
  px_buf->pu8_arr = pu8_arr;
}

bool _b_BUF8Q_Init(_x_BUF8Q_t* px_q, uint8_t* pu8_arr, uint16_t u16_arr_len, uint16_t u16_q_cnt, uint16_t* pu16_heap){
  if(u16_arr_len < u16_q_cnt){return false;}
  uint16_t u16_HeapSize = sizeof(_x_BUF8_t) * u16_q_cnt;
  if(*pu16_heap < u16_HeapSize){return false;}
  uint16_t u16_i=0;
  uint16_t u16_len = u16_arr_len / u16_q_cnt; //mutiple of 2 .. cut.
  uint16_t u16_idx = 0;
  
  px_q->u16_max = u16_q_cnt;
  px_q->u16_cnt = px_q->u16_in = px_q->u16_out = 0;
  px_q->p_buf = (_x_BUF8_t*)malloc(u16_HeapSize);

  while(u16_q_cnt--){
    px_q->p_buf[u16_i].u16_cnt = 0;
    px_q->p_buf[u16_i].pu8_arr = pu8_arr + u16_idx;
    px_q->p_buf[u16_i++].u16_max = u16_len;
    u16_idx += u16_len;
  }
  return true;
}

bool _b_BUF8Q_Deinit(_x_BUF8Q_t* px_q, uint16_t* pu16_heap){
  if(px_q->p_buf == NULL){return false;}
  free(px_q->p_buf);
  *pu16_heap += sizeof(_x_BUF8_t) * px_q->u16_cnt;
  return true;
}

bool _b_BUF8Q_Put(_x_BUF8Q_t* px_q, uint8_t* pu8_arr, uint16_t u16_arr_len){
  //buffer size is samll than input array length
  if(px_q->p_buf[px_q->u16_in].u16_max < u16_arr_len){return false;}
  if(px_q->u16_cnt > px_q->u16_max){
    px_q->u16_out = (px_q->u16_out + 1) & (px_q->u16_max - 1);
  }
  else{
    ++px_q->u16_cnt;
  }
  memcpy(px_q->p_buf[px_q->u16_in].pu8_arr, pu8_arr, u16_arr_len);
  px_q->p_buf[px_q->u16_in].u16_cnt = u16_arr_len;

  px_q->u16_in = (px_q->u16_in + 1) & (px_q->u16_max - 1);
  return true;
}

bool _b_BUF8Q_Get(_x_BUF8Q_t* px_q, uint8_t* pu8_arr, uint16_t* pu16_arr_len, uint16_t u16_arr_max){
  if(!px_q->u16_cnt){return false;}
  _x_BUF8_t* px_buf = &px_q->p_buf[px_q->u16_out];
  if(px_buf->u16_cnt > u16_arr_max){return false;}

  memcpy(pu8_arr, px_buf->pu8_arr, px_buf->u16_cnt);
  --px_q->u16_cnt;
  px_q->u16_out = (px_q->u16_out + 1) & (px_q->u16_max - 1);
  *pu16_arr_len = px_buf->u16_cnt;
  return true;
}







void _v_BUF16_Init(_x_BUF16_t* px_buf, uint16_t* pu16_arr, uint16_t u16_arr_len){
  px_buf->u16_cnt = 0;
  px_buf->u16_max = u16_arr_len;
  px_buf->pu16_arr = pu16_arr;
}

bool _b_BUF16Q_Init(_x_BUF16Q_t* px_q, uint16_t* pu16_arr, uint16_t u16_arr_len, uint16_t u16_q_cnt, uint16_t* pu16_heap){
  if(u16_arr_len < u16_q_cnt){return false;}
  uint16_t u16_HeapSize = sizeof(_x_BUF16_t) * u16_q_cnt;
  if(*pu16_heap < u16_HeapSize){return false;}
  uint16_t u16_i=0;
  uint16_t u16_len = u16_arr_len / u16_q_cnt; //mutiple of 2 .. cut.
  uint16_t u16_idx = 0;
  
  px_q->u16_max = u16_q_cnt;
  px_q->u16_cnt = px_q->u16_in = px_q->u16_out = 0;
  px_q->p_buf = (_x_BUF16_t*)malloc(u16_HeapSize);

  while(u16_q_cnt--){
    px_q->p_buf[u16_i].u16_cnt = 0;
    px_q->p_buf[u16_i].pu16_arr = pu16_arr + u16_idx;
    px_q->p_buf[u16_i++].u16_max = u16_len;
    u16_idx += u16_len;
  }
  return true;
}

bool _b_BUF16Q_Deinit(_x_BUF16Q_t* px_q, uint16_t* pu16_heap){
  if(px_q->p_buf == NULL){return false;}
  free(px_q->p_buf);
  *pu16_heap += sizeof(_x_BUF16_t) * px_q->u16_cnt;
  return true;
}

bool _b_BUF16Q_Put(_x_BUF16Q_t* px_q, uint16_t* pu16_arr, uint16_t u16_arr_len){
  //buffer size is samll than input array length
  if(px_q->p_buf[px_q->u16_in].u16_max < u16_arr_len){return false;}
  if(px_q->u16_cnt > px_q->u16_max){
    px_q->u16_out = (px_q->u16_out + 1) & (px_q->u16_max - 1);
  }
  else{
    ++px_q->u16_cnt;
  }
  memcpy(px_q->p_buf[px_q->u16_in].pu16_arr, pu16_arr, u16_arr_len);
  px_q->p_buf[px_q->u16_in].u16_cnt = u16_arr_len;

  px_q->u16_in = (px_q->u16_in + 1) & (px_q->u16_max - 1);
  return true;
}

bool _b_BUF16Q_Get(_x_BUF16Q_t* px_q, uint16_t* pu16_arr, uint16_t* pu16_arr_len, uint16_t u16_arr_max){
  if(!px_q->u16_cnt){return false;}
  _x_BUF16_t* px_buf = &px_q->p_buf[px_q->u16_out];
  if(px_buf->u16_cnt > u16_arr_max){return false;}

  memcpy(pu16_arr, px_buf->pu16_arr, px_buf->u16_cnt);
  --px_q->u16_cnt;
  px_q->u16_out = (px_q->u16_out + 1) & (px_q->u16_max - 1);
  *pu16_arr_len = px_buf->u16_cnt;
  return true;
}




void _v_BUF32_Init(_x_BUF32_t* px_buf, uint32_t* pu32_arr, uint16_t u16_arr_len){
  px_buf->u16_cnt = 0;
  px_buf->u16_max = u16_arr_len;
  px_buf->pu32_arr = pu32_arr;
}

bool _b_BUF32Q_Init(_x_BUF32Q_t* px_q, uint32_t* pu32_arr, uint16_t u16_arr_len, uint16_t u16_q_cnt, uint16_t* pu16_heap){
  if(u16_arr_len < u16_q_cnt){return false;}
  uint16_t u16_HeapSize = sizeof(_x_BUF16_t) * u16_q_cnt;
  if(*pu16_heap < u16_HeapSize){return false;}
  uint16_t u16_i=0;
  uint16_t u16_len = u16_arr_len / u16_q_cnt; //mutiple of 2 .. cut.
  uint16_t u16_idx = 0;
  
  px_q->u16_max = u16_q_cnt;
  px_q->u16_cnt = px_q->u16_in = px_q->u16_out = 0;
  px_q->p_buf = (_x_BUF32_t*)malloc(u16_HeapSize);

  while(u16_q_cnt--){
    px_q->p_buf[u16_i].u16_cnt = 0;
    px_q->p_buf[u16_i].pu32_arr = pu32_arr + u16_idx;
    px_q->p_buf[u16_i++].u16_max = u16_len;
    u16_idx += u16_len;
  }
  return true;
}

bool _b_BUF32Q_Deinit(_x_BUF32Q_t* px_q, uint16_t* pu16_heap){
  if(px_q->p_buf == NULL){return false;}
  free(px_q->p_buf);
  *pu16_heap += sizeof(_x_BUF32_t) * px_q->u16_cnt;
  return true;
}

bool _b_BUF32Q_Put(_x_BUF32Q_t* px_q, uint32_t* pu32_arr, uint16_t u16_arr_len){
  //buffer size is samll than input array length
  if(px_q->p_buf[px_q->u16_in].u16_max < u16_arr_len){return false;}
  if(px_q->u16_cnt > px_q->u16_max){
    px_q->u16_out = (px_q->u16_out + 1) & (px_q->u16_max - 1);
  }
  else{
    ++px_q->u16_cnt;
  }
  memcpy(px_q->p_buf[px_q->u16_in].pu32_arr, pu32_arr, u16_arr_len);
  px_q->p_buf[px_q->u16_in].u16_cnt = u16_arr_len;

  px_q->u16_in = (px_q->u16_in + 1) & (px_q->u16_max - 1);
  return true;
}

bool _b_BUF32Q_Get(_x_BUF32Q_t* px_q, uint32_t* pu32_arr, uint16_t* pu16_arr_len, uint16_t u16_arr_max){
  if(!px_q->u16_cnt){return false;}
  _x_BUF32_t* px_buf = &px_q->p_buf[px_q->u16_out];
  if(px_buf->u16_cnt > u16_arr_max){return false;}

  memcpy(pu32_arr, px_buf->pu32_arr, px_buf->u16_cnt);
  --px_q->u16_cnt;
  px_q->u16_out = (px_q->u16_out + 1) & (px_q->u16_max - 1);
  *pu16_arr_len = px_buf->u16_cnt;
  return true;
}