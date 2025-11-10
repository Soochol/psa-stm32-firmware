#ifndef __JH_LIB_CONSOLE_H
#define __JH_LIB_CONSOLE_H

#include "lib_def.h"

typedef struct _consoleNODE{
	struct _consoleNODE* next;
	char* pc_name;
	char* pc_hint;
	void (*v_fn)(const char*);
} _x_consoleNODE_t;

#define consoleNODE_CREATE(name, hint, fn)			\
	static const char* name = #name;		\
	static const char* name##_hint = #hint;	\
	static _x_consoleNODE_t name##_node = {	\
		.next = NULL,						\
		.pc_name = name,					\
		.pc_hint = name##_hint,				\
		.v_fn = fn,							\
	}

void _v_Console_NodeLink(_x_consoleNODE_t* px_head, _x_consoleNODE_t* px_node);
_x_consoleNODE_t* _px_Console_GetNode(_x_consoleNODE_t* px_head, const char* pc_name);
bool _b_Console_CallFunct(_x_consoleNODE_t* px_head, char* pc_text);
bool _b_Console_ShowName(_x_consoleNODE_t* px_head, char* pc_print, uint32_t u32_max);
bool _b_Console_Create(_x_consoleNODE_t* px_head, const char* pc_name, const char* pc_hint, void(*v_fn)(const char*), uint32_t* pu32_heap);


//example
void v_Console_Init();

#endif


