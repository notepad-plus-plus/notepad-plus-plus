#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Requires Python 2.7 or later

# These are tests that run only on Win32 as they use Win32 SendMessage call
# to send WM_* messages to Scintilla that are not implemented on other platforms.
# These help Scintilla behave like a Win32 text control and can help screen readers,
# for example.

from __future__ import with_statement
from __future__ import unicode_literals

import ctypes, unittest

from MessageNumbers import msgs

user32 = ctypes.windll.user32

import XiteWin as Xite

class TestWins(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.sciHwnd = self.xite.sciHwnd
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.SetCodePage(0)
		self.ed.SetStatus(0)

	# Helper methods

	def Send(self, msg, wp, lp):
		return user32.SendMessageW(self.sciHwnd, msgs[msg], wp, lp)

	def GetTextLength(self):
		return self.Send("WM_GETTEXTLENGTH", 0, 0)

	def GetText(self, n, s):
		# n = The maximum number of characters to be copied, including the terminating null character.
		# returns the number of characters copied, not including the terminating null character
		return self.Send("WM_GETTEXT", n, s)

	def TextValue(self):
		self.assertEqual(self.ed.GetStatus(), 0)
		lenValue = self.GetTextLength()
		lenValueWithNUL = lenValue + 1
		value = ctypes.create_unicode_buffer(lenValueWithNUL)
		lenData = self.GetText(lenValueWithNUL, value)
		self.assertEqual(self.ed.GetStatus(), 0)
		self.assertEqual(lenData, lenValue)
		return value.value

	def SetText(self, s):
		return self.Send("WM_SETTEXT", 0, s)

	# Tests

	def testSetText(self):
		self.SetText(b"ab")
		self.assertEqual(self.ed.Length, 2)

	def testGetTextLength(self):
		self.SetText(b"ab")
		self.assertEqual(self.GetTextLength(), 2)

	def testGetText(self):
		self.SetText(b"ab")
		data = ctypes.create_unicode_buffer(100)
		lenData = self.GetText(100, data)
		self.assertEqual(lenData, 2)
		self.assertEqual(len(data.value), 2)
		self.assertEqual(data.value, "ab")

	def testGetUTF8Text(self):
		self.ed.SetCodePage(65001)
		t = "å"
		tu8 = t.encode("UTF-8")
		self.SetText(tu8)
		value = self.TextValue()
		self.assertEqual(value, t)

	def testGetBadUTF8Text(self):
		self.ed.SetCodePage(65001)
		tu8 = b't\xc2'
		t = "t\xc2"
		self.SetText(tu8)
		value = self.TextValue()
		self.assertEqual(len(value), 2)
		self.assertEqual(value, t)

	def testGetJISText(self):
		self.ed.SetCodePage(932)
		t = "\N{HIRAGANA LETTER KA}"
		tu8 = t.encode("shift-jis")
		self.SetText(tu8)
		value = self.TextValue()
		self.assertEqual(len(value), 1)
		self.assertEqual(value, t)

	def testGetBadJISText(self):
		self.ed.SetCodePage(932)
		# This is invalid Shift-JIS, surrounded by []
		tu8 = b'[\x85\xff]'
		# Win32 uses Katakana Middle Dot to indicate some invalid Shift-JIS text
		# At other times \uF8F3 is used which is a private use area character
		# See https://unicodebook.readthedocs.io/operating_systems.html
		katakanaMiddleDot = '[\N{KATAKANA MIDDLE DOT}]'
		privateBad = '[\uf8f3]'
		self.SetText(tu8)
		value = self.TextValue()
		self.assertEqual(len(value), 3)
		self.assertEqual(value, katakanaMiddleDot)

		# This is even less valid Shift-JIS
		tu8 = b'[\xff]'
		self.SetText(tu8)
		value = self.TextValue()
		self.assertEqual(len(value), 3)
		self.assertEqual(value, privateBad)

	def testGetTextLong(self):
		self.assertEqual(self.ed.GetStatus(), 0)
		self.SetText(b"ab")
		data = ctypes.create_unicode_buffer(100)
		lenData = self.GetText(4, data)
		self.assertEqual(self.ed.GetStatus(), 0)
		self.assertEqual(lenData, 2)
		self.assertEqual(data.value, "ab")

	def testGetTextLongNonASCII(self):
		# With 1 multibyte character in document ask for 4 and ensure 1 character
		# returned correctly.
		self.ed.SetCodePage(65001)
		t = "å"
		tu8 = t.encode("UTF-8")
		self.SetText(tu8)
		data = ctypes.create_unicode_buffer(100)
		lenData = self.GetText(4, data)
		self.assertEqual(self.ed.GetStatus(), 0)
		self.assertEqual(lenData, 1)
		self.assertEqual(data.value, t)

	def testGetTextShort(self):
		self.assertEqual(self.ed.GetStatus(), 0)
		self.SetText(b"ab")
		data = ctypes.create_unicode_buffer(100)
		lenData = self.GetText(2, data)
		self.assertEqual(self.ed.GetStatus(), 0)
		self.assertEqual(lenData, 1)
		self.assertEqual(data.value, "a")

	def testGetTextJustNUL(self):
		self.assertEqual(self.ed.GetStatus(), 0)
		self.SetText(b"ab")
		data = ctypes.create_unicode_buffer(100)
		lenData = self.GetText(1, data)
		self.assertEqual(self.ed.GetStatus(), 0)
		#~ print(data)
		self.assertEqual(lenData, 0)
		self.assertEqual(data.value, "")

	def testGetTextZeroLength(self):
		self.assertEqual(self.ed.GetStatus(), 0)
		self.SetText(b"ab")
		data = ctypes.create_unicode_buffer(100)
		lenData = self.GetText(0, data)
		self.assertEqual(self.ed.GetStatus(), 0)
		#~ print(data)
		self.assertEqual(lenData, 0)
		self.assertEqual(data.value, "")

if __name__ == '__main__':
	uu = Xite.main("win32Tests")
