# Module for reading and parsing Scintilla.iface file
import string

def sanitiseLine(line):
	if line[-1:] == '\n': line = line[:-1]
	if string.find(line, "##") != -1:
		line = line[:string.find(line, "##")]
	line = string.strip(line)
	return line
	
def decodeFunction(featureVal):
	retType, rest = string.split(featureVal, " ", 1)
	nameIdent, params = string.split(rest, "(")
	name, value = string.split(nameIdent, "=")
	params, rest = string.split(params, ")")
	param1, param2 = string.split(params, ",")[0:2]
	return retType, name, value, param1, param2
	
def decodeEvent(featureVal):
	retType, rest = string.split(featureVal, " ", 1)
	nameIdent, params = string.split(rest, "(")
	name, value = string.split(nameIdent, "=")
	return retType, name, value
	
def decodeParam(p):
	param = string.strip(p)
	type = ""
	name = ""
	value = ""
	if " " in param:
		type, nv = string.split(param, " ")
		if "=" in nv:
			name, value = string.split(nv, "=")
		else:
			name = nv
	return type, name, value

class Face:

	def __init__(self):
		self.order = []
		self.features = {}
		self.values = {}
		self.events = {}
		
	def ReadFromFile(self, name):
		currentCategory = ""
		currentComment = []
		currentCommentFinished = 0
		file = open(name)
		for line in file.readlines():
			line = sanitiseLine(line)
			if line:
				if line[0] == "#":
					if line[1] == " ":
						if currentCommentFinished:
							currentComment = []
							currentCommentFinished = 0
						currentComment.append(line[2:])
				else:
					currentCommentFinished = 1
					featureType, featureVal = string.split(line, " ", 1)
					if featureType in ["fun", "get", "set"]:
						retType, name, value, param1, param2 = decodeFunction(featureVal)
						p1 = decodeParam(param1)
						p2 = decodeParam(param2)
						self.features[name] = { 
							"FeatureType": featureType, 
							"ReturnType": retType,
							"Value": value, 
							"Param1Type": p1[0], "Param1Name": p1[1], "Param1Value": p1[2], 
							"Param2Type": p2[0],	"Param2Name": p2[1], "Param2Value": p2[2],
							"Category": currentCategory, "Comment": currentComment
						}
						if self.values.has_key(value):
							raise "Duplicate value " + value + " " + name
						self.values[value] = 1
						self.order.append(name)
					elif featureType == "evt":
						retType, name, value = decodeEvent(featureVal)
						self.features[name] = { 
							"FeatureType": featureType, 
							"ReturnType": retType,
							"Value": value, 
							"Category": currentCategory, "Comment": currentComment
						}
						if self.events.has_key(value):
							raise "Duplicate event " + value + " " + name
						self.events[value] = 1
						self.order.append(name)
					elif featureType == "cat":
						currentCategory = featureVal
					elif featureType == "val":
						name, value = string.split(featureVal, "=", 1)
						self.features[name] = { 
							"FeatureType": featureType, 
							"Category": currentCategory, 
							"Value": value }
						self.order.append(name)
					elif featureType == "enu" or featureType == "lex":
						name, value = string.split(featureVal, "=", 1)
						self.features[name] = { 
							"FeatureType": featureType, 
							"Category": currentCategory, 
							"Value": value }
						self.order.append(name)

