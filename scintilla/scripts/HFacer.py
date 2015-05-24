#!/usr/bin/env python
# HFacer.py - regenerate the Scintilla.h and SciLexer.h files from the Scintilla.iface interface
# definition file.
# Implemented 2000 by Neil Hodgson neilh@scintilla.org
# Requires Python 2.5 or later

import sys
import os
import Face

from FileGenerator import UpdateFile, Generate, Regenerate, UpdateLineInFile, lineEnd

def printLexHFile(f):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["FeatureType"] in ["val"]:
			if "SCE_" in name or "SCLEX_" in name:
				out.append("#define " + name + " " + v["Value"])
	return out

def printHFile(f):
	out = []
	previousCategory = ""
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["Category"] == "Provisional" and previousCategory != "Provisional":
				out.append("#ifndef SCI_DISABLE_PROVISIONAL")
			previousCategory = v["Category"]
			if v["FeatureType"] in ["fun", "get", "set"]:
				featureDefineName = "SCI_" + name.upper()
				out.append("#define " + featureDefineName + " " + v["Value"])
			elif v["FeatureType"] in ["evt"]:
				featureDefineName = "SCN_" + name.upper()
				out.append("#define " + featureDefineName + " " + v["Value"])
			elif v["FeatureType"] in ["val"]:
				if not ("SCE_" in name or "SCLEX_" in name):
					out.append("#define " + name + " " + v["Value"])
	out.append("#endif")
	return out

f = Face.Face()
try:
	f.ReadFromFile("../include/Scintilla.iface")
	Regenerate("../include/Scintilla.h", "/* ", printHFile(f))
	Regenerate("../include/SciLexer.h", "/* ", printLexHFile(f))
	print("Maximum ID is %s" % max([x for x in f.values if int(x) < 3000]))
except:
	raise
