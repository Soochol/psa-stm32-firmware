#ifndef __JH_LIB_RINGBUF_H
#define __JH_LIB_RINGBUF_H

#include "lib_def.h"




/*
 * brief	: ring buffer : uint32_t
 */

#define _RING_TYPE(type)	\
	_CONCAT_2(_x_RING_, type)

#define _RING_TYPE_DEF(type)	\
	typedef struct _RING_TYPE(type)	_RING_TYPE(type);																\
	typedef void(*_CONCAT_2(_v_RingPutPtr_, type))(_RING_TYPE(type)* px, type src);												\
	typedef void(*_CONCAT_2(_v_RingPutArrPtr_, type))(_RING_TYPE(type)* px, type* srcArr, uint16_t srcCnt);						\
	typedef bool(*_CONCAT_2(_b_RingGetPtr_, type))(_RING_TYPE(type)* px, type* dst);											\
	typedef bool(*_CONCAT_2(_b_RingGetArrPtr_, type))(_RING_TYPE(type)* px, type* dstArr, uint16_t dstCnt);						\
	typedef bool(*_CONCAT_2(_b_RingGetRefPtr_, type))(_RING_TYPE(type)* px, type* dst, uint16_t idx);							\
	typedef bool(*_CONCAT_2(_b_RingGetRefArrPtr_, type))(_RING_TYPE(type)* px, type* dstArr, uint16_t dstCnt, uint16_t idx);	\
	typedef bool(*_CONCAT_2(_b_RingJmpPtr_, type))(_RING_TYPE(type)* px, uint16_t jmpCnt);										\
	struct _RING_TYPE(type) {	\
		type *p_arr;												\
		uint16_t u16_in, u16_out, u16_cnt;							\
		uint16_t u16_mask;											\
		struct {													\
			_CONCAT_2(_v_RingPutPtr_, type)			v_Put;			\
			_CONCAT_2(_v_RingPutArrPtr_, type)		v_PutArr;		\
			_CONCAT_2(_b_RingGetPtr_, type)			b_Get;			\
			_CONCAT_2(_b_RingGetArrPtr_, type)		b_GetArr;		\
			_CONCAT_2(_b_RingGetRefPtr_, type)		b_GetRef;		\
			_CONCAT_2(_b_RingGetRefArrPtr_, type)	b_GetRefArr;	\
			_CONCAT_2(_b_RingJmpPtr_, type)			b_Jmp;			\
		}fn;														\
	}

#define _RING_VAR_DEF(name, type, cnt)	\
	type _CONCAT_2(name, Arr)[cnt+1];							\
	_RING_TYPE(type) _CONCAT_2(name, Ring) = {			\
		.p_arr = _CONCAT_2(name, Arr),							\
		.u16_mask = cnt - 1,									\
		.u16_in = 0,											\
		.u16_out = 0,											\
		.u16_cnt = 0,											\
		.fn.v_Put = _CONCAT_2(_v_RingPut_, type),				\
		.fn.v_PutArr = _CONCAT_2(_v_RingPutArr_, type),			\
		.fn.b_Get = _CONCAT_2(_b_RingGet_, type),				\
		.fn.b_GetArr = _CONCAT_2(_b_RingGetArr_, type),			\
		.fn.b_GetRef = _CONCAT_2(_b_RingGetRef_, type),			\
		.fn.b_GetRefArr = _CONCAT_2(_b_RingGetRefArr_, type),	\
		.fn.b_Jmp = _CONCAT_2(_b_RingJmp_, type)				\
	};															\
	_RING_TYPE(type)* name = &_CONCAT_2(name, Ring)

#define _RING_FN_DEC(type)	\
	void _CONCAT_2(_v_RingPut_, type)(_RING_TYPE(type)* px, type src);														\
	void _CONCAT_2(_v_RingPutArr_, type)(_RING_TYPE(type)* px, type* p_srcArr, uint16_t u16_srcCnt);						\
	bool _CONCAT_2(_b_RingGet_, type)(_RING_TYPE(type)* px, type* p_dst);													\
	bool _CONCAT_2(_b_RingGetArr_, type)(_RING_TYPE(type)* px, type* p_dstArr, uint16_t u16_dstCnt);						\
	bool _CONCAT_2(_b_RingGetRef_, type)(_RING_TYPE(type)* px, type* p_dst, uint16_t u16_idx);								\
	bool _CONCAT_2(_b_RingGetRefArr_, type)(_RING_TYPE(type)* px, type* p_dstArr, uint16_t u16_dstCnt, uint16_t u16_idx);	\
	bool _CONCAT_2(_b_RingJmp_, type)(_RING_TYPE(type)* px, uint16_t u16_jmpCnt)




_RING_TYPE_DEF(int8_t);
_RING_TYPE_DEF(uint8_t);
_RING_TYPE_DEF(int16_t);
_RING_TYPE_DEF(uint16_t);
_RING_TYPE_DEF(int32_t);
_RING_TYPE_DEF(uint32_t);
_RING_TYPE_DEF(float);


_RING_FN_DEC(int8_t);
_RING_FN_DEC(uint8_t);
_RING_FN_DEC(int16_t);
_RING_FN_DEC(uint16_t);
_RING_FN_DEC(int32_t);
_RING_FN_DEC(uint32_t);
_RING_FN_DEC(float);






/*
 * brief	: ring buffer : matrix
 * create	: 25.03.19
 * modify	: -
 */
#define _RING_MAT_TYPE(typeX, typeY)	\
	_CONCAT_4(_x_RING_MAT_X, typeX, _Y, typeY)

#define _RING_MATRIX_TYPE_DEF(typeX, typeY)	\
	typedef struct _RING_MAT_TYPE(typeX, typeY)	_RING_MAT_TYPE(typeX, typeY);																								\
	typedef void(*_CONCAT_4(_v_RingPutPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, typeX srcX, typeY srcY);													\
	typedef void(*_CONCAT_4(_v_RingPutArrPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, typeX* srcArrX, typeY* srcArrY, uint16_t srcCnt);						\
	typedef bool(*_CONCAT_4(_b_RingGetPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, typeX* dstX, typeY* dstY);												\
	typedef bool(*_CONCAT_4(_b_RingGetArrPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, typeX* dstX, typeY* dstY, uint16_t u16_dstCnt);						\
	typedef bool(*_CONCAT_4(_b_RingGetRefPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, typeX* dstX, typeY* dstY, uint16_t u16_idx);							\
	typedef bool(*_CONCAT_4(_b_RingGetRefArrPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, typeX* dstX, typeY* dstY, uint16_t u16_dstCnt, uint16_t u16_idx);	\
	typedef bool(*_CONCAT_4(_b_RingJmpPtr_X, typeX, _Y, typeY))(_RING_MAT_TYPE(typeX, typeY)* px, uint16_t u16_jmpCnt);														\
	struct _RING_MAT_TYPE(typeX, typeY)										\
	{																		\
		typeX *p_arrX;														\
		typeY *p_arrY;														\
		uint16_t u16_in, u16_out, u16_cnt;									\
		uint16_t u16_mask;													\
		struct {															\
			_CONCAT_4(_v_RingPutPtr_X, typeX, _Y, typeY)		v_Put;		\
			_CONCAT_4(_v_RingPutArrPtr_X, typeX, _Y, typeY)		v_PutArr;	\
			_CONCAT_4(_b_RingGetPtr_X, typeX, _Y, typeY)		b_Get;		\
			_CONCAT_4(_b_RingGetArrPtr_X, typeX, _Y, typeY)		b_GetArr;	\
			_CONCAT_4(_b_RingGetRefPtr_X, typeX, _Y, typeY)		b_GetRef;	\
			_CONCAT_4(_b_RingGetRefArrPtr_X, typeX, _Y, typeY)	b_GetRefArr;\
			_CONCAT_4(_b_RingJmpPtr_X, typeX, _Y, typeY)		b_Jmp;		\
		}fn;																\
	}

#define _RING_MAT_VAR_DEF(name, typeX, typeY, cnt)\
	typeX _CONCAT_2(name, ArrX)[cnt+1];											\
	typeY _CONCAT_2(name, ArrY)[cnt+1];											\
	_RING_MAT_TYPE(typeX, typeY) _CONCAT_2(name, Ring) = {						\
		.p_arrX = _CONCAT_2(name, ArrX),										\
		.p_arrY = _CONCAT_2(name, ArrY),										\
		.u16_mask = cnt - 1,													\
		.u16_in = 0,															\
		.u16_out = 0,															\
		.u16_cnt = 0,															\
		.fn.v_Put = _CONCAT_4(_v_RingPut_X, typeX, _Y, typeY),					\
		.fn.v_PutArr = _CONCAT_4(_v_RingPutArr_X, typeX, _Y, typeY),			\
		.fn.b_Get = _CONCAT_4(_b_RingGet_X, typeX, _Y, typeY),					\
		.fn.b_GetArr = _CONCAT_4(_b_RingGetArr_X, typeX, _Y, typeY),			\
		.fn.b_GetRef = _CONCAT_4(_b_RingGetRef_X, typeX, _Y, typeY),			\
		.fn.b_GetRefArr = _CONCAT_4(_b_RingGetRefArr_X, typeX, _Y, typeY),		\
		.fn.b_Jmp = _CONCAT_4(_b_RingJmp_X, typeX, _Y, typeY)					\
	};																			\
	_RING_MAT_TYPE(typeX, typeY)* name = &_CONCAT_2(name, Ring)

#define _RING_MAT_FN_DEC(typeX, typeY)	\
	void _CONCAT_4(_v_RingPut_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX srcX, typeY srcY);										\
	void _CONCAT_4(_v_RingPutArr_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_srcArrX, typeY* p_srcArrY, uint16_t u16_srcCnt);	\
	bool _CONCAT_4(_b_RingGet_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstX, typeY* p_dstY);									\
	bool _CONCAT_4(_b_RingGetArr_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstArrX, typeY* p_dstArrY, uint16_t u16_dstCnt);	\
	bool _CONCAT_4(_b_RingGetRef_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstX, typeY* p_dstY, uint16_t u16_idx);			\
	bool _CONCAT_4(_b_RingGetRefArr_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, typeX* p_dstArrX, typeY* p_dstArrY, uint16_t u16_dstCnt, uint16_t u16_idx);\
	bool _CONCAT_4(_b_RingJmp_X, typeX, _Y, typeY)(_RING_MAT_TYPE(typeX, typeY)* px, uint16_t u16_jmpCnt)


_RING_MATRIX_TYPE_DEF(uint32_t, float);
_RING_MAT_FN_DEC(uint32_t, float);






#endif



