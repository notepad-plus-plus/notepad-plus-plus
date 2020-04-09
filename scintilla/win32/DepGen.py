#!/usr/bin/env python
# DepGen.py - produce a make dependencies file for Scintilla
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# Requires Python 2.7 or later

import sys

sys.path.append("..")

from scripts import Dependencies

topComment = "# Created by DepGen.py. To recreate, run 'python DepGen.py'.\n"

def Generate():
	sources = ["../src/*.cxx", "../lexlib/*.cxx", "../lexers/*.cxx"]
	includes = ["../include", "../src", "../lexlib"]

	# Create the dependencies file for g++
	deps = Dependencies.FindDependencies(["../win32/*.cxx"] + sources,  ["../win32"] + includes, ".o", "../win32/")

	# Add ScintillaBaseL as the same as ScintillaBase
	deps = Dependencies.InsertSynonym(deps, "ScintillaBase.o", "ScintillaBaseL.o")

	# Add ScintillaWinL as the same as ScintillaWin
	deps = Dependencies.InsertSynonym(deps, "ScintillaWin.o", "ScintillaWinL.o")

	Dependencies.UpdateDependencies("../win32/deps.mak", deps, topComment)

	# Create the dependencies file for MSVC

	# Place the objects in $(DIR_O) and change extension from ".o" to ".obj"
	deps = [["$(DIR_O)/"+Dependencies.PathStem(obj)+".obj", headers] for obj, headers in deps]

	Dependencies.UpdateDependencies("../win32/nmdeps.mak", deps, topComment)

if __name__ == "__main__":
	Generate()