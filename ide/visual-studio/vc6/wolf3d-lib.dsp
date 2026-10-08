# Microsoft Developer Studio Project File - Name="wolf3d_lib" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Generic Project" 0x010a

CFG=wolf3d_lib - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "wolf3d_lib.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "wolf3d_lib.mak" CFG="wolf3d_lib - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "wolf3d_lib - Win32 MinSizeRel" (based on "Win32 (x86) Generic Project")
!MESSAGE "wolf3d_lib - Win32 Release" (based on "Win32 (x86) Generic Project")
!MESSAGE "wolf3d_lib - Win32 RelWithDebInfo" (based on "Win32 (x86) Generic Project")
!MESSAGE "wolf3d_lib - Win32 Debug" (based on "Win32 (x86) Generic Project")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
MTL=midl.exe

!IF  "$(CFG)" == "wolf3d_lib - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""



!ELSEIF  "$(CFG)" == "wolf3d_lib - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""



!ELSEIF  "$(CFG)" == "wolf3d_lib - Win32 MinSizeRel"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "MinSizeRel"
# PROP BASE Intermediate_Dir "MinSizeRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Intermediate_Dir "MinSizeRel"
# PROP Target_Dir ""



!ELSEIF  "$(CFG)" == "wolf3d_lib - Win32 RelWithDebInfo"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "RelWithDebInfo"
# PROP BASE Intermediate_Dir "RelWithDebInfo"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Intermediate_Dir "RelWithDebInfo"
# PROP Target_Dir ""



!ENDIF

# Begin Target

# Name "wolf3d_lib - Win32 Release"
# Name "wolf3d_lib - Win32 Debug"
# Name "wolf3d_lib - Win32 MinSizeRel"
# Name "wolf3d_lib - Win32 RelWithDebInfo"
# Begin Source File

SOURCE=.\build.cmd

!IF  "$(CFG)" == "wolf3d_lib - Win32 Release"
USERDEP__HACK=.\build.cmd
# Begin Custom Build - Building wolf3d_lib through the repository dispatcher

wolf3d_lib.dsp.dispatch :  "$(SOURCE)" "$(INTDIR)" "$(OUTDIR)"
	call .\build.cmd Release build
	if errorlevel 1 exit 1

# End Custom Build

!ELSEIF  "$(CFG)" == "wolf3d_lib - Win32 Debug"
USERDEP__HACK=.\build.cmd
# Begin Custom Build - Building wolf3d_lib through the repository dispatcher

wolf3d_lib.dsp.dispatch :  "$(SOURCE)" "$(INTDIR)" "$(OUTDIR)"
	call .\build.cmd Debug build
	if errorlevel 1 exit 1

# End Custom Build

!ELSEIF  "$(CFG)" == "wolf3d_lib - Win32 MinSizeRel"
USERDEP__HACK=.\build.cmd
# Begin Custom Build - Building wolf3d_lib through the repository dispatcher

wolf3d_lib.dsp.dispatch :  "$(SOURCE)" "$(INTDIR)" "$(OUTDIR)"
	call .\build.cmd Release build
	if errorlevel 1 exit 1

# End Custom Build

!ELSEIF  "$(CFG)" == "wolf3d_lib - Win32 RelWithDebInfo"
USERDEP__HACK=.\build.cmd
# Begin Custom Build - Building wolf3d_lib through the repository dispatcher

wolf3d_lib.dsp.dispatch :  "$(SOURCE)" "$(INTDIR)" "$(OUTDIR)"
	call .\build.cmd Release build
	if errorlevel 1 exit 1

# End Custom Build

!ENDIF

# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_CA.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_CA.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_IN.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_IN.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_PM.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_PM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_SD.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_SD.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_US_1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_US_1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_VH.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_VH.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_VL.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\ID_VL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_ASSETS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_ASSETS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_AUDIO.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_AUDIO.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_COMPAT.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_CONFIG.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_CONFIG.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_DATA.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_DATA.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_ENDIAN.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_FILE.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_FILE.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_FIXED.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_FIXED.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_GRAPHICS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_GRAPHICS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_MAPS.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_MAPS.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_OPL_DBOPL.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_OPL_NUKED.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_OPL_SILENT.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_OPL.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_PALETTE.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_PALETTE.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_PLATFORM.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_PLATFORM.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_RENDERER.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_RENDERER.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_SAVE.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_SAVE.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_SIGNON.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WG_SIGNON.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_ACT1.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_ACT1.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_ACT2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_ACT2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_AGENT.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_AGENT.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_DRAW.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_DRAW.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_GAME.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_GAME.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_INTER.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_INTER.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_MAIN.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_MAIN.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_MENU.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_MENU.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_PLAY.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_PLAY.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_SCALE.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_SCALE.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_STATE.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_STATE.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_TEXT.c
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WL_TEXT.h
# End Source File
# Begin Source File

SOURCE=..\..\..\src\WOLF3D.c
# End Source File
# Begin Source File

SOURCE=..\..\..\include\WOLF3D_STDINT.h
# End Source File
# Begin Source File

SOURCE=..\..\..\include\WOLF3D.h
# End Source File
# Begin Source File

SOURCE=..\..\..\third_party\Nuked-OPL3\opl3.c
# End Source File
# Begin Source File

SOURCE=..\..\..\third_party\Nuked-OPL3\opl3.h
# End Source File
# End Target
# End Project
