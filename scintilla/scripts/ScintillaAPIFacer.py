#!/usr/bin/env python3
# ScintillaAPIFacer.py - regenerate the ScintillaTypes.h, and ScintillaMessages.h
# from the Scintilla.iface interface definition file.
# Implemented 2019 by Neil Hodgson neilh@scintilla.org
# Requires Python 3.6 or later

import pathlib

import Face
import FileGenerator
import HFacer

namespace = "Scintilla::"

typeAliases = {
	# Convert iface types to C++ types
	# bool and void are OK as is
	"cells": "const char *",
	"colour": "Colour",
	"colouralpha": "ColourAlpha",
	"findtext": "void *",
	"findtextfull": "TextToFindFull *",
	"formatrange": "void *",
	"formatrangefull": "RangeToFormatFull *",
	"int": "int",
	"keymod": "int",
	"line": "Line",
	"pointer": "void *",
	"position": "Position",
	"string": "const char *",
	"stringresult": "char *",
	"textrange": "void *",
	"textrangefull": "TextRangeFull *",
}

basicTypes = [
	"bool",
	"char *",
	"Colour",
	"ColourAlpha",
	"const char *",
	"int",
	"intptr_t",
	"Line",
	"Position",
	"void",
	"void *",
]

deadValues = [
	"INDIC_CONTAINER",
	"INDIC_IME",
	"INDIC_IME_MAX",
	"INDIC_MAX",
]

def ActualTypeName(type, identifier=None):
	if type == "pointer" and identifier in ["doc", "DocPointer", "CreateDocument"]:
		return "IDocumentEditable *"
	elif type in typeAliases:
		return typeAliases[type]
	else:
		return type

def IsEnumeration(s):
	if s in ["Position", "Line", "Colour", "ColourAlpha"]:
		return False
	if s.endswith("*"):
		return False
	return s[:1].isupper()

def JoinTypeAndIdentifier(type, identifier):
	# Add a space to separate type from identifier unless type is pointer
	if type.endswith("*"):
		return type + identifier
	else:
		return type + " " + identifier

def ParametersArgsCallname(v):
	parameters = ""
	args = ""
	callName = "Call"

	param1TypeBase = v["Param1Type"]
	param1Name = v["Param1Name"]
	param1Type = ActualTypeName(param1TypeBase, param1Name)
	param1Arg = ""
	if param1Type:
		castName = param1Name
		if param1Type.endswith("*"):
			castName = "reinterpret_cast<uintptr_t>(" + param1Name + ")"
		elif param1Type not in basicTypes:
			castName = "static_cast<uintptr_t>(" + param1Name + ")"
		if IsEnumeration(param1TypeBase):
			param1Type = namespace + param1Type
		param1Arg = JoinTypeAndIdentifier(param1Type, param1Name)
		parameters = param1Arg
		args = castName

	param2TypeBase = v["Param2Type"]
	param2Name = v["Param2Name"]
	param2Type = ActualTypeName(param2TypeBase, param2Name)
	param2Arg = ""
	if param2Type:
		castName = param2Name
		if param2Type.endswith("*"):
			if param2Type == "const char *":
				callName = "CallString"
			else:
				callName = "CallPointer"
		elif param2Type not in basicTypes:
			castName = "static_cast<intptr_t>(" + param2Name + ")"
		if IsEnumeration(param2TypeBase):
			param2Type = namespace + param2Type
		param2Arg = JoinTypeAndIdentifier(param2Type, param2Name)
		if param1Arg:
			parameters = parameters + ", "
		parameters = parameters + param2Arg
		if not args:
			args = args + "0"
		if args:
			args = args + ", "
		args = args + castName

	if args:
		args = ", " + args
	return (parameters, args, callName)

def ParametersExceptLast(parameters):
	if "," in parameters:
		return parameters[:parameters.rfind(",")]
	else:
		return ""

def HMessages(f):
	out = ["enum class Message {"]
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "get", "set"]:
				out.append("\t" + name + " = " + v["Value"] + ",")
	out.append("};")
	return out

def HEnumerations(f):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			# Only want non-deprecated enumerations and lexers are not part of Scintilla API
			if v["FeatureType"] in ["enu"] and name != "Lexer":
				out.append("")
				prefixes = v["Value"].split()
				#out.append("enum class " + name + " {" + " // " + ",".join(prefixes))
				out.append("enum class " + name + " {")
				for valueName in f.order:
					prefixMatched = ""
					for p in prefixes:
						if valueName.startswith(p) and valueName not in deadValues:
							prefixMatched = p
					if prefixMatched:
						vEnum = f.features[valueName]
						valueNameNoPrefix = ""
						if valueName in f.aliases:
							valueNameNoPrefix = f.aliases[valueName]
						else:
							valueNameNoPrefix = valueName[len(prefixMatched):]
							if not valueNameNoPrefix:	# Removed whole name
								valueNameNoPrefix = valueName
							if valueNameNoPrefix.startswith("SC_"):
								valueNameNoPrefix = valueNameNoPrefix[len("SC_"):]
						pascalName = Face.PascalCase(valueNameNoPrefix)
						out.append("\t" + pascalName + " = " + vEnum["Value"] + ",")
				out.append("};")

	out.append("")
	out.append("enum class Notification {")
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["evt"]:
				out.append("\t" + name + " = " + v["Value"] + ",")
	out.append("};")

	return out

def HConstants(f):
	# Constants not in an eumeration
	out = []
	allEnumPrefixes = [
		"SCE_", # Lexical styles
		"SCI_", # Message number allocation
		"SCEN_", # Notifications sent with WM_COMMAND
	]
	for _n, v in f.features.items():
		if v["Category"] != "Deprecated":
			# Only want non-deprecated enumerations and lexers are not part of Scintilla API
			if v["FeatureType"] in ["enu"]:
				allEnumPrefixes.extend(v["Value"].split())
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			# Only want non-deprecated enumerations and lexers are not part of Scintilla API
			if v["FeatureType"] in ["val"]:
				hasPrefix = False
				for prefix in allEnumPrefixes:
					if name.startswith(prefix):
						hasPrefix = True
				if not hasPrefix:
					if name.startswith("SC_"):
						name = name[3:]
					type = "int"
					if name == "INVALID_POSITION":
						type = "Position"
					out.append("constexpr " + type + " " + Face.PascalCase(name) + " = " + v["Value"] + ";")
	return out

def HMethods(f):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "get", "set"]:
				if v["FeatureType"] == "get" and name.startswith("Get"):
					name = name[len("Get"):]
				retType = ActualTypeName(v["ReturnType"], name)
				if IsEnumeration(retType):
					retType = namespace + retType
				parameters, args, callName = ParametersArgsCallname(v)

				out.append("\t" + JoinTypeAndIdentifier(retType, name) + "(" + parameters + ");")

				# Extra method for stringresult that returns std::string
				if v["Param2Type"] == "stringresult":
					out.append("\t" + JoinTypeAndIdentifier("std::string", name) + \
						"(" + ParametersExceptLast(parameters) + ");")
	return out

def CXXMethods(f):
	out = []
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["FeatureType"] in ["fun", "get", "set"]:
				msgName = "Message::" + name
				if v["FeatureType"] == "get" and name.startswith("Get"):
					name = name[len("Get"):]
				retType = ActualTypeName(v["ReturnType"], name)
				parameters, args, callName = ParametersArgsCallname(v)
				returnIfNeeded = "return " if retType != "void" else ""

				out.append(JoinTypeAndIdentifier(retType, "ScintillaCall::" + name) + "(" + parameters + ")" + " {")
				retCast = ""
				retCastEnd = ""
				if retType.endswith("*"):
					retCast = "reinterpret_cast<" + retType + ">("
					retCastEnd = ")"
				elif retType not in basicTypes or retType in ["int", "Colour", "ColourAlpha"]:
					if IsEnumeration(retType):
						retType = namespace + retType
					retCast = "static_cast<" + retType + ">("
					retCastEnd = ")"
				out.append("\t" + returnIfNeeded + retCast + callName + "(" + msgName + args + ")" + retCastEnd + ";")
				out.append("}")
				out.append("")

				# Extra method for stringresult that returns std::string
				if v["Param2Type"] == "stringresult":
					paramList = ParametersExceptLast(parameters)
					argList = ParametersExceptLast(args)
					out.append(JoinTypeAndIdentifier("std::string", "ScintillaCall::" + name) + \
						"(" + paramList + ") {")
					out.append("\treturn CallReturnString(" + msgName + argList + ");")
					out.append("}")
					out.append("")

	return out

def RegenerateAll(root):
	HFacer.RegenerateAll(root, False)
	f = Face.Face()
	include = root / "include"
	f.ReadFromFile(include / "Scintilla.iface")
	FileGenerator.Regenerate(include / "ScintillaMessages.h", "//", HMessages(f))
	FileGenerator.Regenerate(include / "ScintillaTypes.h", "//", HEnumerations(f), HConstants(f))
	FileGenerator.Regenerate(include / "ScintillaCall.h", "//", HMethods(f))
	FileGenerator.Regenerate(root / "call" / "ScintillaCall.cxx", "//", CXXMethods(f))

if __name__ == "__main__":
	RegenerateAll(pathlib.Path(__file__).resolve().parent.parent)
