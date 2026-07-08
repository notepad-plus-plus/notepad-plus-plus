# This is a temporary aid for migrating to allowing a 'pixels' type in Scintilla.iface.
# API wrappers that take pixels arguments or return values may scale these values when
# they are running in a scaled environment.
# Scintilla.iface has been augmented with comments that define the desired future API.
# This script, writes out two versions of Scintilla.iface:
#   filtered.iface does not have any mention of 'pixels' type so can be used by old scripts
#   pixels.iface uses the 'pixels' type and removes commented 'pixels' definition lines
# Written by Neil Hodgson June 2026 and published as public domain

import pathlib, re

def ExtractName(line):
	before, _, _ = line.partition("=")
	_, _, after = before.rpartition(" ")
	return after

directory = "../include/"
filtered = ""
pixelDefinitions = {}
iface = pathlib.Path(directory, "Scintilla.iface").read_text()
for line in iface.splitlines(keepends=True):
	if re.match("^##.*pixels", line):
		# print(line, end="")
		if "=" in line:
			name = ExtractName(line)
			pixelDefinitions[name] = line[3:]
	else:
		filtered += line
		
pathlib.Path(directory, "filtered.iface").write_text(filtered)

pixelText = ""
for line in iface.splitlines(keepends=True):
	if "=" in line and "##" not in line:
		name = ExtractName(line)
		if name in pixelDefinitions:
			pixelText += pixelDefinitions[name]
		else:
			pixelText += line
	elif "=" not in line or not re.match("^##.*pixels", line):
			pixelText += line

pathlib.Path(directory, "pixels.iface").write_text(pixelText)
