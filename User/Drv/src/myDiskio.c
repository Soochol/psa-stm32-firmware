#include "myDiskio.h"

extern Disk_drvTypeDef disk;

DSTATUS disk_reinitialize(BYTE pdrv)
{
	disk.is_initialized[pdrv] = 0;
	return RES_OK;
}
