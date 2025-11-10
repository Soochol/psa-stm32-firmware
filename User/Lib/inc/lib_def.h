#ifndef __JH_LIB_DEF_H
#define __JH_LIB_DEF_H



#ifndef bool
#include "stdbool.h"
#endif

#ifndef uint8_t
#include "stdint.h"
#endif

#ifndef size_t
#include "stddef.h"
#endif

#ifndef CLR_BIT
#define CLR_BIT(port, bit)	((port) &= ~(bit))
#endif

#ifndef SET_BIT
#define SET_BIT(port, bit)	((port) |= (bit))
#endif

#ifndef CHK_BIT
#define CHK_BIT(port, bit)	((port) & (bit))
#endif

#define IS_VALUE(val)	(val ? 1 : 0)


#define MAX(x,y)	(x > y ? x : y)
#define MIN(x,y)	(x < y ? x : y)


//pointer return
#define _CONCAT_2(p1,p2)		p1##p2
#define _CONCAT_3(p1,p2,p3)		p1##p2##p3
#define _CONCAT_4(p1,p2,p3,p4)	p1##p2##p3##p4



typedef enum {
	COMM_STAT_READY=0,
	COMM_STAT_OK,
	COMM_STAT_BUSY,
	COMM_STAT_ERR,
	COMM_STAT_DONE,
	COMM_STAT_ERR_LEN,
	COMM_STAT_FIFO_IN,
	COMM_STAT_FIFO_FULL,
} e_COMM_STAT_t;

typedef enum {
	BUS_STAT_OK=0,
	BUS_STAT_BUSY,
	BUS_STAT_ERR,
} e_BUS_STAT_t;


#endif



