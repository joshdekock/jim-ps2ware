/*
#     ___  _ _      ___
#    |    | | |    |
# ___|    |   | ___|    PS2DEV Open Source Project.
#----------------------------------------------------------
# (c) 2005-2006 Eugene Plotnikov <e-plotnikov@operamail.com>
# (c) 2005 USB support by weltall
# (c) 2005 HOST support by Ronald Andersson (AKA: dlanor)
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
*/
#include "SMS.h"
#include "SMS_FileDir.h"
#include "SMS_GUIcons.h"
#include "SMS_GUI.h"
#include "SMS_Locale.h"
#include "SMS_Config.h"
#include "SMS_CDVD.h"
#include "SMS_CDDA.h"
#include "SMS_FileContext.h"

#include <tamtypes.h>
#include <string.h>
#include <fileXio.h>
#include <fileXio_rpc.h>
#include <fileio.h>
#include <fcntl.h>
#include <libhdd.h>

unsigned char g_pUSB   [] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "mass";
unsigned char g_pCDROM [] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "cdfs";
unsigned char g_pHDD0  [] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "hdd0";
unsigned char g_pCDDA  [] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "cdda";
unsigned char g_pHOST  [] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "host";
unsigned char g_pDVD   [] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "cdfs";
unsigned char g_pCDDAFS[] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "cddafs:/";

static unsigned char s_pAVI[] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = ".avi";
static unsigned char s_pMP3[] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = ".mp3";
static unsigned char s_pM3U[] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = ".m3u";
static unsigned char s_pELL[] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "host:elflist.txt";
static unsigned char s_pHST[] __attribute__(   (  aligned( 4 ), section( ".data" )  )   ) = "host:";

unsigned char* g_pDevName[ 6 ] = {
 g_pUSB, g_pCDROM, g_pHDD0, g_pCDDA, g_pHOST, g_pDVD
};

SMS_List*    g_pFileList;
int          g_CMedia;
CDDAContext* g_pCDDACtx;
char         g_CWD[ 1024 ] __attribute__(   (  section( ".bss" )  )   );

int _set_id ( char* apName ) {

 int retVal = GUICON_FILE;
 int lLen   = strlen ( apName );

 if ( lLen > 4 ) {

  char* lpExt = apName + lLen - 4;

  if (       !stricmp ( lpExt, s_pAVI )  )
   retVal = GUICON_AVI;
  else if (  !stricmp ( lpExt, s_pMP3 )  )
   retVal = GUICON_MP3;
  else if (  !stricmp ( lpExt, s_pM3U )  )
   retVal = GUICON_M3U;

 }  /* end if */

 return retVal;

}  /* end set_id */

void SMS_FileDirInit ( unsigned char* apPath ) {

 int           lFD;
 int           lfSort = g_Config.m_BrowserFlags & SMS_BF_SORT;
 SMS_List*     lpDirList;
 SMS_List*     lpFileList;
 SMS_List*     lpList;
 iox_dirent_t  lEntry;
 char          lPath[ sizeof ( g_CWD ) + 5 ] __attribute__(   (  aligned( 4 )  )   );
 char*         lpPtr;
 SMS_ListNode* lpNode;

 GUI_Status ( STR_READING_MEDIA.m_pStr );

 if ( !g_pFileList )

  g_pFileList = SMS_ListInit ();

 else SMS_ListDestroy ( g_pFileList, 0 );

 if ( apPath[ 0 ] == '\x00' ) {

  strcpy ( g_CWD, g_pDevName[ g_CMedia ] );

  *( unsigned int* )&g_CWD[ 4 ] = 0x0000003A;

  if ( g_CMedia == 2 ) {

   lFD = fileXioDopen ( g_CWD );

   SMS_Strcat ( g_CWD, g_SlashStr );

   if ( lFD >= 0 ) {

    while (  fileXioDread ( lFD, &lEntry ) > 0  ) {

     if (   !(  ( lEntry.stat.attr  & ATTR_SUB_PARTITION ) ||
                ( lEntry.stat.mode == FS_TYPE_EMPTY      )
           )
     ) SMS_ListPushBack ( g_pFileList, lEntry.name ) -> m_Param = GUICON_PARTITION;

    }  /* end while */

    fileXioDclose ( lFD );

    if ( lfSort ) SMS_ListSort ( g_pFileList );

   }  /* end if */

   goto end;

  }  /* end if */

 }  /* end if */

 lpDirList  = SMS_ListInit ();
 lpFileList = SMS_ListInit ();
 lFD        = strlen ( g_CWD ) - 1;

__asm__ __volatile__(
 ".set noreorder\n\t"
 "pxor $a0, $a0, $a0\r\n"
 "pxor $a1, $a1, $a1\r\n"
 "pxor $a2, $a2, $a2\r\n"
 "pxor $a3, $a3, $a3\r\n"
 ".set reorder\n\t"
 ::: "a0", "a1", "a2", "a3"
 );

 if ( g_CMedia == 4 ) {

  if ( apPath[ 0 ] && g_CWD[ 5 ] && g_CWD[ lFD ] != '\\' && apPath[ 0 ] != '\\' ) SMS_Strcat ( g_CWD, g_BSlashStr );

 } else if ( apPath[ 0 ] != '/' && g_CWD[ lFD ] != '/' ) SMS_Strcat ( g_CWD, g_SlashStr );

 SMS_Strcat ( g_CWD, apPath );

 if ( g_CMedia == 1 && g_pCDDACtx ) {

  const CDDADirectory* lpDirs  = CDDA_DirectoryList ( g_pCDDACtx );
  const CDDADirectory* lpDir   = NULL;

  if ( !apPath[ 0 ] )

   *( int* )&lPath[ 0 ] = 0x2E;

  else strcpy ( lPath, apPath );

  while ( lpDirs ) {

   if (  !strcmp ( lpDirs -> m_pName, lPath )  ) {

    lpDir = lpDirs;

    if ( apPath[ 0 ] ) break;

   } else if ( !apPath[ 0 ] ) SMS_ListPushBack ( lpDirList, lpDirs -> m_pName ) -> m_Param = GUICON_FOLDER;

   lpDirs = lpDirs -> m_pNext;

  }  /* end while */

  if ( lpDir ) {

   CDDAFile* lpFiles = CDDA_GetFileList ( g_pCDDACtx, lpDir );

   if ( lpFiles ) {

    CDDAFile* lpFile = lpFiles;

    while ( lpFile ) {

     SMS_ListPushBack ( lpFileList, lpFile -> m_pName ) -> m_Param = _set_id ( lpFile -> m_pName );

     lpFile = lpFile -> m_pNext;

    }  /* end while */

    CDDA_DestroyFileList ( lpFiles );

   }  /* end if */

  }  /* end if */

  strcpy ( g_CWD, g_pCDDAFS );
  SMS_Strcat ( g_CWD, apPath );

 } else if ( g_CMedia != 3 ) {

  char* lpName = g_CMedia == 2 ? lEntry.name : (  ( io_dirent_t* )&lEntry  ) -> name;

  if ( g_CMedia == 4 && !apPath[ 0 ] ) {

   FileContext* lpFileCtx = STIO_InitFileContext ( s_pELL, NULL );

   if ( lpFileCtx ) {

    char lBuf [ sizeof ( g_CWD ) ];
    int  lID;

    while ( 1 ) {

     File_GetString (  lpFileCtx, lBuf, sizeof ( lBuf )  );

     if ( !lBuf[ 0 ] ) {

      if (  FILE_EOF( lpFileCtx )  )

       break;

      else continue;

     }  /* end if */

     strcpy ( lPath, s_pHST );
     SMS_Strcat ( lPath, lBuf );

     lID = fioOpen ( lPath, O_RDONLY );

     if ( lID >= 0 ) {

      fioClose ( lID );
      lpList = lpFileList;
      lID    = _set_id ( lBuf );

     } else {

      lID = fioDopen ( lPath );

      if ( lID >= 0 ) {

       fioDclose ( lID );
       lpList = lpDirList;
       lID    = GUICON_FOLDER;

      } else continue;

     }  /* end else */

     SMS_ListPushBack ( lpList, lBuf ) -> m_Param = lID;

    }  /* end while */

    lpFileCtx -> Destroy ( lpFileCtx );

   } else goto doScan;

  } else {
doScan:
   lFD = fileXioDopen ( g_CWD );

   if ( lFD >= 0 ) {

    strcpy ( lPath, g_CWD );

    if (  lPath[ strlen ( lPath ) - 1 ] != '\\'  ) SMS_Strcat ( lPath, g_BSlashStr );

    lpPtr = lPath + strlen ( lPath );

    while (  fileXioDread ( lFD, &lEntry ) > 0  ) {

     int lID;

     if ( !lEntry.stat.mode ) {

      strcpy ( lpPtr, lpName );

      lID = fioOpen ( lPath, O_RDONLY );

      if ( lID >= 0 ) {

       fioClose ( lID );
       lEntry.stat.mode = FIO_S_IFREG;

      } else {

       lID = fioDopen ( lPath );

       if ( lID >= 0 ) {

        fioDclose ( lID );
        lEntry.stat.mode = FIO_S_IFDIR;

       } else continue;

      }  /* end else */

     }  /* end if */

     if ( lEntry.stat.mode & FIO_S_IFDIR ) {

      if (  !strcmp ( lpName, "."  ) ||
            !strcmp ( lpName, ".." )
      ) continue;

      lpList = lpDirList;
      lID    = GUICON_FOLDER;

     } else if ( lEntry.stat.mode & FIO_S_IFREG ) {

      lID = _set_id ( lpName );

      lpList = lpFileList;

     } else continue;

     SMS_ListPushBack ( lpList, lpName ) -> m_Param = lID;

    }  /* end while */

    fileXioDclose ( lFD );

   }  /* end if */

  }  /* end else */

 } else {


 }  /* end else */

 if ( lfSort ) {

  SMS_ListSort ( lpDirList  );
  SMS_ListSort ( lpFileList );

 }  /* end if */

 lpNode = lpFileList -> m_pHead;

 while ( lpNode ) {

  if ( lpNode -> m_Param == GUICON_AVI ) {

   int lPos = strlen ( lpNode -> m_pString ) - 3;

   strcpy ( lPath, lpNode -> m_pString );
   strcpy ( lPath + lPos, g_pSrtStr    );

   if (  SMS_ListFindI ( lpFileList, lPath )  ) {
setSub:
    lpNode -> m_Param = GUICON_AVIS;
    goto next;

   }  /* end if */

   strcpy ( lPath + lPos, g_pSubStr );

   if (  SMS_ListFindI ( lpFileList, lPath )  ) goto setSub;

  }  /* end if */
next:
  lpNode = lpNode -> m_pNext;

 }  /* end while */

 SMS_ListAppend ( g_pFileList, lpDirList  );
 SMS_ListAppend ( g_pFileList, lpFileList );

 SMS_ListDestroy ( lpDirList,  1 );
 SMS_ListDestroy ( lpFileList, 1 );
end:
 GUI_Status ( g_CWD );

 if ( g_CMedia & 1 ) CDVD_Stop ();

}  /* end SMS_FileDirInit */