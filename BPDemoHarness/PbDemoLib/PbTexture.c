/*
 * PbTexture.c - Texture functions for Pb demo library
 *
 * Copyright (c) 2004   emoon <daniel@collin.com>
 *
 * Some bits based on code from SuperDuper texture handler with
 * some mods by adresd
 *
 * Licensed under the AFL v2.0. See the file LICENSE included with this
 * distribution for licensing terms.
 */

#include <kernel.h>
#include "PbTexture.h"
#include "PbDma.h"
#include "PbSpr.h"
#include "PbGs.h"
#include "PbVram.h"
#include "PbMisc.h"

///////////////////////////////////////////////////////////////////////////////
// Variables, NOT to be accessed from outside this file (use access functions)
///////////////////////////////////////////////////////////////////////////////

static u32 g_TextureCount = 0;
static u32 g_TexturePtr   = 0;
static PbTexture ga_Textures[PB_MAX_TEXTURES];

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload
///////////////////////////////////////////////////////////////////////////////

PbTexture* PbTextureAlloc( int Width, int Height, int Psm )
{
  PbTexture* pTexture;

  if( g_TextureCount > PB_MAX_TEXTURES )
    g_TextureCount = 0;

  g_TextureCount++;

  pTexture = &ga_Textures[g_TextureCount-1];

  switch( Psm )
  {
    case GS_PSMCT32 : pTexture->Vram = PbVramAlloc( Width*Height*4 ); break;
    case GS_PSMT8   : pTexture->Vram = PbVramAlloc( Width*Height   ); break;
    case GS_PSMT4   : pTexture->Vram = PbVramAlloc( Width*Height/4 ); break;
  }

  pTexture->pMem  = NULL;
  pTexture->pClut = NULL;
  pTexture->x     = Width;
  pTexture->y     = Height;
  pTexture->psm   = Psm;

  // if not texture ptr is set we set it to the first alloced texture ptr

  if( g_TexturePtr == 0 )
    g_TexturePtr = pTexture->Vram;  

  return pTexture;
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload
///////////////////////////////////////////////////////////////////////////////

PbTexture* PbTextureCreate32( u32* pData, int Width, int Height )
{
  PbTexture* p_texture = PbTextureAlloc( Width, Height, GS_PSMCT32 );

  p_texture->format = 32;
  p_texture->x      = (u16)Width;
  p_texture->y      = (u16)Height;
  p_texture->pMem   = pData;

  out( "pTexture->format: %d\n", p_texture->format );

  return p_texture;
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload
///////////////////////////////////////////////////////////////////////////////

PbTexture* PbTextureCreate8( char* pData,u32* pPal, int Width, int Height )
{
  PbTexture* p_texture = PbTextureAlloc( Width, Height, GS_PSMT8 );

  p_texture->format = 8;
  p_texture->x      = (u16)Width;
  p_texture->y      = (u16)Height;
  p_texture->pMem   = pData;
  p_texture->pClut  = pPal; 

  return p_texture;
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload( PbTexture* pTexture )
{
  u64* p_data       = NULL;
  u64* p_store      = NULL;

  /////////////////////////////////////////////////////////////////////////////
  // Setup registers

  p_data = p_store = PbSprAlloc( 6*16 );
  *p_data++ = DMA_CNT_TAG( 5 );
  *p_data++ = 0;

  *p_data++ = GIF_TAG( 4, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD;

  /////////////////////////////////////////////////////////////////////////////
  // Setup the blitting & blit it

  *p_data++ = (u64)( ((pTexture->psm & 0x3f) << 24) | (((pTexture->x>>6) & 0x3f)<<16) | 
                      ((pTexture->Vram/256) & 0x3fff) ) << 32;
  *p_data++ = GS_BITBLTBUF;

  *p_data++ = (unsigned long)( ((0)<<16) | (0) ) << 32;  // x,y offsets 0 for now
  *p_data++ = GS_TRXPOS;

  *p_data++= ((unsigned long)(pTexture->y) << 32) | pTexture->x;
  *p_data++ = GS_TRXREG;

  *p_data++ = 0;
  *p_data++ = GS_TRXDIR;

  PbDmaSend02ChainSpr( p_store );

  out( "pTexture->format: %d\n", pTexture->format );
  PB_LINE

  /////////////////////////////////////////////////////////////////////////////
  // select correct method of upload depending on format

  switch( pTexture->psm )
  {
    case GS_PSMCT32 : PbTextureUpload32( pTexture ); break;
    case GS_PSMT8   : PbTextureUpload8( pTexture ); break;
//    case GS_PSMCT4  : PbTextureUpload4( pTexture ); break;
  }

  out( "pTexture->format: %d\n", pTexture->format );
  PB_LINE
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload32
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload32( PbTexture* pTexture )
{
  char* p_tmap_data = NULL;       
  u64* p_data       = NULL;
  u64* p_store      = NULL;
  int n_quads       = 0; 
  int curr_quads    = 0;

  out( "pTexture->format: %d\n", pTexture->format );
  PB_LINE

  n_quads = pTexture->x * pTexture->y;  
  n_quads = (n_quads >> 2) + ((n_quads & 0x03 ) != 0 ? 1 : 0);

  p_tmap_data = pTexture->pMem;

  p_data = p_store = PbSprAlloc( 32*16 );

  /////////////////////////////////////////////////////////////////////////////
  // send the data

  while( n_quads )
  {
    if( n_quads > PB_IMAGE_MAX_COUNT ) 
       curr_quads = PB_IMAGE_MAX_COUNT;
    else 
      curr_quads = n_quads;

    *p_data++ = DMA_CNT_TAG( 1 );
    *p_data++ = 0; // pad

    *p_data++ = 0x0800000000000000 | curr_quads; // TODO: use proper macro
    *p_data++ = 0; // pad

    *p_data++ = DMA_REF_TAG( (u32)p_tmap_data, curr_quads );
    *p_data++ = 0; // pad

    n_quads     -= curr_quads;
    p_tmap_data += (curr_quads << 4);
  }

  /////////////////////////////////////////////////////////////////////////////
  // end the list

  *p_data++ = DMA_END_TAG( 0 );
  *p_data++ = 0;

  out( "pTexture->format: %d\n", pTexture->format );
  PB_LINE

  /////////////////////////////////////////////////////////////////////////////
  // And send it

  PbDmaSend02ChainSpr( p_store );
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload8
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload8( PbTexture* pTexture )
{
  char* p_tmap_data = NULL;       
  u64* p_data       = NULL;
  u64* p_store      = NULL;
  int n_quads       = 0; 
  int curr_quads    = 0;

  n_quads     = pTexture->x * pTexture->y / 4;  
  p_tmap_data = pTexture->pMem;

  p_data = p_store = PbSprAlloc( 32*16 );

  out( "n_quads %d\n", n_quads );

  /////////////////////////////////////////////////////////////////////////////
  // send the data

  while( n_quads )
  {
    if( n_quads > PB_IMAGE_MAX_COUNT ) 
       curr_quads = PB_IMAGE_MAX_COUNT;
    else 
      curr_quads = n_quads;

    out( "curr_quads %d\n", curr_quads );

    *p_data++ = DMA_CNT_TAG( 1 );
    *p_data++ = 0; // pad

    *p_data++ = 0x0800000000000000 | curr_quads; // TODO: use proper macro
    *p_data++ = 0; // pad

    *p_data++ = DMA_REF_TAG( (u32)p_tmap_data, curr_quads );
    *p_data++ = 0; // pad

    n_quads     -= curr_quads;
    p_tmap_data += (curr_quads << 4);
  }

  /////////////////////////////////////////////////////////////////////////////
  // end the list

  *p_data++ = DMA_END_TAG( 0 );
  *p_data++ = 0;

  /////////////////////////////////////////////////////////////////////////////
  // And send it

  PbDmaSend02ChainSpr( p_store );

  /////////////////////////////////////////////////////////////////////////////
  // Setup pal

  PbTextureSetupPal( pTexture );  
}

///////////////////////////////////////////////////////////////////////////////
// void PbTextureUpload4
///////////////////////////////////////////////////////////////////////////////

void PbTextureUpload4( PbTexture* pTexture )
{
}

///////////////////////////////////////////////////////////////////////////////
// u64 PbTextureGetTex0
// TODO: Clean this up
///////////////////////////////////////////////////////////////////////////////

u64 PbTextureGetTex0( PbTexture* pTexture )
{
  if( pTexture->format == 32 )
  {
    return ((u64)1<<34) + 
           ((u64)8<<30) + 
           ((u64)8<<26) + 
           (0<<20) + 
           (((pTexture->x/64)&63)<<14) + 
           (pTexture->Vram/256);
  }
  
  // 8bit
/*
  return ((int64)1<<61)+
         (((int64)tex->clutaddr/256)<<37)+
         ((int64)1<<34)+
         (tex->TH<<30)+
         (tex->TW<<26)+
         (0x13<<20)+
         (((tex->width/64)&63)<<14)+
         (tex->addr/256);
*/

  return ((u64)1<<61)+
         (((u64)pTexture->VramClut/256)<<37) + 
         (((u64)0)<<34) + 
         (((u64)8)<<30) +
         (((u64)8)<<26) +
         (((u64)GS_PSMT8)<<20) + 
         (((pTexture->x/64)&63)<<14) + 
         (pTexture->Vram/256);
}

///////////////////////////////////////////////////////////////////////////////
// Setup pal

void  PbTextureSetupPal( PbTexture* pTexture )
{
  u64* p_data  = NULL;
  u64* p_store = NULL;
  int i = 0;

  p_data = p_store = PbSprAlloc( 32*16 );

  /////////////////////////////////////////////////////////////////////////////
  // Alloc vram for the clut

  pTexture->VramClut = PbVramAlloc( 16*16*4 );

  out( "pTexture->VramClut 0x%x\n", pTexture->VramClut );

  /////////////////////////////////////////////////////////////////////////////
  // Setup the blitting for the clut

  *p_data++ = DMA_CNT_TAG( 5 );
  *p_data++ = 0;

  *p_data++ = GIF_TAG( 4, 1, 0, 0, 0, 1 ); 
  *p_data++ = GIF_AD;

  *p_data++ = (u64)( ((0 & 0x3f) << 24) | ((16 & 0x3f)<<16) | 
                     ((pTexture->VramClut/256) & 0x3fff) ) << 32;
  *p_data++ = GS_BITBLTBUF;

  *p_data++ = (unsigned long)( ((0)<<16) | (0) ) << 32;  // x,y offsets 0 for now
  *p_data++ = GS_TRXPOS;

  *p_data++= ((unsigned long)(16) << 32) | 16;
  *p_data++ = GS_TRXREG;

  *p_data++ = 0;
  *p_data++ = GS_TRXDIR;

  ////////////////////////////////////////////////////////////////////////////
  // Swap the colors in the clut to CSM1 format

  for( i = 0; i < 256; i++ )
  {   
    if( ( i & 0x18 ) == 8 )
    {
      int temp;

      temp = pTexture->pClut[i];
      pTexture->pClut[i] = pTexture->pClut[i+8];
      pTexture->pClut[i+8] = temp;
    }
  }

  FlushCache( 0 );

  ////////////////////////////////////////////////////////////////////////////
  // setup sending of the clut

  *p_data++ = DMA_CNT_TAG( 1 );
  *p_data++ = 0; // pad

  *p_data++ = 0x0800000000000000 | 64;
  *p_data++ = 0; // pad

  *p_data++ = DMA_REF_TAG( (u32)pTexture->pClut, 64 );
  *p_data++ = 0; // pad

  *p_data++ = DMA_END_TAG( 0 );
  *p_data++ = 0; // pad

  PbDmaSend02ChainSpr( p_store );
  PbDmaWait02();

  ////////////////////////////////////////////////////////////////////////////
  // Swap the colors back to normal format

  for( i = 0; i < 256; i++ )
  {   
    if( ( i & 0x18 ) == 8 )
    {
      int temp;

      temp = pTexture->pClut[i];
      pTexture->pClut[i] = pTexture->pClut[i+8];
      pTexture->pClut[i+8] = temp;
    }
  }
}

