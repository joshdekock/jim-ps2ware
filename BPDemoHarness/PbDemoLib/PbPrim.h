/*
 * PbPrim.h - Primitive functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#ifndef _PBPRIM_H_
#define _PBPRIM_H_

#include <tamtypes.h>
#include "PbTexture.h"
#include "PbMisc.h"

///////////////////////////////////////////////////////////////////////////////
// Defines
///////////////////////////////////////////////////////////////////////////////

#define PB_ENABLE  TRUE
#define PB_DISABLE FALSE

///////////////////////////////////////////////////////////////////////////////
// Enums
///////////////////////////////////////////////////////////////////////////////

typedef enum
{
  PB_ALPHA_BLENDING,

} PbPrimState;

#define PB_CONTEXT_1 0
#define PB_CONTEXT_2 1

///////////////////////////////////////////////////////////////////////////////
// DATA
///////////////////////////////////////////////////////////////////////////////

extern u64* PbGifPtr; 
extern int  PbGifLength; 

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

void PbPrimSprite( int x1, int y1, int x2, int y2, int z, int color ); 
void PbPrimSpriteNoZtest( int x1, int y1, int x2, int y2, int z, int color ); 

void PbPrimSpriteTexture( PbTexture* pTex, int x1, int y1, int u1, int v1, 
                          int x2, int y2, int u2, int v2, int z, int color ); 

void PbPrimSetState( PbPrimState State, int Value );
void PbPrimSetContext( int State );

void PbPrimSetAlpha( u64 A, u64 B, u64 C, u64 D, u64 FIX );

void PbPrimTriangleTexture( PbTexture* pTex, int x1, int y1, int u1, int v1,  
                                             int x2, int y2, int u2, int v2,  
                                             int x3, int y3, int u3, int v3,  
                                             int z, int color  ); 

void PbPrimQuadTextureGouraud(PbTexture* pTex, int x1, int y1, int u1, int v1,
                               int x2, int y2, int u2, int v2, 
                               int x3, int y3, int u3, int v3, 
                               int x4, int y4, int u4, int v4, int z,
                               int color1, int color2, int color3, int color4);

#define PbGifListAdd(REG, DATA) { *PbGifPtr++ = (u64)(DATA); *PbGifPtr++ = (u64)(REG); PbGifLength++; } 

void PbGifListBegin(); 
void PbGifListSend(); 

#endif // _PBPRIM_H_

