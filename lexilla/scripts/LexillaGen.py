#!/usr/bin/env python3
# LexillaGen.py - implemented 2019 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

"""
Regenerate the Lexilla source files that list all the lexers.
"""

# Should be run whenever a new lexer is added or removed.
# Requires Python 3.6 or later
# Files are regenerated in place with templates stored in comments.
# The format of generation comments is documented in FileGenerator.py.

import os, pathlib, sys, uuid

thisPath = pathlib.Path(__file__).resolve()

sys.path.append(str(thisPath.parent.parent.parent / "scintilla" / "scripts"))

from FileGenerator import Regenerate, UpdateLineInFile, \
    ReplaceREInFile, UpdateLineInPlistFile, UpdateFileFromLines
import LexillaData
import LexFacer

sys.path.append(str(thisPath.parent.parent / "src"))
import DepGen

# RegenerateXcodeProject and assiciated functions is copied from scintilla/scripts/LexGen.py

def uid24():
    """ Last 24 digits of UUID, used for item IDs in Xcode. """
    return str(uuid.uuid4()).replace("-", "").upper()[-24:]

def ciLexerKey(a):
    """ Return 3rd element of string lowered to be used when sorting. """
    return a.split()[2].lower()


"""
		11F35FDB12AEFAF100F0236D /* LexA68k.cxx in Sources */ = {isa = PBXBuildFile; fileRef = 11F35FDA12AEFAF100F0236D /* LexA68k.cxx */; };
		11F35FDA12AEFAF100F0236D /* LexA68k.cxx */ = {isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = LexA68k.cxx; path = ../../lexers/LexA68k.cxx; sourceTree = SOURCE_ROOT; };
				11F35FDA12AEFAF100F0236D /* LexA68k.cxx */,
				11F35FDB12AEFAF100F0236D /* LexA68k.cxx in Sources */,
"""
def RegenerateXcodeProject(path, lexers, lexerReferences):
    """ Regenerate project to include any new lexers. """
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
            linePBXBuildFile = f"\t\t{uid1} /* {lexer}.cxx in Sources */ = {{isa = PBXBuildFile; fileRef = {uid2} /* {lexer}.cxx */; }};"
            linePBXFileReference = f"\t\t{uid2} /* {lexer}.cxx */ = {{isa = PBXFileReference; fileEncoding = 4; lastKnownFileType = sourcecode.cpp.cpp; name = {lexer}.cxx; path = ../../lexers/{lexer}.cxx; sourceTree = SOURCE_ROOT; }};"
            lineLexers = f"\t\t\t\t{uid2} /* {lexer}.cxx */,"
            linePBXSourcesBuildPhase = f"\t\t\t\t{uid1} /* {lexer}.cxx in Sources */,"
            sectionPBXBuildFile.append(linePBXBuildFile)
            sectionPBXFileReference.append(linePBXFileReference)
            sectionLexers.append(lineLexers)
            sectionPBXSourcesBuildPhase.append(linePBXSourcesBuildPhase)

    lines = LexillaData.ReadFileAsList(path)

    sli = LexillaData.FindSectionInList(lines, markersPBXBuildFile)
    lines[sli.stop:sli.stop] = sectionPBXBuildFile

    sli = LexillaData.FindSectionInList(lines, markersPBXFileReference)
    lines[sli.stop:sli.stop] = sectionPBXFileReference

    sli = LexillaData.FindSectionInList(lines, markersLexers)
    # This section is shown in the project outline so sort it to make it easier to navigate.
    allLexers = sorted(lines[sli.start:sli.stop] + sectionLexers, key=ciLexerKey)
    lines[sli] = allLexers

    sli = LexillaData.FindSectionInList(lines, markersPBXSourcesBuildPhase)
    lines[sli.stop:sli.stop] = sectionPBXSourcesBuildPhase

    UpdateFileFromLines(path, lines, os.linesep)

def RegenerateAll(rootDirectory):
    """ Regenerate all the files. """

    root = pathlib.Path(rootDirectory)

    lexillaBase = root.resolve()

    lex = LexillaData.LexillaData(lexillaBase)

    lexillaDir = lexillaBase
    srcDir = lexillaDir / "src"
    docDir = lexillaDir / "doc"

    Regenerate(srcDir / "Lexilla.cxx", "//", lex.lexerModules)
    Regenerate(srcDir / "lexilla.mak", "#", lex.lexFiles)

    # Discover version information
    version = (lexillaDir / "version.txt").read_text().strip()
    versionDotted = version[0:-2] + '.' + version[-2] + '.' + version[-1]
    versionCommad = versionDotted.replace(".", ", ") + ', 0'

    rcPath = srcDir / "LexillaVersion.rc"
    UpdateLineInFile(rcPath, "#define VERSION_LEXILLA",
        "#define VERSION_LEXILLA \"" + versionDotted + "\"")
    UpdateLineInFile(rcPath, "#define VERSION_WORDS",
        "#define VERSION_WORDS " + versionCommad)
    UpdateLineInFile(docDir / "LexillaDownload.html", "       Release",
        "       Release " + versionDotted)
    ReplaceREInFile(docDir / "LexillaDownload.html",
        r"/www.scintilla.org/([a-zA-Z]+)\d{3,5}",
        r"/www.scintilla.org/\g<1>" +  version,
        0)

    pathMain = lexillaDir / "doc" / "Lexilla.html"
    UpdateLineInFile(pathMain,
        '          <font color="#FFCC99" size="3">Release version',
        '          <font color="#FFCC99" size="3">Release version ' + \
        versionDotted + '<br />')
    UpdateLineInFile(pathMain,
        '           Site last modified',
        '           Site last modified ' + lex.mdyModified + '</font>')
    UpdateLineInFile(pathMain,
        '    <meta name="Date.Modified"',
        '    <meta name="Date.Modified" content="' + lex.dateModified + '" />')
    UpdateLineInFile(lexillaDir / "doc" / "LexillaHistory.html",
        '	Released ',
        '	Released ' + lex.dmyModified + '.')

    lexillaXcode = lexillaDir / "src" / "Lexilla"
    lexillaXcodeProject = lexillaXcode / "Lexilla.xcodeproj" / "project.pbxproj"

    lexerReferences = LexillaData.FindLexersInXcode(lexillaXcodeProject)

    UpdateLineInPlistFile(lexillaXcode / "Info.plist",
        "CFBundleShortVersionString", versionDotted)

    ReplaceREInFile(lexillaXcodeProject, "CURRENT_PROJECT_VERSION = [0-9.]+;",
        f'CURRENT_PROJECT_VERSION = {versionDotted};',
        0)

    RegenerateXcodeProject(lexillaXcodeProject, lex.lexFiles, lexerReferences)

    LexFacer.RegenerateAll(root, False)

    currentDirectory = pathlib.Path.cwd()
    os.chdir(srcDir)
    DepGen.Generate()
    os.chdir(currentDirectory)

if __name__=="__main__":
    RegenerateAll(pathlib.Path(__file__).resolve().parent.parent)
