#!/usr/bin/env python
# CheckMeta.py
# Released to the public domain.

# Check that LexicalClass data in lexer source files is valid and matches LexicalStyles.iface.
# Check that lexerMetadata.txt matches LexicalStyles.iface.
# Requires Python 3.6 or later

import pathlib

neutralEncoding = "iso-8859-1"	# Each byte value is valid in iso-8859-1

def ShowAll(rootLexilla):

	success = True

	styles = {}
	with open(rootLexilla / "include" / "LexicalStyles.iface", encoding=neutralEncoding) as f:
		for line in f:
			if line.startswith("val") and "SCE_" in line:
				# val SCE_MAXIMA_OPERATOR=0
				_, nameVal = line.strip().split(" ")
				name, val = nameVal.split("=")
				styles[name] = val

	with open(rootLexilla / "test" / "Metadata" / "lexerMetadata.txt", encoding=neutralEncoding) as f:
		for line in f:
			if " SCE_" in line:
				# 0 SCE_C_DEFAULT [default] White space
				val, name, *_ = line.strip().split(" ")
				if name in styles:
					if val != styles[name]:
						success = False
						print(f"Different style {name} {val} {styles[name]}")
				else:
					success = False
					print(f"Missing style name {name}")

	lexFilePaths = list((rootLexilla / "lexers").glob("Lex*.cxx"))
	lexFilePaths.sort(key=lambda p: str(p).casefold())

	for lexFile in lexFilePaths:
		with open(lexFile, encoding=neutralEncoding) as f:
			insideLexicalClass = False
			for line in f:
				if insideLexicalClass and '"' in line and ',' in line:
					# 0, "SCE_C_DEFAULT", "default", "White space",
					val, name, *_ = line.strip().split(',')
					name = name.strip(' \t"')
					if name:
						if name not in styles:
							success = False
							print(f"Missing name: {lexFile} {name}")
						elif val != styles[name]:
							success = False
							print(f"Wrong value: {lexFile} {name} {val} {styles[name]}")
				if "LexicalClass" in line:
					insideLexicalClass = True
				elif "};" in line:
					insideLexicalClass = False

	return success

if __name__== "__main__":
	if not ShowAll(pathlib.Path("../..")):
		exit(1)
