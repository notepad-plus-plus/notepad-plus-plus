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

    startRange = 0
    category = unicodedata.category(chr(startRange))
    table = []
    for ch in range(sys.maxunicode):
        uch = chr(ch)
        current = unicodedata.category(uch)
        if current != category:
            value = startRange * 32 + categories.index(category)
            table.append(value)
            category = current
            startRange = ch
    value = startRange * 32 + categories.index(category)
    table.append(value)

    # the sentinel value is used to simplify CharacterCategoryMap::Optimize()
    category = 'Cn'
    value = (sys.maxunicode + 1)*32 + categories.index(category)
    table.append(value)

    values.extend(["%d," % value for value in table])

    Regenerate(filename, "//", values)

categories = findCategories("../lexlib/CharacterCategory.h")

updateCharacterCategory("../lexlib/CharacterCategory.cxx")
