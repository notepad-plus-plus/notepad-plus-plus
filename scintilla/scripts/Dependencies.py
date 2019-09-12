#!/usr/bin/env python
# Dependencies.py - discover, read, and write dependencies file for make.
# The format like the output from "g++ -MM" which produces a
# list of header (.h) files used by source files (.cxx).
# As a module, provides
#	FindPathToHeader(header, includePath) -> path
#	FindHeadersInFile(filePath) -> [headers]
#	FindHeadersInFileRecursive(filePath, includePath, renames) -> [paths]
#	FindDependencies(sourceGlobs, includePath, objExt, startDirectory, renames) -> [dependencies]
#	ExtractDependencies(input) -> [dependencies]
#	TextFromDependencies(dependencies)
#	WriteDependencies(output, dependencies)
#	UpdateDependencies(filepath, dependencies)
#	PathStem(p) -> stem
#	InsertSynonym(dependencies, current, additional) -> [dependencies]
# If run as a script reads from stdin and writes to stdout.
# Only tested with ASCII file names.
# Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
# The License.txt file describes the conditions under which this software may be distributed.
# Requires Python 2.7 or later

import codecs, glob, os, sys

if __name__ == "__main__":
	import FileGenerator
else:
	from . import FileGenerator

continuationLineEnd = " \\"

def FindPathToHeader(header, includePath):
	for incDir in includePath:
		relPath = os.path.join(incDir, header)
		if os.path.exists(relPath):
			return relPath
	return ""

fhifCache = {}	# Remember the includes in each file. ~5x speed up.
def FindHeadersInFile(filePath):
	if filePath not in fhifCache:
		headers = []
		with codecs.open(filePath, "r", "utf-8") as f:
			for line in f:
				if line.strip().startswith("#include"):
					parts = line.split()
					if len(parts) > 1:
						header = parts[1]
						if header[0] != '<':	# No system headers
							headers.append(header.strip('"'))
		fhifCache[filePath] = headers
	return fhifCache[filePath]

def FindHeadersInFileRecursive(filePath, includePath, renames):
	headerPaths = []
	for header in FindHeadersInFile(filePath):
		if header in renames:
			header = renames[header]
		relPath = FindPathToHeader(header, includePath)
		if relPath and relPath not in headerPaths:
				headerPaths.append(relPath)
				subHeaders = FindHeadersInFileRecursive(relPath, includePath, renames)
				headerPaths.extend(sh for sh in subHeaders if sh not in headerPaths)
	return headerPaths

def RemoveStart(relPath, start):
	if relPath.startswith(start):
		return relPath[len(start):]
	return relPath

def ciKey(f):
	return f.lower()

def FindDependencies(sourceGlobs, includePath, objExt, startDirectory, renames={}):
	deps = []
	for sourceGlob in sourceGlobs:
		sourceFiles = glob.glob(sourceGlob)
		# Sorting the files minimizes deltas as order returned by OS may be arbitrary
		sourceFiles.sort(key=ciKey)
		for sourceName in sourceFiles:
			objName = os.path.splitext(os.path.basename(sourceName))[0]+objExt
			headerPaths = FindHeadersInFileRecursive(sourceName, includePath, renames)
			depsForSource = [sourceName] + headerPaths
			depsToAppend = [RemoveStart(fn.replace("\\", "/"), startDirectory) for
				fn in depsForSource]
			deps.append([objName, depsToAppend])
	return deps

def PathStem(p):
	""" Return the stem of a filename: "CallTip.o" -> "CallTip" """
	return os.path.splitext(os.path.basename(p))[0]

def InsertSynonym(dependencies, current, additional):
	""" Insert a copy of one object file with dependencies under a different name.
	Used when one source file is used to create two object files with different
	preprocessor definitions. """
	result = []
	for dep in dependencies:
		result.append(dep)
		if (dep[0] == current):
			depAdd = [additional, dep[1]]
			result.append(depAdd)
	return result

def ExtractDependencies(input):
	""" Create a list of dependencies from input list of lines
	Each element contains the name of the object and a list of
	files that it depends on.
	Dependencies that contain "/usr/" are removed as they are system headers. """

	deps = []
	for line in input:
		headersLine = line.startswith(" ") or line.startswith("\t")
		line = line.strip()
		isContinued = line.endswith("\\")
		line = line.rstrip("\\ ")
		fileNames = line.strip().split(" ")
		if not headersLine:
			# its a source file line, there may be headers too
			sourceLine = fileNames[0].rstrip(":")
			fileNames = fileNames[1:]
			deps.append([sourceLine, []])
		deps[-1][1].extend(header for header in fileNames if "/usr/" not in header)
	return deps

def TextFromDependencies(dependencies):
	""" Convert a list of dependencies to text. """
	text = ""
	indentHeaders = "\t"
	joinHeaders = continuationLineEnd + os.linesep + indentHeaders
	for dep in dependencies:
		object, headers = dep
		text += object + ":"
		for header in headers:
			text += joinHeaders
			text += header
		if headers:
			text += os.linesep
	return text

def UpdateDependencies(filepath, dependencies, comment=""):
	""" Write a dependencies file if different from dependencies. """
	FileGenerator.UpdateFile(os.path.abspath(filepath), comment.rstrip() + os.linesep +
		TextFromDependencies(dependencies))

def WriteDependencies(output, dependencies):
	""" Write a list of dependencies out to a stream. """
	output.write(TextFromDependencies(dependencies))

if __name__ == "__main__":
	""" Act as a filter that reformats input dependencies to one per line. """
	inputLines = sys.stdin.readlines()
	deps = ExtractDependencies(inputLines)
	WriteDependencies(sys.stdout, deps)
