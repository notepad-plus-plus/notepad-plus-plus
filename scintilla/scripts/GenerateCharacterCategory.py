# Script to generate CharacterCategory.cxx from Python's Unicode data
# Should be run rarely when a Python with a new version of Unicode data is available.
# Requires Python 3.3 or later
# Should not be run with old versions of Python.

import codecs, os, platform, sys, unicodedata

from FileGenerator import Regenerate

def findCategories(filename):
    with codecs.open(filename, "r", "UTF-8") as infile:
        lines = [x.strip() for x in infile.readlines() if "\tcc" in x]
    values = "".join(lines).replace(" ","").split(",")
    print(values)
    return [v[2:] for v in values]

def updateCharacterCategory(filename):
    values = ["// Created with Python %s,  Unicode %s" % (
        platform.python_version(), unicodedata.unidata_version)]
    category = unicodedata.category(chr(0))
    startRange = 0
    for ch in range(sys.maxunicode):
        uch = chr(ch)
        if unicodedata.category(uch) != category:
            value = startRange * 32 + categories.index(category)
            values.append("%d," % value)
            category = unicodedata.category(uch)
            startRange = ch
    value = startRange * 32 + categories.index(category)
    values.append("%d," % value)

    Regenerate(filename, "//", values)

categories = findCategories("../lexlib/CharacterCategory.h")

updateCharacterCategory("../lexlib/CharacterCategory.cxx")
