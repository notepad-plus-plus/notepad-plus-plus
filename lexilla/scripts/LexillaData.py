#!/usr/bin/env python3
# ScintillaData.py - implemented 2013 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

# Common code used by Scintilla and SciTE for source file regeneration.
# The ScintillaData object exposes information about Scintilla as properties:
# Version properties
#     version
#     versionDotted
#     versionCommad
#
# Date last modified
#     dateModified
#     yearModified
#     mdyModified
#     dmyModified
#     myModified
#
# Information about lexers and properties defined in lexers
#     lexFiles
#         sorted list of lexer files
#     lexerModules
#         sorted list of module names
#     lexerProperties
#         sorted list of lexer properties
#     propertyDocuments
#         dictionary of property documentation { name: document string }
#     sclexFromName
#         dictionary of SCLEX_* IDs { name: SCLEX_ID }
#     fileFromSclex
#         dictionary of file names { SCLEX_ID: file name }

# This file can be run to see the data it provides.
# Requires Python 3.6 or later

import datetime, pathlib, sys, textwrap

thisPath = pathlib.Path(__file__).resolve()

sys.path.append(str(thisPath.parent.parent.parent / "scintilla" / "scripts"))

import FileGenerator

neutralEncoding = "cp437"	# Each byte value is valid in cp437

def FindModules(lexFile):
    modules = []
    partLine = ""
    with lexFile.open(encoding=neutralEncoding) as f:
        lineNum = 0
        for l in f.readlines():
            lineNum += 1
            l = l.rstrip()
            if partLine or l.startswith("LexerModule"):
                if ")" in l:
                    l = partLine + l
                    original = l
                    l = l.replace("(", " ")
                    l = l.replace(")", " ")
                    l = l.replace(",", " ")
                    parts = l.split()
                    lexerName = parts[4]
                    if not (lexerName.startswith('"') and lexerName.endswith('"')):
                        print(f"{lexFile}:{lineNum}: Bad LexerModule statement:\n{original}")
                        exit(1)
                    lexerName = lexerName.strip('"')
                    modules.append([parts[1], parts[2], lexerName])
                    partLine = ""
                else:
                    partLine = partLine + l
    return modules

def FindLexersInXcode(xCodeProject):
    lines = FileGenerator.ReadFileAsList(xCodeProject)

    uidsOfBuild = {}
    markersPBXBuildFile = ["Begin PBXBuildFile section", "", "End PBXBuildFile section"]
    for buildLine in lines[FileGenerator.FindSectionInList(lines, markersPBXBuildFile)]:
        # Occurs for each file in the build. Find the UIDs used for the file.
        #\t\t[0-9A-F]+ /* [a-zA-Z]+.cxx in sources */ = {isa = PBXBuildFile; fileRef = [0-9A-F]+ /* [a-zA-Z]+ */; };
        pieces = buildLine.split()
        uid1 = pieces[0]
        filename = pieces[2].split(".")[0]
        uid2 = pieces[12]
        uidsOfBuild[filename] = [uid1, uid2]

    lexers = {}
    markersLexers = ["/* Lexers */ =", "children", ");"]
    for lexerLine in lines[FileGenerator.FindSectionInList(lines, markersLexers)]:
        #\t\t\t\t[0-9A-F]+ /* [a-zA-Z]+.cxx */,
        uid, _, rest = lexerLine.partition("/* ")
        uid = uid.strip()
        lexer, _, _ = rest.partition(".")
        lexers[lexer] = uidsOfBuild[lexer]

    return lexers

# Properties that start with lexer. or fold. are automatically found but there are some
# older properties that don't follow this pattern so must be explicitly listed.
knownIrregularProperties = [
    "fold",
    "styling.within.preprocessor",
    "tab.timmy.whinge.level",
    "asp.default.language",
    "html.tags.case.sensitive",
    "ps.level",
    "ps.tokenize",
    "sql.backslash.escapes",
    "nsis.uservars",
    "nsis.ignorecase"
]

def FindProperties(lexFile):
    properties = {}
    with open(lexFile, encoding=neutralEncoding) as f:
        for l in f.readlines():
            if ("GetProperty" in l or "DefineProperty" in l) and "\"" in l:
                l = l.strip()
                if not l.startswith("//"):	# Drop comments
                    propertyName = l.split("\"")[1]
                    if propertyName.lower() == propertyName:
                        # Only allow lower case property names
                        if propertyName in knownIrregularProperties or \
                            propertyName.startswith("fold.") or \
                            propertyName.startswith("lexer."):
                            properties[propertyName] = 1
    return properties

def FindPropertyDocumentation(lexFile):
    documents = {}
    with lexFile.open(encoding=neutralEncoding) as f:
        name = ""
        for l in f.readlines():
            l = l.strip()
            if "// property " in l:
                propertyName = l.split()[2]
                if propertyName.lower() == propertyName:
                    # Only allow lower case property names
                    name = propertyName
                    documents[name] = ""
            elif "DefineProperty" in l and "\"" in l:
                propertyName = l.split("\"")[1]
                if propertyName.lower() == propertyName:
                    # Only allow lower case property names
                    name = propertyName
                    documents[name] = ""
            elif name:
                if l.startswith("//"):
                    if documents[name]:
                        documents[name] += " "
                    documents[name] += l[2:].strip()
                elif l.startswith("\""):
                    l = l[1:].strip()
                    if l.endswith(";"):
                        l = l[:-1].strip()
                    if l.endswith(")"):
                        l = l[:-1].strip()
                    if l.endswith("\""):
                        l = l[:-1]
                    # Fix escaped double quotes
                    l = l.replace("\\\"", "\"")
                    documents[name] += l
                else:
                    name = ""
    for name in list(documents.keys()):
        if documents[name] == "":
            del documents[name]
    return documents

def FindCredits(historyFile):
    credits = []
    stage = 0
    with historyFile.open(encoding="utf-8") as f:
        for l in f.readlines():
            l = l.strip()
            if stage == 0 and l == "<table>":
                stage = 1
            elif stage == 1 and l == "</table>":
                stage = 2
            if stage == 1 and l.startswith("<td>"):
                credit = l[4:-5]
                if "<a" in l:
                    title, a, rest = credit.partition("<a href=")
                    urlplus, _bracket, end = rest.partition(">")
                    name = end.split("<")[0]
                    url = urlplus[1:-1]
                    credit = title.strip()
                    if credit:
                        credit += " "
                    credit += name + " " + url
                credits.append(credit)
    return credits

def ciKey(a):
    return str(a).lower()

def SortListInsensitive(l):
    l.sort(key=ciKey)

class LexillaData:
    def __init__(self, scintillaRoot):
        # Discover version information
        self.version = (scintillaRoot / "version.txt").read_text().strip()
        self.versionDotted = self.version[0] + '.' + self.version[1] + '.' + \
            self.version[2]
        self.versionCommad = self.versionDotted.replace(".", ", ") + ', 0'

        with (scintillaRoot / "doc" / "Lexilla.html").open() as f:
            self.dateModified = [l for l in f.readlines() if "Date.Modified" in l]\
                [0].split('\"')[3]
            # 20130602
            # Lexilla.html
            dtModified = datetime.datetime.strptime(self.dateModified, "%Y%m%d")
            self.yearModified = self.dateModified[0:4]
            monthModified = dtModified.strftime("%B")
            dayModified = "%d" % dtModified.day
            self.mdyModified = monthModified + " " + dayModified + " " + self.yearModified
            # May 22 2013
            # Lexilla.html, SciTE.html
            self.dmyModified = dayModified + " " + monthModified + " " + self.yearModified
            # 22 May 2013
            # LexillaHistory.html -- only first should change
            self.myModified = monthModified + " " + self.yearModified

        # Find all the lexer source code files
        lexFilePaths = list((scintillaRoot / "lexers").glob("Lex*.cxx"))
        SortListInsensitive(lexFilePaths)
        self.lexFiles = [f.stem for f in lexFilePaths]
        self.lexerModules = []
        lexerProperties = set()
        self.propertyDocuments = {}
        self.sclexFromName = {}
        self.fileFromSclex = {}
        for lexFile in lexFilePaths:
            modules = FindModules(lexFile)
            for module in modules:
                self.sclexFromName[module[2]] = module[1]
                self.fileFromSclex[module[1]] = lexFile
                self.lexerModules.append(module[0])
            for k in FindProperties(lexFile).keys():
                lexerProperties.add(k)
            documents = FindPropertyDocumentation(lexFile)
            for k in documents.keys():
                if k not in self.propertyDocuments:
                    self.propertyDocuments[k] = documents[k]
        SortListInsensitive(self.lexerModules)
        self.lexerProperties = list(lexerProperties)
        SortListInsensitive(self.lexerProperties)

        self.lexersXcode = FindLexersInXcode(scintillaRoot /
            "src/Lexilla/Lexilla.xcodeproj/project.pbxproj")
        self.credits = FindCredits(scintillaRoot / "doc" / "LexillaHistory.html")

def printWrapped(text):
    print(textwrap.fill(text, subsequent_indent="    "))

if __name__=="__main__":
    sci = LexillaData(pathlib.Path(__file__).resolve().parent.parent)
    print("Version   %s   %s   %s" % (sci.version, sci.versionDotted, sci.versionCommad))
    print("Date last modified    %s   %s   %s   %s   %s" % (
        sci.dateModified, sci.yearModified, sci.mdyModified, sci.dmyModified, sci.myModified))
    printWrapped(str(len(sci.lexFiles)) + " lexer files: " + ", ".join(sci.lexFiles))
    printWrapped(str(len(sci.lexerModules)) + " lexer modules: " + ", ".join(sci.lexerModules))
    #~ printWrapped(str(len(sci.lexersXcode)) + " Xcode lexer references: " + ", ".join(
        #~ [lex+":"+uids[0]+","+uids[1] for lex, uids in sci.lexersXcode.items()]))
    print("Lexer name to ID:")
    lexNames = sorted(sci.sclexFromName.keys())
    for lexName in lexNames:
        sclex = sci.sclexFromName[lexName]
        fileName = sci.fileFromSclex[sclex].name
        print("    " + lexName + " -> " + sclex + " in " + fileName)
    printWrapped("Lexer properties: " + ", ".join(sci.lexerProperties))
    print("Lexer property documentation:")
    documentProperties = list(sci.propertyDocuments.keys())
    SortListInsensitive(documentProperties)
    for k in documentProperties:
        print("    " + k)
        print(textwrap.fill(sci.propertyDocuments[k], initial_indent="        ",
            subsequent_indent="        "))
    print("Credits:")
    for c in sci.credits:
        sys.stdout.buffer.write(b"    " + c.encode("utf-8") + b"\n")
