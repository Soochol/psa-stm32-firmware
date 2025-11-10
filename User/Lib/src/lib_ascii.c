#include "lib_ascii.h"
#include "string.h"


bool _b_Ascii_IsEnter(char c_word, char c_end, char* pc_buf, uint16_t u16_max){
	static uint16_t u16_in;
	static bool b_done;
	if(c_word < 0x80){
		//available
		if(b_done){
			b_done = false;
			memset(pc_buf, 0, u16_in);
			u16_in = 0;
		}
		//search delimiter
		if(c_word == c_end){
			b_done = true;
			return true;
		}

		if(u16_in < u16_max){
			pc_buf[u16_in++] = c_word;
		}
	}
	return false;
}














