/* CRI Sofdec MPEG video: 16x16 block motion compensation, "tuned C" versions. One reference block
 * is copied (1p), horizontally (H2) / vertically (V2) / four-point (4p) half-pel averaged into the
 * word-packed destination (four 8x8 blocks); the source alignment selects the load strategy.
 * PLACEHOLDER: unlike mpv_mc.c the bodies here look compiled (loop counters, folded strides), but
 * no C form reproduced the original schedule, so they are transcribed as inline assembly with
 * scheduling off; OneRef1p keeps its alignment switch in C with register variables in the asm. */
#include "cri_xpt.h"
#include "mpv.h"

#pragma scheduling off

void MPVMC16_OneRef4p_TuneC(MPVMC *mc)
{
	asm {
		li r7, 0x10
		li r4, 0x0
		lwz r0, 0x20(r3)
		lwz r5, 0x24(r3)
		lwz r6, 0x28(r3)
		lwz r3, 0x18(r3)
		mtctr r7
L_802073E4:
		dcbt r6, r0
		lbz r11, 0x1(r6)
		lbz r9, 0x2(r5)
		lbz r8, 0x0(r6)
		lbz r12, 0x1(r5)
		add r7, r9, r11
		lbz r22, 0x2(r6)
		add r10, r12, r8
		lbz r21, 0x3(r5)
		add r8, r7, r22
		lbz r28, 0x5(r6)
		addi r23, r8, 0x2
		add r24, r10, r11
		add r8, r21, r22
		lbz r11, 0x3(r6)
		add r23, r12, r23
		lbz r7, 0x6(r5)
		add r10, r8, r11
		lbz r26, 0x4(r5)
		addi r10, r10, 0x2
		lbz r22, 0x4(r6)
		lbz r27, 0x5(r5)
		add r8, r26, r11
		add r10, r9, r10
		lbz r25, 0x6(r6)
		add r12, r27, r22
		add r11, r7, r28
		add r22, r8, r22
		lbz r9, 0x0(r5)
		add r12, r12, r28
		add r11, r11, r25
		addi r28, r11, 0x2
		lbz r8, 0x7(r5)
		addi r11, r22, 0x2
		addi r22, r24, 0x2
		add r11, r21, r11
		addi r12, r12, 0x2
		add r22, r9, r22
		rlwinm r23, r23, 14, 8, 15
		rlwimi r23, r22, 22, 0, 7
		add r28, r27, r28
		rlwimi r23, r10, 6, 16, 23
		lbz r21, 0x7(r6)
		add r10, r8, r25
		lbz r9, 0x8(r5)
		add r10, r10, r21
		lbz r24, 0x8(r6)
		addi r25, r10, 0x2
		add r12, r26, r12
		add r25, r7, r25
		add r7, r9, r21
		rlwinm r22, r28, 14, 8, 15
		lbz r10, 0x9(r5)
		rlwimi r22, r12, 22, 0, 7
		add r7, r7, r24
		addi r21, r7, 0x2
		lbz r12, 0x9(r6)
		rlwimi r23, r11, 30, 24, 31
		add r7, r10, r24
		add r7, r7, r12
		add r21, r8, r21
		rlwimi r22, r25, 6, 16, 23
		stw r23, 0x0(r3)
		rlwimi r22, r21, 30, 24, 31
		addi r31, r7, 0x2
		stw r22, 0x4(r3)
		add r31, r9, r31
		lbz r11, 0xa(r5)
		lbz r8, 0xa(r6)
		add r7, r11, r12
		lbz r12, 0xb(r5)
		add r7, r7, r8
		lbz r22, 0xb(r6)
		addi r30, r7, 0x2
		add r30, r10, r30
		add r7, r12, r8
		lbz r26, 0xd(r6)
		lbz r8, 0xe(r5)
		add r7, r7, r22
		addi r29, r7, 0x2
		lbz r10, 0xc(r5)
		lbz r23, 0xc(r6)
		add r9, r8, r26
		lbz r27, 0xe(r6)
		add r7, r10, r22
		add r22, r7, r23
		lbz r21, 0xd(r5)
		add r24, r9, r27
		lbz r7, 0xf(r5)
		addi r28, r24, 0x2
		lbz r25, 0x10(r5)
		lbz r9, 0xf(r6)
		add r23, r21, r23
		lbz r24, 0x10(r6)
		add r23, r23, r26
		rlwinm r26, r30, 14, 8, 15
		add r29, r11, r29
		addi r11, r22, 0x2
		addi r30, r23, 0x2
		rlwimi r26, r31, 22, 0, 7
		add r27, r7, r27
		add r25, r25, r9
		add r11, r12, r11
		add r12, r25, r24
		rlwimi r26, r29, 6, 16, 23
		addi r12, r12, 0x2
		add r9, r27, r9
		add r30, r10, r30
		rlwimi r26, r11, 30, 24, 31
		addi r10, r9, 0x2
		add r28, r21, r28
		rlwinm r9, r28, 14, 8, 15
		add r12, r7, r12
		add r8, r8, r10
		stw r26, 0x40(r3)
		rlwimi r9, r30, 22, 0, 7
		cmpwi r4, 0x7
		rlwimi r9, r8, 6, 16, 23
		add r5, r5, r0
		rlwimi r9, r12, 30, 24, 31
		add r6, r6, r0
		stw r9, 0x44(r3)
		addi r3, r3, 0x8
		bne L_802075D8
		addi r3, r3, 0x40
L_802075D8:
		addi r4, r4, 0x1
		bdnz L_802073E4
	}
}
#pragma scheduling on


#pragma scheduling off

void MPVMC16_OneRefH2_TuneC(MPVMC *mc)
{
	asm {
		lwz r4, 0x24(r3)
		lwz r0, 0x20(r3)
		clrlwi r5, r4, 30
		lwz r3, 0x18(r3)
		cmpwi r5, 0x2
		beq L_80207844
		bge L_80207620
		cmpwi r5, 0x0
		beq L_8020762C
		bge L_8020772C
		b L_80207A50
L_80207620:
		cmpwi r5, 0x4
		bge L_80207A50
		b L_8020795C
L_8020762C:
		lis r6, 0x101
		lis r5, 0xfeff
		li r7, 0x10
		li r9, 0x0
		addi r6, r6, 0x101
		subi r5, r5, 0x102
		mtctr r7
L_80207648:
		dcbt r4, r0
		cmpwi r9, 0x7
		lwz r10, 0x4(r4)
		lwz r12, 0x8(r4)
		srwi r7, r10, 24
		lwz r21, 0x0(r4)
		mr r22, r7
		lwz r30, 0xc(r4)
		rlwimi r22, r21, 8, 0, 23
		srwi r8, r12, 24
		xor r24, r21, r22
		srwi r7, r30, 24
		mr r11, r8
		lbz r20, 0x10(r4)
		rlwimi r11, r10, 8, 0, 23
		and r23, r24, r5
		xor r28, r10, r11
		mr r29, r7
		rlwimi r29, r12, 8, 0, 23
		mr r8, r20
		and r25, r28, r5
		and r26, r24, r6
		srwi r23, r23, 1
		xor r31, r12, r29
		rlwimi r8, r30, 8, 0, 23
		and r27, r21, r22
		add r26, r23, r26
		and r28, r28, r6
		add r27, r27, r26
		xor r7, r30, r8
		srwi r26, r25, 1
		stw r27, 0x0(r3)
		and r27, r10, r11
		and r24, r31, r5
		add r11, r26, r28
		and r25, r31, r6
		srwi r10, r24, 1
		and r23, r7, r5
		add r24, r27, r11
		and r11, r12, r29
		add r10, r10, r25
		stw r24, 0x4(r3)
		add r11, r11, r10
		and r8, r30, r8
		and r10, r7, r6
		srwi r7, r23, 1
		add r7, r7, r10
		stw r11, 0x40(r3)
		add r7, r8, r7
		add r4, r4, r0
		stw r7, 0x44(r3)
		addi r3, r3, 0x8
		bne L_80207720
		addi r3, r3, 0x40
L_80207720:
		addi r9, r9, 0x1
		bdnz L_80207648
		b L_80207A50
L_8020772C:
		lis r6, 0x101
		lis r5, 0xfeff
		li r8, 0x10
		li r7, 0x0
		addi r6, r6, 0x101
		subi r5, r5, 0x102
		mtctr r8
		subi r4, r4, 0x1
L_8020774C:
		dcbt r4, r0
		cmpwi r7, 0x7
		lwz r23, 0x4(r4)
		lwz r29, 0x8(r4)
		lwz r22, 0x0(r4)
		srwi r21, r23, 24
		lwz r30, 0xc(r4)
		srwi r20, r23, 16
		srwi r10, r29, 24
		srwi r31, r29, 16
		lwz r12, 0x10(r4)
		srwi r8, r30, 24
		srwi r11, r30, 16
		rlwimi r21, r22, 8, 0, 23
		rlwimi r20, r22, 16, 0, 15
		srwi r9, r12, 24
		xor r24, r21, r20
		srwi r12, r12, 16
		rlwimi r10, r23, 8, 0, 23
		rlwimi r31, r23, 16, 0, 15
		and r23, r24, r5
		and r25, r24, r6
		srwi r24, r23, 1
		xor r27, r10, r31
		add r24, r24, r25
		and r26, r21, r20
		add r26, r26, r24
		and r23, r27, r5
		stw r26, 0x0(r3)
		rlwimi r8, r29, 8, 0, 23
		rlwimi r11, r29, 16, 0, 15
		and r25, r27, r6
		xor r27, r8, r11
		srwi r24, r23, 1
		and r23, r27, r5
		and r26, r10, r31
		add r10, r24, r25
		rlwimi r9, r30, 8, 0, 23
		add r10, r26, r10
		rlwimi r12, r30, 16, 0, 15
		and r25, r27, r6
		srwi r23, r23, 1
		xor r27, r9, r12
		and r24, r8, r11
		stw r10, 0x4(r3)
		add r8, r23, r25
		add r23, r24, r8
		and r10, r27, r5
		and r11, r27, r6
		stw r23, 0x40(r3)
		srwi r8, r10, 1
		and r9, r9, r12
		add r8, r8, r11
		add r4, r4, r0
		add r8, r9, r8
		stw r8, 0x44(r3)
		addi r3, r3, 0x8
		bne L_80207838
		addi r3, r3, 0x40
L_80207838:
		addi r7, r7, 0x1
		bdnz L_8020774C
		b L_80207A50
L_80207844:
		lis r6, 0x101
		lis r5, 0xfeff
		li r8, 0x10
		li r7, 0x0
		addi r6, r6, 0x101
		subi r5, r5, 0x102
		mtctr r8
		subi r4, r4, 0x2
L_80207864:
		dcbt r4, r0
		cmpwi r7, 0x7
		lwz r23, 0x4(r4)
		lwz r30, 0x8(r4)
		lwz r22, 0x0(r4)
		srwi r21, r23, 16
		lwz r29, 0xc(r4)
		srwi r20, r23, 8
		srwi r10, r30, 16
		srwi r31, r30, 8
		lwz r12, 0x10(r4)
		srwi r8, r29, 16
		srwi r11, r29, 8
		rlwimi r21, r22, 16, 0, 15
		rlwimi r20, r22, 24, 0, 7
		srwi r9, r12, 16
		xor r24, r21, r20
		srwi r12, r12, 8
		rlwimi r10, r23, 16, 0, 15
		rlwimi r31, r23, 24, 0, 7
		and r23, r24, r5
		and r25, r24, r6
		srwi r24, r23, 1
		xor r27, r10, r31
		add r24, r24, r25
		and r26, r21, r20
		add r26, r26, r24
		and r23, r27, r5
		stw r26, 0x0(r3)
		rlwimi r8, r30, 16, 0, 15
		rlwimi r11, r30, 24, 0, 7
		and r25, r27, r6
		xor r27, r8, r11
		srwi r24, r23, 1
		and r23, r27, r5
		and r26, r10, r31
		add r10, r24, r25
		rlwimi r9, r29, 16, 0, 15
		add r10, r26, r10
		rlwimi r12, r29, 24, 0, 7
		and r25, r27, r6
		srwi r23, r23, 1
		xor r27, r9, r12
		and r24, r8, r11
		stw r10, 0x4(r3)
		add r8, r23, r25
		add r23, r24, r8
		and r10, r27, r5
		and r11, r27, r6
		stw r23, 0x40(r3)
		srwi r8, r10, 1
		and r9, r9, r12
		add r8, r8, r11
		add r4, r4, r0
		add r8, r9, r8
		stw r8, 0x44(r3)
		addi r3, r3, 0x8
		bne L_80207950
		addi r3, r3, 0x40
L_80207950:
		addi r7, r7, 0x1
		bdnz L_80207864
		b L_80207A50
L_8020795C:
		lis r6, 0x101
		lis r5, 0xfeff
		li r7, 0x10
		li r9, 0x0
		addi r6, r6, 0x101
		subi r5, r5, 0x102
		mtctr r7
		subi r4, r4, 0x3
L_8020797C:
		dcbt r4, r0
		cmpwi r9, 0x7
		lwz r20, 0x4(r4)
		lwz r31, 0x8(r4)
		lwz r7, 0x0(r4)
		srwi r21, r20, 8
		lwz r30, 0xc(r4)
		srwi r10, r31, 8
		rlwimi r21, r7, 24, 0, 7
		lwz r29, 0x10(r4)
		xor r23, r21, r20
		rlwimi r10, r20, 24, 0, 7
		and r8, r23, r5
		srwi r11, r30, 8
		xor r22, r10, r31
		and r25, r23, r6
		srwi r24, r8, 1
		rlwimi r11, r31, 24, 0, 7
		xor r7, r11, r30
		and r26, r22, r5
		add r25, r24, r25
		and r23, r21, r20
		add r25, r23, r25
		srwi r12, r29, 8
		rlwimi r12, r30, 24, 0, 7
		stw r25, 0x0(r3)
		and r25, r10, r31
		and r24, r22, r6
		srwi r26, r26, 1
		xor r8, r12, r29
		add r31, r26, r24
		and r27, r7, r5
		and r10, r7, r6
		and r28, r8, r5
		add r31, r25, r31
		srwi r7, r27, 1
		add r7, r7, r10
		stw r31, 0x4(r3)
		and r11, r11, r30
		and r10, r8, r6
		add r8, r11, r7
		srwi r7, r28, 1
		stw r8, 0x40(r3)
		and r8, r12, r29
		add r7, r7, r10
		add r4, r4, r0
		add r7, r8, r7
		stw r7, 0x44(r3)
		addi r3, r3, 0x8
		bne L_80207A48
		addi r3, r3, 0x40
L_80207A48:
		addi r9, r9, 0x1
		bdnz L_8020797C
L_80207A50:
	}
}
#pragma scheduling on


#pragma scheduling off

void MPVMC16_OneRefV2_TuneC(MPVMC *mc)
{
	asm {
		lwz r6, 0x28(r3)
		lwz r5, 0x24(r3)
		lwz r4, 0x18(r3)
		lwz r0, 0x20(r3)
		dcbt r0, r6
		clrlwi r3, r5, 30
		cmpwi r3, 0x2
		beq L_80207CB4
		bge L_80207A98
		cmpwi r3, 0x0
		beq L_80207AA4
		bge L_80207B88
		b L_80207F10
L_80207A98:
		cmpwi r3, 0x4
		bge L_80207F10
		b L_80207DE0
L_80207AA4:
		lis r7, 0x101
		lis r3, 0xfeff
		li r8, 0x10
		li r10, 0x0
		addi r7, r7, 0x101
		subi r3, r3, 0x102
		mtctr r8
L_80207AC0:
		dcbt r6, r0
		cmpwi r10, 0x7
		lwz r17, 0x0(r5)
		lwz r18, 0x0(r6)
		lwz r11, 0x4(r5)
		xor r9, r17, r18
		lwz r12, 0x4(r6)
		and r19, r9, r3
		lwz r21, 0x8(r5)
		xor r29, r11, r12
		lwz r22, 0x8(r6)
		and r25, r29, r3
		lwz r23, 0xc(r5)
		lwz r24, 0xc(r6)
		xor r8, r21, r22
		and r20, r8, r3
		and r27, r9, r7
		xor r9, r23, r24
		srwi r26, r19, 1
		and r19, r9, r3
		and r28, r17, r18
		add r26, r26, r27
		and r27, r29, r7
		add r26, r28, r26
		srwi r25, r25, 1
		stw r26, 0x0(r4)
		and r26, r11, r12
		add r12, r25, r27
		and r11, r8, r7
		add r12, r26, r12
		srwi r8, r20, 1
		stw r12, 0x4(r4)
		and r12, r21, r22
		add r8, r8, r11
		and r11, r9, r7
		add r9, r12, r8
		srwi r8, r19, 1
		stw r9, 0x40(r4)
		and r9, r23, r24
		add r8, r8, r11
		add r5, r5, r0
		add r8, r9, r8
		add r6, r6, r0
		stw r8, 0x44(r4)
		addi r4, r4, 0x8
		bne L_80207B7C
		addi r4, r4, 0x40
L_80207B7C:
		addi r10, r10, 0x1
		bdnz L_80207AC0
		b L_80207F10
L_80207B88:
		lis r7, 0x101
		lis r3, 0xfeff
		li r8, 0x10
		li r9, 0x0
		addi r7, r7, 0x101
		subi r3, r3, 0x102
		mtctr r8
		subi r5, r5, 0x1
		subi r6, r6, 0x1
L_80207BAC:
		dcbt r6, r0
		cmpwi r9, 0x7
		lwz r8, 0x4(r5)
		lwz r30, 0x4(r6)
		lwz r28, 0x8(r5)
		srwi r10, r8, 24
		lwz r17, 0x0(r5)
		srwi r11, r30, 24
		lwz r18, 0x0(r6)
		srwi r12, r28, 24
		lwz r26, 0x8(r6)
		rlwimi r10, r17, 8, 0, 23
		rlwimi r11, r18, 8, 0, 23
		lwz r24, 0xc(r5)
		lbz r22, 0x10(r5)
		xor r31, r10, r11
		lwz r23, 0xc(r6)
		srwi r29, r26, 24
		lbz r21, 0x10(r6)
		rlwimi r29, r30, 8, 0, 23
		rlwimi r12, r8, 8, 0, 23
		and r19, r31, r3
		srwi r27, r24, 24
		srwi r25, r23, 24
		rlwimi r27, r28, 8, 0, 23
		xor r18, r12, r29
		rlwimi r25, r26, 8, 0, 23
		and r31, r31, r7
		srwi r28, r19, 1
		and r30, r10, r11
		and r20, r18, r3
		xor r8, r27, r25
		add r11, r28, r31
		and r26, r18, r7
		srwi r10, r20, 1
		and r19, r8, r3
		add r11, r30, r11
		rlwimi r22, r24, 8, 0, 23
		rlwimi r21, r23, 8, 0, 23
		stw r11, 0x0(r4)
		and r11, r12, r29
		add r10, r10, r26
		add r11, r11, r10
		xor r18, r22, r21
		and r12, r8, r7
		srwi r10, r19, 1
		and r8, r18, r3
		stw r11, 0x4(r4)
		and r11, r27, r25
		add r10, r10, r12
		add r10, r11, r10
		srwi r8, r8, 1
		and r11, r18, r7
		stw r10, 0x40(r4)
		and r10, r22, r21
		add r5, r5, r0
		add r8, r8, r11
		add r6, r6, r0
		add r8, r10, r8
		stw r8, 0x44(r4)
		addi r4, r4, 0x8
		bne L_80207CA8
		addi r4, r4, 0x40
L_80207CA8:
		addi r9, r9, 0x1
		bdnz L_80207BAC
		b L_80207F10
L_80207CB4:
		lis r7, 0x101
		lis r3, 0xfeff
		li r8, 0x10
		li r9, 0x0
		addi r7, r7, 0x101
		subi r3, r3, 0x102
		mtctr r8
		subi r5, r5, 0x2
		subi r6, r6, 0x2
L_80207CD8:
		dcbt r6, r0
		cmpwi r9, 0x7
		lwz r31, 0x4(r5)
		lwz r30, 0x4(r6)
		lwz r28, 0x8(r5)
		srwi r10, r31, 16
		lwz r8, 0x0(r5)
		srwi r11, r30, 16
		lwz r17, 0x0(r6)
		srwi r12, r28, 16
		lwz r26, 0x8(r6)
		rlwimi r10, r8, 16, 0, 15
		rlwimi r11, r17, 16, 0, 15
		lwz r24, 0xc(r5)
		lhz r22, 0x10(r5)
		xor r18, r10, r11
		lwz r23, 0xc(r6)
		srwi r29, r26, 16
		lhz r21, 0x10(r6)
		rlwimi r12, r31, 16, 0, 15
		rlwimi r29, r30, 16, 0, 15
		srwi r27, r24, 16
		xor r17, r12, r29
		srwi r25, r23, 16
		rlwimi r25, r26, 16, 0, 15
		rlwimi r27, r28, 16, 0, 15
		and r19, r18, r3
		and r31, r18, r7
		srwi r28, r19, 1
		and r30, r10, r11
		and r20, r17, r3
		xor r8, r27, r25
		add r11, r28, r31
		and r26, r17, r7
		srwi r10, r20, 1
		and r19, r8, r3
		add r11, r30, r11
		rlwimi r22, r24, 16, 0, 15
		rlwimi r21, r23, 16, 0, 15
		stw r11, 0x0(r4)
		and r11, r12, r29
		add r10, r10, r26
		add r11, r11, r10
		xor r17, r22, r21
		and r12, r8, r7
		srwi r10, r19, 1
		and r8, r17, r3
		stw r11, 0x4(r4)
		and r11, r27, r25
		add r10, r10, r12
		add r10, r11, r10
		srwi r8, r8, 1
		and r11, r17, r7
		stw r10, 0x40(r4)
		and r10, r22, r21
		add r5, r5, r0
		add r8, r8, r11
		add r6, r6, r0
		add r8, r10, r8
		stw r8, 0x44(r4)
		addi r4, r4, 0x8
		bne L_80207DD4
		addi r4, r4, 0x40
L_80207DD4:
		addi r9, r9, 0x1
		bdnz L_80207CD8
		b L_80207F10
L_80207DE0:
		lis r7, 0x101
		lis r3, 0xfeff
		li r8, 0x10
		li r9, 0x0
		addi r7, r7, 0x101
		subi r3, r3, 0x102
		mtctr r8
		subi r5, r5, 0x3
		subi r6, r6, 0x3
L_80207E04:
		dcbt r6, r0
		cmpwi r9, 0x7
		lwz r31, 0x4(r5)
		lwz r29, 0x4(r6)
		lwz r27, 0x8(r5)
		srwi r10, r31, 8
		lwz r8, 0x0(r5)
		srwi r11, r29, 8
		lwz r17, 0x0(r6)
		srwi r12, r27, 8
		lwz r25, 0x8(r6)
		rlwimi r10, r8, 24, 0, 7
		rlwimi r11, r17, 24, 0, 7
		lwz r23, 0xc(r6)
		lwz r19, 0x10(r6)
		srwi r30, r25, 8
		lwz r18, 0x10(r5)
		srwi r26, r23, 8
		lwz r24, 0xc(r5)
		srwi r21, r19, 8
		xor r17, r10, r11
		rlwimi r26, r25, 24, 0, 7
		srwi r22, r18, 8
		srwi r28, r24, 8
		rlwimi r28, r27, 24, 0, 7
		rlwimi r12, r31, 24, 0, 7
		rlwimi r30, r29, 24, 0, 7
		and r25, r17, r3
		xor r18, r12, r30
		and r19, r17, r7
		srwi r31, r25, 1
		and r20, r10, r11
		and r27, r18, r3
		xor r8, r28, r26
		add r11, r31, r19
		and r29, r18, r7
		srwi r10, r27, 1
		and r25, r8, r3
		add r11, r20, r11
		rlwimi r22, r24, 24, 0, 7
		rlwimi r21, r23, 24, 0, 7
		stw r11, 0x0(r4)
		and r11, r12, r30
		add r10, r10, r29
		add r11, r11, r10
		xor r17, r22, r21
		and r12, r8, r7
		srwi r10, r25, 1
		and r8, r17, r3
		stw r11, 0x4(r4)
		and r11, r28, r26
		add r10, r10, r12
		add r10, r11, r10
		srwi r8, r8, 1
		and r11, r17, r7
		stw r10, 0x40(r4)
		and r10, r22, r21
		add r5, r5, r0
		add r8, r8, r11
		add r6, r6, r0
		add r8, r10, r8
		stw r8, 0x44(r4)
		addi r4, r4, 0x8
		bne L_80207F08
		addi r4, r4, 0x40
L_80207F08:
		addi r9, r9, 0x1
		bdnz L_80207E04
L_80207F10:
	}
}
#pragma scheduling on


void MPVMC16_OneRef1p_TuneC(MPVMC *mc)
{
	register Uint8 *s = mc->src;

	asm { dcbt r0, s }
	switch ((Uint32)s & 7) {
	case 0:
	{
		register Uint32 t0;
		register Uint32 t4;

		asm {
		lwz t0, 0x20(r3)
		lwz r3, 0x18(r3)
		clrrwi t0, t0, 3
		lfd f0, 0x0(s)
		add t4, s, t0
		lfd f1, 0x8(s)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0x0(r3)
		stfd f1, 0x40(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0x8(r3)
		stfd f3, 0x48(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0x10(r3)
		stfd f1, 0x50(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0x18(r3)
		stfd f3, 0x58(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0x20(r3)
		stfd f1, 0x60(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0x28(r3)
		stfd f3, 0x68(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0x30(r3)
		stfd f1, 0x70(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0x38(r3)
		stfd f3, 0x78(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0x80(r3)
		stfd f1, 0xc0(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0x88(r3)
		stfd f3, 0xc8(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0x90(r3)
		stfd f1, 0xd0(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0x98(r3)
		stfd f3, 0xd8(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		add t4, t4, t0
		stfd f0, 0xa0(r3)
		stfd f1, 0xe0(r3)
		lfd f0, 0x0(t4)
		lfd f1, 0x8(t4)
		add t4, t4, t0
		stfd f2, 0xa8(r3)
		stfd f3, 0xe8(r3)
		lfd f2, 0x0(t4)
		lfd f3, 0x8(t4)
		stfd f0, 0xb0(r3)
		stfd f1, 0xf0(r3)
		stfd f2, 0xb8(r3)
		stfd f3, 0xf8(r3)
		}
		break;
	}
	case 4:
	{
		register Uint32 t0;
		register Uint32 t4;
		register Uint32 t5;

		asm {
		lwz t0, 0x20(r3)
		lwz r3, 0x18(r3)
		lwz r6, 0x0(s)
		clrrwi t0, t0, 2
		lwz r7, 0x4(s)
		add t4, s, t0
		lwz r8, 0x8(s)
		lwz t5, 0xc(s)
		stw r6, 0x0(r3)
		stw r7, 0x4(r3)
		stw r8, 0x40(r3)
		stw t5, 0x44(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x8(r3)
		stw r6, 0xc(r3)
		stw r7, 0x48(r3)
		stw r8, 0x4c(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x10(r3)
		stw r6, 0x14(r3)
		stw r7, 0x50(r3)
		stw r8, 0x54(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x18(r3)
		stw r6, 0x1c(r3)
		stw r7, 0x58(r3)
		stw r8, 0x5c(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x20(r3)
		stw r6, 0x24(r3)
		stw r7, 0x60(r3)
		stw r8, 0x64(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x28(r3)
		stw r6, 0x2c(r3)
		stw r7, 0x68(r3)
		stw r8, 0x6c(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x30(r3)
		stw r6, 0x34(r3)
		stw r7, 0x70(r3)
		stw r8, 0x74(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x38(r3)
		stw r6, 0x3c(r3)
		stw r7, 0x78(r3)
		stw r8, 0x7c(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x80(r3)
		stw r6, 0x84(r3)
		stw r7, 0xc0(r3)
		stw r8, 0xc4(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x88(r3)
		stw r6, 0x8c(r3)
		stw r7, 0xc8(r3)
		stw r8, 0xcc(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x90(r3)
		stw r6, 0x94(r3)
		stw r7, 0xd0(r3)
		stw r8, 0xd4(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0x98(r3)
		stw r6, 0x9c(r3)
		stw r7, 0xd8(r3)
		stw r8, 0xdc(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0xa0(r3)
		stw r6, 0xa4(r3)
		stw r7, 0xe0(r3)
		stw r8, 0xe4(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0xa8(r3)
		stw r6, 0xac(r3)
		stw r7, 0xe8(r3)
		stw r8, 0xec(r3)
		lwz t5, 0x0(t4)
		lwz r6, 0x4(t4)
		lwz r7, 0x8(t4)
		lwz r8, 0xc(t4)
		add t4, t4, t0
		stw t5, 0xb0(r3)
		stw r6, 0xb4(r3)
		stw r7, 0xf0(r3)
		stw r8, 0xf4(r3)
		lwz t5, 0x4(t4)
		lwz r6, 0x8(t4)
		lwz r7, 0xc(t4)
		lwz t0, 0x0(t4)
		stw t0, 0xb8(r3)
		stw t5, 0xbc(r3)
		stw r6, 0xf8(r3)
		stw r7, 0xfc(r3)
		}
		break;
	}
	case 2:
	case 6:
	{
		register Uint32 t0;
		register Uint32 t4;
		register Uint32 t5;

		asm {
		lwz t4, 0x20(r3)
		li t0, 0x8
		mr r8, s
		lwz r7, 0x18(r3)
		clrrwi r3, t4, 1
		li r6, 0x0
		mtctr t0
L_802082FC:
		lwz r9, 0x2(r8)
		cmpwi r6, 0x7
		lwz r11, 0xa(r8)
		lhz t4, 0x0(r8)
		srwi t0, r9, 16
		lwz r10, 0x6(r8)
		srwi t5, r11, 16
		lhz r12, 0xe(r8)
		rlwimi t0, t4, 16, 0, 15
		srwi t4, r10, 16
		rlwimi t5, r10, 16, 0, 15
		stw t0, 0x0(r7)
		rlwimi t4, r9, 16, 0, 15
		mr t0, r12
		add r8, r8, r3
		stw t4, 0x4(r7)
		rlwimi t0, r11, 16, 0, 15
		stw t5, 0x40(r7)
		stw t0, 0x44(r7)
		addi r7, r7, 0x8
		bne L_80208354
		addi r7, r7, 0x40
L_80208354:
		lwz r9, 0x2(r8)
		addi r6, r6, 0x1
		lwz r11, 0xa(r8)
		cmpwi r6, 0x7
		lhz t4, 0x0(r8)
		srwi t0, r9, 16
		lwz r10, 0x6(r8)
		srwi t5, r11, 16
		lhz r12, 0xe(r8)
		rlwimi t0, t4, 16, 0, 15
		srwi t4, r10, 16
		rlwimi t5, r10, 16, 0, 15
		stw t0, 0x0(r7)
		rlwimi t4, r9, 16, 0, 15
		mr t0, r12
		add r8, r8, r3
		stw t4, 0x4(r7)
		rlwimi t0, r11, 16, 0, 15
		stw t5, 0x40(r7)
		stw t0, 0x44(r7)
		addi r7, r7, 0x8
		bne L_802083B0
		addi r7, r7, 0x40
L_802083B0:
		addi r6, r6, 0x1
		bdnz L_802082FC
		}
		break;
	}
	case 1:
	case 5:
	{
		register Uint32 t0;
		register Uint32 t4;
		register Uint32 t5;

		asm {
		li t0, 0x8
		mr r8, s
		lwz r7, 0x18(r3)
		li r6, 0x0
		lwz t5, 0x20(r3)
		mtctr t0
L_802083D4:
		dcbt r8, t5
		cmpwi r6, 0x7
		lwz r9, 0x3(r8)
		lwz r10, 0x7(r8)
		lwz t4, -0x1(r8)
		srwi t0, r9, 24
		srwi r3, r10, 24
		lwz r11, 0xb(r8)
		lbz r12, 0xf(r8)
		rlwimi t0, t4, 8, 0, 23
		srwi t4, r11, 24
		rlwimi r3, r9, 8, 0, 23
		stw t0, 0x0(r7)
		rlwimi t4, r10, 8, 0, 23
		mr t0, r12
		add r8, r8, t5
		stw r3, 0x4(r7)
		rlwimi t0, r11, 8, 0, 23
		stw t4, 0x40(r7)
		stw t0, 0x44(r7)
		addi r7, r7, 0x8
		bne L_80208430
		addi r7, r7, 0x40
L_80208430:
		addi r6, r6, 0x1
		dcbt r8, t5
		cmpwi r6, 0x7
		lwz r9, 0x3(r8)
		lwz r10, 0x7(r8)
		lwz t4, -0x1(r8)
		srwi t0, r9, 24
		srwi r3, r10, 24
		lwz r11, 0xb(r8)
		lbz r12, 0xf(r8)
		rlwimi t0, t4, 8, 0, 23
		srwi t4, r11, 24
		rlwimi r3, r9, 8, 0, 23
		stw t0, 0x0(r7)
		rlwimi t4, r10, 8, 0, 23
		mr t0, r12
		add r8, r8, t5
		stw r3, 0x4(r7)
		rlwimi t0, r11, 8, 0, 23
		stw t4, 0x40(r7)
		stw t0, 0x44(r7)
		addi r7, r7, 0x8
		bne L_80208490
		addi r7, r7, 0x40
L_80208490:
		addi r6, r6, 0x1
		bdnz L_802083D4
		}
		break;
	}
	case 3:
	case 7:
	{
		register Uint32 t0;
		register Uint32 t4;
		register Uint32 t5;

		asm {
		li t0, 0x8
		mr r8, s
		lwz r7, 0x18(r3)
		li r6, 0x0
		lwz t5, 0x20(r3)
		mtctr t0
L_802084B4:
		dcbt r8, t5
		cmpwi r6, 0x7
		lwz r9, 0x1(r8)
		lwz r10, 0x5(r8)
		lwz t4, -0x3(r8)
		srwi t0, r9, 8
		srwi r3, r10, 8
		lwz r11, 0x9(r8)
		lwz r12, 0xd(r8)
		rlwimi t0, t4, 24, 0, 7
		srwi t4, r11, 8
		rlwimi r3, r9, 24, 0, 7
		stw t0, 0x0(r7)
		rlwimi t4, r10, 24, 0, 7
		srwi t0, r12, 8
		add r8, r8, t5
		stw r3, 0x4(r7)
		rlwimi t0, r11, 24, 0, 7
		stw t4, 0x40(r7)
		stw t0, 0x44(r7)
		addi r7, r7, 0x8
		bne L_80208510
		addi r7, r7, 0x40
L_80208510:
		addi r6, r6, 0x1
		dcbt r8, t5
		cmpwi r6, 0x7
		lwz r9, 0x1(r8)
		lwz r10, 0x5(r8)
		lwz t4, -0x3(r8)
		srwi t0, r9, 8
		srwi r3, r10, 8
		lwz r11, 0x9(r8)
		lwz r12, 0xd(r8)
		rlwimi t0, t4, 24, 0, 7
		srwi t4, r11, 8
		rlwimi r3, r9, 24, 0, 7
		stw t0, 0x0(r7)
		rlwimi t4, r10, 24, 0, 7
		srwi t0, r12, 8
		add r8, r8, t5
		stw r3, 0x4(r7)
		rlwimi t0, r11, 24, 0, 7
		stw t4, 0x40(r7)
		stw t0, 0x44(r7)
		addi r7, r7, 0x8
		bne L_80208570
		addi r7, r7, 0x40
L_80208570:
		addi r6, r6, 0x1
		bdnz L_802084B4
		}
		break;
	}
	}
}


/* the generic (non-tuned) versions were dead-stripped; the table keeps their slots */
void (*const mpvmc16_oneref1p_func_table[4])(MPVMC *mc) = {NULL, NULL, NULL, NULL};

void MPVMC16_Init(MPVMC *mc)
{
	mc->oneref16[0] = mpvmc16_oneref1p_func_table[0];
	mc->oneref16[1] = mpvmc16_oneref1p_func_table[1];
	mc->oneref16[2] = mpvmc16_oneref1p_func_table[2];
	mc->oneref16[3] = mpvmc16_oneref1p_func_table[3];
}
