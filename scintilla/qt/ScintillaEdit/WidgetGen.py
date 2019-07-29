#!/usr/bin/env python
# WidgetGen.py - regenerate the ScintillaWidgetCpp.cpp and ScintillaWidgetCpp.h files
# Check that API includes all gtkscintilla2 functions

import sys
import os
import getopt

scintillaDirectory = "../.."
scintillaScriptsDirectory = os.path.join(scintillaDirectory, "scripts")
sys.path.append(scintillaScriptsDirectory)
import Face
from FileGenerator import GenerateFile

def underscoreName(s):
	# Name conversion fixes to match gtkscintilla2
	irregular = ['WS', 'EOL', 'AutoC', 'KeyWords', 'BackSpace', 'UnIndents', 'RE', 'RGBA']
	for word in irregular:
		replacement = word[0] + word[1:].lower()
		s = s.replace(word, replacement)

	out = ""
	for c in s:
		if c.isupper():
			if out:
				out += "_"
			out += c.lower()
		else:
			out += c
	return out

def normalisedName(s, options, role=None):
	if options["qtStyle"]:
		if role == "get":
			s = s.replace("Get", "")
		return s[0].lower() + s[1:]
	else:
		return underscoreName(s)

typeAliases = {
	"position": "int",
	"line": "int",
	"pointer": "int",
	"colour": "int",
	"keymod": "int",
	"string": "const char *",
	"stringresult": "const char *",
	"cells": "const char *",
}

def cppAlias(s):
	if s in typeAliases:
		return typeAliases[s]
	elif Face.IsEnumeration(s):
		return "int"
	else:
		return s

understoodTypes = ["", "void", "int", "bool", "position", "line", "pointer",
	"colour", "keymod", "string", "stringresult", "cells"]

def understoodType(t):
	return t in understoodTypes or Face.IsEnumeration(t)

def checkTypes(name, v):
	understandAllTypes = True
	if not understoodType(v["ReturnType"]):
		#~ print("Do not understand", v["ReturnType"], "for", name)
		understandAllTypes = False
	if not understoodType(v["Param1Type"]):
		#~ print("Do not understand", v["Param1Type"], "for", name)
		understandAllTypes = False
	if not understoodType(v["Param2Type"]):
		#~ print("Do not understand", v["Param2Type"], "for", name)
		understandAllTypes = False
	return understandAllTypes

def arguments(v, stringResult, options):
	ret = ""
	p1Type = cppAlias(v["Param1Type"])
	if p1Type == "int":
		p1Type = "sptr_t"
	if p1Type:
		ret = ret + p1Type + " " + normalisedName(v["Param1Name"], options)
	p2Type = cppAlias(v["Param2Type"])
	if p2Type == "int":
		p2Type = "sptr_t"
	if p2Type and not stringResult:
		if p1Type:
			ret = ret + ", "
		ret = ret + p2Type + " " + normalisedName(v["Param2Name"], options)
	return ret

def printPyFile(f, options):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			feat = v["FeatureType"]
			if feat in ["val"]:
				out.append(name + "=" + v["Value"])
			if feat in ["evt"]:
				out.append("SCN_" + name.upper() + "=" + v["Value"])
			if feat in ["fun"]:
				out.append("SCI_" + name.upper() + "=" + v["Value"])
	return out

def printHFile(f, options):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			feat = v["FeatureType"]
			if feat in ["fun", "get", "set"]:
				if checkTypes(name, v):
					constDeclarator = " const" if feat == "get" else ""
					returnType = cppAlias(v["ReturnType"])
					if returnType == "int":
						returnType = "sptr_t"
					stringResult = v["Param2Type"] == "stringresult"
					if stringResult:
						returnType = "QByteArray"
					out.append("\t" + returnType + " " + normalisedName(name, options, feat) + "(" +
						arguments(v, stringResult, options)+
						")" + constDeclarator + ";")
	return out

def methodNames(f, options):
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			feat = v["FeatureType"]
			if feat in ["fun", "get", "set"]:
				if checkTypes(name, v):
					yield normalisedName(name, options)

def printCPPFile(f, options):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			feat = v["FeatureType"]
			if feat in ["fun", "get", "set"]:
				if checkTypes(name, v):
					constDeclarator = " const" if feat == "get" else ""
					featureDefineName = "SCI_" + name.upper()
					returnType = cppAlias(v["ReturnType"])
					if returnType == "int":
						returnType = "sptr_t"
					stringResult = v["Param2Type"] == "stringresult"
					if stringResult:
						returnType = "QByteArray"
					returnStatement = ""
					if returnType != "void":
						returnStatement = "return "
					out.append(returnType + " ScintillaEdit::" + normalisedName(name, options, feat) + "(" +
						arguments(v, stringResult, options) +
						")" + constDeclarator + " {")
					returns = ""
					if stringResult:
						returns += "    " + returnStatement + "TextReturner(" + featureDefineName + ", "
						if "*" in cppAlias(v["Param1Type"]):
							returns += "(sptr_t)"
						if v["Param1Name"]:
							returns += normalisedName(v["Param1Name"], options)
						else:
							returns += "0"
						returns += ");"
					else:
						returns += "    " + returnStatement + "send(" + featureDefineName + ", "
						if "*" in cppAlias(v["Param1Type"]):
							returns += "(sptr_t)"
						if v["Param1Name"]:
							returns += normalisedName(v["Param1Name"], options)
						else:
							returns += "0"
						returns += ", "
						if "*" in cppAlias(v["Param2Type"]):
							returns += "(sptr_t)"
						if v["Param2Name"]:
							returns += normalisedName(v["Param2Name"], options)
						else:
							returns += "0"
						returns += ");"
					out.append(returns)
					out.append("}")
					out.append("")
	return out

def gtkNames():
	# The full path on my machine: should be altered for anyone else
	p = "C:/Users/Neil/Downloads/wingide-source-4.0.1-1/wingide-source-4.0.1-1/external/gtkscintilla2/gtkscintilla.c"
	with open(p) as f:
		for l in f.readlines():
			if "gtk_scintilla_" in l:
				name = l.split()[1][14:]
				if '(' in name:
					name = name.split('(')[0]
					yield name

def usage():
	print("WidgetGen.py [-c|--clean][-h|--help][-u|--underscore-names]")
	print("")
	print("Generate full APIs for ScintillaEdit class and ScintillaConstants.py.")
	print("")
	print("options:")
	print("")
	print("-c --clean remove all generated code from files")
	print("-h --help  display this text")
	print("-u --underscore-names  use method_names consistent with GTK+ standards")

def readInterface(cleanGenerated):
	f = Face.Face()
	if not cleanGenerated:
		f.ReadFromFile("../../include/Scintilla.iface")
	return f

def main(argv):
	# Using local path for gtkscintilla2 so don't default to checking
	checkGTK = False
	cleanGenerated = False
	qtStyleInterface = True
	# The --gtk-check option checks for full coverage of the gtkscintilla2 API but
	# depends on a particular directory so is not mentioned in --help.
	opts, args = getopt.getopt(argv, "hcgu", ["help", "clean", "gtk-check", "underscore-names"])
	for opt, arg in opts:
		if opt in ("-h", "--help"):
			usage()
			sys.exit()
		elif opt in ("-c", "--clean"):
			cleanGenerated = True
		elif opt in ("-g", "--gtk-check"):
			checkGTK = True
		elif opt in ("-u", "--underscore-names"):
			qtStyleInterface = False

	options = {"qtStyle": qtStyleInterface}
	f = readInterface(cleanGenerated)
	try:
		GenerateFile("ScintillaEdit.cpp.template", "ScintillaEdit.cpp",
			"/* ", True, printCPPFile(f, options))
		GenerateFile("ScintillaEdit.h.template", "ScintillaEdit.h",
			"/* ", True, printHFile(f, options))
		GenerateFile("../ScintillaEditPy/ScintillaConstants.py.template",
			"../ScintillaEditPy/ScintillaConstants.py",
			"# ", True, printPyFile(f, options))
		if checkGTK:
			names = set(methodNames(f))
			#~ print("\n".join(names))
			namesGtk = set(gtkNames())
			for name in namesGtk:
				if name not in names:
					print(name, "not found in Qt version")
			for name in names:
				if name not in namesGtk:
					print(name, "not found in GTK+ version")
	except:
		raise

	if cleanGenerated:
		for file in ["ScintillaEdit.cpp", "ScintillaEdit.h", "../ScintillaEditPy/ScintillaConstants.py"]:
			try:
				os.remove(file)
			except OSError:
				pass

if __name__ == "__main__":
	main(sys.argv[1:])
