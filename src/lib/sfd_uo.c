#include "sfd.h"

Sint32 SFUO_Seek(void)
{
	return 0;
}

Sint32 SFUO_AddRead(SFD sfd)
{
	SFLIB_SetErr(sfd, 0xFF000601);
}

Sint32 SFUO_GetRead(SFD sfd)
{
	SFLIB_SetErr(sfd, 0xFF000601);
}

Sint32 SFUO_AddWrite(SFD sfd)
{
	SFLIB_SetErr(sfd, 0xFF000601);
}

Sint32 SFUO_GetWrite(SFD sfd)
{
	SFLIB_SetErr(sfd, 0xFF000601);
}

Sint32 SFUO_Pause(void)
{
	return 0;
}

Sint32 SFUO_Stop(void)
{
	return 0;
}

Sint32 SFUO_Start(void)
{
	return 0;
}

Sint32 SFUO_Standby(void)
{
	return 0;
}

Sint32 SFUO_Destroy(void)
{
	return 0;
}

Sint32 SFUO_Create(SFD sfd)
{
	Sint32 i;
	SFUO *uo;
	Sint32 uobuf;

	uo = &sfd->uo_tbl;
	sfd->uo = uo;
	uobuf = sfd->uobuf;
	sfd->uo_tbl.nch = 0;
	for (i = 0; i < 3; i++) {
		uo->ch[i].sj = NULL;
		uo->ch[i].prm = NULL;
		uo->ch[i].rsv1 = 0;
		uo->ch[i].rsv2 = 0;
		SFBUF_SetUoch(sfd, uobuf, i, &uo->ch[i]);
	}
	return 0;
}

Sint32 SFUO_ExecServer(SFD sfd)
{
	if (SFTRN_GetTermFlg(sfd, 8) != 1) {
		if (SFBUF_GetTermFlg(sfd, sfd->uobuf) == 1) {
			SFTRN_SetTermFlg(sfd, 8, 1);
		}
	}
	if (SFTRN_GetPrepFlg(sfd, 8) != 1) {
		if (SFBUF_GetPrepFlg(sfd, sfd->uobuf) == 1) {
			SFTRN_SetPrepFlg(sfd, 8, 1);
		}
	}
	return 0;
}

Sint32 SFUO_Finish(void)
{
	return 0;
}

Sint32 SFUO_Init(void)
{
	return 0;
}

Sint32 SFD_SetUsrSj(SFD sfd, Sint32 chno, void *sj, void *prm)
{
	SFUO *uo;
	Sint32 uobuf;

	if (SFLIB_CheckHn(sfd) != 0) {
		return SFLIB_SetErr(NULL, 0xFF000191);
	}
	uobuf = sfd->uobuf;
	uo = sfd->uo;
	if (uobuf == 8) {
		return SFLIB_SetErr(sfd, 0xFF000602);
	}
	uo->ch[chno].sj = sj;
	uo->ch[chno].prm = prm;
	uo->ch[chno].rsv1 = 0;
	uo->ch[chno].rsv2 = 0;
	SFBUF_SetUoch(sfd, uobuf, chno, &uo->ch[chno]);
	return 0;
}

const SFD_TR_IF SFD_tr_uo = {
	SFUO_Init,
	SFUO_Finish,
	SFUO_ExecServer,
	SFUO_Create,
	SFUO_Destroy,
	SFUO_Standby,
	SFUO_Start,
	SFUO_Stop,
	SFUO_Pause,
	SFUO_GetWrite,
	SFUO_AddWrite,
	SFUO_GetRead,
	SFUO_AddRead,
	SFUO_Seek,
};
