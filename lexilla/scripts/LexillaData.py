#!/usr/bin/env python3
# LexillaData.py - implemented 2013 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.
# Requires FileGenerator from Scintilla so scintilla must be a peer directory of lexilla.

"""
Common code used by Lexilla and SciTE for source file regeneration.
"""

# The LexillaData object exposes information about Lexilla as properties:
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
#         sorted list of lexer file stems like LexAbaqus
#     lexerModules
#         sorted list of module names like lmAbaqus
#     lexerProperties
#         sorted list of lexer properties like lexer.bash.command.substitution
#     propertyDocuments
#         dictionary of property documentation { name: document string }
#         like lexer.bash.special.parameter: Set shell (default is Bash) special parameters.
#     sclexFromName
#         dictionary of SCLEX_* IDs { name: SCLEX_ID } like ave: SCLEX_AVE
#     fileFromSclex
#         dictionary of file names { SCLEX_ID: file name } like SCLEX_AU3: LexAU3.cxx
#     lexersXcode
#         dictionary of project file UUIDs { file name: [build UUID, file UUID] }
#         like  LexTCL: [28BA733B24E34D9700272C2D,28BA72C924E34D9100272C2D]
#     credits
#         list of names of contributors like Atsuo Ishimoto

# This file can be run to see the data it provides.
# Requires Python 3.6 or later

import datetime, pathlib, sys, textwrap

neutralEncoding = "iso-8859-1"	# Each byte value is valid in iso-8859-1

def ReadFileAsList(path):
    """Read all the lnes in the file and return as a list of strings without line ends.
    """
    with path.open(encoding="utf-8") as f:
        return [line.rstrip('\n') for line in f]

def FindModules(lexFile):
    """ Return a list of modules found within a lexer implementation file. """
    modules = []
    partLine = ""
    with lexFile.open(encoding=neutralEncoding) as f:
        lineNum = 0
        for line in f.readlines():
            lineNum += 1
            line = line.rstrip()
            if partLine or line.startswith("extern const LexerModule"):
                if ")" in line:
                    line = partLine + line
                    original = line
                    line = line.replace("(", " ")
                    line = line.replace(")", " ")
                    line = line.replace(",", " ")
                    parts = line.split()[2:]
                    lexerName = parts[4]
                    if not (lexerName.startswith('"') and lexerName.endswith('"')):
                        print(f"{lexFile}:{lineNum}: Bad LexerModule statement:\n{original}")
                        sys.exit(1)
                    lexerName = lexerName.strip('"')
                    modules.append([parts[1], parts[2], lexerName])
                    partLine = ""
                else:
                    partLine = partLine + line
    return modules

def FindSectionInList(lines, markers):
    """Find a section defined by an initial start marker, an optional secondary
    marker and an end marker.
    The section is between the secondary/initial start and the end.
    Report as a slice object so the section can be extracted or replaced.
    Raises an exception if the markers can't be found.
    Currently only used for Xcode project files.
    """
    start = -1
    end = -1
    state = 0
    for i, line in enumerate(lines):
        if markers[0] in line:
            if markers[1]:
                state = 1
            else:
                start = i+1
                state = 2
        elif state == 1:
            if markers[1] in line:
                start = i+1
                state = 2
        elif state == 2:
            if markers[2] in line:
                end = i
                state = 3
    # Check that section was found
    if start == -1:
        raise ValueError("Could not find start marker(s) |" + markers[0] + "|" + markers[1] + "|")
    if end == -1:
        raise ValueError("Could not find end marker " + markers[2])
    return slice(start, end)

def FindLexersInXcode(xCodeProject):
    """ Return a dictionary { file name: [build UUID, file UUID] } of lexers in Xcode project. """
    lines = ReadFileAsList(xCodeProject)

    # PBXBuildFile section is a list of all buildable files in the project so extract the file
    # basename and its build and file IDs
    uidsOfBuild = {}
    markersPBXBuildFile = ["Begin PBXBuildFile section", "", "End PBXBuildFile section"]
    for buildLine in lines[FindSectionInList(lines, markersPBXBuildFile)]:
        # Occurs for each file in the build. Find the UIDs used for the file.
        #\t\t[0-9A-F]+ /* [a-zA-Z]+.cxx in sources */ = {isa = PBXBuildFile; fileRef = [0-9A-F]+ /* [a-zA-Z]+ */; };
        pieces = buildLine.split()
        uid1 = pieces[0]
        filename = pieces[2].split(".")[0]
        uid2 = pieces[12]
        uidsOfBuild[filename] = [uid1, uid2]

    # PBXGroup section contains the folders (Lexilla, Lexers, LexLib, ...) so is used to find the lexers
    lexers = {}
    markersLexers = ["/* Lexers */ =", "children", ");"]
    for lexerLine in lines[FindSectionInList(lines, markersLexers)]:
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
    """ Return a set of property names in a lexer implementation file. """
    properties = set()
    with open(lexFile, encoding=neutralEncoding) as f:
        for s in f.readlines():
            if ("GetProperty" in s or "DefineProperty" in s) and "\"" in s:
                s = s.strip()
                if not s.startswith("//"):	# Drop comments
                    propertyName = s.split("\"")[1]
                    if propertyName.lower() == propertyName:
                        # Only allow lower case property names
                        if propertyName in knownIrregularProperties or \
                            propertyName.startswith("fold.") or \
                            propertyName.startswith("lexer."):
                            properties.add(propertyName)
    return properties

def FindPropertyDocumentation(lexFile):
    """ Return a dictionary { name: document string } of property documentation in a lexer. """
    documents = {}
    with lexFile.open(encoding=neutralEncoding) as f:
        name = ""
        for line in f.readlines():
            line = line.strip()
            if "// property " in line:
                propertyName = line.split()[2]
                if propertyName.lower() == propertyName:
                    # Only allow lower case property names
                    name = propertyName
                    documents[name] = ""
            elif "DefineProperty" in line and "\"" in line:
                propertyName = line.split("\"")[1]
                if propertyName.lower() == propertyName:
                    # Only allow lower case property names
                    name = propertyName
                    documents[name] = ""
            elif name:
                if line.startswith("//"):
                    if documents[name]:
                        documents[name] += " "
                    documents[name] += line[2:].strip()
                elif line.startswith("\""):
                    line = line[1:].strip()
                    if line.endswith(";"):
                        line = line[:-1].strip()
                    if line.endswith(")"):
                        line = line[:-1].strip()
                    if line.endswith("\""):
                        line = line[:-1]
                    # Fix escaped double quotes
                    line = line.replace("\\\"", "\"")
                    documents[name] += line
                else:
                    name = ""
    for name in list(documents.keys()):
        if documents[name] == "":
            del documents[name]
    return documents

def FindCredits(historyFile):
    """ Return a list of contributors in a history file. """
    creditList = []
    stage = 0
    with historyFile.open(encoding="utf-8") as f:
        for line in f.readlines():
            line = line.strip()
            if stage == 0 and line == "<table>":
                stage = 1
            elif stage == 1 and line == "</table>":
                stage = 2
            if stage == 1 and line.startswith("<td>"):
                credit = line[4:-5]
                if "<a" in line:
                    title, dummy, rest = credit.partition("<a href=")
                    urlplus, _bracket, end = rest.partition(">")
                    name = end.split("<")[0]
                    url = urlplus[1:-1]
                    credit = title.strip()
                    if credit:
                        credit += " "
                    credit += name + " " + url
                creditList.append(credit)
    return creditList

def ciKey(a):
    """ Return a string lowered to be used when sorting. """
    return str(a).lower()

def SortListInsensitive(l):
    """ Sort a list of strings case insensitively. """
    l.sort(key=ciKey)

class LexillaData:
    """ Expose information about Lexilla as properties. """

    def __init__(self, scintillaRoot):
        # Discover version information
        self.version = (scintillaRoot / "version.txt").read_text().strip()
        self.versionDotted = self.version[0:-2] + '.' + self.version[-2] + '.' + \
            self.version[-1]
        self.versionCommad = self.versionDotted.replace(".", ", ") + ', 0'

        with (scintillaRoot / "doc" / "Lexilla.html").open() as f:
            self.dateModified = [d for d in f.readlines() if "Date.Modified" in d]\
                [0].split('\"')[3]
            # 20130602
            # Lexilla.html
            dtModified = datetime.datetime.strptime(self.dateModified, "%Y%m%d")
            self.yearModified = self.dateModified[0:4]
            monthModified = dtModified.strftime("%B")
            dayModified = f"{dtModified.day}"
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
            for prop in FindProperties(lexFile):
                lexerProperties.add(prop)
            documents = FindPropertyDocumentation(lexFile)
            for prop, doc in documents.items():
                if prop not in self.propertyDocuments:
                    self.propertyDocuments[prop] = doc
        SortListInsensitive(self.lexerModules)
        self.lexerProperties = list(lexerProperties)
        SortListInsensitive(self.lexerProperties)

        self.lexersXcode = FindLexersInXcode(scintillaRoot /
            "src/Lexilla/Lexilla.xcodeproj/project.pbxproj")
        self.credits = FindCredits(scintillaRoot / "doc" / "LexillaHistory.html")

def printWrapped(text):
    """ Print string wrapped with subsequent lines indented. """
    print(textwrap.fill(text, subsequent_indent="    "))

if __name__=="__main__":
    sci = LexillaData(pathlib.Path(__file__).resolve().parent.parent)
    print(f"Version   {sci.version}   {sci.versionDotted}   {sci.versionCommad}")
    print(f"Date last modified    {sci.dateModified}   {sci.yearModified}   {sci.mdyModified}"
        f"   {sci.dmyModified}   {sci.myModified}")
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
