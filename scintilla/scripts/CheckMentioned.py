#!/usr/bin/env python3
# CheckMentioned.py
# Find all the symbols in scintilla/include/Scintilla.h and check if they
# are mentioned in scintilla/doc/ScintillaDoc.html.
# Requires Python 3.6 or later

import re, string, sys

srcRoot = "../.."

sys.path.append(srcRoot + "/scintilla/scripts")

import Face

uninteresting = {
	"SCINTILLA_H", "SCI_START", "SCI_LEXER_START", "SCI_OPTIONAL_START",
	# These archaic names are #defined to the Sci_ prefixed modern equivalents.
	# They are not documented so they are not used in new code.
	"CharacterRange", "TextRange", "TextToFind", "RangeToFormat", "NotifyHeader",
}

incFileName = srcRoot + "/scintilla/include/Scintilla.h"
docFileName = srcRoot + "/scintilla/doc/ScintillaDoc.html"
identCharacters = "_" + string.ascii_letters + string.digits

# Convert all punctuation characters except '_' into spaces.
def depunctuate(s):
	d = ""
	for ch in s:
		if ch in identCharacters:
			d = d + ch
		else:
			d = d + " "
	return d

symbols = {}
with open(incFileName, "rt") as incFile:
	for line in incFile.readlines():
		if line.startswith("#define"):
			identifier = line.split()[1]
			symbols[identifier] = 0

with open(docFileName, "rt") as docFile:
	for line in docFile.readlines():
		for word in depunctuate(line).split():
			if word in symbols.keys():
				symbols[word] = 1

def convertIFaceTypeToC(t):
	if t == "keymod":
		return "int "
	elif t == "string":
		return "const char *"
	elif t == "stringresult":
		return "char *"
	elif t == "cells":
		return "cell *"
	elif t == "textrange":
		return "Sci_TextRange *"
	elif t == "findtext":
		return "Sci_TextToFind *"
	elif t == "formatrange":
		return "Sci_RangeToFormat *"
	elif Face.IsEnumeration(t):
		return "int "
	return t + " "

def makeParm(t, n, v):
	return (convertIFaceTypeToC(t) + n).rstrip()

def makeSig(params):
	p1 = makeParm(params["Param1Type"], params["Param1Name"], params["Param1Value"])
	p2 = makeParm(params["Param2Type"], params["Param2Name"], params["Param2Value"])

	retType = params["ReturnType"]
	if retType in ["void", "string", "stringresult"]:
		retType = ""
	elif Face.IsEnumeration(retType):
		retType = "int"
	if retType:
		retType = " &rarr; " + retType

	if p1 == "" and p2 == "":
		return retType

	if p1 == "":
		p1 = "&lt;unused&gt;"
	joiner = ""
	if p2 != "":
		joiner = ", "
	return "(" + p1 + joiner + p2 + ")" + retType

pathIface = srcRoot + "/scintilla/include/Scintilla.iface"

def retrieveFeatures():
	face = Face.Face()
	face.ReadFromFile(pathIface)
	sciToFeature = {}
	sccToValue = { "true":"1", "false":"0", "EN_SETFOCUS":"256", "EN_KILLFOCUS":"512"}
	for name in face.order:
		v = face.features[name]
		if v["FeatureType"] in ["fun", "get", "set"]:
			featureDefineName = "SCI_" + name.upper()
			sciToFeature[featureDefineName] = name
		elif v["FeatureType"] in ["val"]:
			featureDefineName = name.upper()
			sccToValue[featureDefineName] = v["Value"]
		elif v["FeatureType"] in ["evt"]:
			featureDefineName = "SCN_" + name.upper()
			sccToValue[featureDefineName] = v["Value"]
	return (face, sciToFeature, sccToValue)

def flattenSpaces(s):
	return s.replace("\n", " ").replace("  ", " ").replace("  ", " ").replace("  ", " ").strip()

def printCtag(ident, path):
	print(ident.strip() + "\t" + path + "\t" + "/^" + ident + "$/")

showCTags = True

def checkDocumentation():
	with open(docFileName, "rt") as docFile:
		docs = docFile.read()

	face, sciToFeature, sccToValue = retrieveFeatures()

	headers = {}
	definitions = {}

	# Examine header sections which point to definitions
	#<a class="message" href="#SCI_SETLAYOUTCACHE">SCI_SETLAYOUTCACHE(int cacheMode)</a><br />
	dirPattern = re.compile(r'<a class="message" href="#([A-Z0-9_]+)">([A-Z][A-Za-z0-9_() *&;,\n]+)</a>')
	for api, sig in re.findall(dirPattern, docs):
		sigApi = re.split('\W+', sig)[0]
		sigFlat = flattenSpaces(sig)
		sigFlat = sigFlat.replace('colouralpha ', 'xxxx ')	# Temporary to avoid next line
		sigFlat = sigFlat.replace('alpha ', 'int ')
		sigFlat = sigFlat.replace('xxxx ', 'colouralpha ')

		sigFlat = sigFlat.replace("document *", "int ")
		sigFlat = sigFlat.rstrip()
		if '(' in sigFlat or api.startswith("SCI_"):
			name = sciToFeature[api]
			sigFromFace = api + makeSig(face.features[name])
			if sigFlat != sigFromFace:
				print(sigFlat, "|", sigFromFace)
				if showCTags:
					printCtag(api, docFileName)
				#~ printCtag(" " + name, pathIface)
		if api != sigApi:
			print(sigApi, ";;", sig, ";;", api)
		headers[api] = 1
	# Warns for most keyboard commands so not enabled
	#~ for api in sorted(sciToFeature.keys()):
		#~ if api not in headers:
			#~ print("No header for ", api)

	# Examine  definitions
	#<b id="SCI_SETLAYOUTCACHE">SCI_SETLAYOUTCACHE(int cacheMode)</b>
	defPattern = re.compile(r'<b id="([A-Z_0-9]+)">([A-Z][A-Za-z0-9_() *#\"=<>/&;,\n-]+?)</b>')
	for api, sig in re.findall(defPattern, docs):
		sigFlat = flattenSpaces(sig)
		if '<a' in sigFlat	:	# Remove anchors
			sigFlat = re.sub('<a.*>(.+)</a>', '\\1', sigFlat)
		sigFlat = sigFlat.replace('colouralpha ', 'xxxx ')	# Temporary to avoid next line
		sigFlat = sigFlat.replace('alpha ', 'int ')
		sigFlat = sigFlat.replace('xxxx ', 'colouralpha ')
		sigFlat = sigFlat.replace("document *", "int ")

		sigFlat = sigFlat.replace(' NUL-terminated', '')
		sigFlat = sigFlat.rstrip()
		#~ sigFlat = sigFlat.replace(' NUL-terminated', '')
		sigApi = re.split('\W+', sigFlat)[0]
		#~ print(sigFlat, ";;", sig, ";;", api)
		if '(' in sigFlat or api.startswith("SCI_"):
			try:
				name = sciToFeature[api]
				sigFromFace = api + makeSig(face.features[name])
				if sigFlat != sigFromFace:
					print(sigFlat, "|", sigFromFace)
					if showCTags:
						printCtag('="' + api, docFileName)
					#~ printCtag(" " + name, pathIface)
			except KeyError:
				pass		# Feature removed but still has documentation
		if api != sigApi:
			print(sigApi, ";;", sig, ";;", api)
		definitions[api] = 1
	# Warns for most keyboard commands so not enabled
	#~ for api in sorted(sciToFeature.keys()):
		#~ if api not in definitions:
			#~ print("No definition for ", api)

	outName = docFileName.replace("Doc", "Dox")
	with open(outName, "wt") as docFile:
		docFile.write(docs)

	# Examine  constant definitions
	#<code>SC_CARETSTICKY_WHITESPACE</code> (2)
	constPattern = re.compile(r'<code>(\w+)</code> *\((\w+)\)')
	for name, val in re.findall(constPattern, docs):
		try:
			valOfName = sccToValue[name]
			if val != valOfName:
				print(val, "<-", name, ";;", valOfName)
		except KeyError:
			print("***", val, "<-", name)

	for name in sccToValue.keys():
		if name not in ["SCI_OPTIONAL_START", "SCI_LEXER_START"] and name not in docs:
			print(f"Unknown {name}")

for identifier in sorted(symbols.keys()):
	if not symbols[identifier] and identifier not in uninteresting:
		print(identifier)

checkDocumentation()
