/*
 * PbDma.h - Dma functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBDMA_H_
#define _PBDMA_H_

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////

#define DMA_REF_TAG(ADDR, COUNT) ((((unsigned long)ADDR) << 32) | (0x3 << 28) | COUNT )
#define DMA_CNT_TAG(COUNT)       (((unsigned long)(0x1) << 28) | COUNT)
#define DMA_END_TAG(COUNT)       (((unsigned long)(0x7) << 28) | COUNT)
#define DMA_CHCR(DIR,MOD,ASP,TTE,TIE,STR) ( (((u32)DIR)<<0) | (((u32)MOD)<<2) | \
                                            (((u32)ASP)<<4) | (((u32)TTE)<<6) | \
                                            (((u32)TIE)<<7) | (((u32)STR)<<8) )

///////////////////////////////////////////////////////////////////////////////
// Hwregs
///////////////////////////////////////////////////////////////////////////////

#define D0_CHCR   ((volatile unsigned int *)(0x10008000))
#define D0_MADR   ((volatile unsigned int *)(0x10008010))
#define D0_QWC    ((volatile unsigned int *)(0x10008020))
#define D0_TADR   ((volatile unsigned int *)(0x10008030))
#define D0_ASR0   ((volatile unsigned int *)(0x10008040))
#define D0_ASR1   ((volatile unsigned int *)(0x10008050))

#define D1_CHCR   ((volatile unsigned int *)(0x10009000))
#define D1_MADR   ((volatile unsigned int *)(0x10009010))
#define D1_QWC    ((volatile unsigned int *)(0x10009020))
#define D1_TADR   ((volatile unsigned int *)(0x10009030))
#define D1_ASR0   ((volatile unsigned int *)(0x10009040))
#define D1_ASR1   ((volatile unsigned int *)(0x10009050))

#define D2_CHCR   ((volatile unsigned int *)(0x1000a000))
#define D2_MADR   ((volatile unsigned int *)(0x1000a010))
#define D2_QWC    ((volatile unsigned int *)(0x1000a020))
#define D2_TADR   ((volatile unsigned int *)(0x1000a030))
#define D2_ASR0   ((volatile unsigned int *)(0x1000a040))
#define D2_ASR1   ((volatile unsigned int *)(0x1000a050))

#define D_CTRL    ((volatile unsigned int *)(0x1000e000))
#define D_STAT    ((volatile unsigned int *)(0x1000e010))
#define D_PCR     ((volatile unsigned int *)(0x1000e020))
#define D_SQWC    ((volatile unsigned int *)(0x1000e030))
#define D_RBSR    ((volatile unsigned int *)(0x1000e040))
#define D_RBOR    ((volatile unsigned int *)(0x1000e050))
#define D_STADR   ((volatile unsigned int *)(0x1000e060))
#define D_ENABLER ((volatile unsigned int *)(0x1000f520))
#define D_ENABLEW ((volatile unsigned int *)(0x1000f590))

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void* PbDmaBeginSpr();
int   PbDmaEndSpr( void* pData );

void PbDmaSend01ChainSpr( void* pList );
void PbDmaSend01Chain( void* pList );
void PbDmaWait01();

void PbDmaSend02ChainSpr( void* pList );
void PbDmaSend02Chain( void* pList );
void PbDmaSend02Spr( void* pList,int Size );
void PbDmaSend02( void* pList,int Size );
void PbDmaWait02();

#endif//_PBDMA_H_



