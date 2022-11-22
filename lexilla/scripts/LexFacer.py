#!/usr/bin/env python3
# LexFacer.py - regenerate the SciLexer.h files from the Scintilla.iface interface
# definition file.
# Implemented 2000 by Neil Hodgson neilh@scintilla.org
# Requires Python 3.6 or later

import os, pathlib, sys

sys.path.append(os.path.join("..", "..", "scintilla", "scripts"))

import Face
import FileGenerator

def printLexHFile(f):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["FeatureType"] in ["val"]:
			if "SCE_" in name or "SCLEX_" in name:
				out.append("#define " + name + " " + v["Value"])
	return out

def RegenerateAll(root, _showMaxID):
	f = Face.Face()
	f.ReadFromFile(root / "include/LexicalStyles.iface")
	FileGenerator.Regenerate(root / "include/SciLexer.h", "/* ", printLexHFile(f))

if __name__ == "__main__":
	RegenerateAll(pathlib.Path(__file__).resolve().parent.parent, True)
