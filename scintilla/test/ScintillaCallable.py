# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import ctypes, os, sys

from ctypes import c_int, c_ulong, c_char_p, c_wchar_p, c_ushort, c_uint, c_long, c_ssize_t

def IsEnumeration(t):
	return t[:1].isupper()

class TEXTRANGE(ctypes.Structure):
	_fields_= (\
		('cpMin', c_long),
		('cpMax', c_long),
		('lpstrText', ctypes.POINTER(ctypes.c_char)),
	)

class FINDTEXT(ctypes.Structure):
	_fields_= (\
		('cpMin', c_long),
		('cpMax', c_long),
		('lpstrText', c_char_p),
		('cpMinText', c_long),
		('cpMaxText', c_long),
	)

class SciCall:
	def __init__(self, fn, ptr, msg, stringResult=False):
		self._fn = fn
		self._ptr = ptr
		self._msg = msg
		self._stringResult = stringResult
	def __call__(self, w=0, l=0):
		ww = ctypes.cast(w, c_char_p)
		if self._stringResult:
			lengthBytes = self._fn(self._ptr, self._msg, ww, None)
			if lengthBytes == 0:
				return bytearray()
			result = (ctypes.c_byte * lengthBytes)(0)
			lengthBytes2 = self._fn(self._ptr, self._msg, ww, ctypes.cast(result, c_char_p))
			assert lengthBytes == lengthBytes2
			return bytearray(result)[:lengthBytes]
		else:
			ll = ctypes.cast(l, c_char_p)
			return self._fn(self._ptr, self._msg, ww, ll)

sciFX = ctypes.CFUNCTYPE(c_ssize_t, c_char_p, c_int, c_char_p, c_char_p)

class ScintillaCallable:
	def __init__(self, face, scifn, sciptr):
		self.__dict__["face"] = face
		self.__dict__["used"] = set()
		self.__dict__["all"] = set()
		# The k member is for accessing constants as a dictionary
		self.__dict__["k"] = {}
		for f in face.features:
			self.all.add(f)
			if face.features[f]["FeatureType"] == "val":
				self.k[f] = int(self.face.features[f]["Value"], 0)
			elif face.features[f]["FeatureType"] == "evt":
				self.k["SCN_"+f] = int(self.face.features[f]["Value"], 0)
		scifn = sciFX(scifn)
		self.__dict__["_scifn"] = scifn
		self.__dict__["_sciptr"] = sciptr
	def __getattr__(self, name):
		if name in self.face.features:
			self.used.add(name)
			feature = self.face.features[name]
			value = int(feature["Value"], 0)
			#~ print("Feature", name, feature)
			if feature["FeatureType"] == "val":
				self.__dict__[name] = value
				return value
			else:
				if feature["Param2Type"] == "stringresult" and \
					name not in ["GetText", "GetLine", "GetCurLine"]:
					return SciCall(self._scifn, self._sciptr, value, True)
				else:
					return SciCall(self._scifn, self._sciptr, value)
		elif ("Get" + name) in self.face.features:
			self.used.add("Get" + name)
			feature = self.face.features["Get" + name]
			value = int(feature["Value"], 0)
			if feature["FeatureType"] == "get" and \
				not name.startswith("Get") and \
				not feature["Param1Type"] and \
				not feature["Param2Type"] and \
				(feature["ReturnType"] in ["bool", "int", "position", "line", "pointer"] or \
				IsEnumeration(feature["ReturnType"])):
				#~ print("property", feature)
				return self._scifn(self._sciptr, value, None, None)
		elif name.startswith("SCN_") and name in self.k:
			self.used.add(name)
			feature = self.face.features[name[4:]]
			value = int(feature["Value"], 0)
			#~ print("Feature", name, feature)
			if feature["FeatureType"] == "val":
				return value
		raise AttributeError(name)
	def __setattr__(self, name, val):
		if ("Set" + name) in self.face.features:
			self.used.add("Set" + name)
			feature = self.face.features["Set" + name]
			value = int(feature["Value"], 0)
			#~ print("setproperty", feature)
			if feature["FeatureType"] == "set" and not name.startswith("Set"):
				if feature["Param1Type"] in ["bool", "int", "position", "line"] or IsEnumeration(feature["Param1Type"]):
					return self._scifn(self._sciptr, value, c_char_p(val), None)
				elif feature["Param2Type"] in ["string"]:
					return self._scifn(self._sciptr, value, None, c_char_p(val))
				raise AttributeError(name)
		raise AttributeError(name)
	def getvalue(self, name):
		if name in self.face.features:
			feature = self.face.features[name]
			if feature["FeatureType"] != "evt":
				try:
					return int(feature["Value"], 0)
				except ValueError:
					return -1
		return -1


	def ByteRange(self, start, end):
		tr = TEXTRANGE()
		tr.cpMin = start
		tr.cpMax = end
		length = end - start
		tr.lpstrText = ctypes.create_string_buffer(length + 1)
		self.GetTextRange(0, ctypes.byref(tr))
		text = tr.lpstrText[:length]
		text += b"\0" * (length - len(text))
		return text
	def StyledTextRange(self, start, end):
		tr = TEXTRANGE()
		tr.cpMin = start
		tr.cpMax = end
		length = 2 * (end - start)
		tr.lpstrText = ctypes.create_string_buffer(length + 2)
		self.GetStyledText(0, ctypes.byref(tr))
		styledText = tr.lpstrText[:length]
		styledText += b"\0" * (length - len(styledText))
		return styledText
	def FindBytes(self, start, end, s, flags):
		ft = FINDTEXT()
		ft.cpMin = start
		ft.cpMax = end
		ft.lpstrText = s
		ft.cpMinText = 0
		ft.cpMaxText = 0
		pos = self.FindText(flags, ctypes.byref(ft))
		#~ print(start, end, ft.cpMinText, ft.cpMaxText)
		return pos

	def Contents(self):
		return self.ByteRange(0, self.Length)

	def SetContents(self, s):
		self.TargetStart = 0
		self.TargetEnd = self.Length
		self.ReplaceTarget(len(s), s)

