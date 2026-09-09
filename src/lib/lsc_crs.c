#include "cri_xpt.h"

extern void SJCRS_Lock(void);
extern void SJCRS_Unlock(void);

void LSC_UnlockCrs(void)
{
	SJCRS_Unlock();
}

void LSC_LockCrs(void)
{
	SJCRS_Lock();
}
