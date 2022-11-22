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
	sources = ["../src/*.cxx"]
	includes = ["../include", "../src"]

	deps = Dependencies.FindDependencies(["../gtk/*.cxx"] + sources, ["../gtk"] + includes, ".o", "../gtk/")
	Dependencies.UpdateDependencies("../gtk/deps.mak", deps, topComment)

if __name__ == "__main__":
	Generate()