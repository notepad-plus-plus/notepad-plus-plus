#!/usr/bin/env python3
# Script to check that headers are in a consistent order
# Canonical header order is defined in a file, normally scripts/HeaderOrder.txt
# Requires Python 3.6 or later

import pathlib, sys

def IsHeader(x):
    return x.strip().startswith("#") and \
        ("include" in x or "import" in x) and \
        "dllimport" not in x

def HeaderFromIncludeLine(s):
    #\s*#\s*(include|import)\s+\S+\s*
    return s.strip()[1:].strip()[7:].strip()

def ExtractHeaders(file):
    with file.open(encoding="cp437") as infile:
        return [HeaderFromIncludeLine(l) for l in infile if IsHeader(l)]

def ExtractWithPrefix(file, prefix):
    with file.open(encoding="cp437") as infile:
        return [l.strip()[len(prefix):] for l in infile if l.startswith(prefix)]

def ExcludeName(name, excludes):
    return any(exclude in name for exclude in excludes)

def SortLike(incs, order):
    return sorted(incs, key = lambda i: order.index(i))

basePrefix = "//base:"
sourcePrefix = "//source:"
excludePrefix = "//exclude:"

def CheckFiles(headerOrderTxt):
    headerOrderFile = pathlib.Path(headerOrderTxt).resolve()
    bases = ExtractWithPrefix(headerOrderFile, basePrefix)
    base = bases[0] if len(bases) > 0 else ".."
    orderDirectory = headerOrderFile.parent
    root = (orderDirectory / base).resolve()

    # Find all the source code files
    patterns = ExtractWithPrefix(headerOrderFile, sourcePrefix)
    excludes = ExtractWithPrefix(headerOrderFile, excludePrefix)

    filePaths = []
    for p in patterns:
        filePaths += root.glob(p)
    headerOrder = ExtractHeaders(headerOrderFile)
    originalOrder = headerOrder[:]
    orderedPaths = [p for p in sorted(filePaths) if not ExcludeName(str(p), excludes)]
    allIncs = set()
    for f in orderedPaths:
        #~ print("   File ", f.relative_to(root))
        incs = ExtractHeaders(f)
        allIncs = allIncs.union(set(incs))

        m = 0
        i = 0
        # Detect headers not in header order list and insert at OK position
        needs = []
        while i < len(incs):
            if m == len(headerOrder):
                #~ print("**** extend", incs[i:])
                headerOrder.extend(incs[i:])
                needs.extend(incs[i:])
                break
            if headerOrder[m] == incs[i]:
                #~ print("equal", headerOrder[m])
                i += 1
                m += 1
            else:
                if headerOrder[m] not in incs:
                    #~ print("skip", headerOrder[m])
                    m += 1
                elif incs[i] not in headerOrder:
                    #~ print(str(f) + ":1: Add master", incs[i])
                    headerOrder.insert(m, incs[i])
                    needs.append(incs[i])
                    i += 1
                    m += 1
                else:
                    i += 1
        if needs:
            print(f"{f}:1: needs these headers:")
            for header in needs:
                print("#include " + header)

        # Detect out of order
        ordered = SortLike(incs, headerOrder)
        if incs != ordered:
            print(f"{f}:1: is out of order")
            fOrdered = pathlib.Path(str(f) + ".ordered")
            with fOrdered.open("w") as headerOut:
                for header in ordered:
                    headerOut.write("#include " + header + "\n")
            print(f"{fOrdered}:1: is ordered")

    if headerOrder != originalOrder:
        newIncludes = set(headerOrder) - set(originalOrder)
        headerOrderNew = orderDirectory / "NewOrder.txt"
        print(f"{headerOrderFile}:1: changed to {headerOrderNew}")
        print(f"   Added {', '.join(newIncludes)}.")
        with headerOrderNew.open("w") as headerOut:
            for header in headerOrder:
                headerOut.write("#include " + header + "\n")

    unused = sorted(set(headerOrder) - allIncs)
    if unused:
        print("In HeaderOrder.txt but not used")
        print("\n".join(unused))

if len(sys.argv) > 1:
    CheckFiles(sys.argv[1])
else:
    CheckFiles("HeaderOrder.txt")
