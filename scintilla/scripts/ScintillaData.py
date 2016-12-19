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

# This file can be run to see the data it provides.
# Requires Python 2.5 or later

from __future__ import with_statement

import codecs, datetime, glob, os, sys, textwrap

import FileGenerator

def FindModules(lexFile):
    modules = []
    with open(lexFile) as f:
        for l in f.readlines():
            if l.startswith("LexerModule"):
                l = l.replace("(", " ")
                modules.append(l.split()[1])
    return modules

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
    with open(lexFile) as f:
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
    with open(lexFile) as f:
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
    with codecs.open(historyFile, "r", "utf-8") as f:
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
                    urlplus, bracket, end = rest.partition(">")
                    name = end.split("<")[0]
                    url = urlplus[1:-1]
                    credit = title.strip()
                    if credit:
                        credit += " "
                    credit += name + " " + url
                credits.append(credit)
    return credits

def ciCompare(a,b):
    return cmp(a.lower(), b.lower())

def ciKey(a):
    return a.lower()

def SortListInsensitive(l):
    try:    # Try key function
        l.sort(key=ciKey)
    except TypeError:    # Earlier version of Python, so use comparison function
        l.sort(ciCompare)

class ScintillaData:
    def __init__(self, scintillaRoot):
        # Discover verion information
        with open(scintillaRoot + "version.txt") as f:
            self.version = f.read().strip()
        self.versionDotted = self.version[0] + '.' + self.version[1] + '.' + \
            self.version[2]
        self.versionCommad = self.version[0] + ', ' + self.version[1] + ', ' + \
            self.version[2] + ', 0'

        with open(scintillaRoot + "doc/index.html") as f:
            self.dateModified = [l for l in f.readlines() if "Date.Modified" in l]\
                [0].split('\"')[3]
            # 20130602
            # index.html, SciTE.html
            dtModified = datetime.datetime.strptime(self.dateModified, "%Y%m%d")
            self.yearModified = self.dateModified[0:4]
            monthModified = dtModified.strftime("%B")
            dayModified = "%d" % dtModified.day
            self.mdyModified = monthModified + " " + dayModified + " " + self.yearModified
            # May 22 2013
            # index.html, SciTE.html
            self.dmyModified = dayModified + " " + monthModified + " " + self.yearModified
            # 22 May 2013
            # ScintillaHistory.html -- only first should change
            self.myModified = monthModified + " " + self.yearModified

        # Find all the lexer source code files
        lexFilePaths = glob.glob(scintillaRoot + "lexers/Lex*.cxx")
        SortListInsensitive(lexFilePaths)
        self.lexFiles = [os.path.basename(f)[:-4] for f in lexFilePaths]
        self.lexerModules = []
        lexerProperties = set()
        self.propertyDocuments = {}
        for lexFile in lexFilePaths:
            self.lexerModules.extend(FindModules(lexFile))
            for k in FindProperties(lexFile).keys():
                lexerProperties.add(k)
            documents = FindPropertyDocumentation(lexFile)
            for k in documents.keys():
                if k not in self.propertyDocuments:
                    self.propertyDocuments[k] = documents[k]
        SortListInsensitive(self.lexerModules)
        self.lexerProperties = list(lexerProperties)
        SortListInsensitive(self.lexerProperties)

        self.credits = FindCredits(scintillaRoot + "doc/ScintillaHistory.html")

def printWrapped(text):
    print(textwrap.fill(text, subsequent_indent="    "))

if __name__=="__main__":
    sci = ScintillaData("../")
    print("Version   %s   %s   %s" % (sci.version, sci.versionDotted, sci.versionCommad))
    print("Date last modified    %s   %s   %s   %s   %s" % (
        sci.dateModified, sci.yearModified, sci.mdyModified, sci.dmyModified, sci.myModified))
    printWrapped(str(len(sci.lexFiles)) + " lexer files: " + ", ".join(sci.lexFiles))
    printWrapped(str(len(sci.lexerModules)) + " lexer modules: " + ", ".join(sci.lexerModules))
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
        if sys.version_info[0] == 2:
            print("    " + c.encode("utf-8"))
        else:
            sys.stdout.buffer.write(b"    " + c.encode("utf-8") + b"\n")
