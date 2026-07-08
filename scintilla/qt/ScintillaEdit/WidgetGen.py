#!/usr/bin/env python3
# WidgetGen.py - regenerate the ScintillaEdit.cpp and ScintillaEdit.h files
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
	"colouralpha": "int",
	"keymod": "int",
	"pixels": "int",
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
	"colour", "colouralpha", "keymod", "pixels", "string", "stringresult", "cells"]

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

def isPixel(t):
	return t == "pixels"

def hasPixels(v):
	return isPixel(v["ReturnType"]) or isPixel(v["Param1Type"]) or isPixel(v["Param2Type"])

def floatSuffix(options):
	return "F" if options["qtStyle"] else "_f"

def argCppType(t, floatVariant):
	# The float (_f) twin takes logical pixels as a double; the int variant
	# takes the plain integer alias.
	if floatVariant and isPixel(t):
		return "double"
	a = cppAlias(t)
	if a == "int":
		a = "sptr_t"
	return a

def returnCppType(v, stringResult, floatVariant):
	if stringResult:
		return "QByteArray"
	if floatVariant and isPixel(v["ReturnType"]):
		return "double"
	r = cppAlias(v["ReturnType"])
	if r == "int":
		r = "sptr_t"
	return r

def arguments(v, stringResult, options, floatVariant):
	ret = ""
	p1Type = argCppType(v["Param1Type"], floatVariant)
	if p1Type:
		ret = ret + p1Type + " " + normalisedName(v["Param1Name"], options)
	p2Type = argCppType(v["Param2Type"], floatVariant)
	if p2Type and not stringResult:
		if p1Type:
			ret = ret + ", "
		ret = ret + p2Type + " " + normalisedName(v["Param2Name"], options)
	return ret

def sendArg(ptype, pname, options):
	# Expression passed to send() for one parameter slot. A pixels slot takes
	# logical pixels from the caller; the engine expects device pixels.
	if not cppAlias(ptype):
		return "0"
	name = normalisedName(pname, options) if pname else "0"
	if isPixel(ptype):
		return "qRound64(" + name + " * sciScale)"
	if "*" in cppAlias(ptype):
		return "(sptr_t)" + name
	return name

def declaration(name, v, feat, options, floatVariant):
	constDeclarator = " const" if feat == "get" else ""
	stringResult = v["Param2Type"] == "stringresult"
	returnType = returnCppType(v, stringResult, floatVariant)
	suffix = floatSuffix(options) if floatVariant else ""
	return ("\t" + returnType + " " + normalisedName(name, options, feat) + suffix + "(" +
		arguments(v, stringResult, options, floatVariant) +
		")" + constDeclarator + ";")

def definition(name, v, feat, options, floatVariant):
	out = []
	constDeclarator = " const" if feat == "get" else ""
	featureDefineName = "SCI_" + name.upper()
	stringResult = v["Param2Type"] == "stringresult"
	returnType = returnCppType(v, stringResult, floatVariant)
	suffix = floatSuffix(options) if floatVariant else ""
	out.append(returnType + " ScintillaEdit::" + normalisedName(name, options, feat) + suffix + "(" +
		arguments(v, stringResult, options, floatVariant) +
		")" + constDeclarator + " {")
	if hasPixels(v):
		# The "ScintillaScale" viewport property (set by ScintillaQt::SetScaleProperty)
		# is the device-pixel ratio under SCALE_TECHNIQUE_PIXEL_ALIGNED and 0.0
		# otherwise; treat 0.0 as 1.0 so the conversion is a no-op when unscaled.
		out.append("    const double sciScaleProp = viewport()->property(\"ScintillaScale\").toDouble();")
		out.append("    const double sciScale = sciScaleProp ? sciScaleProp : 1.0;")
	returnStatement = "return " if returnType != "void" else ""
	if stringResult:
		out.append("    " + returnStatement + "TextReturner(" + featureDefineName + ", " +
			sendArg(v["Param1Type"], v["Param1Name"], options) + ");")
	else:
		call = ("send(" + featureDefineName + ", " +
			sendArg(v["Param1Type"], v["Param1Name"], options) + ", " +
			sendArg(v["Param2Type"], v["Param2Name"], options) + ")")
		if isPixel(v["ReturnType"]):
			# Engine returns device pixels; hand back logical pixels.
			if floatVariant:
				out.append("    " + returnStatement + call + " / sciScale;")
			else:
				out.append("    " + returnStatement + "qRound64(" + call + " / sciScale);")
		else:
			out.append("    " + returnStatement + call + ";")
	out.append("}")
	out.append("")
	return out

def printHFile(f, options):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			feat = v["FeatureType"]
			if feat in ["fun", "get", "set"]:
				if checkTypes(name, v):
					out.append(declaration(name, v, feat, options, False))
					if hasPixels(v):
						out.append(declaration(name, v, feat, options, True))
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
					out += definition(name, v, feat, options, False)
					if hasPixels(v):
						out += definition(name, v, feat, options, True)
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
	print("Generate full APIs for ScintillaEdit class.")
	print("")
	print("options:")
	print("")
	print("-c --clean remove all generated code from files")
	print("-h --help  display this text")
	print("-u --underscore-names  use method_names consistent with GTK+ standards")

def readInterface(cleanGenerated):
	f = Face.Face()
	if not cleanGenerated:
		# pickUpPixels lets Face apply the '## ... pixels ...' annotations so the
		# pixel slots come back typed as "pixels"; the int + _f twins follow.
		f.ReadFromFile("../../include/Scintilla.iface", pickUpPixels=True)
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
		for file in ["ScintillaEdit.cpp", "ScintillaEdit.h"]:
			try:
				os.remove(file)
			except OSError:
				pass

if __name__ == "__main__":
	main(sys.argv[1:])
