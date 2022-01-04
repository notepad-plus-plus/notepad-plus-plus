#!/usr/bin/env python3
# DepGen.py - produce a make dependencies file for Scintilla
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# Requires Python 3.6 or later

import os, sys

scintilla = os.path.join("..", "..", "scintilla")
sys.path.append(scintilla)

from scripts import Dependencies

topComment = "# Created by DepGen.py. To recreate, run DepGen.py.\n"

def Generate():
	lexilla = os.path.join("..")
	sources = [
		os.path.join(lexilla, "src", "Lexilla.cxx"),
		os.path.join(lexilla, "lexlib", "*.cxx"),
		os.path.join(lexilla, "lexers", "*.cxx")]
	includes = [
		os.path.join(lexilla, "include"),
		os.path.join(scintilla, "include"),
		os.path.join(lexilla, "lexlib")]

	# Create the dependencies file for g++
	deps = Dependencies.FindDependencies(sources,  includes, ".o", "../lexilla/")

	# Place the objects in $(DIR_O)
	deps = [["$(DIR_O)/"+obj, headers] for obj, headers in deps]

	Dependencies.UpdateDependencies(os.path.join(lexilla, "src", "deps.mak"), deps, topComment)

	# Create the dependencies file for MSVC

	# Place the objects in $(DIR_O) and change extension from ".o" to ".obj"
	deps = [["$(DIR_O)/"+Dependencies.PathStem(obj)+".obj", headers] for obj, headers in deps]

	Dependencies.UpdateDependencies(os.path.join(lexilla, "src", "nmdeps.mak"), deps, topComment)

if __name__ == "__main__":
	Generate()