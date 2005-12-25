/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 USB support by weltall
# (c) 2005 HOST support by Ronald Andersson (AKA: dlanor)
# (c) 2005 second gamepad support by bix64
# Special thanks to 'bigboss'/PS2Reality for valuable information
# about SifAddCmdHandler function
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "GUI.h"
#include "GUI_Data.h"

#include "SMS.h"
#include "DMA.h"
#include "PAD.h"

#include "Timer.h"
#include "CDDA.h"
#include "Config.h"
#include "ExecIOP.h"

#include <kernel.h>
#include <string.h>
#include <malloc.h>
#include <libmc.h>
#include <loadfile.h>
#include <iopcontrol.h>
#include <sifcmd.h>
#include <stdio.h>
#include <libhdd.h>

#define STR_AVAILABLE_MEDIA "Available media: "
#define USB_FLAG_CONNECT    0x00000001
#define USB_FLAG_DISCONNECT 0x00000002

extern void* _gp;

static GUIContext                 s_GUICtx;
static unsigned char              s_PadBuf0[  256 ] __attribute__(   (  aligned( 64 )  )   );
static unsigned char              s_PadBuf1[  256 ] __attribute__(   (  aligned( 64 )  )   );
static unsigned char              s_Stack[  32768 ] __attribute__(   (  aligned( 16 )  )   );
static int                        s_GUIThreadID;
static int                        s_MainThreadID;
static int                        s_Stop;
static int                        s_CDROM;
static int                        s_USBFlags;
static int                        s_HostFlags;
static int                        s_CDFSFlags;
static int                        s_DiskDetect;
static int                        s_NoDevCheck;
static int                        s_PadSema;
static volatile unsigned long int s_Event;
static unsigned int               s_PrevBtn;
static unsigned int               s_Init;
static unsigned long int          s_PPTS;

static void ( *DoTimer ) ( void );

static unsigned char* const s_FSIcons[ 2 ] = {
 g_ImgFolder, g_ImgPart
};

static unsigned char* const s_FileIcons[ 4 ] = {
 g_ImgFile, g_ImgAVI, g_ImgMP3, g_ImgM3U
};

extern void GUIStub_DrawBackground ( void );

static void GUI_Status ( char* apSts ) {

 int lLen      = strlen ( apSts );
 int lWidth    = g_GSCtx.TextWidth ( apSts, lLen );
 int lY        = g_GSCtx.m_Height - 36;
 int lCY       = lY;
 int lScrWidth = g_GSCtx.m_Width - 24;

 lCY = lY >> 1;

 while ( lWidth >= lScrWidth ) {

  --lLen;
  lWidth = g_GSCtx.TextWidth ( apSts, lLen );

 }  /* end while */

 g_GSCtx.CopyFBuffer ( 0, 1, lCY, g_GSCtx.m_Width, 16 );
 g_GSCtx.m_Font.m_BkMode = GSBkMode_Transparent;
 g_GSCtx.DrawText ( 6, lY, 0, apSts, lLen, 0 );

}  /* end GUI_Status */

static void GUI_Progress ( char* apSts, unsigned int aPos ) {

 static u64* s_lpPrgList;
 static int  s_lY;
 static int  s_lCY;
 static int  s_lLen;
 static int  s_lScrWidth;

 int lWidth;

 aPos = SMS_clip ( aPos, 0, 100 );

 if ( apSts ) {

  int lWidth;

  s_lY        = g_GSCtx.m_Height - 36;
  s_lLen      = strlen ( apSts );
  s_lCY       = s_lY / 2;
  s_lScrWidth = g_GSCtx.m_Width - 28;
  lWidth      = g_GSCtx.TextWidth ( apSts, s_lLen );

  while ( lWidth >= s_lScrWidth ) {

   --s_lLen;
   lWidth = g_GSCtx.TextWidth ( apSts, s_lLen );

  }  /* end while */

  if ( s_lpPrgList ) {

   free ( s_lpPrgList );
   s_lpPrgList = NULL;

  }  /* end if */

  s_lLen = g_GSCtx.TextGSPacket ( 6, s_lY, 0, apSts, 0, &s_lpPrgList, 0 );

 } else if ( !s_lpPrgList ) return;

 g_GSCtx.CopyFBuffer ( 0, 1, s_lCY, g_GSCtx.m_Width, 16 );

 g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0x80, 0x80, 0xFF, 0x50 );
 g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0x80, 0x80, 0xFF, 0x50 );

 lWidth = (  ( g_GSCtx.m_Width - 2 ) * aPos  ) / 100;

 if ( lWidth > 16 ) g_GSCtx.RoundRect ( 1, s_lY, lWidth, s_lY + 30, 8 );

 DMA_Send ( DMA_CHANNEL_VIF1, s_lpPrgList, s_lLen );

}  /* end GUI_Progress */

static void GUI_DrawDesktop ( void ) {

 g_GSCtx.ClearScreen (  GS_SETREG_RGBA( 0, 0, 0, 0 )  );

 GUIStub_DrawBackground ();

 g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0, 0, 0, 0x80 );
 g_GSCtx.m_LineColor = g_Palette[ g_Config.m_BrowserIBCIdx - 1 ];
 g_GSCtx.RoundRect (
  0, g_GSCtx.m_Height - 38, g_GSCtx.m_Width - 1, g_GSCtx.m_Height - 2, 8
 );
 g_GSCtx.RoundRect ( 0,  2, g_GSCtx.m_Width - 1, 56, 8 );
 g_GSCtx.RoundRect ( 0, 62, g_GSCtx.m_Width - 1, g_GSCtx.m_Height - 44, 8 );

 g_GSCtx.CopyFBuffer (
  1, 0, 0, g_GSCtx.m_Width, g_GSCtx.m_Height >> 1
 );

 g_GSCtx.DrawText ( 6, s_GUICtx.m_DevMenu.m_StartY + 10, 0, STR_AVAILABLE_MEDIA, 0, 0 );

}  /* end GUI_DrawDesktop */

static void GUI_Destroy ( void ) {

 void* lpPtr;

 s_Stop = 1;

 WakeupThread ( s_GUIThreadID );
 SleepThread ();

 Timer_RegisterHandler ( 0, NULL );

 DeleteSema ( s_PadSema );

 lpPtr = s_GUICtx.m_DevMenu.m_pItems;

 while ( lpPtr ) {

  void* lpNext = (  ( GUIDeviceMenuItem* )lpPtr  ) -> m_pNext;

  free ( lpPtr );

  lpPtr = lpNext;

 }  /* end while */

 s_GUICtx.ClearFileMenu ();

}  /* end GUI_Destroy */

unsigned long int GUI_WaitEvent ( void ) {

 unsigned long int retVal;

 while ( 1 ) {

  s_PPTS = g_Timer + g_Config.m_PowerOff;

  WaitSema ( s_PadSema );

  retVal  = s_Event;
  s_Event = 0LL;

  if ( retVal & PAD_SELECT )

   switch ( retVal ) {

    case PAD_SELECT | PAD_R1    : g_GSCtx.AdjustDisplay (  1,  0 ); continue;
    case PAD_SELECT | PAD_R2    : g_GSCtx.AdjustDisplay (  0,  1 ); continue;
    case PAD_SELECT | PAD_L1    : g_GSCtx.AdjustDisplay ( -1,  0 ); continue;
    case PAD_SELECT | PAD_L2    : g_GSCtx.AdjustDisplay (  0, -1 ); continue;
    case PAD_SELECT | PAD_CIRCLE: hddPowerOff ();

   }  /* end switch */

  break;

 }  /* end while */

 return retVal;

}  /* end GUI_WaitEvent */

static void TimerProcRepeat ( void );

static void TimerProc ( void ) {

 static int s_Repeat = 0;

 unsigned int lCurrBtn = PAD_Read ( 0, 0 );

 if ( !lCurrBtn ) lCurrBtn = PAD_Read ( 1, 0 );

 if ( lCurrBtn ) {

  if ( lCurrBtn != s_PrevBtn ) {

   s_PrevBtn  = lCurrBtn;
   s_Repeat   = 0;
   s_Event   |= lCurrBtn;
   SignalSema ( s_PadSema );

  } else if ( ++s_Repeat == 8 ) {

   s_Repeat = 0;
   DoTimer  = TimerProcRepeat;

  }  /* end if */

 } else {

  s_PrevBtn = 0;
  s_Repeat  = 0;

  if ( g_Config.m_PowerOff > 0 && g_Timer >= s_PPTS ) hddPowerOff ();

 }  /* end else */

}  /* end TimerProc */

static void TimerProcRepeat ( void ) {

 unsigned int lCurrBtn = PAD_Read ( 0, 0 );

 if ( !lCurrBtn ) lCurrBtn = PAD_Read ( 1, 0 );
 
 if ( lCurrBtn ) {

  if ( lCurrBtn != s_PrevBtn ) {

   s_PrevBtn  = lCurrBtn;
   s_Event   |= lCurrBtn;

   DoTimer = TimerProc;

  } else s_Event |= lCurrBtn;

  SignalSema ( s_PadSema );

 } else s_PrevBtn = 0;

}  /* end TimerProcRepeat */

static int _gui_thread ( void* apParam ) {

 DiskType lDiskType;

 while ( 1 ) {

  SleepThread ();

  if ( s_Stop ) break;

  if ( !s_NoDevCheck ) {

   if ( g_SMSFlags & SMS_FLAG_USB ) {

    if ( s_USBFlags & USB_FLAG_CONNECT ) {

     s_Event    |= GUI_EF_USBM_MOUNT;
     s_USBFlags &= ~USB_FLAG_CONNECT;
     SignalSema ( s_PadSema );

     continue;

    } else if ( s_USBFlags & USB_FLAG_DISCONNECT ) {

     s_Event    |= GUI_EF_USBM_UMOUNT;
     s_USBFlags &= ~USB_FLAG_DISCONNECT;
     SignalSema ( s_PadSema );

     continue;

    }  /* end if */

   }  /* end if */

   lDiskType = CDDA_DiskType ();

   if ( s_CDROM && !s_CDFSFlags && lDiskType != DiskType_CDDA ) {

    s_Event |= GUI_EF_CDROM_UMOUNT;
    SignalSema ( s_PadSema );

    continue;

   } else if (  !s_CDROM && lDiskType == DiskType_CDDA  ) {

    s_Event |= GUI_EF_CDROM_MOUNT;
    SignalSema ( s_PadSema );

    continue;

   } else if ( s_CDFSFlags && lDiskType != DiskType_CD  &&
                              lDiskType != DiskType_DVD &&
                              lDiskType != DiskType_DVDV
          ) {

    s_Event    |= GUI_EF_CDFS_UMOUNT;
    s_CDFSFlags = 0;
    SignalSema ( s_PadSema );

    continue;

   } else if (  !s_CDFSFlags && ( lDiskType == DiskType_CD  ||
                                  lDiskType == DiskType_DVD ||
                                  lDiskType == DiskType_DVDV
                                )
          ) {

    s_Event    |= GUI_EF_CDFS_MOUNT;
    s_CDFSFlags = 1;
    SignalSema ( s_PadSema );

    continue;

   } else if ( lDiskType == DiskType_Detect ) {

    s_Event     |= GUI_EF_DISK_DETECT;
    s_DiskDetect = 1;
    SignalSema ( s_PadSema );

    continue;

   } else if ( lDiskType == DiskType_Unknown && s_DiskDetect ) {

    s_Event     |= GUI_EF_DISK_NODISK;
    s_DiskDetect = 0;
    SignalSema ( s_PadSema );

   }  /* end if */
#ifndef NO_HOST_SUPPORT
   if ( g_SMSFlags & SMS_FLAG_NET ) {

    static int s_Cntr;

    int lFD;

    if (  !( s_Cntr++ & 0xF )  ) {

     lFD = fioDopen ( "host:" );

     if ( lFD >= 0 ) {

      fioDclose ( lFD );

      if ( !s_HostFlags ) {

       s_Event    |= GUI_EF_HOST_MOUNT;
       s_HostFlags = 1;
       SignalSema ( s_PadSema );

       continue;

      }  /* end if */

     } else {

      if ( s_HostFlags ) {

       s_Event    |= GUI_EF_HOST_UMOUNT;
       s_HostFlags = 0;
       SignalSema ( s_PadSema );

       continue;

      }  /* end if */

     }  /* end else */

    }  /* end if */

   }  /* end if */
#endif  /* NO_HOST_SUPPORT */
  }  /* end if */

  DoTimer ();

 }  /* end while */

 WakeupThread ( s_MainThreadID );

 return ExitDeleteThread (), 0;

}  /* end _gui_thread */

static void GUI_DrawDeviceMenu ( GUIDeviceMenu* apMenu ) {

 int                lX, lY;
 GUIDeviceMenuItem* lpItem = apMenu -> m_pItems;

 lX = apMenu -> m_StartX;
 lY = apMenu -> m_StartY;

 g_GSCtx.CopyFBuffer (
  0, lX, lY, g_GSCtx.m_Width - lX - 10, 24
 );

 if ( !lpItem ) {

  g_GSCtx.DrawText (
   s_GUICtx.m_DevMenu.m_StartX, s_GUICtx.m_DevMenu.m_StartY + 10, 0, "none", 0, 0
  );

  return;

 }  /* end if */

 while ( lpItem ) {

  g_GSCtx.DrawIcon ( lX + 2, lY + 2, GSIS_48x48, lpItem -> m_pImage );

  if ( lpItem == apMenu -> m_pCurr ) {

   g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x60 );
   g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x60 );
   g_GSCtx.RoundRect ( lX + 4, lY + 6, lX + 50, lY + 48, 8 );

  }  /* end if */

  if ( lpItem == apMenu -> m_pSel ) {

   g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x80 );
   g_GSCtx.m_LineColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x00 );
   g_GSCtx.RoundRect ( lX + 4, lY + 6, lX + 50, lY + 48, 8 );

  }  /* end if */

  lX    += 52;
  lpItem = lpItem -> m_pNext;

 }  /* end while */

}  /* end GUI_DrawDeviceMenu */

static void GUI_AddDevice ( int aDev ) {

 GUIDeviceMenuItem* lpItem = ( GUIDeviceMenuItem* )calloc (  1, sizeof ( GUIDeviceMenuItem )  );

 if ( aDev == GUI_DF_CDROM ) {

  lpItem -> m_pImage = g_ImgCDROM;
  s_CDROM = 1;

 } else if ( aDev == GUI_DF_HDD )

  lpItem -> m_pImage = g_ImgHDD;

 else if ( aDev == GUI_DF_CDDA ) {

  lpItem -> m_pImage = g_ImgCDDA;
  s_CDROM = 1;

 } else if ( aDev == GUI_DF_USBM ) {

  lpItem -> m_pImage = g_ImgUSB;

 } else if ( aDev == GUI_DF_HOST ) {

  lpItem -> m_pImage = g_ImgHost;

 } else if ( aDev == GUI_DF_CDFS ) {

  lpItem -> m_pImage = g_ImgCDROM;

 } else if ( aDev == GUI_DF_DVD ) {

  lpItem -> m_pImage = g_ImgDVD;

 }  /* end if */

 if ( !s_GUICtx.m_DevMenu.m_pItems ) {

  s_GUICtx.m_DevMenu.m_pItems = lpItem;
  s_GUICtx.m_DevMenu.m_pCurr  = lpItem;
  s_GUICtx.m_DevMenu.m_pSel   = lpItem;

 } else {

  GUIDeviceMenuItem* lpItems = s_GUICtx.m_DevMenu.m_pItems;

  while ( lpItems -> m_pNext ) lpItems = lpItems -> m_pNext;

  lpItems -> m_pNext = lpItem;
  lpItem  -> m_pPrev = lpItems;

 }  /* end else */

 lpItem -> m_Flags = aDev;

 GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu );

}  /* end GUI_AddDevice */

static void GUI_DelDevice ( int aDev ) {

 GUIDeviceMenuItem* lpItem = s_GUICtx.m_DevMenu.m_pItems;

 while ( lpItem ) {

  if ( aDev == lpItem -> m_Flags ) {

   if ( lpItem -> m_pPrev ) lpItem -> m_pPrev -> m_pNext = lpItem -> m_pNext;
   if ( lpItem -> m_pNext ) lpItem -> m_pNext -> m_pPrev = lpItem -> m_pPrev;

   if ( s_GUICtx.m_DevMenu.m_pCurr == lpItem ) s_GUICtx.m_DevMenu.m_pCurr = lpItem -> m_pPrev ? lpItem -> m_pPrev : lpItem -> m_pNext;
   if ( s_GUICtx.m_DevMenu.m_pSel  == lpItem ) s_GUICtx.m_DevMenu.m_pSel  = lpItem -> m_pPrev ? lpItem -> m_pPrev : lpItem -> m_pNext;

   if ( lpItem == s_GUICtx.m_DevMenu.m_pItems ) s_GUICtx.m_DevMenu.m_pItems = s_GUICtx.m_DevMenu.m_pCurr;

   free ( lpItem );

   if ( aDev == GUI_DF_CDROM || aDev == GUI_DF_CDDA ) s_CDROM = 0;

   break;

  }  /* end if */

  lpItem = lpItem -> m_pNext;

 }  /* end while */

 GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu );

}  /* end GUI_DelDevice */

static GUIDeviceMenuItem* GUI_NavigateDevice ( unsigned long int anEvent ) {

 if ( !s_GUICtx.m_DevMenu.m_pSel ) return ( GUIDeviceMenuItem* )GUI_EV_MENU_EMPTY;

 if ( anEvent & PAD_LEFT ) {

  if ( s_GUICtx.m_DevMenu.m_pSel -> m_pPrev ) {

   s_GUICtx.m_DevMenu.m_pSel = s_GUICtx.m_DevMenu.m_pSel -> m_pPrev;
   GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu );

  }  /* end if */

 } else if ( anEvent & PAD_RIGHT ) {

  if ( s_GUICtx.m_DevMenu.m_pSel -> m_pNext ) {

   s_GUICtx.m_DevMenu.m_pSel = s_GUICtx.m_DevMenu.m_pSel -> m_pNext;
   GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu );

  }  /* end if */

 } else if ( anEvent & PAD_CROSS ) {

  s_GUICtx.m_DevMenu.m_pCurr = s_GUICtx.m_DevMenu.m_pSel;
  GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu );

  return s_GUICtx.m_DevMenu.m_pCurr;

 } else if ( anEvent & PAD_DOWN ) {

  return ( GUIDeviceMenuItem* )GUI_EV_MENU_QUIT;

 }  /* end if */

 return ( GUIDeviceMenuItem* )GUI_EV_CONSUMED;

}  /* end GUI_NavigateDevice */

static void TimerHandler ( void ) {

 iWakeupThread ( s_GUIThreadID );

}  /* end TimerHandler */

static void GUI_DrawDimmedFileItem ( GUIFileMenuItem* apItem, int anY ) {

 int           i = 3, lTextWidth = g_GSCtx.TextWidth ( apItem -> m_pFileName, 0 );
 int           lStrLen = strlen ( apItem -> m_pFileName );
 unsigned char lBuff[ 4096 ] __attribute__(   (  aligned( 16 )  )   );

 memcpy ( lBuff, apItem -> m_pIcon, 4096 );

 while ( i < 4096 ) {

  lBuff[ i ] >>= 1;
  i += 4;

 }  /* end while */

 SyncDCache ( lBuff, &lBuff[ 4096 ] );

 while ( lTextWidth >= s_GUICtx.m_FileMenu.m_Width ) lTextWidth = g_GSCtx.TextWidth ( apItem -> m_pFileName, --lStrLen );

 g_GSCtx.DrawIcon ( 6, anY, GSIS_32x32, lBuff );
 g_GSCtx.DrawText ( 40, anY + 2, 0, apItem -> m_pFileName, lStrLen, 2 );

}  /* end GUI_DrawDimmedFileItem */

static void GUI_DrawFileItem ( GUIFileMenuItem* apItem, int anY ) {

 int lTextWidth = g_GSCtx.TextWidth ( apItem -> m_pFileName, 0 );
 int lStrLen    = strlen ( apItem -> m_pFileName );

 while ( lTextWidth >= s_GUICtx.m_FileMenu.m_Width ) lTextWidth = g_GSCtx.TextWidth ( apItem -> m_pFileName, --lStrLen );

 g_GSCtx.DrawIcon ( 6, anY, GSIS_32x32, apItem -> m_pIcon );
 g_GSCtx.DrawText ( 40, anY + 2, 0, apItem -> m_pFileName, lStrLen, 1 );

 if ( apItem -> m_Flags & GUI_FF_SELECTED ) {

  g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0xFF, 0xFF, 0xFF, 0x60 );
  g_GSCtx.m_LineColor = g_Palette[ g_Config.m_BrowserSCIdx - 1 ];
  g_GSCtx.RoundRect ( 6, anY, g_GSCtx.m_Width - 6, anY + 34, 4 );

 }  /* end if */

}  /* end GUI_DrawFileItem */

static void GUI_DrawFileMenu ( GUIFileMenu* apMenu ) {

 int              lY     = 1;
 int              lH     = apMenu -> m_Height - 1;
 GUIFileMenuItem* lpItem = apMenu -> m_pFirst;

 g_GSCtx.CopyFBuffer (
  0, 4, 33, g_GSCtx.m_Width - 6, ( g_GSCtx.m_Height >> 1 ) - 53
 );

 if ( lpItem ) {

  if ( lpItem != apMenu -> m_pItems )

   GUI_DrawDimmedFileItem (  lpItem, 66 + ( lpItem -> m_Y - apMenu -> m_Offset ) * 34  );

  else GUI_DrawFileItem (  lpItem, 66 + ( lpItem -> m_Y - apMenu -> m_Offset ) * 34  );

  lpItem = lpItem -> m_pNext;

  while ( lpItem && lY < lH ) {

   GUI_DrawFileItem (  lpItem, 66 + ( lpItem -> m_Y - apMenu -> m_Offset ) * 34  );

   ++lY; lpItem = lpItem -> m_pNext;

  }  /* end while */

  if ( lpItem ) {

   if ( lpItem != apMenu -> m_pLast )

    GUI_DrawDimmedFileItem (  lpItem, 66 + ( lpItem -> m_Y - apMenu -> m_Offset ) * 34  );

   else GUI_DrawFileItem (  lpItem, 66 + ( lpItem -> m_Y - apMenu -> m_Offset ) * 34 );

  }  /* end if */

 }  /* end if */

}  /* end GUI_DrawFileMenu */

static void GUI_AddFile ( char* apFileName, int aType ) {

 int              lLen   = strlen ( apFileName ) + 1;
 GUIFileMenuItem* lpItem = ( GUIFileMenuItem* )calloc (  1, sizeof ( GUIFileMenuItem )  );

 lpItem -> m_pFileName = ( char* )malloc ( lLen );
 strcpy ( lpItem -> m_pFileName, apFileName );

 lpItem -> m_Flags = aType;

 if ( aType & 0x0000000F )

  lpItem -> m_pIcon = s_FSIcons[ ( aType & 0x0000000F ) - 1 ];

 else lpItem -> m_pIcon = s_FileIcons[ ( aType >> 4 ) & 0x0000000F ];

 if ( s_GUICtx.m_FileMenu.m_pItems == NULL ) {

  s_GUICtx.m_FileMenu.m_pItems =
  s_GUICtx.m_FileMenu.m_pCurr  =
  s_GUICtx.m_FileMenu.m_pLast  =
  s_GUICtx.m_FileMenu.m_pFirst = lpItem;

  lpItem -> m_Y      = 0;
  lpItem -> m_Flags |= GUI_FF_SELECTED;

 } else {

  lpItem -> m_pPrev                      = s_GUICtx.m_FileMenu.m_pLast;
  s_GUICtx.m_FileMenu.m_pLast -> m_pNext = lpItem;
  lpItem -> m_Y                          = s_GUICtx.m_FileMenu.m_pLast -> m_Y + 1;
  s_GUICtx.m_FileMenu.m_pLast            = lpItem;

 }  /* end else */

}  /* end GUI_AddFile */

static void GUI_ClearFileMenu ( void ) {

 GUIFileMenuItem* lpItem = s_GUICtx.m_FileMenu.m_pItems;

 while ( lpItem ) {

  GUIFileMenuItem* lpNext = lpItem -> m_pNext;

  free ( lpItem -> m_pFileName );
  free ( lpItem );

  lpItem = lpNext;

 }  /* end while */

 s_GUICtx.m_FileMenu.m_pItems =
 s_GUICtx.m_FileMenu.m_pFirst =
 s_GUICtx.m_FileMenu.m_pLast  =
 s_GUICtx.m_FileMenu.m_pCurr  = NULL;
 s_GUICtx.m_FileMenu.m_Offset = 0;

 GUI_DrawFileMenu ( &s_GUICtx.m_FileMenu );

}  /* end GUI_ClearFileMenu */

static void GUI_ActivateMenu ( int anIndex ) {

 int          lYOrg, lHOrg;
 int          lY,    lH;
 unsigned int lColor = g_Palette[ g_Config.m_BrowserABCIdx - 1 ];

 if ( anIndex == 0 ) {

  lYOrg = 62;
  lHOrg = g_GSCtx.m_Height - 44;

  lY = 2;
  lH = 56;

  s_GUICtx.m_pCurrentMenu = ( GUIMenu* )&s_GUICtx.m_DevMenu;
  GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu );

 } else {

  lY = 62;
  lH = g_GSCtx.m_Height - 44;

  lYOrg = 2;
  lHOrg = 56;

  if ( anIndex != -1 ) {

   s_GUICtx.m_pCurrentMenu = ( GUIMenu* )&s_GUICtx.m_FileMenu;
   GUI_DrawFileMenu ( &s_GUICtx.m_FileMenu );

  } else {

   s_GUICtx.m_pCurrentMenu = NULL;
   lColor                  = g_Palette[ g_Config.m_BrowserIBCIdx - 1 ];

  }  /* end else */

 }  /* end else */

 g_GSCtx.m_FillColor = GS_SETREG_RGBA( 0x00, 0, 0, 0x80 );
 g_GSCtx.m_LineColor = g_Palette[ g_Config.m_BrowserIBCIdx - 1 ];
 g_GSCtx.RoundRect ( 0, lYOrg, g_GSCtx.m_Width - 1, lHOrg, 8 );
 g_GSCtx.m_LineColor = lColor;
 g_GSCtx.RoundRect ( 0, lY, g_GSCtx.m_Width - 1, lH, 8 );

}  /* end GUI_ActivateMenu */

static GUIFileMenuItem* GUI_NavigateFile ( unsigned long int anEvent ) {

 int              lIdx;
 GUIFileMenuItem* lpItem = s_GUICtx.m_FileMenu.m_pCurr;

 if (  anEvent & ( PAD_LEFT | PAD_RIGHT )  ) return ( GUIFileMenuItem* )GUI_EV_MENU_QUIT;

 if ( !lpItem ) return anEvent & PAD_TRIANGLE ? ( GUIFileMenuItem* )GUI_EV_UPLEVEL : ( GUIFileMenuItem* )GUI_EV_MENU_EMPTY;

 lIdx = lpItem -> m_Y - s_GUICtx.m_FileMenu.m_Offset;

 g_GSCtx.VSync ();

 if (  ( anEvent & PAD_DOWN ) && lpItem -> m_pNext  ) {

  lpItem -> m_Flags &= ~GUI_FF_SELECTED;
  g_GSCtx.CopyFBuffer (
   0, 1, ( 66 + lIdx * 34 ) >> 1, g_GSCtx.m_Width - 2, 18
  );
  GUI_DrawFileItem ( lpItem, 66 + lIdx++ * 34 );

  if ( lIdx < s_GUICtx.m_FileMenu.m_Height - 1 ||
       lpItem -> m_pNext -> m_pNext == NULL
  ) {

   lpItem -> m_pNext -> m_Flags |= GUI_FF_SELECTED;
   GUI_DrawFileItem ( lpItem -> m_pNext, 66 + lIdx * 34 );

  } else {

   ++s_GUICtx.m_FileMenu.m_Offset;

   s_GUICtx.m_FileMenu.m_pFirst  = s_GUICtx.m_FileMenu.m_pFirst -> m_pNext;
   lpItem -> m_pNext -> m_Flags |= GUI_FF_SELECTED;

   GUI_DrawFileMenu ( &s_GUICtx.m_FileMenu );
   GUI_ActivateMenu ( 1 );

  }  /* end else */

  s_GUICtx.m_FileMenu.m_pCurr = lpItem -> m_pNext;

 } else if (  ( anEvent & PAD_UP ) && lpItem -> m_pPrev  ) {

  lpItem -> m_Flags &= ~GUI_FF_SELECTED;
  g_GSCtx.CopyFBuffer (
   0, 1, ( 66 + lIdx * 34 ) >> 1, g_GSCtx.m_Width - 2, 18
  );
  GUI_DrawFileItem ( lpItem, 66 + lIdx-- * 34 );

  if ( lIdx > 0 || lpItem -> m_pPrev -> m_pPrev == NULL ) {

   lpItem -> m_pPrev -> m_Flags |= GUI_FF_SELECTED;
   GUI_DrawFileItem ( lpItem -> m_pPrev, 66 + lIdx * 34 );

  } else {

   --s_GUICtx.m_FileMenu.m_Offset;
   s_GUICtx.m_FileMenu.m_pFirst  = s_GUICtx.m_FileMenu.m_pFirst -> m_pPrev;
   lpItem -> m_pPrev -> m_Flags |= GUI_FF_SELECTED;

   GUI_DrawFileMenu ( &s_GUICtx.m_FileMenu );
   GUI_ActivateMenu ( 1 );

  }  /* end else */

  s_GUICtx.m_FileMenu.m_pCurr = lpItem -> m_pPrev;

 } else if ( anEvent & PAD_TRIANGLE ) {

  return ( GUIFileMenuItem* )GUI_EV_UPLEVEL;

 } else if ( anEvent & PAD_CROSS ) return lpItem;

 return ( GUIFileMenuItem* )GUI_EV_CONSUMED;

}  /* end GUI_NavigateFile */

static int GUI_Run ( void** apRetVal ) {

 int retVal;

 s_NoDevCheck = 0;
 s_PPTS       = g_Timer + g_Config.m_PowerOff;

 Timer_RegisterHandler ( 0, TimerHandler );

 while ( 1 ) {

  unsigned long int lEvent = GUI_WaitEvent ();

  if ( lEvent == PAD_START ) {

   retVal = GUI_EV_MENU;
   break;

  } else if (  lEvent == ( PAD_R1 | PAD_L1 | PAD_R2 | PAD_L2 )  ) {

   retVal = GUI_EV_ABOUT;
   break;

  } else if (  lEvent == ( PAD_SELECT | PAD_TRIANGLE )  ) {

   retVal = GUI_EV_EXIT;
   break;

  } if ( lEvent & GUI_EF_CDROM_MOUNT ) {

   retVal = GUI_EV_CDROM_MOUNT;
   break;

  } else if ( lEvent & GUI_EF_CDROM_UMOUNT ) {

   retVal = GUI_EV_CDROM_UMOUNT;
   break;

  } else if ( lEvent & GUI_EF_USBM_MOUNT ) {

   retVal = GUI_EV_USBM_MOUNT;
   break;

  } else if ( lEvent & GUI_EF_USBM_UMOUNT ) {

   retVal = GUI_EV_USBM_UMOUNT;
   break;

  } else if ( lEvent & GUI_EF_HOST_MOUNT ) {

   retVal = GUI_EV_HOST_MOUNT;
   break;

  } else if ( lEvent & GUI_EF_HOST_UMOUNT ) {

   retVal = GUI_EV_HOST_UMOUNT;
   break;

  } else if ( lEvent & GUI_EF_CDFS_MOUNT ) {

   retVal = GUI_EV_CDFS_MOUNT;
   break;

  } else if ( lEvent & GUI_EF_CDFS_UMOUNT ) {

   retVal = GUI_EV_CDFS_UMOUNT;
   break;

  } else if ( lEvent & GUI_EF_DISK_DETECT ) {

   retVal = GUI_EV_DISK_DETECT;
   break;

  } else if ( lEvent & GUI_EF_DISK_NODISK ) {

   retVal = GUI_EV_DISK_NODISK;
   break;

  } else if ( s_GUICtx.m_pCurrentMenu ) {

   void* lpResult = s_GUICtx.m_pCurrentMenu -> Navigate ( lEvent );

   if (  ( int )lpResult > GUI_EV_MENU_EMPTY  ) {

    if (  ( void* )s_GUICtx.m_pCurrentMenu == ( void* )&s_GUICtx.m_DevMenu ) {

     retVal    = GUI_EV_DEV_SELECT;
     *apRetVal = lpResult;

     break;

    } else {

     retVal    = GUI_EV_FILE_SELECT;
     *apRetVal = lpResult;

     break;

    }  /* end else */

   } else if (  ( int )lpResult == GUI_EV_UPLEVEL  ) {

    retVal = GUI_EV_UPLEVEL;

    break;

   } else if (  ( int )lpResult == GUI_EV_MENU_QUIT  )

    GUI_ActivateMenu (  ( void* )s_GUICtx.m_pCurrentMenu == ( void* )&s_GUICtx.m_DevMenu ? 1 : 0  );

   else if (  lEvent == ( PAD_SELECT | PAD_SQUARE )  ) {

    retVal = GUI_EV_SAVE_CONFIG;

    break;

   }  /* end if */

  }  /* end else */

 }  /* end while */

 s_NoDevCheck = 1;

 Timer_RegisterHandler ( 0, NULL );

 return retVal;

}  /* end GUI_Run */

static void GUI_Redraw ( int aMode ) {

 unsigned int lColor  = g_Palette[ g_Config.m_BrowserTxtIdx - 1 ] & ~0xFF000000;
 unsigned int lDColor = lColor | 0x20000000;

 lColor |= 0x80000000;

 if ( aMode > 0 ) {

  g_GSCtx.m_fDblBuf = GS_ON;
  g_GSCtx.ClearScreen (  GS_SETREG_RGBA( 0x00, 0x00, 0x00, 0x00 )  );
  g_GSCtx.VSync ();
  g_GSCtx.InitScreen ( g_Config.m_DisplayCharset );
  g_GSCtx.VSync ();

  g_GSCtx.SetTextColor ( 3, 0x50A06060 );

 }  /* end if */

 g_GSCtx.SetTextColor ( 0, g_Palette[ g_Config.m_BrowserSBCIdx - 1 ] | 0x80000000 );
 g_GSCtx.SetTextColor ( 1, lColor  );
 g_GSCtx.SetTextColor ( 2, lDColor );

 s_GUICtx.m_FileMenu.m_Width  = g_GSCtx.m_Width - 46;
 s_GUICtx.m_FileMenu.m_Height = ( g_GSCtx.m_Height - 110 ) / 34;
 s_GUICtx.m_DevMenu.m_StartX  = 6 + g_GSCtx.TextWidth ( STR_AVAILABLE_MEDIA, 0 );

 if ( aMode >= 0 )

  GUI_DrawDesktop ();

 else {

  g_GSCtx.CopyFBuffer ( 0, 1, 0, g_GSCtx.m_Width, g_GSCtx.m_Height >> 1 );
  g_GSCtx.DrawText ( 6, s_GUICtx.m_DevMenu.m_StartY + 10, 0, STR_AVAILABLE_MEDIA, 0, 0 );

 }  /* end else */

 GUI_DrawDeviceMenu ( &s_GUICtx.m_DevMenu  );
 GUI_DrawFileMenu   ( &s_GUICtx.m_FileMenu );

}  /* end GUI_Redraw */

unsigned long int GUI_WaitButton ( int aButton, int aTimeout ) {

 unsigned long int lEvent;
 unsigned long int lTime;

 s_NoDevCheck = 1;

 Timer_RegisterHandler ( 0, TimerHandler );

 lTime = g_Timer + aTimeout;

 while ( 1 ) {

  lEvent = GUI_WaitEvent ();

  if ( g_Timer < lTime   ) continue;
  if ( lEvent  & aButton ) break;

 }  /* end while */

 Timer_RegisterHandler ( 0, NULL );

 s_NoDevCheck = 0;

 return lEvent;

}  /* end GUI_WaitButton */

int GUI_ReadButtons ( void ) {

 int retVal = PAD_Read ( 0, 0 );

 if ( !retVal ) retVal = PAD_Read ( 1, 0 );

 return retVal;

}  /* end GUI_ReadButtons */

static void GUI_ActivateFileItem ( int anOffset, char* apName, char* apFirst ) {

 GUIFileMenuItem* lpItem  = s_GUICtx.m_FileMenu.m_pItems;
 int              lfFound = 0;

 if ( s_GUICtx.m_FileMenu.m_pCurr ) s_GUICtx.m_FileMenu.m_pCurr -> m_Flags &= ~GUI_FF_SELECTED;

 while ( lpItem ) {

  if (  !lfFound && !strcmp ( lpItem -> m_pFileName, apFirst )  ) {

   s_GUICtx.m_FileMenu.m_pFirst = lpItem;
   lfFound = 1;

  }  /* end if */

  if (  lfFound && !strcmp ( lpItem -> m_pFileName, apName )  ) {

   s_GUICtx.m_FileMenu.m_Offset = anOffset;
   s_GUICtx.m_FileMenu.m_pCurr  = lpItem;
   lpItem -> m_Flags           |= GUI_FF_SELECTED;

   break;

  }  /* end if */

  lpItem = lpItem -> m_pNext;

 }  /* end while */

 s_GUICtx.ActivateMenu ( 1 );

}  /* end GUI_ActivateFileItem */

static int GUI_SelectFile ( char* apFileName ) {

 int              retVal = 0;
 GUIFileMenuItem* lpItem = s_GUICtx.m_FileMenu.m_pItems;

 if ( lpItem ) {

  s_GUICtx.m_FileMenu.m_pCurr  = lpItem;
  s_GUICtx.m_FileMenu.m_pFirst = lpItem;
  s_GUICtx.m_FileMenu.m_Offset = 0;
  s_GUICtx.m_FileMenu.m_pCurr -> m_Flags |= GUI_FF_SELECTED;

  do {

   int lIdx = lpItem -> m_Y - s_GUICtx.m_FileMenu.m_Offset + 1;

   if (  !strcmp ( lpItem -> m_pFileName, apFileName )  ) {

    retVal = 1;
    break;

   }  /* end if */

   lpItem -> m_Flags &= ~GUI_FF_SELECTED;

   if ( !lpItem -> m_pNext ) break;
   
   if ( lIdx < s_GUICtx.m_FileMenu.m_Height - 1 || lpItem -> m_pNext -> m_pNext == NULL )

    lpItem -> m_pNext -> m_Flags |= GUI_FF_SELECTED;

   else {

    ++s_GUICtx.m_FileMenu.m_Offset;

    s_GUICtx.m_FileMenu.m_pFirst  = s_GUICtx.m_FileMenu.m_pFirst -> m_pNext;
    lpItem -> m_pNext -> m_Flags |= GUI_FF_SELECTED;

   }  /* end else */

   s_GUICtx.m_FileMenu.m_pCurr = lpItem = lpItem -> m_pNext;

  } while ( lpItem );

 }  /* end if */

 if ( !retVal ) {

  s_GUICtx.m_FileMenu.m_pCurr  =
  s_GUICtx.m_FileMenu.m_pFirst = s_GUICtx.m_FileMenu.m_pItems;
  s_GUICtx.m_FileMenu.m_Offset = 0;

  if ( s_GUICtx.m_FileMenu.m_pCurr ) s_GUICtx.m_FileMenu.m_pCurr -> m_Flags |= GUI_FF_SELECTED;

 }  /* end if */

 return retVal;

}  /* end GUI_SelectFile */

static void _usb_handler_connect ( void* apHdr ) {

 if ( s_USBFlags & USB_FLAG_DISCONNECT )
  s_USBFlags &= ~USB_FLAG_DISCONNECT;
 else  s_USBFlags |= USB_FLAG_CONNECT;

 iWakeupThread ( s_GUIThreadID );

}  /* end _usb_handler_connect */

static void _usb_handler_disconnect ( void* apHdr ) {

 if ( s_USBFlags & USB_FLAG_CONNECT )
  s_USBFlags &= ~USB_FLAG_CONNECT;
 else s_USBFlags |= USB_FLAG_DISCONNECT;

 iWakeupThread ( s_GUIThreadID );

}  /* end _usb_handler_disconnect */

GSDisplayMode GUI_InitPad ( void ) {

 int           lBtn;
 uint64_t      lTime;
 GSDisplayMode retVal = GSDisplayMode_AutoDetect;

 SifLoadModule ( "rom0:SIO2MAN", 0, NULL );
 SifLoadModule ( "rom0:PADMAN",  0, NULL );
 SifLoadModule ( "rom0:MCMAN",   0, NULL );
 SifLoadModule ( "rom0:MCSERV",  0, NULL );

 mcInit ( MC_TYPE_MC );

 PAD_Init ();
 PAD_OpenPort ( 0, 0, s_PadBuf0 );
 PAD_OpenPort ( 1, 0, s_PadBuf1 );

 lBtn  = PAD_State ( 0, 0 );
 lTime = g_Timer;

 while (  lBtn != PAD_STATE_STABLE && lBtn != PAD_STATE_FINDCTP1 ) {

  lBtn = PAD_State ( 0, 0 );

  if ( g_Timer - lTime > 2000 ) break;

 }  /* end while */

 PAD_SetMainMode ( 0, 0, PAD_MMODE_DIGITAL, PAD_MMODE_LOCK );

 lTime = g_Timer;

 while ( 1 ) {

  lBtn = GUI_ReadButtons ();

  if (   (  lBtn & ( PAD_SELECT | PAD_R1 )  ) == ( PAD_SELECT | PAD_R1 )   ) {

   retVal = lBtn & PAD_SQUARE ? GSDisplayMode_NTSC : GSDisplayMode_NTSC_I;
   break;

  } else if (   (  lBtn & ( PAD_SELECT | PAD_R2 )  ) == ( PAD_SELECT | PAD_R2 )   ) {

   retVal = lBtn & PAD_SQUARE ? GSDisplayMode_PAL : GSDisplayMode_PAL_I;
   break;

  } else if (   (  lBtn & ( PAD_SELECT | PAD_L1 )  ) == ( PAD_SELECT | PAD_L1 )   ) {

   retVal = GSDisplayMode_VGA_640x480_60Hz;
   break;

  } else if (   (  lBtn & ( PAD_SELECT | PAD_L2 )  ) == ( PAD_SELECT | PAD_L2 )   ) {

   retVal = GSDisplayMode_VGA_640x480_75Hz;
   break;

  } else if ( g_Timer - lTime > 1000 ) break;

 }  /* end while */

 return retVal;

}  /* end GUI_InitPad */

GUIContext* GUI_InitContext ( void ) {

 ee_sema_t   lSema;
 ee_thread_t lThread;

 s_MainThreadID = GetThreadId ();
 s_Stop         = 0;
 s_Event        = 0;
 s_CDROM        = 0;
 s_USBFlags     = 0;
 s_HostFlags    = 0;
 s_CDFSFlags    = 0;
 s_DiskDetect   = 0;
 s_PrevBtn      = 0;
 s_Init         = 0;

 s_GUICtx.m_DevMenu.m_StartY = 4;
 s_GUICtx.m_DevMenu.m_pItems = NULL;
 s_GUICtx.m_DevMenu.m_pCurr  = NULL;
 s_GUICtx.m_DevMenu.Navigate = GUI_NavigateDevice;

 s_GUICtx.m_FileMenu.m_pItems = NULL;
 s_GUICtx.m_FileMenu.m_pCurr  = NULL;
 s_GUICtx.m_FileMenu.m_pFirst = NULL;
 s_GUICtx.m_FileMenu.m_pLast  = NULL;
 s_GUICtx.m_FileMenu.m_Offset = 0;
 s_GUICtx.m_FileMenu.Navigate = GUI_NavigateFile;

 s_GUICtx.m_pCurrentMenu = NULL;

 s_GUICtx.Status           = GUI_Status;
 s_GUICtx.Destroy          = GUI_Destroy;
 s_GUICtx.Run              = GUI_Run;
 s_GUICtx.AddDevice        = GUI_AddDevice;
 s_GUICtx.DelDevice        = GUI_DelDevice;
 s_GUICtx.AddFile          = GUI_AddFile;
 s_GUICtx.ClearFileMenu    = GUI_ClearFileMenu;
 s_GUICtx.ActivateMenu     = GUI_ActivateMenu;
 s_GUICtx.Redraw           = GUI_Redraw;
 s_GUICtx.ActivateFileItem = GUI_ActivateFileItem;
 s_GUICtx.SelectFile       = GUI_SelectFile;
 s_GUICtx.Progress         = GUI_Progress;

 lSema.init_count = 0;
 lSema.max_count  = 1;
 s_PadSema = CreateSema ( &lSema );

 lThread.stack_size       = sizeof ( s_Stack );
 lThread.stack            = s_Stack;
 lThread.gp_reg           = &_gp;
 lThread.initial_priority = 1;
 lThread.func             = _gui_thread;
 StartThread (  s_GUIThreadID = CreateThread ( &lThread ), NULL  );

 SMS_SetSifCmdHandler ( _usb_handler_connect,    1 );
 SMS_SetSifCmdHandler ( _usb_handler_disconnect, 2 );

 GUI_Redraw ( 1 );

 DoTimer = TimerProc;
 s_Init  = 1;

 return &s_GUICtx;

}  /* end GUI_InitContext */