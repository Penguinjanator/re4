/* Sofdec: condition (setting) table access */
#include "cri_xpt.h"
#include "sfd.h"

Sint32 SFD_GetTrHn(SFD sfd, Sint32 strm, void **hn)
{
	void **trhn;

	if (SFLIB_CheckHn(sfd) != 0) {
		return SFLIB_SetErr(NULL, 0xFF000117);
	}
	trhn = sfd->tr[strm].hn;
	if (trhn == NULL) {
		*hn = NULL;
	} else {
		*hn = *trhn;
	}
	return 0;
}

Sint32 SFD_GetPlyInf(SFD sfd, SFD_PLYINF *inf)
{
	if (SFLIB_CheckHn(sfd) != 0) {
		return SFLIB_SetErr(NULL, 0xFF000119);
	}
	*inf = sfd->plyinf;
	return 0;
}

Sint32 SFSET_GetCond(SFD sfd, Sint32 id)
{
	return sfd->cond[id];
}

Sint32 SFD_GetCond(SFD sfd, Sint32 id, Sint32 *val)
{
	if (sfd == NULL) {
		*val = SFLIB_libwork.cond[id];
	} else {
		if (SFLIB_CheckHn(sfd) != 0) {
			return SFLIB_SetErr(NULL, 0xFF000113);
		}
		*val = sfd->cond[id];
	}
	return 0;
}

static Sint32 sfset_IsSettable(SFD sfd, Sint32 id, Sint32 val)
{
	Sint32 ok;

	if (id == 6 && val == 1 && SFTRN_IsSetup(sfd, 3) == 0) {
		ok = 0;
	} else if (id == 5 && val == 1 && SFTRN_IsSetup(sfd, 2) == 0) {
		ok = 0;
	} else {
		ok = 1;
	}
	return ok;
}

void SFSET_SetCond(SFD sfd, Sint32 id, Sint32 val)
{
	if (sfset_IsSettable(sfd, id, val)) {
		sfd->cond[id] = val;
	}
}

static void sfset_SetCondDef(SFD sfd, Sint32 id, Sint32 val)
{
	if (sfset_IsSettable(sfd, id, val)) {
		sfd->cond_def[id] = val;
	}
}

/* COMPILER-DIFF: M1 -- the original hoists id*4 into the dead sfd register (r28) and gives hn r31;
 * ours ranks the compiler temporary first. An asm-defined `register` local takes the freed parameter
 * register, and hn declared first takes r31. */
Sint32 SFD_SetCond(SFD sfd, register Sint32 id, Sint32 val)
{
	SFD hn;
	Sint32 i;
	SFD *p;
	register Sint32 ofs; /* COMPILER-DIFF: M1 */

	if (sfd == NULL) {
		p = SFLIB_libwork.hn;
		asm { slwi ofs, id, 2 } /* COMPILER-DIFF: M1 */
		for (i = 0; i < 8; i++, p++) {
			hn = *p;
			if (SFLIB_CheckHn(hn) == 0) {
				SFSET_SetCond(hn, id, val);
			}
		}
		*(Sint32 *)((Uint8 *)SFLIB_libwork.cond + ofs) = val; /* COMPILER-DIFF: M1: SFLIB_libwork.cond[id] = val */
	} else {
		if (SFLIB_CheckHn(sfd) != 0) {
			return SFLIB_SetErr(NULL, 0xFF000112);
		}
		SFSET_SetCond(sfd, id, val);
		sfset_SetCondDef(sfd, id, val);
	}
	return 0;
}

Sint32 SFD_GetHnStat(SFD sfd)
{
	if (SFLIB_CheckHn(sfd) != 0) {
		SFLIB_SetErr(NULL, 0xFF000111);
	}
	return sfd->stat;
}
