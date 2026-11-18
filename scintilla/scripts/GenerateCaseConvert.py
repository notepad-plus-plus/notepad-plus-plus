#!/usr/bin/env python3
# Script to generate CaseConvert.cxx from Python's Unicode data
# Should be run rarely when a Python with a new version of Unicode data is available.
# Requires Python 3.3 or later
# Should not be run with old versions of Python.

# Current best approach divides case conversions into two cases:
# simple symmetric and complex.
# Simple symmetric is where a lower and upper case pair convert to each
# other and the folded form is the same as the lower case.
# There are 1006 symmetric pairs.
# These are further divided into ranges (stored as lower, upper, range length,
# range pitch and singletons (stored as lower, upper).
# Complex is for cases that don't fit the above: where there are multiple
# characters in one of the forms or fold is different to lower or
# lower(upper(x)) or upper(lower(x)) are not x. These are represented as UTF-8
# strings with original, folded, upper, and lower separated by '|'.
# There are 126 complex cases.

import itertools, string, sys

from FileGenerator import Regenerate

def contiguousRanges(ll, diff):
    # ll is a list of lists
    # group into lists where first element of each element differs by diff
    out = [[ll[0]]]
    for s in ll[1:]:
        if s[0] != out[-1][-1][0] + diff:
            out.append([])
        out[-1].append(s)
    return out

def flatten(listOfLists):
    "Flatten one level of nesting"
    return itertools.chain.from_iterable(listOfLists)

def conversionSets():
    # For all Unicode characters, see whether they have case conversions
    # Return 2 sets: one of simple symmetric conversion cases and another
    # with complex cases.
    complexes = []
    symmetrics = []
    for ch in range(sys.maxunicode + 1):
        if ch >= 0xd800 and ch <= 0xDBFF:
            continue
        if ch >= 0xdc00 and ch <= 0xDFFF:
            continue
        uch = chr(ch)

        fold = uch.casefold()
        upper = uch.upper()
        lower = uch.lower()
        symmetric = False
        if uch != upper and len(upper) == 1 and uch == lower and uch == fold:
            lowerUpper = upper.lower()
            foldUpper = upper.casefold()
            if lowerUpper == foldUpper and lowerUpper == uch:
                symmetric = True
                symmetrics.append((ch, ord(upper), ch - ord(upper)))
        if uch != lower and len(lower) == 1 and uch == upper and lower == fold:
            upperLower = lower.upper()
            if upperLower == uch:
                symmetric = True

        if fold == uch:
            fold = ""
        if upper == uch:
            upper = ""
        if lower == uch:
            lower = ""

        if (fold or upper or lower) and not symmetric:
            complexes.append((uch, fold, upper, lower))

    return symmetrics, complexes

def groupRanges(symmetrics):
    # Group the symmetrics into groups where possible, returning a list
    # of ranges and a list of symmetrics that didn't fit into a range

    def distance(s):
        return s[2]

    groups = []
    uniquekeys = []
    for k, g in itertools.groupby(symmetrics, distance):
        groups.append(list(g))      # Store group iterator as a list
        uniquekeys.append(k)

    contiguousGroups = flatten([contiguousRanges(g, 1) for g in groups])
    longGroups = [(x[0][0], x[0][1], len(x), 1) for x in contiguousGroups if len(x) > 4]

    oneDiffs = [s for s in symmetrics if s[2] == 1]
    contiguousOnes = flatten([contiguousRanges(g, 2) for g in [oneDiffs]])
    longOneGroups = [(x[0][0], x[0][1], len(x), 2) for x in contiguousOnes if len(x) > 4]

    rangeGroups = sorted(longGroups+longOneGroups, key=lambda s: s[0])

    rangeCoverage = list(flatten([range(r[0], r[0]+r[2]*r[3], r[3]) for r in rangeGroups]))

    nonRanges = [(x, u) for x, u, _d in symmetrics if x not in rangeCoverage]

    return rangeGroups, nonRanges

def escape(s):
    return "".join((chr(c) if chr(c) in string.ascii_letters else "\\x%x" % c) for c in s.encode('utf-8'))

def updateCaseConvert():
    symmetrics, complexes = conversionSets()

    rangeGroups, nonRanges = groupRanges(symmetrics)

    print(len(rangeGroups), "ranges")
    rangeLines = ["%d,%d,%d,%d," % x for x in rangeGroups]

    print(len(nonRanges), "non ranges")
    nonRangeLines = ["%d,%d," % x for x in nonRanges]

    print(len(symmetrics), "symmetric")

    complexLines = ['"%s|%s|%s|%s|"' % tuple(escape(t) for t in x) for x in complexes]
    print(len(complexLines), "complex")

    Regenerate("../src/CaseConvert.cxx", "//", rangeLines, nonRangeLines, complexLines)

updateCaseConvert()
