// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// Note that the GPL places important restrictions on "derived works", yet
// it does not provide a detailed definition of that term.  To avoid      
// misunderstandings, we consider an application to constitute a          
// "derivative work" for the purpose of this license if it does any of the
// following:                                                             
// 1. Integrates source code from Notepad++.
// 2. Integrates/includes/aggregates Notepad++ into a proprietary executable
//    installer, such as those produced by InstallShield.
// 3. Links to a library or executes a program that does any of the above.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


#ifndef FINDREPLACE_DLG_H
#define FINDREPLACE_DLG_H

#define	IDD_FIND_REPLACE_DLG			1600
#define	IDFINDWHAT						1601
#define	IDREPLACEWITH					1602
#define	IDWHOLEWORD						1603
#define IDF_WHOLEWORD	1
#define	IDMATCHCASE						1604
#define IDF_MATCHCASE	2

#define IDREGEXP						1605

#define	IDWRAP							1606
#define	IDF_WRAP	256
#define	IDUNSLASH						1607
#define	IDREPLACE						1608
#define	IDREPLACEALL					1609
#define	IDREPLACEINSEL					1610
#define	ID_STATICTEXT_REPLACE			1611
#define	IDDIRECTIONUP					1612
#define	IDDIRECTIONDOWN 				1613
#define IDF_WHICH_DIRECTION	512
#define	IDCCOUNTALL						1614
#define	IDCMARKALL						1615
#define	IDC_MARKLINE_CHECK				1616
#define IDF_MARKLINE_CHECK	16
//#define	IDC_STYLEFOUND_CHECK			1617
#define IDF_STYLEFOUND_CHECK	8
#define	IDC_PURGE_CHECK					1618
#define IDF_PURGE_CHECK	4
#define	IDC_FINDALL_STATIC				1619
#define	IDFINDWHAT_STATIC				1620
#define	IDC_DIR_STATIC					1621

#define	IDC_PERCENTAGE_SLIDER			1622
#define	IDC_TRANSPARENT_GRPBOX 			1623

#define IDC_MODE_STATIC					1624
#define IDNORMAL						1625
#define IDEXTENDED						1626

#define	IDC_FIND_IN_STATIC				1628
//#define	IDC_CURRENT_FILE_RADIO		1629
//#define	IDC_OPENED_FILES_RADIO		1630
//#define	IDC_FILES_RADIO				1631
#define	IDC_IN_SELECTION_CHECK   		1632
#define	IDF_IN_SELECTION_CHECK	128
#define	IDC_CLEAR_ALL            		1633
#define	IDC_REPLACEINSELECTION   		1634
#define	IDC_REPLACE_OPENEDFILES  		1635
#define	IDC_FINDALL_OPENEDFILES  		1636
//#define	IDC_FINDINFILES  1637
#define	IDC_FINDINFILES_LAUNCH			1638
#define	IDC_GETCURRENTDOCTYPE			1639
#define	IDC_FINDALL_CURRENTFILE			1641
//#define	IDSWITCH  1640

#define	IDD_FINDINFILES_DLG				1650
#define	IDD_FINDINFILES_BROWSE_BUTTON	1651
#define	IDD_FINDINFILES_FILTERS_COMBO	1652
#define	IDD_FINDINFILES_DIR_COMBO		1653
#define	IDD_FINDINFILES_FILTERS_STATIC	1654
#define	IDD_FINDINFILES_DIR_STATIC		1655
#define	IDD_FINDINFILES_FIND_BUTTON		1656
#define	IDD_FINDINFILES_GOBACK_BUTTON	1657
#define	IDD_FINDINFILES_RECURSIVE_CHECK		1658
#define IDF_FINDINFILES_RECURSIVE_CHECK	32
#define	IDD_FINDINFILES_INHIDDENDIR_CHECK	1659
#define	IDF_FINDINFILES_INHIDDENDIR_CHECK	64
#define	IDD_FINDINFILES_REPLACEINFILES	1660
#define	IDD_FINDINFILES_FOLDERFOLLOWSDOC_CHECK	1661

#define	IDD_FINDRESULT					1670

#define	IDD_INCREMENT_FIND				1680
#define	IDC_INCSTATIC					1681
#define	IDC_INCFINDTEXT					1682
#define	IDC_INCFINDPREVOK				1683
#define	IDC_INCFINDNXTOK				1684
#define	IDC_INCFINDMATCHCASE			1685
#define	IDC_TRANSPARENT_CHECK				1686
#define	IDC_TRANSPARENT_LOSSFOCUS_RADIO		1687
#define	IDC_TRANSPARENT_ALWAYS_RADIO		1688
#define IDC_INCFINDSTATUS				1689
#define	IDC_INCFINDHILITEALL			1690

#define	IDB_INCREMENTAL_BG				1691


#define IDC_FRCOMMAND_INIT				1700
#define IDC_FRCOMMAND_EXEC				1701
#define IDC_FRCOMMAND_BOOLEANS			1702

#define IDREDOTMATCHNL					1703
#define IDF_REDOTMATCHNL	1024
#endif //FINDREPLACE_DLG_H
