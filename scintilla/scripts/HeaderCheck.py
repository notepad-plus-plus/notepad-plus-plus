# Script to check that headers are in a consistent order
# Requires Python 2.6 or later

from __future__ import print_function
import codecs, glob, os, platform, sys, unicodedata

def ciCompare(a,b):
    return cmp(a.lower(), b.lower())

def ciKey(a):
    return a.lower()

def SortListInsensitive(l):
    try:    # Try key function
        l.sort(key=ciKey)
    except TypeError:    # Earlier version of Python, so use comparison function
        l.sort(ciCompare)

def IsHeader(x):
    return x.strip().startswith("#") and ("include" in x or "import" in x)

def ExtractHeaders(filename):
    with codecs.open(filename, "r", "UTF-8") as infile:
        includeLines = [x.strip()[1:].strip()[7:].strip() for x in infile.readlines() if \
            IsHeader(x)]
    if '.' not in filename:
        print(filename)
        for n in includeLines:
            print(n)
        print()
    return includeLines

def CheckFiles(root):
    # Find all the lexer source code files
    filePaths = glob.glob(root + "/include/*.h")
    filePaths += glob.glob(root + "/src/*.cxx")
    SortListInsensitive(filePaths)
    filePaths += glob.glob(root + "/lexlib/*.cxx")
    filePaths += glob.glob(root + "/lexers/*.cxx")
    filePaths += glob.glob(root + "/win32/*.cxx")
    filePaths += glob.glob(root + "/gtk/*.cxx")
    filePaths += glob.glob(root + "/cocoa/*.mm")
    filePaths += glob.glob(root + "/cocoa/*.h")
    filePaths += glob.glob(root + "/test/unit/*.cxx")
    # The Qt platform code interleaves system and Scintilla headers
    #~ filePaths += glob.glob(root + "/qt/ScintillaEditBase/*.cpp")
    #~ filePaths += glob.glob(root + "/qt/ScintillaEdit/*.cpp")
    #~ print(filePaths)
    masterHeaderList = ExtractHeaders(root + "/scripts/HeaderOrder.txt")
    for f in filePaths:
        if "LexCaml" in f:
            continue
        print("   File ", f)
        try:
            incs = ExtractHeaders(f)
        except UnicodeDecodeError:
            #~ print("UnicodeDecodeError\n")
            continue
        #~ print("\n".join(incs))
        news = set(incs) - set(masterHeaderList)
        #~ print("")
        #~ print("\n".join(incs))
        #~ print("")
        ended = False
        m = 0
        i = 0
        while i < len(incs):
            if m == len(masterHeaderList):
                print("**** extend", incs[i:])
                masterHeaderList.extend(incs[i:])
                break
            if masterHeaderList[m] == incs[i]:
                #~ print("equal", masterHeaderList[m])
                i += 1
                m += 1
            else:
                if masterHeaderList[m] not in incs:
                    #~ print("skip", masterHeaderList[m])
                    m += 1
                elif incs[i] not in masterHeaderList:
                    print(f + ":1: Add master", incs[i])
                    masterHeaderList.insert(m, incs[i])
                    i += 1
                    m += 1
                else:
                    print(f + ":1: Header out of order", incs[i], masterHeaderList[m])
                    print("incs", " ".join(incs))
                    i += 1
                    #~ return
        #print("Master header list", " ".join(masterHeaderList))

CheckFiles("..")
