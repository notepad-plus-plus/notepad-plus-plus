# Microsoft Developer Studio Project File - Name="SciLexer" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=SciLexer - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE
!MESSAGE NMAKE /f "SciLexer.mak".
!MESSAGE
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE
!MESSAGE NMAKE /f "SciLexer.mak" CFG="SciLexer - Win32 Debug"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "SciLexer - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "SciLexer - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "SciLexer - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SciLexer_EXPORTS" /Yu"stdafx.h" /FD /c
# ADD CPP /nologo /G6 /MT /W3 /GX /O1 /I "..\include" /I "..\src" /I "..\lexlib" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCI_LEXER" /D "_CRT_SECURE_NO_WARNINGS" /FD /c
# SUBTRACT CPP /Fr /YX /Yc /Yu
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib comctl32.lib /nologo /dll /pdb:none /map /machine:I386 /opt:nowin98

!ELSEIF  "$(CFG)" == "SciLexer - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "../bin"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SciLexer_EXPORTS" /Yu"stdafx.h" /FD /GZ /c
# ADD CPP /nologo /G6 /MTd /W3 /Gm /GX /ZI /Od /I "..\include" /I "..\src" /I "..\lexlib" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SCI_LEXER" /D "_CRT_SECURE_NO_WARNINGS" /FR /FD /GZ /c
# SUBTRACT CPP /YX /Yc /Yu
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib comctl32.lib /nologo /dll /debug /machine:I386

!ENDIF

# Begin Target

# Name "SciLexer - Win32 Release"
# Name "SciLexer - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\lexlib\Accessor.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\LexerBase.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\LexerSimple.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\LexerModule.cxx
# End Source File
# Begin Source File

SOURCE=..\src\AutoComplete.cxx
# End Source File
# Begin Source File

SOURCE=..\src\CallTip.cxx
# End Source File
# Begin Source File

SOURCE=..\src\Catalogue.cxx
# End Source File
# Begin Source File

SOURCE=..\src\CellBuffer.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\CharacterSet.cxx
# End Source File
# Begin Source File

SOURCE=..\src\CharClassify.cxx
# End Source File
# Begin Source File

SOURCE=..\src\ContractionState.cxx
# End Source File
# Begin Source File

SOURCE=..\src\Decoration.cxx
# End Source File
# Begin Source File

SOURCE=..\src\Document.cxx
# End Source File
# Begin Source File

SOURCE=..\src\Editor.cxx
# End Source File
# Begin Source File

SOURCE=..\src\ExternalLexer.cxx
# End Source File
# Begin Source File

SOURCE=..\src\Indicator.cxx
# End Source File
# Begin Source File

SOURCE=..\src\KeyMap.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAbaqus.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAda.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAPDL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAsm.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAsn1.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexASY.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAU3.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexAVE.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexBaan.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexBash.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexBasic.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexBullant.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCaml.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCLW.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCmake.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCOBOL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexConf.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCPP.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCrontab.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCsound.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexCSS.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexD.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexEiffel.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexErlang.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexEScript.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexFlagship.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexForth.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexFortran.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexGAP.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexGui4Cli.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexHaskell.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexHTML.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexInno.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexKix.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexLisp.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexLout.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexLua.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMagik.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMarkdown.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMatlab.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMetapost.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMMIXAL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMPT.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMSSQL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexMySQL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexNimrod.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexNsis.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexOpal.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexOthers.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPascal.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPB.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPerl.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPLM.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPOV.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPowerPro.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPowerShell.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexProgress.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPS.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexPython.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexR.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexRebol.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexRuby.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexScriptol.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexSmalltalk.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexSML.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexSorcus.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexSpecman.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexSpice.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexSQL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexTACL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexTADS3.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexTAL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexTCL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexTeX.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexTxt2tags.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexVB.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexVerilog.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexVHDL.cxx
# End Source File
# Begin Source File

SOURCE=..\lexers\LexYAML.cxx
# End Source File
# Begin Source File

SOURCE=..\src\LineMarker.cxx
# End Source File
# Begin Source File

SOURCE=..\src\PerLine.cxx
# End Source File
# Begin Source File

SOURCE=..\win32\PlatWin.cxx
# End Source File
# Begin Source File

SOURCE=..\src\PositionCache.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\PropSetSimple.cxx
# End Source File
# Begin Source File

SOURCE=..\src\RESearch.cxx
# End Source File
# Begin Source File

SOURCE=..\src\RunStyles.cxx
# End Source File
# Begin Source File

SOURCE=..\src\ScintillaBase.cxx
# End Source File
# Begin Source File

SOURCE=..\win32\ScintillaWin.cxx
# End Source File
# Begin Source File

SOURCE=..\win32\ScintRes.rc
# End Source File
# Begin Source File

SOURCE=..\src\Selection.cxx
# End Source File
# Begin Source File

SOURCE=..\src\Style.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\StyleContext.cxx
# End Source File
# Begin Source File

SOURCE=..\src\UniConversion.cxx
# End Source File
# Begin Source File

SOURCE=..\src\ViewStyle.cxx
# End Source File
# Begin Source File

SOURCE=..\lexlib\WordList.cxx
# End Source File
# Begin Source File

SOURCE=..\src\XPM.cxx
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\include\Platform.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\win32\Margin.cur
# End Source File
# End Group
# Begin Source File

SOURCE=.\ReadMe.txt
# End Source File
# End Target
# End Project
