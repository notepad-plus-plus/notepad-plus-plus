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

	deps = Dependencies.FindDependencies(["../gtk/*.cxx"] + sources, ["../gtk"] + includes, ".o", "../gtk/")
	Dependencies.UpdateDependencies("../gtk/deps.mak", deps, topComment)

if __name__ == "__main__":
	Generate()