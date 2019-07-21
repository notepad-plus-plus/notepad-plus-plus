#!/usr/bin/env python3
# LexGen.py - implemented 2002 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

# Regenerate the Scintilla source files that list all the lexers.
# Should be run whenever a new lexer is added or removed.
# Requires Python 3.6 or later
# Files are regenerated in place with templates stored in comments.
# The format of generation comments is documented in FileGenerator.py.

from FileGenerator import Regenerate, UpdateLineInFile, \
    ReplaceREInFile, UpdateLineInPlistFile, ReadFileAsList, UpdateFileFromLines, \
    FindSectionInList
import ScintillaData
import HFacer
import os
import uuid
import sys

baseDirectory = os.path.dirname(os.path.dirname(ScintillaData.__file__))
sys.path.append(baseDirectory)

import win32.DepGen
import gtk.DepGen

def UpdateVersionNumbers(sci, root):
    UpdateLineInFile(root + "win32/ScintRes.rc", "#define VERSION_SCINTILLA",
        "#define VERSION_SCINTILLA \"" + sci.versionDotted + "\"")
    UpdateLineInFile(root + "win32/ScintRes.rc", "#define VERSION_WORDS",
        "#define VERSION_WORDS " + sci.versionCommad)
    UpdateLineInFile(root + "qt/ScintillaEditBase/ScintillaEditBase.pro",
        "VERSION =",
        "VERSION = " + sci.versionDotted)
    UpdateLineInFile(root + "qt/ScintillaEdit/ScintillaEdit.pro",
        "VERSION =",
        "VERSION = " + sci.versionDotted)
    UpdateLineInFile(root + "doc/ScintillaDownload.html", "       Release",
        "       Release " + sci.versionDotted)
    ReplaceREInFile(root + "doc/ScintillaDownload.html",
        r"/www.scintilla.org/([a-zA-Z]+)\d\d\d",
        r"/www.scintilla.org/\g<1>" +  sci.version)
    UpdateLineInFile(root + "doc/index.html",
        '          <font color="#FFCC99" size="3"> Release version',
        '          <font color="#FFCC99" size="3"> Release version ' +\
        sci.versionDotted + '<br />')
    UpdateLineInFile(root + "doc/index.html",
        '           Site last modified',
        '           Site last modified ' + sci.mdyModified + '</font>')
    UpdateLineInFile(root + "doc/ScintillaHistory.html",
        '	Released ',
        '	Released ' + sci.dmyModified + '.')
    UpdateLineInPlistFile(root + "cocoa/ScintillaFramework/Info.plist",
        "CFBundleVersion", sci.versionDotted)
    UpdateLineInPlistFile(root + "cocoa/ScintillaFramework/Info.plist",
        "CFBundleShortVersionString", sci.versionDotted)

# Last 24 digits of UUID, used for item IDs in Xcode
def uid24():
    return str(uuid.uuid4()).replace("-", "").upper()[-24:]

def ciLexerKey(a):
    return a.split()[2].lower()

"""
		11F35FDB12AEFAF100F0236D /* LexA68k.cxx in Sources */ = {isa = PBXBuildFile; fileRef = 11F35FDA12AEFAF100F0236D /* LexA68k.cxx */; };
		11F35FDA12AEFAF100F0236D /* LexA68k.cxx */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = LexA68k.cxx; path = ../../lexers/LexA68k.cxx; sourceTree = SOURCE_ROOT; };
				11F35FDA12AEFAF100F0236D /* LexA68k.cxx */,
				11F35FDB12AEFAF100F0236D /* LexA68k.cxx in Sources */,
"""
def RegenerateXcodeProject(path, lexers, lexerReferences):
    # Build 4 blocks for insertion:
    # Each markers contains a unique section start, an optional wait string, and a section end

    markersPBXBuildFile = ["Begin PBXBuildFile section", "", "End PBXBuildFile section"]
    sectionPBXBuildFile = []

    markersPBXFileReference = ["Begin PBXFileReference section", "", "End PBXFileReference section"]
    sectionPBXFileReference = []

    markersLexers = ["/* Lexers */ =", "children", ");"]
    sectionLexers = []

    markersPBXSourcesBuildPhase = ["Begin PBXSourcesBuildPhase section", "files", ");"]
    sectionPBXSourcesBuildPhase = []

    for lexer in lexers:
        if lexer not in lexerReferences:
            uid1 = uid24()
            uid2 = uid24()
            print("Lexer", lexer, "is not in Xcode project. Use IDs", uid1, uid2)
            lexerReferences[lexer] = [uid1, uid2]
            linePBXBuildFile = "\t\t{} /* {}.cxx in Sources */ = {{isa = PBXBuildFile; fileRef = {} /* {}.cxx */; }};".format(uid1, lexer, uid2, lexer)
            linePBXFileReference = "\t\t{} /* {}.cxx */ = {{isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = {}.cxx; path = ../../lexers/{}.cxx; sourceTree = SOURCE_ROOT; }};".format(uid2, lexer, lexer, lexer)
            lineLexers = "\t\t\t\t{} /* {}.cxx */,".format(uid2, lexer)
            linePBXSourcesBuildPhase = "\t\t\t\t{} /* {}.cxx in Sources */,".format(uid1, lexer)
            sectionPBXBuildFile.append(linePBXBuildFile)
            sectionPBXFileReference.append(linePBXFileReference)
            sectionLexers.append(lineLexers)
            sectionPBXSourcesBuildPhase.append(linePBXSourcesBuildPhase)

    lines = ReadFileAsList(path)

    sli = FindSectionInList(lines, markersPBXBuildFile)
    lines[sli.stop:sli.stop] = sectionPBXBuildFile

    sli = FindSectionInList(lines, markersPBXFileReference)
    lines[sli.stop:sli.stop] = sectionPBXFileReference

    sli = FindSectionInList(lines, markersLexers)
    # This section is shown in the project outline so sort it to make it easier to navigate.
    allLexers = sorted(lines[sli.start:sli.stop] + sectionLexers, key=ciLexerKey)
    lines[sli] = allLexers

    sli = FindSectionInList(lines, markersPBXSourcesBuildPhase)
    lines[sli.stop:sli.stop] = sectionPBXSourcesBuildPhase

    UpdateFileFromLines(path, lines, "\n")

def RegenerateAll(root):

    scintillaBase = os.path.abspath(root)

    sci = ScintillaData.ScintillaData(root)

    Regenerate(root + "src/Catalogue.cxx", "//", sci.lexerModules)
    Regenerate(root + "win32/scintilla.mak", "#", sci.lexFiles)

    startDir = os.getcwd()
    os.chdir(os.path.join(scintillaBase, "win32"))
    win32.DepGen.Generate()
    os.chdir(os.path.join(scintillaBase, "gtk"))
    gtk.DepGen.Generate()
    os.chdir(startDir)

    RegenerateXcodeProject(root + "cocoa/ScintillaFramework/ScintillaFramework.xcodeproj/project.pbxproj",
        sci.lexFiles, sci.lexersXcode)

    UpdateVersionNumbers(sci, root)

    HFacer.RegenerateAll(root, False)

if __name__=="__main__":
    RegenerateAll("../")
