/* CRI Sofdec MPEG video: 8x8 block motion compensation, "tuned C" versions. One reference block is
 * copied (1p), horizontally (H2) / vertically (V2) / four-point (4p) half-pel averaged into the
 * word-packed destination; the source alignment selects the load strategy. The bodies are still the
 * original's instruction stream as inline assembly (integer SWAR code: update-form loads, lwbrx byte
 * reversal, byte averages `(w & a) + (x & 0x01010101) + ((x & 0xFEFEFEFE) >> 1)` with x = w ^ a,
 * four-point sums `a0 + a1 + b0 + b1 + 2` packed with rlwinm/rlwimi); OneRef1p keeps its alignment
 * switch in C with register variables in the asm. The pragmas below do not affect asm bodies. The
 * C reconstruction (AGENTS.md "CRI paired-single kernels pass 1") reproduces every arithmetic tree
 * and the loop shapes but not the original's instruction schedule / register assignment, so the
 * asm stays until that is understood; mpv_mcy.c holds the pure-C 16x16 counterparts. */
#include "cri_xpt.h"
#include "mpv.h"

void MPVMC08_OneRef4p_TuneC(MPVMC *mc)
{
	asm {
		li r0, 0x8
		lwz r4, 0x20(r3)
		lwz r5, 0x24(r3)
		lwz r6, 0x28(r3)
		lwz r3, 0x18(r3)
		mtctr r0
L_80217908:
		lbz r9, 0x0(r5)
		lbz r0, 0x0(r6)
		dcbt r6, r4
		lbz r10, 0x1(r5)
		lbz r7, 0x1(r6)
		add r0, r10, r0
		lbz r31, 0x2(r5)
		add r8, r0, r7
		lbz r11, 0x2(r6)
		addi r27, r8, 0x2
		add r0, r31, r7
		add r7, r0, r11
		lbz r28, 0x3(r5)
		addi r30, r7, 0x2
		add r27, r9, r27
		add r30, r10, r30
		add r7, r28, r11
		rlwinm r11, r30, 14, 8, 15
		lbz r8, 0x3(r6)
		lbz r29, 0x4(r5)
		rlwimi r11, r27, 22, 0, 7
		add r7, r7, r8
		lbz r9, 0x4(r6)
		add r8, r29, r8
		lbz r30, 0x5(r5)
		add r10, r8, r9
		addi r7, r7, 0x2
		add r7, r31, r7
		lbz r12, 0x5(r6)
		add r8, r30, r9
		lbz r0, 0x6(r5)
		add r9, r8, r12
		lbz r31, 0x6(r6)
		addi r8, r10, 0x2
		add r10, r0, r12
		addi r9, r9, 0x2
		lbz r12, 0x8(r5)
		add r9, r29, r9
		lbz r29, 0x7(r5)
		add r8, r28, r8
		add r10, r10, r31
		addi r27, r10, 0x2
		lbz r28, 0x7(r6)
		add r10, r29, r31
		lbz r31, 0x8(r6)
		add r12, r12, r28
		rlwimi r11, r7, 6, 16, 23
		add r10, r10, r28
		add r27, r30, r27
		add r12, r12, r31
		rlwimi r11, r8, 30, 24, 31
		addi r10, r10, 0x2
		rlwinm r7, r27, 14, 8, 15
		addi r12, r12, 0x2
		stw r11, 0x0(r3)
		add r0, r0, r10
		rlwimi r7, r9, 22, 0, 7
		add r12, r29, r12
		add r5, r5, r4
		rlwimi r7, r0, 6, 16, 23
		add r6, r6, r4
		rlwimi r7, r12, 30, 24, 31
		stw r7, 0x4(r3)
		addi r3, r3, 0x8
		bdnz L_80217908
	}
}

#pragma scheduling off

void MPVMC08_OneRefH2_TuneC(MPVMC *mc)
{
	asm {
		lwz r7, 0x24(r3)
		lis r5, 0xfeff
		lis r4, 0x101
		lwz r6, 0x18(r3)
		clrlwi r0, r7, 30
		lwz r3, 0x20(r3)
		cmpwi r0, 0x2
		subi r11, r5, 0x102
		addi r12, r4, 0x101
		beq L_80217C2C
		bge L_80217A54
		cmpwi r0, 0x0
		beq L_80217A60
		bge L_80217B34
		blr
L_80217A54:
		cmpwi r0, 0x4
		bgelr
		b L_80217D2C
L_80217A60:
		li r0, 0x4
		mtctr r0
L_80217A68:
		dcbt r7, r3
		lwz r4, 0x0(r7)
		lwz r8, 0x4(r7)
		lbz r9, 0x8(r7)
		slwi r5, r4, 8
		rlwimi r5, r8, 8, 24, 31
		rlwimi r9, r8, 8, 0, 23
		xor r10, r4, r5
		add r7, r7, r3
		and r4, r4, r5
		xor r0, r8, r9
		and r5, r10, r11
		and r10, r10, r12
		and r8, r8, r9
		and r9, r0, r11
		and r0, r0, r12
		srwi r5, r5, 1
		add r4, r4, r10
		srwi r9, r9, 1
		add r4, r4, r5
		add r8, r8, r0
		stw r4, 0x0(r6)
		add r8, r8, r9
		stw r8, 0x4(r6)
		dcbt r7, r3
		lwz r4, 0x0(r7)
		lwz r8, 0x4(r7)
		lbz r9, 0x8(r7)
		slwi r5, r4, 8
		rlwimi r5, r8, 8, 24, 31
		rlwimi r9, r8, 8, 0, 23
		xor r10, r4, r5
		add r7, r7, r3
		and r4, r4, r5
		xor r0, r8, r9
		and r5, r10, r11
		and r10, r10, r12
		and r8, r8, r9
		and r9, r0, r11
		and r0, r0, r12
		srwi r5, r5, 1
		add r4, r4, r10
		srwi r9, r9, 1
		add r4, r4, r5
		add r8, r8, r0
		stw r4, 0x8(r6)
		add r8, r8, r9
		stw r8, 0xc(r6)
		addi r6, r6, 0x10
		bdnz L_80217A68
		blr
L_80217B34:
		li r0, 0x4
		mtctr r0
		subi r7, r7, 0x1
L_80217B40:
		dcbt r7, r3
		lwz r8, 0x4(r7)
		lwz r5, 0x0(r7)
		lhz r9, 0x8(r7)
		srwi r4, r8, 24
		rlwimi r4, r5, 8, 0, 23
		rlwimi r5, r8, 0, 0, 15
		rotlwi r5, r5, 16
		rlwimi r9, r8, 16, 0, 15
		xor r10, r4, r5
		slwi r8, r8, 8
		rlwimi r8, r9, 24, 24, 31
		and r4, r4, r5
		and r5, r10, r11
		and r10, r10, r12
		xor r0, r8, r9
		and r8, r8, r9
		and r9, r0, r11
		srwi r5, r5, 1
		add r4, r4, r10
		and r0, r0, r12
		add r4, r4, r5
		srwi r9, r9, 1
		add r8, r8, r0
		stw r4, 0x0(r6)
		add r8, r8, r9
		add r7, r7, r3
		stw r8, 0x4(r6)
		dcbt r7, r3
		lwz r8, 0x4(r7)
		lhz r9, 0x8(r7)
		lwz r5, 0x0(r7)
		srwi r4, r8, 24
		rlwimi r9, r8, 16, 0, 15
		add r7, r7, r3
		rlwimi r4, r5, 8, 0, 23
		rlwimi r5, r8, 0, 0, 15
		rotlwi r5, r5, 16
		slwi r8, r8, 8
		xor r10, r4, r5
		rlwimi r8, r9, 24, 24, 31
		and r4, r4, r5
		and r5, r10, r11
		and r10, r10, r12
		xor r0, r8, r9
		and r8, r8, r9
		and r9, r0, r11
		srwi r5, r5, 1
		add r4, r4, r10
		and r0, r0, r12
		add r4, r4, r5
		srwi r9, r9, 1
		add r8, r8, r0
		stw r4, 0x8(r6)
		add r8, r8, r9
		stw r8, 0xc(r6)
		addi r6, r6, 0x10
		bdnz L_80217B40
		blr
L_80217C2C:
		li r0, 0x4
		mtctr r0
		subi r7, r7, 0x2
L_80217C38:
		dcbt r7, r3
		lwz r4, 0x0(r7)
		lwz r8, 0x4(r7)
		slwi r5, r4, 24
		slwi r4, r4, 16
		lwz r0, 0x8(r7)
		slwi r9, r8, 24
		rlwimi r5, r8, 24, 8, 31
		rlwimi r4, r8, 16, 16, 31
		xor r10, r4, r5
		slwi r8, r8, 16
		and r4, r4, r5
		rlwimi r9, r0, 24, 8, 31
		and r5, r10, r11
		rlwimi r8, r0, 16, 16, 31
		and r10, r10, r12
		add r7, r7, r3
		xor r0, r8, r9
		and r8, r8, r9
		and r9, r0, r11
		srwi r5, r5, 1
		add r4, r4, r10
		and r0, r0, r12
		add r4, r4, r5
		srwi r9, r9, 1
		add r8, r8, r0
		stw r4, 0x0(r6)
		add r8, r8, r9
		stw r8, 0x4(r6)
		dcbt r7, r3
		lwz r4, 0x0(r7)
		lwz r8, 0x4(r7)
		slwi r5, r4, 24
		slwi r4, r4, 16
		lwz r0, 0x8(r7)
		slwi r9, r8, 24
		rlwimi r5, r8, 24, 8, 31
		rlwimi r4, r8, 16, 16, 31
		xor r10, r4, r5
		slwi r8, r8, 16
		and r4, r4, r5
		rlwimi r9, r0, 24, 8, 31
		and r5, r10, r11
		rlwimi r8, r0, 16, 16, 31
		and r10, r10, r12
		add r7, r7, r3
		xor r0, r8, r9
		and r8, r8, r9
		and r9, r0, r11
		srwi r5, r5, 1
		add r4, r4, r10
		and r0, r0, r12
		add r4, r4, r5
		srwi r9, r9, 1
		add r8, r8, r0
		stw r4, 0x8(r6)
		add r8, r8, r9
		stw r8, 0xc(r6)
		addi r6, r6, 0x10
		bdnz L_80217C38
		blr
L_80217D2C:
		li r0, 0x4
		mtctr r0
		subi r7, r7, 0x3
L_80217D38:
		dcbt r7, r3
		lwz r5, 0x4(r7)
		lwbrx r4, r0, r7
		rlwimi r4, r5, 24, 8, 31
		lwz r9, 0x8(r7)
		xor r10, r4, r5
		slwi r8, r5, 24
		rlwimi r8, r9, 24, 8, 31
		and r4, r4, r5
		and r5, r10, r11
		and r10, r10, r12
		xor r0, r8, r9
		and r8, r8, r9
		and r9, r0, r11
		srwi r5, r5, 1
		add r4, r4, r10
		and r0, r0, r12
		add r4, r4, r5
		srwi r9, r9, 1
		add r8, r8, r0
		stw r4, 0x0(r6)
		add r8, r8, r9
		add r7, r7, r3
		stw r8, 0x4(r6)
		dcbt r7, r3
		lwz r5, 0x4(r7)
		lwbrx r4, r0, r7
		rlwimi r4, r5, 24, 8, 31
		lwz r9, 0x8(r7)
		xor r10, r4, r5
		slwi r8, r5, 24
		rlwimi r8, r9, 24, 8, 31
		and r4, r4, r5
		and r5, r10, r11
		and r10, r10, r12
		xor r0, r8, r9
		and r8, r8, r9
		and r9, r0, r11
		srwi r5, r5, 1
		add r4, r4, r10
		and r0, r0, r12
		add r4, r4, r5
		srwi r9, r9, 1
		add r8, r8, r0
		stw r4, 0x8(r6)
		add r8, r8, r9
		add r7, r7, r3
		stw r8, 0xc(r6)
		addi r6, r6, 0x10
		bdnz L_80217D38
	}
}

#pragma peephole off

void MPVMC08_OneRefV2_TuneC(MPVMC *mc)
{
	asm {
		lwz r5, 0x24(r3)
		lwz r6, 0x28(r3)
		clrlwi r7, r5, 30
		lwz r4, 0x18(r3)
		cmpwi r7, 0x2
		lwz r0, 0x20(r3)
		beq L_80217FD0
		bge L_80217E48
		cmpwi r7, 0x0
		beq L_80217E54
		bge L_80217F28
		b L_8021811C
L_80217E48:
		cmpwi r7, 0x4
		bge L_8021811C
		b L_80218074
L_80217E54:
		lis r8, 0x101
		lis r7, 0xfeff
		li r3, 0x4
		addi r11, r8, 0x101
		subi r10, r7, 0x102
		mtctr r3
L_80217E6C:
		lwz r8, 0x0(r5)
		lwz r9, 0x0(r6)
		lwz r28, 0x4(r5)
		add r5, r5, r0
		lwz r29, 0x4(r6)
		xor r7, r8, r9
		and r3, r7, r10
		and r9, r8, r9
		xor r12, r28, r29
		and r8, r7, r11
		srwi r3, r3, 1
		and r7, r28, r29
		add r8, r3, r8
		and r3, r12, r10
		add r9, r9, r8
		and r8, r12, r11
		srwi r3, r3, 1
		stw r9, 0x0(r4)
		add r3, r3, r8
		add r6, r6, r0
		add r3, r7, r3
		stw r3, 0x4(r4)
		lwz r8, 0x0(r5)
		lwz r9, 0x0(r6)
		lwz r28, 0x4(r5)
		add r5, r5, r0
		lwz r29, 0x4(r6)
		xor r7, r8, r9
		and r3, r7, r10
		and r9, r8, r9
		xor r12, r28, r29
		and r8, r7, r11
		srwi r3, r3, 1
		and r7, r28, r29
		add r8, r3, r8
		and r3, r12, r10
		add r9, r9, r8
		and r8, r12, r11
		srwi r3, r3, 1
		stw r9, 0x8(r4)
		add r3, r3, r8
		add r6, r6, r0
		add r3, r7, r3
		stw r3, 0xc(r4)
		addi r4, r4, 0x10
		bdnz L_80217E6C
		b L_8021811C
L_80217F28:
		lis r8, 0x101
		lis r7, 0xfeff
		li r3, 0x8
		addi r12, r8, 0x101
		subi r10, r7, 0x102
		mtctr r3
		subi r5, r5, 0x1
		subi r6, r6, 0x1
L_80217F48:
		lwz r11, 0x4(r5)
		lwz r30, 0x4(r6)
		lbz r31, 0x8(r6)
		srwi r3, r11, 24
		lwz r7, 0x0(r5)
		srwi r9, r30, 24
		lwz r8, 0x0(r6)
		mr r28, r31
		lbz r29, 0x8(r5)
		rlwimi r3, r7, 8, 0, 23
		rlwimi r9, r8, 8, 0, 23
		rlwimi r29, r11, 8, 0, 23
		xor r8, r3, r9
		rlwimi r28, r30, 8, 0, 23
		and r7, r8, r10
		and r9, r3, r9
		xor r31, r29, r28
		and r11, r8, r12
		srwi r7, r7, 1
		add r5, r5, r0
		and r3, r31, r10
		and r8, r31, r12
		add r7, r7, r11
		add r6, r6, r0
		add r9, r9, r7
		srwi r3, r3, 1
		and r7, r29, r28
		stw r9, 0x0(r4)
		add r3, r3, r8
		add r3, r7, r3
		stw r3, 0x4(r4)
		addi r4, r4, 0x8
		bdnz L_80217F48
		b L_8021811C
L_80217FD0:
		lis r8, 0x101
		lis r7, 0xfeff
		li r3, 0x8
		addi r12, r8, 0x101
		subi r10, r7, 0x102
		mtctr r3
		subi r5, r5, 0x2
		subi r6, r6, 0x2
L_80217FF0:
		lwz r11, 0x4(r5)
		lwz r30, 0x4(r6)
		lwz r7, 0x0(r5)
		srwi r3, r11, 16
		lhz r29, 0x8(r5)
		srwi r9, r30, 16
		lwz r8, 0x0(r6)
		rlwimi r3, r7, 16, 0, 15
		lhz r28, 0x8(r6)
		rlwimi r29, r11, 16, 0, 15
		rlwimi r9, r8, 16, 0, 15
		rlwimi r28, r30, 16, 0, 15
		xor r8, r3, r9
		add r5, r5, r0
		and r7, r8, r10
		xor r31, r29, r28
		and r11, r8, r12
		and r9, r3, r9
		srwi r7, r7, 1
		and r3, r31, r10
		add r7, r7, r11
		and r8, r31, r12
		add r9, r9, r7
		srwi r3, r3, 1
		and r7, r29, r28
		stw r9, 0x0(r4)
		add r3, r3, r8
		add r6, r6, r0
		add r3, r7, r3
		stw r3, 0x4(r4)
		addi r4, r4, 0x8
		bdnz L_80217FF0
		b L_8021811C
L_80218074:
		lis r8, 0x101
		lis r7, 0xfeff
		li r3, 0x8
		addi r12, r8, 0x101
		subi r10, r7, 0x102
		mtctr r3
		subi r5, r5, 0x3
		subi r6, r6, 0x3
L_80218094:
		lwz r31, 0x4(r5)
		lwz r30, 0x4(r6)
		lwz r11, 0x8(r5)
		srwi r3, r31, 8
		lwz r7, 0x0(r5)
		srwi r8, r30, 8
		lwz r9, 0x0(r6)
		srwi r29, r11, 8
		lwz r28, 0x8(r6)
		rlwimi r3, r7, 24, 0, 7
		rlwimi r8, r9, 24, 0, 7
		rlwimi r29, r31, 24, 0, 7
		srwi r28, r28, 8
		add r5, r5, r0
		xor r7, r3, r8
		and r11, r3, r8
		and r3, r7, r10
		rlwimi r28, r30, 24, 0, 7
		and r7, r7, r12
		add r6, r6, r0
		srwi r3, r3, 1
		xor r8, r29, r28
		add r9, r3, r7
		and r7, r29, r28
		and r3, r8, r10
		and r8, r8, r12
		add r9, r11, r9
		srwi r3, r3, 1
		stw r9, 0x0(r4)
		add r3, r3, r8
		add r3, r7, r3
		stw r3, 0x4(r4)
		addi r4, r4, 0x8
		bdnz L_80218094
L_8021811C:
	}
}

#pragma peephole on
#pragma scheduling on

void MPVMC08_OneRef1p_TuneC(MPVMC *mc)
{
	register Uint8 *s = mc->src;

	asm { dcbt r0, s }
	switch ((Uint32)s & 7) {
	case 0:
	{
		register Sint32 st;

		asm {
		lwz st, 0x20(r3)
		mr r8, s
		lwz r3, 0x18(r3)
		lfdux f1, r8, st
		lfdux f2, r8, st
		lfdux f3, r8, st
		lfdux f4, r8, st
		lfdux f5, r8, st
		lfdux f6, r8, st
		lfd f0, 0x0(s)
		lfdux f7, r8, st
		stfd f0, 0x0(r3)
		stfd f1, 0x8(r3)
		stfd f2, 0x10(r3)
		stfd f3, 0x18(r3)
		stfd f4, 0x20(r3)
		stfd f5, 0x28(r3)
		stfd f6, 0x30(r3)
		stfd f7, 0x38(r3)
		}
		break;
	}
	case 4:
	{
		register Sint32 st;
		register Uint8 *p;
		register Uint32 w;

		asm {
		lwz st, 0x20(r3)
		mr p, s
		lwz r3, 0x18(r3)
		lwzux r7, p, st
		lwz r6, 0x0(s)
		lwz w, 0x4(s)
		lwz r8, 0x4(p)
		stw r6, 0x0(r3)
		stw w, 0x4(r3)
		lwzux r6, p, st
		lwz w, 0x4(p)
		stw r7, 0x8(r3)
		stw r8, 0xc(r3)
		lwzux r7, p, st
		lwz r8, 0x4(p)
		stw r6, 0x10(r3)
		stw w, 0x14(r3)
		lwzux r6, p, st
		lwz w, 0x4(p)
		stw r7, 0x18(r3)
		stw r8, 0x1c(r3)
		lwzux r7, p, st
		lwz r8, 0x4(p)
		stw r6, 0x20(r3)
		stw w, 0x24(r3)
		lwzux r6, p, st
		lwz w, 0x4(p)
		stw r7, 0x28(r3)
		stw r8, 0x2c(r3)
		lwzux r7, p, st
		lwz r8, 0x4(p)
		stw r6, 0x30(r3)
		stw w, 0x34(r3)
		stw r7, 0x38(r3)
		stw r8, 0x3c(r3)
		}
		break;
	}
	case 2:
	case 6:
	{
		register Uint32 t0;
		register Uint32 st;

		asm {
		lwz st, 0x20(r3)
		mr r6, s
		li t0, 0x2
		lwz s, 0x18(r3)
		clrrwi r3, st, 1
		mtctr t0
L_80218258:
		lhz st, 0x0(r6)
		lwz r7, 0x2(r6)
		lhz r8, 0x6(r6)
		slwi st, st, 16
		rlwimi st, r7, 16, 16, 31
		slwi t0, r7, 16
		stw st, 0x0(s)
		or r8, t0, r8
		add r6, r6, r3
		stw r8, 0x4(s)
		lwz r7, 0x2(r6)
		lhz st, 0x0(r6)
		lhz r8, 0x6(r6)
		slwi t0, r7, 16
		slwi st, st, 16
		add r6, r6, r3
		rlwimi st, r7, 16, 16, 31
		or r8, t0, r8
		stw st, 0x8(s)
		stw r8, 0xc(s)
		lwz r7, 0x2(r6)
		lhz st, 0x0(r6)
		lhz r8, 0x6(r6)
		slwi t0, r7, 16
		slwi st, st, 16
		add r6, r6, r3
		rlwimi st, r7, 16, 16, 31
		or r8, t0, r8
		stw st, 0x10(s)
		stw r8, 0x14(s)
		lwz r7, 0x2(r6)
		lhz st, 0x0(r6)
		lhz r8, 0x6(r6)
		slwi t0, r7, 16
		slwi st, st, 16
		add r6, r6, r3
		rlwimi st, r7, 16, 16, 31
		or r8, t0, r8
		stw st, 0x18(s)
		stw r8, 0x1c(s)
		addi s, s, 0x20
		bdnz L_80218258
		}
		break;
	}
	case 1:
	case 5:
	{
		register Sint32 st;
		register Uint32 *d;

		asm {
		lwz st, 0x20(r3)
		subi s, s, 0x1
		lwz d, 0x18(r3)
		slwi r3, st, 1
		dcbt s, st
		lwz r6, 0x0(s)
		lwz r7, 0x4(s)
		lbz r8, 0x8(s)
		dcbt s, r3
		slwi r6, r6, 8
		lwzux r9, s, st
		rlwimi r6, r7, 8, 24, 31
		rlwimi r8, r7, 8, 0, 23
		lwz r7, 0x4(s)
		lbz r10, 0x8(s)
		stw r6, 0x0(d)
		stw r8, 0x4(d)
		dcbt s, r3
		slwi r9, r9, 8
		lwzux r6, s, st
		rlwimi r9, r7, 8, 24, 31
		rlwimi r10, r7, 8, 0, 23
		lwz r7, 0x4(s)
		lbz r8, 0x8(s)
		stw r9, 0x8(d)
		stw r10, 0xc(d)
		dcbt s, r3
		slwi r6, r6, 8
		lwzux r9, s, st
		rlwimi r6, r7, 8, 24, 31
		rlwimi r8, r7, 8, 0, 23
		lwz r7, 0x4(s)
		lbz r10, 0x8(s)
		stw r6, 0x10(d)
		stw r8, 0x14(d)
		dcbt s, r3
		slwi r9, r9, 8
		lwzux r6, s, st
		rlwimi r9, r7, 8, 24, 31
		rlwimi r10, r7, 8, 0, 23
		lwz r7, 0x4(s)
		lbz r8, 0x8(s)
		stw r9, 0x18(d)
		stw r10, 0x1c(d)
		dcbt s, r3
		slwi r6, r6, 8
		lwzux r9, s, st
		rlwimi r6, r7, 8, 24, 31
		rlwimi r8, r7, 8, 0, 23
		lwz r7, 0x4(s)
		lbz r10, 0x8(s)
		stw r6, 0x20(d)
		stw r8, 0x24(d)
		dcbt s, r3
		slwi r9, r9, 8
		lwzux r6, s, st
		rlwimi r9, r7, 8, 24, 31
		rlwimi r10, r7, 8, 0, 23
		lwz r7, 0x4(s)
		slwi r6, r6, 8
		lbz r8, 0x8(s)
		rlwimi r6, r7, 8, 24, 31
		rlwimi r8, r7, 8, 0, 23
		stw r9, 0x28(d)
		stw r10, 0x2c(d)
		lwzux r9, s, st
		lwz r7, 0x4(s)
		slwi r9, r9, 8
		lbz r10, 0x8(s)
		rlwimi r9, r7, 8, 24, 31
		rlwimi r10, r7, 8, 0, 23
		stw r6, 0x30(d)
		stw r8, 0x34(d)
		stw r9, 0x38(d)
		stw r10, 0x3c(d)
		}
		break;
	}
	case 3:
	case 7:
	{
		register Sint32 st;
		register Uint32 *d;

		asm {
		lwz st, 0x20(r3)
		subi s, s, 0x3
		lwz d, 0x18(r3)
		slwi r3, st, 1
		dcbt s, st
		lwz r6, 0x0(s)
		lwz r7, 0x4(s)
		lwz r8, 0x8(s)
		dcbt s, r3
		slwi r6, r6, 24
		lwzux r9, s, st
		rlwimi r6, r7, 24, 8, 31
		srwi r8, r8, 8
		lwz r10, 0x4(s)
		rlwimi r8, r7, 24, 0, 7
		lwz r11, 0x8(s)
		stw r6, 0x0(d)
		stw r8, 0x4(d)
		dcbt s, r3
		slwi r9, r9, 24
		lwzux r6, s, st
		srwi r11, r11, 8
		rlwimi r9, r10, 24, 8, 31
		lwz r7, 0x4(s)
		rlwimi r11, r10, 24, 0, 7
		lwz r8, 0x8(s)
		stw r9, 0x8(d)
		stw r11, 0xc(d)
		dcbt s, r3
		slwi r6, r6, 24
		lwzux r9, s, st
		srwi r8, r8, 8
		rlwimi r6, r7, 24, 8, 31
		lwz r10, 0x4(s)
		rlwimi r8, r7, 24, 0, 7
		lwz r11, 0x8(s)
		stw r6, 0x10(d)
		stw r8, 0x14(d)
		dcbt s, st
		slwi r9, r9, 24
		lwzux r6, s, st
		srwi r11, r11, 8
		rlwimi r9, r10, 24, 8, 31
		lwz r7, 0x4(s)
		rlwimi r11, r10, 24, 0, 7
		lwz r8, 0x8(s)
		stw r9, 0x18(d)
		stw r11, 0x1c(d)
		dcbt s, r3
		slwi r6, r6, 24
		lwzux r9, s, st
		srwi r8, r8, 8
		rlwimi r6, r7, 24, 8, 31
		lwz r10, 0x4(s)
		rlwimi r8, r7, 24, 0, 7
		lwz r11, 0x8(s)
		stw r6, 0x20(d)
		stw r8, 0x24(d)
		dcbt s, r3
		slwi r9, r9, 24
		lwzux r6, s, st
		srwi r11, r11, 8
		rlwimi r9, r10, 24, 8, 31
		lwz r8, 0x8(s)
		rlwimi r11, r10, 24, 0, 7
		lwz r7, 0x4(s)
		slwi r6, r6, 24
		srwi r8, r8, 8
		stw r9, 0x28(d)
		rlwimi r6, r7, 24, 8, 31
		rlwimi r8, r7, 24, 0, 7
		stw r11, 0x2c(d)
		lwzux r9, s, st
		lwz r11, 0x8(s)
		slwi r9, r9, 24
		lwz r10, 0x4(s)
		srwi r11, r11, 8
		stw r6, 0x30(d)
		rlwimi r9, r10, 24, 8, 31
		rlwimi r11, r10, 24, 0, 7
		stw r8, 0x34(d)
		stw r9, 0x38(d)
		stw r11, 0x3c(d)

		}
		break;
	}
	}
}

/* the generic (non-tuned) versions were dead-stripped; the table keeps their slots */
void (*const mpvmc_oneref1p_func_table[4])(MPVMC *mc) = {NULL, NULL, NULL, NULL};

void MPVMC08_Init(MPVMC *mc)
{
	mc->oneref08[0] = mpvmc_oneref1p_func_table[0];
	mc->oneref08[1] = mpvmc_oneref1p_func_table[1];
	mc->oneref08[2] = mpvmc_oneref1p_func_table[2];
	mc->oneref08[3] = mpvmc_oneref1p_func_table[3];
}
