#include "lib_ringbuf.h"



/*
 * brief	: ring buffer put single
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_PUT_FN_DEF(type)	\
	void _CONCAT_2(_v_RingPut_, type)(_RING_TYPE(type)* px, type src)			\
	{														\
		px->p_arr[px->u16_in] = src;						\
		px->u16_in = (px->u16_in + 1) & px->u16_mask;		\
		if(px->u16_cnt > px->u16_mask)						\
		{													\
			px->u16_out = (px->u16_out + 1) & px->u16_mask;	\
		}													\
		else												\
		{													\
			++px->u16_cnt;									\
		}													\
	}

/*
 * brief	: ring buffer put array
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_PUT_ARR_FN_DEF(type)	\
	void _CONCAT_2(_v_RingPutArr_, type)(_RING_TYPE(type)* px, type* p_srcArr, uint16_t u16_srcCnt)	\
	{																			\
		uint16_t rem = u16_srcCnt;												\
		if(u16_srcCnt > px->u16_mask)											\
		{																		\
			px->u16_in = (px->u16_in + u16_srcCnt) & px->u16_mask;				\
			rem = px->u16_mask + 1;												\
			p_srcArr += u16_srcCnt - (px->u16_mask + 1);						\
		}																		\
		while(rem--)															\
		{																		\
			px->p_arr[px->u16_in] = *p_srcArr++;								\
			px->u16_in = (px->u16_in + 1) & px->u16_mask;						\
		}																		\
		if(px->u16_cnt + u16_srcCnt > px->u16_mask)								\
		{																		\
			px->u16_out = (px->u16_out + u16_srcCnt) & px->u16_mask;			\
			px->u16_cnt = px->u16_mask + 1;										\
		}																		\
		else																	\
		{																		\
			px->u16_cnt += u16_srcCnt;											\
		}																		\
	}

/*
 * brief	: ring buffer get single
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_GET_FN_DEF(type)\
	bool _CONCAT_2(_b_RingGet_, type)(_RING_TYPE(type)* px, type* p_dst)	\
	{																		\
		if(px->u16_cnt == 0){return false;}									\
		*p_dst = px->p_arr[px->u16_out];									\
		px->u16_out = (px->u16_out + 1) & px->u16_mask;						\
		px->u16_cnt--;														\
		return true;														\
	}

/*
 * brief	: ring buffer get array
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_GET_ARR_FN_DEF(type)\
	bool _CONCAT_2(_b_RingGetArr_, type)(_RING_TYPE(type)* px, type* p_dstArr, uint16_t u16_dstCnt)	\
	{																								\
		if(px->u16_cnt < u16_dstCnt){return false;}													\
		uint16_t rem = u16_dstCnt;																	\
		while(rem--)																				\
		{																							\
			*p_dstArr++ = px->p_arr[px->u16_out];													\
			px->u16_out = (px->u16_out + 1) & px->u16_mask;											\
		}																							\
		px->u16_cnt -= u16_dstCnt;																	\
		return true;																				\
	}


/*
 * brief	: ring buffer get reference single
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_GET_REF_FN_DEF(type)\
	bool _CONCAT_2(_b_RingGetRef_, type)(_RING_TYPE(type)* px, type* p_dst, uint16_t u16_idx)	\
	{																							\
		if(u16_idx > px->u16_mask){return false;}												\
		*p_dst = px->p_arr[u16_idx];															\
		return true;																			\
	}


/*
 * brief	: ring buffer get reference array
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_GET_REF_ARR_FN_DEF(type)\
	bool _CONCAT_2(_b_RingGetRefArr_, type)(_RING_TYPE(type)* px, type* p_dstArr, uint16_t u16_dstCnt, uint16_t u16_idx)	\
	{																														\
		if(u16_idx > px->u16_mask){return false;}																			\
		while(u16_dstCnt--)																									\
		{																													\
			*p_dstArr++ = px->p_arr[u16_idx];																				\
			u16_idx = (u16_idx + 1) & px->u16_mask;																			\
		}																													\
		return true;																										\
	}

/*
 * brief	: ring buffer out index jump
 * date
 * create	: 25.04.10
 * modify	: -
 */
#define _RING_JMP_FN_DEF(type)\
	bool _CONCAT_2(_b_RingJmp_, type)(_RING_TYPE(type)* px, uint16_t u16_jmpCnt)	\
	{																				\
		if(u16_jmpCnt > px->u16_cnt){return false;}									\
		px->u16_out = (px->u16_out + u16_jmpCnt) & px->u16_mask;					\
		px->u16_cnt -= u16_jmpCnt;													\
		return true;																\
	}


#define _RING_FN_DEF(type)\
	_RING_PUT_FN_DEF(type)			\
	_RING_PUT_ARR_FN_DEF(type)		\
	_RING_GET_FN_DEF(type)			\
	_RING_GET_ARR_FN_DEF(type)		\
	_RING_GET_REF_FN_DEF(type)		\
	_RING_GET_REF_ARR_FN_DEF(type)	\
	_RING_JMP_FN_DEF(type)


_RING_FN_DEF(int8_t)
_RING_FN_DEF(uint8_t)
_RING_FN_DEF(int16_t)
_RING_FN_DEF(uint16_t)
_RING_FN_DEF(int32_t)
_RING_FN_DEF(uint32_t)
_RING_FN_DEF(float)



/*
 * brief	: ring buffer : matrix
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MATRIX_PUT_FN_DEF(typeX, typeY)	\
	void _CONCAT_4(_v_RingPut_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX srcX, typeY srcY)			\
	{														\
		px->p_arrX[px->u16_in] = srcX;						\
		px->p_arrY[px->u16_in] = srcY;						\
		px->u16_in = (px->u16_in + 1) & px->u16_mask;		\
		if(px->u16_cnt > px->u16_mask)						\
		{													\
			px->u16_out = (px->u16_out + 1) & px->u16_mask;	\
		}													\
		else												\
		{													\
			++px->u16_cnt;									\
		}													\
	}

/*
 * brief	: ring buffer put array
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MATRIX_PUT_ARR_FN_DEF(typeX, typeY)	\
	void _CONCAT_4(_v_RingPutArr_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_srcArrX, typeY* p_srcArrY, uint16_t u16_srcCnt)	\
	{																			\
		uint16_t rem = u16_srcCnt;												\
		if(u16_srcCnt > px->u16_mask)											\
		{																		\
			px->u16_in = (px->u16_in + u16_srcCnt) & px->u16_mask;				\
			rem = px->u16_mask + 1;												\
			p_srcArrX += u16_srcCnt - (px->u16_mask + 1);						\
			p_srcArrY += u16_srcCnt - (px->u16_mask + 1);						\
		}																		\
		while(rem--)															\
		{																		\
			px->p_arrX[px->u16_in] = *p_srcArrX++;								\
			px->p_arrY[px->u16_in] = *p_srcArrY++;								\
			px->u16_in = (px->u16_in + 1) & px->u16_mask;						\
		}																		\
		if(px->u16_cnt + u16_srcCnt > px->u16_mask)								\
		{																		\
			px->u16_out = (px->u16_out + u16_srcCnt) & px->u16_mask;			\
			px->u16_cnt = px->u16_mask + 1;										\
		}																		\
		else																	\
		{																		\
			px->u16_cnt += u16_srcCnt;											\
		}																		\
	}

/*
 * brief	: ring buffer get single
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MATRIX_GET_FN_DEF(typeX, typeY)\
	bool _CONCAT_4(_b_RingGet_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstX, typeY* p_dstY)	\
	{																		\
		if(px->u16_cnt == 0){return false;}									\
		*p_dstX = px->p_arrX[px->u16_out];									\
		*p_dstY = px->p_arrY[px->u16_out];									\
		px->u16_out = (px->u16_out + 1) & px->u16_mask;						\
		px->u16_cnt--;														\
		return true;														\
	}

/*
 * brief	: ring buffer get array
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MATRIX_GET_ARR_FN_DEF(typeX, typeY)\
	bool _CONCAT_4(_b_RingGetArr_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstArrX, typeY* p_dstArrY, uint16_t u16_dstCnt)	\
	{																								\
		if(px->u16_cnt < u16_dstCnt){return false;}													\
		uint16_t rem = u16_dstCnt;																	\
		while(rem--)																				\
		{																							\
			*p_dstArrX++ = px->p_arrX[px->u16_out];													\
			*p_dstArrY++ = px->p_arrY[px->u16_out];													\
			px->u16_out = (px->u16_out + 1) & px->u16_mask;											\
		}																							\
		px->u16_cnt -= u16_dstCnt;																	\
		return true;																				\
	}


/*
 * brief	: ring buffer get reference single
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MATRIX_GET_REF_FN_DEF(typeX, typeY)\
	bool _CONCAT_4(_b_RingGetRef_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstX, typeY* p_dstY, uint16_t u16_idx)	\
	{																							\
		if(u16_idx > px->u16_mask){return false;}												\
		*p_dstX = px->p_arrX[u16_idx];															\
		*p_dstY = px->p_arrY[u16_idx];															\
		return true;																			\
	}


/*
 * brief	: ring buffer get reference array
 * date
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MATRIX_GET_REF_ARR_FN_DEF(typeX, typeY)\
	bool _CONCAT_4(_b_RingGetRefArr_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstArrX, typeY* p_dstArrY, uint16_t u16_dstCnt, uint16_t u16_idx)	\
	{																														\
		if(u16_idx > px->u16_mask){return false;}																			\
		while(u16_dstCnt--)																									\
		{																													\
			*p_dstArrX++ = px->p_arrX[u16_idx];																				\
			*p_dstArrY++ = px->p_arrY[u16_idx];																				\
			u16_idx = (u16_idx + 1) & px->u16_mask;																			\
		}																													\
		return true;																										\
	}

/*
 * brief	: ring buffer out index jump
 * date
 * create	: 25.04.10
 * modify	: -
 */
#define _RING_MATRIX_JMP_FN_DEF(typeX, typeY)\
	bool _CONCAT_4(_b_RingJmp_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, uint16_t u16_jmpCnt)\
	{																\
		if(u16_jmpCnt > px->u16_cnt){return false;}					\
		px->u16_out = (px->u16_out + u16_jmpCnt) & px->u16_mask;	\
		px->u16_cnt -= u16_jmpCnt;									\
		return true;												\
	}


#define _RING_MATRIX_FN_DEF(typeX, typeY)\
	_RING_MATRIX_PUT_FN_DEF(typeX, typeY)			\
	_RING_MATRIX_PUT_ARR_FN_DEF(typeX, typeY)		\
	_RING_MATRIX_GET_FN_DEF(typeX, typeY)			\
	_RING_MATRIX_GET_ARR_FN_DEF(typeX, typeY)		\
	_RING_MATRIX_GET_REF_FN_DEF(typeX, typeY)		\
	_RING_MATRIX_GET_REF_ARR_FN_DEF(typeX, typeY)	\
	_RING_MATRIX_JMP_FN_DEF(typeX, typeY)

_RING_MATRIX_FN_DEF(uint32_t, float)

































