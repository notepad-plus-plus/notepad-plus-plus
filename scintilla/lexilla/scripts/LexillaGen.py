#!/usr/bin/env python3
# LexillaGen.py - implemented 2019 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

# Regenerate the Lexilla source files that list all the lexers.
# Should be run whenever a new lexer is added or removed.
# Requires Python 3.6 or later
# Files are regenerated in place with templates stored in comments.
# The format of generation comments is documented in FileGenerator.py.

import os, pathlib, sys, uuid

thisPath = pathlib.Path(__file__).resolve()

sys.path.append(str(thisPath.parent.parent.parent / "scripts"))

from FileGenerator import Regenerate, UpdateLineInFile, \
    ReplaceREInFile, UpdateLineInPlistFile, ReadFileAsList, UpdateFileFromLines, \
    FindSectionInList
import ScintillaData

sys.path.append(str(thisPath.parent.parent / "src"))
import DepGen

# RegenerateXcodeProject and assiciated functions is copied from scintilla/scripts/LexGen.py

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

def RegenerateAll(rootDirectory):

    root = pathlib.Path(rootDirectory)

    scintillaBase = root.resolve()

    sci = ScintillaData.ScintillaData(scintillaBase)

    lexillaDir = scintillaBase / "lexilla"
    srcDir = lexillaDir / "src"

    Regenerate(srcDir / "Lexilla.cxx", "//", sci.lexerModules)
    Regenerate(srcDir / "lexilla.mak", "#", sci.lexFiles)

    # Discover version information
    version = (lexillaDir / "version.txt").read_text().strip()
    versionDotted = version[0] + '.' + version[1] + '.' + version[2]
    versionCommad = versionDotted.replace(".", ", ") + ', 0'

    rcPath = srcDir / "LexillaVersion.rc"
    UpdateLineInFile(rcPath, "#define VERSION_LEXILLA",
        "#define VERSION_LEXILLA \"" + versionDotted + "\"")
    UpdateLineInFile(rcPath, "#define VERSION_WORDS",
        "#define VERSION_WORDS " + versionCommad)

    lexillaXcode = lexillaDir / "src" / "Lexilla"
    lexillaXcodeProject = lexillaXcode / "Lexilla.xcodeproj" / "project.pbxproj"

    lexerReferences = ScintillaData.FindLexersInXcode(lexillaXcodeProject)

    UpdateLineInPlistFile(lexillaXcode / "Info.plist",
        "CFBundleShortVersionString", versionDotted)

    ReplaceREInFile(lexillaXcodeProject, "CURRENT_PROJECT_VERSION = [0-9.]+;",
        f'CURRENT_PROJECT_VERSION = {versionDotted};')

    RegenerateXcodeProject(lexillaXcodeProject, sci.lexFiles, lexerReferences)

    currentDirectory = pathlib.Path.cwd()
    os.chdir(srcDir)
    DepGen.Generate()
    os.chdir(currentDirectory)

if __name__=="__main__":
    RegenerateAll(pathlib.Path(__file__).resolve().parent.parent.parent)
