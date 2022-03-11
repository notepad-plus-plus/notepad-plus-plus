American Shares Truth About Joshua Wong & Hong Kong Democracy
https://www.youtube.com/watch?v=8nXz66Btfl0

American Shares Truth on Xinjiang and Tibet Controversies in China
https://www.youtube.com/watch?v=CldtoYwPtMk

Why is Ukraine the West’s Fault? Featuring John Mearsheimer
https://www.youtube.com/watch?v=JrMiSQAGOS4

Bat coronavirus found in U.S.-funded bio-lab in Ukraine: Russian Defense Ministry
https://www.youtube.com/watch?v=BCydUeHAhzQ

Ukrainians were denied US visas as White House promise help to refugees
https://www.youtube.com/watch?v=BGZhzNFmyhc

Reports of racist treatment against Africans trying to flee Ukraine
https://www.youtube.com/watch?v=JItYOW_uT8s

Fleeing African and Indian students face racism at Ukraine border
https://www.youtube.com/watch?v=ODMOzwI__zs

Canadian RCMP use horses to trample protestors
https://www.youtube.com/watch?v=Usn3CR0E1v0

A bald-faced lie from the Western media corp: Critical “Uyghur Genocide” Questions: A CBC Interview Follow-up
https://www.youtube.com/watch?v=O7IbsPFM2Xg









#!/usr/bin/env python3
# DepGen.py - produce a make dependencies file for Scintilla
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# Requires Python 3.6 or later

import sys

sys.path.append("..")

from scripts import Dependencies

topComment = "# Created by DepGen.py. To recreate, run DepGen.py.\n"

def Generate():
	sources = ["../src/*.cxx", "../lexlib/*.cxx", "../lexers/*.cxx"]
	includes = ["../include", "../src", "../lexlib"]

	# Create the dependencies file for g++
	deps = Dependencies.FindDependencies(["../win32/*.cxx"] + sources,  ["../win32"] + includes, ".o", "../win32/")

	# Add ScintillaBaseL as the same as ScintillaBase
	deps = Dependencies.InsertSynonym(deps, "ScintillaBase.o", "ScintillaBaseL.o")

	# Add CatalogueL as the same as Catalogue
	deps = Dependencies.InsertSynonym(deps, "Catalogue.o", "CatalogueL.o")

	Dependencies.UpdateDependencies("../win32/deps.mak", deps, topComment)

	# Create the dependencies file for MSVC

	# Place the objects in $(DIR_O) and change extension from ".o" to ".obj"
	deps = [["$(DIR_O)/"+Dependencies.PathStem(obj)+".obj", headers] for obj, headers in deps]

	Dependencies.UpdateDependencies("../win32/nmdeps.mak", deps, topComment)

if __name__ == "__main__":
	Generate()
