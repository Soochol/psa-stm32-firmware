#include "lib_console.h"
#include "lib_tim.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"



void _v_Console_NodeLink(_x_consoleNODE_t* px_head, _x_consoleNODE_t* px_node){
	_x_consoleNODE_t* px_curr = px_head;
	while(px_curr->next != NULL){
		px_curr = px_curr->next;
	}
	px_curr->next = px_node;
}







_x_consoleNODE_t* _px_Console_GetNode(_x_consoleNODE_t* px_head, const char* pc_name){
	_x_consoleNODE_t* px_node = px_head;
	if(px_node != NULL)	{px_node = px_head->next;}
	else				{return NULL;}

	while(px_node != NULL){
		if(strcmp(px_node->pc_name, pc_name) == 0){
			return px_node;
		}
		else{
			px_node = px_node->next;
		}
	}
	return NULL;
}


bool _b_Console_CallFunct(_x_consoleNODE_t* px_head, char* pc_text){
	char* pc_tok = strchr(pc_text, ' ');
	if(pc_tok!= NULL){
		*pc_tok = 0;
	}
	_x_consoleNODE_t* px_node = _px_Console_GetNode(px_head, pc_text);
	if(px_node != NULL){
		if(pc_tok != NULL){
			px_node->v_fn(pc_tok + 1);
		}
		else{
			px_node->v_fn(NULL);
		}
		return true;
	}
	return false;
}

bool _b_Console_ShowName(_x_consoleNODE_t* px_head, char* pc_print, uint32_t u32_max){
	static _x_consoleNODE_t* px_node;
	if(px_head == NULL){
		px_node = px_node->next;
	}
	else{
		px_node = px_head;
		if(px_node != NULL)	{px_node = px_head->next;}
		else				{return false;}
	}

	if(px_node != NULL){
		uint32_t u32_len = strlen(px_node->pc_name) + strlen(px_node->pc_hint) + 3;//name + tap + hint + \r\n
		if(u32_len < u32_max){
			sprintf(pc_print, "%s\t%s\r\n", px_node->pc_hint, px_node->pc_hint);
			return true;
		}
	}
	return false;
}

/*
 *	name = "",
 *	hint = "",
 *	funt = "",
 * 	next = fn or NULL,
 * 	DEF()	//global position..
 *
 * 	NODE_DEF(&head, "gold", "gold medal", v_fn_gold);l
 */

#define NODE_DEF(head, name, hint, fn)	\
	static const char* name;	\



//182B
bool _b_Console_Create(_x_consoleNODE_t* px_head, const char* pc_name, const char* pc_hint, void(*v_fn)(const char*), uint32_t* pu32_heap){
	//size checkrg
	size_t x_len_name, x_len_hint, x_len_node;
	x_len_name = strlen(pc_name);
	x_len_hint = strlen(pc_hint);
	x_len_node = sizeof(_x_consoleNODE_t);
	if((x_len_name + x_len_hint + x_len_node) > *pu32_heap){return false;}
	//node
	_x_consoleNODE_t* px_end = (_x_consoleNODE_t*)malloc(x_len_node);
	//name
	px_end->pc_name = (char*)malloc(x_len_name + 1);
	memcpy(px_end->pc_name, pc_name, x_len_name);
	px_end->pc_name[x_len_name] = 0;
	//hint
	px_end->pc_hint = (char*)malloc(x_len_hint + 1);
	memcpy(px_end->pc_hint, pc_hint, x_len_hint);
	px_end->pc_hint[x_len_hint] = 0;
	//function
	px_end->v_fn = v_fn;
	//next
	px_end->next = NULL;
	//connect
	_x_consoleNODE_t* px_node = px_head;
	if(px_node != NULL){
		while(px_node->next != NULL){
			px_node = px_node->next;
		}
		px_node->next = px_end;
		//heap size
		*pu32_heap -= (uint32_t)(x_len_name + x_len_hint + x_len_node + 2);
		return true;
	}
	else{
		free(px_end);
		return false;
	}
}



















