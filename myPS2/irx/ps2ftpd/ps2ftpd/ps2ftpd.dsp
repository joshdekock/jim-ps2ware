# Microsoft Developer Studio Project File - Name="ps2ftpd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) External Target" 0x0106

CFG=ps2ftpd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "ps2ftpd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "ps2ftpd.mak" CFG="ps2ftpd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "ps2ftpd - Win32 Release" (based on "Win32 (x86) External Target")
!MESSAGE "ps2ftpd - Win32 Debug" (based on "Win32 (x86) External Target")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "ps2ftpd - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Cmd_Line "NMAKE /f ps2ftpd.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ps2ftpd.exe"
# PROP BASE Bsc_Name "ps2ftpd.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../../vc6/Release"
# PROP Intermediate_Dir "../../vc6/Release"
# PROP Cmd_Line "make"
# PROP Rebuild_Opt "clean"
# PROP Target_File "ps2ftpd.irx"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ELSEIF  "$(CFG)" == "ps2ftpd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Cmd_Line "NMAKE /f ps2ftpd.mak"
# PROP BASE Rebuild_Opt "/a"
# PROP BASE Target_File "ps2ftpd.exe"
# PROP BASE Bsc_Name "ps2ftpd.bsc"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../../vc6/Debug"
# PROP Intermediate_Dir "../../vc6/Debug"
# PROP Cmd_Line "make"
# PROP Rebuild_Opt "clean"
# PROP Target_File "ps2ftpd.irx"
# PROP Bsc_Name ""
# PROP Target_Dir ""

!ENDIF 

# Begin Target

# Name "ps2ftpd - Win32 Release"
# Name "ps2ftpd - Win32 Debug"

!IF  "$(CFG)" == "ps2ftpd - Win32 Release"

!ELSEIF  "$(CFG)" == "ps2ftpd - Win32 Debug"

!ENDIF 

# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\FileSystem.c
# End Source File
# Begin Source File

SOURCE=.\FtpClient.c
# End Source File
# Begin Source File

SOURCE=.\FtpCommands.c
# End Source File
# Begin Source File

SOURCE=.\FtpMessages.c
# End Source File
# Begin Source File

SOURCE=.\FtpServer.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\FileSystem.h
# End Source File
# Begin Source File

SOURCE=.\FtpClient.h
# End Source File
# Begin Source File

SOURCE=.\FtpMessages.h
# End Source File
# Begin Source File

SOURCE=.\FtpServer.h
# End Source File
# Begin Source File

SOURCE=.\irx_imports.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
