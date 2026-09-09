#include "cri_xpt.h"

typedef struct ADXSTM_OBJ *ADXSTM;

extern Sint32 ADXSTM_GetStat(ADXSTM stm);
extern void ADXSTM_StopNw(ADXSTM stm);
extern void ADXSTM_ReleaseFileNw(ADXSTM stm);
extern void ADXSTM_Start(ADXSTM stm);
extern void ADXSTM_BindFileNw(ADXSTM stm, void *fs, const Char8 *fname, Sint32 ofst, Sint32 nsct);
extern void ADXSTM_SetEos(ADXSTM stm, Sint32 nsct);
extern void ADXSTM_Destroy(ADXSTM stm);
extern ADXSTM ADXSTM_Create(void *sj, Sint32 mode);
extern void ADXSTM_SetBufSize(ADXSTM stm, Sint32 nsct);

Sint32 MWSTM_GetStat(ADXSTM stm)
{
	return ADXSTM_GetStat(stm);
}

void MWSTM_ReqStop(ADXSTM stm)
{
	ADXSTM_StopNw(stm);
	ADXSTM_ReleaseFileNw(stm);
}

void MWSTM_ReqStart(ADXSTM stm)
{
	ADXSTM_Start(stm);
}

void MWSTM_SetFileRange(ADXSTM stm, void *fs, const Char8 *fname, Sint32 ofst, Sint32 nsct)
{
	ADXSTM_ReleaseFileNw(stm);
	ADXSTM_BindFileNw(stm, fs, fname, ofst, nsct);
	ADXSTM_SetEos(stm, nsct);
}

void MWSTM_Destroy(ADXSTM stm)
{
	ADXSTM_Destroy(stm);
}

ADXSTM MWSTM_Create(void *sj)
{
	return ADXSTM_Create(sj, 0);
}

void MWSTM_SetFlowLimit(ADXSTM stm, Sint32 nsct)
{
	if (stm != NULL) {
		ADXSTM_SetBufSize(stm, nsct);
	}
}

Bool MWSTM_IsFsStatErr(ADXSTM stm)
{
	return ADXSTM_GetStat(stm) == 4;
}

Sint32 MWSTM_InitStatic(void)
{
	return 0;
}
