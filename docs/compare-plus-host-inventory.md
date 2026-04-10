# ComparePlus Host-Surface Inventory

Generated as part of Phase 0, Issue #100.

## Tier 1: Blocks Basic Compare (MUST implement first)

| Message | Used In | Host Status |
|---------|---------|-------------|
| `NPPM_GETCURRENTBUFFERID` | NppHelpers.h:473 | **NOT IMPLEMENTED** |
| `NPPM_GETPOSFROMBUFFERID` | NppHelpers.h:459,466; NppHelpers.cpp:302 | **NOT IMPLEMENTED** |
| `NPPM_GETBUFFERIDFROMPOS` | NppHelpers.cpp:508,516 | **NOT IMPLEMENTED** |
| `NPPM_GETFULLPATHFROMBUFFERID` | Compare.h:77; NppHelpers.cpp:509,511,517,519; Compare.cpp:1135 | **NOT IMPLEMENTED** |
| `NPPM_DOOPEN` | Compare.cpp:2983 | **NOT IMPLEMENTED** |
| `NPPM_SWITCHTOFILE` | Compare.cpp:5108,5113 | **NOT IMPLEMENTED** |
| `NPPM_SETBUFFERLANGTYPE` | Compare.cpp (createTempFile) | **NOT IMPLEMENTED** |
| `NPPM_GETBUFFERLANGTYPE` | Compare.cpp (createTempFile) | **NOT IMPLEMENTED** |
| `NPPN_BUFFERACTIVATED` | Compare.cpp (onBufferActivated) | **NOT EMITTED** |

## Tier 2: Blocks UI/Toolbar

| Message | Used In | Host Status |
|---------|---------|-------------|
| `NPPM_ADDTOOLBARICON_FORDARKMODE` | Compare.cpp (onToolBarReady) | **NOT IMPLEMENTED** |
| `NPPM_HIDETABBAR` | Compare.cpp (5 calls) | **NOT IMPLEMENTED** |
| `NPPM_GETMENUHANDLE` | Compare.cpp (NppState, 8 calls) | **NOT IMPLEMENTED** |
| `NPPM_DMMREGASDCKDLG` | NavDialog.cpp | **NOT IMPLEMENTED** |
| `NPPM_DMMSHOW` / `NPPM_DMMHIDE` | NavDialog.cpp | **NOT IMPLEMENTED** |
| `NPPN_TBMODIFICATION` | Compare.cpp (onToolBarReady) | **NOT EMITTED** |
| `NPPN_BEFORESHUTDOWN` | Compare.cpp (onBeforeShutdown) | **NOT EMITTED** |

## Tier 3: Polish

| Message | Used In | Host Status |
|---------|---------|-------------|
| `NPPM_SETSTATUSBAR` | Compare.cpp (setStatus) | **NOT IMPLEMENTED** |
| `NPPM_ADDSCNMODIFIEDFLAGS` | Compare.cpp (onNppReady) | **NOT IMPLEMENTED** |
| `NPPM_GETLINENUMBERWIDTHMODE` | Compare.cpp (NppState) | **NOT IMPLEMENTED** |
| `NPPM_SETLINENUMBERWIDTHMODE` | Compare.cpp (3 calls) | **NOT IMPLEMENTED** |
| `NPPM_GETCURRENTCMDLINE` | Compare.cpp (checkCmdLine) | **NOT IMPLEMENTED** |
| `NPPN_GLOBALMODIFIED` | Compare.cpp | **NOT EMITTED** |
| `NPPN_DARKMODECHANGED` | Compare.cpp | **NOT EMITTED** |
| `NPPN_WORDSTYLESUPDATED` | Compare.cpp | **NOT EMITTED** |

## Already Implemented (working)

NPPM: GETCURRENTSCINTILLA, GETCURRENTLANGTYPE, SETCURRENTLANGTYPE, GETCURRENTVIEW,
GETNBOPENFILES, MENUCOMMAND, SETMENUITEMCHECK, GETPLUGINSCONFIGDIR, GETNPPVERSION,
ALLOCATECMDID, ALLOCATEMARKER, ALLOCATEINDICATOR, GETFULLCURRENTPATH, GETFILENAME,
GETCURRENTDIRECTORY, GETNAMEPART, GETEXTPART, GETCURRENTWORD, GETCURRENTLINE, GETCURRENTCOLUMN

NPPN: NPPN_READY, NPPN_SHUTDOWN, NPPN_LANGCHANGED, NPPN_FILEBEFORECLOSE, NPPN_FILESAVED
