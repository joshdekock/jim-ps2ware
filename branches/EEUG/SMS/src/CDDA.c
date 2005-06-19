/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2002 Nicholas Van Veen (nickvv@xtra.co.nz)
#     2003 loser (loser@internalreality.com)
# (c) 2004 Marcus R. Brown <mrbrown@0xd6.org>
#     2005 - Adopted for SMS by Eugene Plotnikov <e-plotnikov@operamail.com>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "CDDA.h"
#include "SIF.h"

#include <kernel.h>
#include <string.h>

#define CD_SERVER_INIT 0x80000592
#define CD_SERVER_SCMD 0x80000593
#define CD_SERVER_NCMD 0x80000595

#define CDVD_INIT_INIT 0x00
#define CDVD_INIT_EXIT 0x05

#define CD_CMD_CDDAREAD    0x02
#define CD_CMD_GETDISCTYPE 0x03
#define CD_CMD_GETTOC      0x04
#define CD_CMD_STANDBY     0x06
#define CD_CMD_STOP        0x07
#define CD_CMD_PAUSE       0x08

static volatile int       s_SyncFlag;
static int                s_Size;
static void*              s_pBuf;
static SifRpcClientData_t s_ClientInit         __attribute__ (   (  aligned( 64 )  )   );
static SifRpcClientData_t s_ClientNCmd         __attribute__ (   (  aligned( 64 )  )   );
static SifRpcClientData_t s_ClientSCmd         __attribute__ (   (  aligned( 64 )  )   );
static unsigned char      s_NCmdRecvBuff[ 48 ] __attribute__ (   (  aligned( 64 )  )   );
static unsigned char      s_SCmdRecvBuff[ 48 ] __attribute__ (   (  aligned( 64 )  )   );
static unsigned int       s_InitMode           __attribute__ (   (  aligned( 64 )  )   );
static unsigned int       s_GetTOCmd[    3 ]   __attribute__ (   (  aligned( 64 )  )   );
static unsigned char      s_TOCBuf  [ 2064 ]   __attribute__ (   (  aligned( 64 )  )   );
static unsigned int       s_ReadData[    6 ]   __attribute__ (   (  aligned( 64 )  )   );
static unsigned int       s_ReadResp[   64 ]   __attribute__ (   (  aligned( 64 )  )   );

int CDDA_Init ( void ) {

 int retVal = 1;

 if ( !s_ClientInit.server )

  if (   (  retVal = SIF_BindRPC ( &s_ClientInit, CD_SERVER_INIT )  ) &&
         (  retVal = SIF_BindRPC ( &s_ClientNCmd, CD_SERVER_NCMD )  ) &&
         (  retVal = SIF_BindRPC ( &s_ClientSCmd, CD_SERVER_SCMD )  )
  )  {

   s_InitMode = CDVD_INIT_INIT;
   SifWriteBackDCache ( &s_InitMode, 4 );

   retVal = SifCallRpc ( &s_ClientInit, 0, 0, &s_InitMode, 4, 0, 0, 0, 0 ) >= 0;

  }  /* end if */

 return retVal;

}  /* end CDDA_Init */

void CDDA_Exit ( void ) {

 if ( s_ClientInit.server ) {

  s_InitMode = CDVD_INIT_EXIT;

  SifWriteBackDCache ( &s_InitMode, 4 );
  SifCallRpc ( &s_ClientInit, 0, 0, &s_InitMode, 4, 0, 0, 0, 0 );

  s_ClientInit.server = 0;

 }  /* end if */

}  /* end CDDA_Exit */

int CDDA_ReadTOC ( CDDA_TOC* apTOC ) {

 int retVal = 0;
 unsigned char* lpTOCbegin;
 unsigned char* lpTOCend;

 s_GetTOCmd[ 0 ] = ( unsigned int )s_TOCBuf;

 SifWriteBackDCache ( s_TOCBuf, 2064 );
 SifWriteBackDCache ( s_GetTOCmd, 12 );

 if (  SifCallRpc (
        &s_ClientNCmd, CD_CMD_GETTOC, 0, s_GetTOCmd, 12, s_NCmdRecvBuff, 8, 0, 0
       ) >= 0
 ) {

  lpTOCbegin = UNCACHED_SEG( s_TOCBuf );

  if (  *( unsigned int* )( s_NCmdRecvBuff + 4 )  ) {

   lpTOCend = lpTOCbegin + 1024;

   do {

    memcpy ( apTOC, lpTOCbegin, 32 );
    lpTOCbegin += 32;
    apTOC       = ( CDDA_TOC* )(   (  ( unsigned char* )apTOC  ) + 32   );

   } while ( lpTOCbegin < lpTOCend );

  } else {

   lpTOCend = lpTOCbegin + 2048;

   do {

    memcpy ( apTOC, lpTOCbegin, 32 );
    lpTOCbegin += 32;
    apTOC       = ( CDDA_TOC* )(   (  ( unsigned char* )apTOC  ) + 32   );

   } while ( lpTOCbegin < lpTOCend );

   memcpy ( apTOC, lpTOCbegin, 16 );

  }  /* end else */

  retVal = 1;

 }  /* end if */

 return retVal;

}  /* end CDDA_ReadTOC */

int CDDA_IsAudioDisk ( void ) {

 int retVal = 0;

 if (  SifCallRpc (
        &s_ClientSCmd, CD_CMD_GETDISCTYPE, 0, 0, 0, s_SCmdRecvBuff, 4, 0, 0
       ) >= 0
 ) retVal = *( int* )UNCACHED_SEG( s_SCmdRecvBuff ) == 0xFD;

 return retVal;

}  /* end CDDA_IsAudioDisk */

int CDDA_Synchronize ( void ) {

 SifWriteBackDCache ( s_pBuf, s_Size );

 while ( s_SyncFlag );

 return 1;

}  /* end CDDA_Syncronize */

static void _cd_callback ( void* apParam ) {

 s_SyncFlag = 0;

}  /* end _cd_callback */

void CDDA_Standby ( void ) {

 s_SyncFlag = 1;

 if (  SifCallRpc (
        &s_ClientNCmd, CD_CMD_STANDBY, SIF_RPC_M_NOWAIT,
        0, 0, 0, 0, _cd_callback, NULL
       ) < 0
 ) s_SyncFlag = 0;

}  /* end CDDA_Standby */

void CDDA_Pause ( void ) {

 s_SyncFlag = 1;

 if ( SifCallRpc (
       &s_ClientNCmd, CD_CMD_PAUSE, SIF_RPC_M_NOWAIT,
       0, 0, 0, 0, _cd_callback, NULL
      ) < 0
 ) s_SyncFlag = 0;

}  /* end CDDA_Pause */

int CDDA_Stop ( void ) {

 int retVal = 0;

 s_SyncFlag = 1;

 if (  SifCallRpc ( 
        &s_ClientNCmd, CD_CMD_STOP, SIF_RPC_M_NOWAIT, 0, 0, 0, 0, _cd_callback, NULL
       ) >= 0
 )

  retVal = 1;

 else s_SyncFlag = 0;

 return retVal;

}  /* end CDDA_Stop */
/* This is a compromise between speed and noise issued by DVD drive */
/* Using higher speeds results annoyning ticks in my SCPH-30004R    */
#define READ_SPEED 3

int CDDA_RawRead ( int aStartSec, int aCount, unsigned char* apBuf ) {

 int retVal = 0;

 s_ReadData[ 0 ] = aStartSec;
 s_ReadData[ 1 ] = aCount;
 s_ReadData[ 2 ] = ( unsigned int  )apBuf;
 s_ReadData[ 3 ] = 50 | ( READ_SPEED << 8 );
 s_ReadData[ 4 ] = ( unsigned int )s_ReadResp;

 SifWriteBackDCache ( s_ReadResp, 144 );
 SifWriteBackDCache ( s_ReadData, 24  );

 s_SyncFlag = 1;
 s_Size     = aCount * 2352;
 s_pBuf     = apBuf;

 if (  SifCallRpc (
        &s_ClientNCmd, CD_CMD_CDDAREAD, SIF_RPC_M_NOWAIT | SIF_RPC_M_NOWBDC, s_ReadData, 24,
        NULL, 0, ( void* )_cd_callback, s_ReadResp
       ) >= 0
 )

  retVal = 1;

 else s_SyncFlag = 0;

 return retVal;

}  /* end CDDA_RawRead */
