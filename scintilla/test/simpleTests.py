#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# @file simpleTests.py
# Requires Python 2.7 or later

from __future__ import with_statement
from __future__ import unicode_literals

import ctypes, string, sys, unittest

import XiteWin as Xite

# Unicode line ends are only available for lexers that support the feature so requires lexers
lexersAvailable = Xite.lexillaAvailable or Xite.scintillaIncludesLexers
unicodeLineEndsAvailable = lexersAvailable

class TestSimple(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testStatus(self):
		self.assertEqual(self.ed.GetStatus(), 0)
		self.ed.SetStatus(1)
		self.assertEqual(self.ed.GetStatus(), 1)
		self.ed.SetStatus(0)
		self.assertEqual(self.ed.GetStatus(), 0)

	def testLength(self):
		self.assertEqual(self.ed.Length, 0)

	def testAddText(self):
		self.ed.AddText(1, b"x")
		self.assertEqual(self.ed.Length, 1)
		self.assertEqual(self.ed.GetCharAt(0), ord("x"))
		self.assertEqual(self.ed.GetStyleAt(0), 0)
		self.ed.ClearAll()
		self.assertEqual(self.ed.Length, 0)

	def testDeleteRange(self):
		self.ed.AddText(5, b"abcde")
		self.assertEqual(self.ed.Length, 5)
		self.ed.DeleteRange(1, 2)
		self.assertEqual(self.ed.Length, 3)
		self.assertEqual(self.ed.Contents(), b"ade")

	def testAddStyledText(self):
		self.assertEqual(self.ed.EndStyled, 0)
		self.ed.AddStyledText(4, b"x\002y\377")
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.ed.GetCharAt(0), ord("x"))
		self.assertEqual(self.ed.GetStyleAt(0), 2)
		self.assertEqual(self.ed.GetStyleIndexAt(0), 2)
		self.assertEqual(self.ed.GetStyleIndexAt(1), 255)
		self.assertEqual(self.ed.StyledTextRange(0, 1), b"x\002")
		self.assertEqual(self.ed.StyledTextRange(1, 2), b"y\377")
		self.ed.ClearDocumentStyle()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.ed.GetCharAt(0), ord("x"))
		self.assertEqual(self.ed.GetStyleAt(0), 0)
		self.assertEqual(self.ed.StyledTextRange(0, 1), b"x\0")

	def testStyledTextRangeFull(self):
		self.assertEqual(self.ed.EndStyled, 0)
		self.ed.AddStyledText(4, b"x\002y\377")
		self.assertEqual(self.ed.StyledTextRangeFull(0, 1), b"x\002")
		self.assertEqual(self.ed.StyledTextRangeFull(1, 2), b"y\377")
		self.ed.ClearDocumentStyle()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.ed.StyledTextRangeFull(0, 1), b"x\0")

	def testStyling(self):
		self.assertEqual(self.ed.EndStyled, 0)
		self.ed.AddStyledText(4, b"x\002y\003")
		self.assertEqual(self.ed.StyledTextRange(0, 2), b"x\002y\003")
		self.ed.StartStyling(0,0xf)
		self.ed.SetStyling(1, 5)
		self.assertEqual(self.ed.StyledTextRange(0, 2), b"x\005y\003")

		self.ed.StartStyling(0,0xff)
		self.ed.SetStylingEx(2, b"\100\101")
		self.assertEqual(self.ed.StyledTextRange(0, 2), b"x\100y\101")

	def testPosition(self):
		self.assertEqual(self.ed.CurrentPos, 0)
		self.assertEqual(self.ed.Anchor, 0)
		self.ed.AddText(1, b"x")
		# Caret has automatically moved
		self.assertEqual(self.ed.CurrentPos, 1)
		self.assertEqual(self.ed.Anchor, 1)
		self.ed.SelectAll()
		self.assertEqual(self.ed.CurrentPos, 0)
		self.assertEqual(self.ed.Anchor, 1)
		self.ed.Anchor = 0
		self.assertEqual(self.ed.Anchor, 0)
		# Check line positions
		self.assertEqual(self.ed.PositionFromLine(0), 0)
		self.assertEqual(self.ed.GetLineEndPosition(0), 1)
		self.assertEqual(self.ed.PositionFromLine(1), 1)

		self.ed.CurrentPos = 1
		self.assertEqual(self.ed.Anchor, 0)
		self.assertEqual(self.ed.CurrentPos, 1)

	def testBeyonEnd(self):
		self.ed.AddText(1, b"x")
		self.assertEqual(self.ed.GetLineEndPosition(0), 1)
		self.assertEqual(self.ed.GetLineEndPosition(1), 1)
		self.assertEqual(self.ed.GetLineEndPosition(2), 1)

	def testSelection(self):
		self.assertEqual(self.ed.CurrentPos, 0)
		self.assertEqual(self.ed.Anchor, 0)
		self.assertEqual(self.ed.SelectionStart, 0)
		self.assertEqual(self.ed.SelectionEnd, 0)
		self.ed.AddText(1, b"x")
		self.ed.SelectionStart = 0
		self.assertEqual(self.ed.CurrentPos, 1)
		self.assertEqual(self.ed.Anchor, 0)
		self.assertEqual(self.ed.SelectionStart, 0)
		self.assertEqual(self.ed.SelectionEnd, 1)
		self.ed.SelectionStart = 1
		self.assertEqual(self.ed.CurrentPos, 1)
		self.assertEqual(self.ed.Anchor, 1)
		self.assertEqual(self.ed.SelectionStart, 1)
		self.assertEqual(self.ed.SelectionEnd, 1)

		self.ed.SelectionEnd = 0
		self.assertEqual(self.ed.CurrentPos, 0)
		self.assertEqual(self.ed.Anchor, 0)

	def testSetSelection(self):
		self.ed.AddText(4, b"abcd")
		self.ed.SetSel(1, 3)
		self.assertEqual(self.ed.SelectionStart, 1)
		self.assertEqual(self.ed.SelectionEnd, 3)
		result = self.ed.GetSelText(0)
		self.assertEqual(result, b"bc")
		self.ed.ReplaceSel(0, b"1234")
		self.assertEqual(self.ed.Length, 6)
		self.assertEqual(self.ed.Contents(), b"a1234d")

	def testReadOnly(self):
		self.ed.AddText(1, b"x")
		self.assertEqual(self.ed.ReadOnly, 0)
		self.assertEqual(self.ed.Contents(), b"x")
		self.ed.ReadOnly = 1
		self.assertEqual(self.ed.ReadOnly, 1)
		self.ed.AddText(1, b"x")
		self.assertEqual(self.ed.Contents(), b"x")
		self.ed.ReadOnly = 0
		self.ed.AddText(1, b"x")
		self.assertEqual(self.ed.Contents(), b"xx")
		self.ed.Null()
		self.assertEqual(self.ed.Contents(), b"xx")

	def testAddLine(self):
		data = b"x" * 70 + b"\n"
		for i in range(5):
			self.ed.AddText(len(data), data)
			self.assertEqual(self.ed.LineCount, i + 2)
		self.assertTrue(self.ed.Length > 0)

	def testInsertText(self):
		data = b"xy"
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(data, self.ed.ByteRange(0,2))

		self.ed.InsertText(1, data)
		# Should now be "xxyy"
		self.assertEqual(self.ed.Length, 4)
		self.assertEqual(b"xxyy", self.ed.ByteRange(0,4))

	def testTextRangeFull(self):
		data = b"xy"
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(data, self.ed.ByteRangeFull(0,2))

		self.ed.InsertText(1, data)
		# Should now be "xxyy"
		self.assertEqual(self.ed.Length, 4)
		self.assertEqual(b"xxyy", self.ed.ByteRangeFull(0,4))

	def testInsertNul(self):
		data = b"\0"
		self.ed.AddText(1, data)
		self.assertEqual(self.ed.Length, 1)
		self.assertEqual(data, self.ed.ByteRange(0,1))

	def testNewLine(self):
		self.ed.SetContents(b"12")
		self.ed.SetSel(1, 1)
		self.ed.NewLine()
		self.assertEqual(self.ed.Contents(), b"1\r\n2")

	def testUndoRedo(self):
		data = b"xy"
		self.assertEqual(self.ed.Modify, 0)
		self.assertEqual(self.ed.UndoCollection, 1)
		self.assertEqual(self.ed.CanRedo(), 0)
		self.assertEqual(self.ed.CanUndo(), 0)
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.ed.Modify, 1)
		self.assertEqual(self.ed.CanRedo(), 0)
		self.assertEqual(self.ed.CanUndo(), 1)
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.ed.Modify, 0)
		self.assertEqual(self.ed.CanRedo(), 1)
		self.assertEqual(self.ed.CanUndo(), 0)
		self.ed.Redo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.ed.Modify, 1)
		self.assertEqual(data, self.ed.Contents())
		self.assertEqual(self.ed.CanRedo(), 0)
		self.assertEqual(self.ed.CanUndo(), 1)

	def testUndoSequence(self):
		data = b"xy"
		self.assertEqual(self.ed.UndoSequence, 0)
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.UndoSequence, 0)
		# Check that actions between BeginUndoAction and EndUndoAction are undone together
		self.ed.BeginUndoAction()
		self.assertEqual(self.ed.UndoSequence, 1)
		self.ed.InsertText(0, data)
		self.ed.InsertText(1, data)
		# xxyyxy
		self.assertEqual(self.ed.Length, 6)
		self.ed.EndUndoAction()
		self.assertEqual(self.ed.UndoSequence, 0)
		self.ed.Undo()
		# xy as 2 inserts removed
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.ed.UndoSequence, 0)

	def testUndoSavePoint(self):
		data = b"xy"
		self.assertEqual(self.ed.Modify, 0)
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.Modify, 1)
		self.ed.SetSavePoint()
		self.assertEqual(self.ed.Modify, 0)
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.Modify, 1)

	def testUndoCollection(self):
		data = b"xy"
		self.assertEqual(self.ed.UndoCollection, 1)
		self.ed.UndoCollection = 0
		self.assertEqual(self.ed.UndoCollection, 0)
		self.ed.InsertText(0, data)
		self.assertEqual(self.ed.CanRedo(), 0)
		self.assertEqual(self.ed.CanUndo(), 0)
		self.ed.UndoCollection = 1

	def testGetColumn(self):
		self.ed.AddText(1, b"x")
		self.assertEqual(self.ed.GetColumn(0), 0)
		self.assertEqual(self.ed.GetColumn(1), 1)
		# Next line caused infinite loop in 1.71
		self.assertEqual(self.ed.GetColumn(2), 1)
		self.assertEqual(self.ed.GetColumn(3), 1)

	def testTabWidth(self):
		self.assertEqual(self.ed.TabWidth, 8)
		self.ed.AddText(3, b"x\tb")
		self.assertEqual(self.ed.GetColumn(0), 0)
		self.assertEqual(self.ed.GetColumn(1), 1)
		self.assertEqual(self.ed.GetColumn(2), 8)
		for col in range(10):
			if col == 0:
				self.assertEqual(self.ed.FindColumn(0, col), 0)
			elif col == 1:
				self.assertEqual(self.ed.FindColumn(0, col), 1)
			elif col == 8:
				self.assertEqual(self.ed.FindColumn(0, col), 2)
			elif col == 9:
				self.assertEqual(self.ed.FindColumn(0, col), 3)
			else:
				self.assertEqual(self.ed.FindColumn(0, col), 1)
		self.ed.TabWidth = 4
		self.assertEqual(self.ed.TabWidth, 4)
		self.assertEqual(self.ed.GetColumn(0), 0)
		self.assertEqual(self.ed.GetColumn(1), 1)
		self.assertEqual(self.ed.GetColumn(2), 4)

	def testIndent(self):
		self.assertEqual(self.ed.Indent, 0)
		self.assertEqual(self.ed.UseTabs, 1)
		self.ed.Indent = 8
		self.ed.UseTabs = 0
		self.assertEqual(self.ed.Indent, 8)
		self.assertEqual(self.ed.UseTabs, 0)
		self.ed.AddText(3, b"x\tb")
		self.assertEqual(self.ed.GetLineIndentation(0), 0)
		self.ed.InsertText(0, b" ")
		self.assertEqual(self.ed.GetLineIndentation(0), 1)
		self.assertEqual(self.ed.GetLineIndentPosition(0), 1)
		self.assertEqual(self.ed.Contents(), b" x\tb")
		self.ed.SetLineIndentation(0,2)
		self.assertEqual(self.ed.Contents(), b"  x\tb")
		self.assertEqual(self.ed.GetLineIndentPosition(0), 2)
		self.ed.UseTabs = 1
		self.ed.SetLineIndentation(0,8)
		self.assertEqual(self.ed.Contents(), b"\tx\tb")
		self.assertEqual(self.ed.GetLineIndentPosition(0), 1)

	def testGetCurLine(self):
		self.ed.AddText(1, b"x")
		data = ctypes.create_string_buffer(b"\0" * 100)
		caret = self.ed.GetCurLine(len(data), data)
		self.assertEqual(caret, 1)
		self.assertEqual(data.value, b"x")

	def testGetLine(self):
		self.ed.AddText(1, b"x")
		data = ctypes.create_string_buffer(b"\0" * 100)
		self.ed.GetLine(0, data)
		self.assertEqual(data.value, b"x")

	def testLineEnds(self):
		self.ed.AddText(3, b"x\ny")
		self.assertEqual(self.ed.GetLineEndPosition(0), 1)
		self.assertEqual(self.ed.GetLineEndPosition(1), 3)
		self.assertEqual(self.ed.LineLength(0), 2)
		self.assertEqual(self.ed.LineLength(1), 1)
		# Test lines out of range.
		self.assertEqual(self.ed.LineLength(2), 0)
		self.assertEqual(self.ed.LineLength(-1), 0)
		if sys.platform == "win32":
			self.assertEqual(self.ed.EOLMode, self.ed.SC_EOL_CRLF)
		else:
			self.assertEqual(self.ed.EOLMode, self.ed.SC_EOL_LF)
		lineEnds = [b"\r\n", b"\r", b"\n"]
		for lineEndType in [self.ed.SC_EOL_CR, self.ed.SC_EOL_LF, self.ed.SC_EOL_CRLF]:
			self.ed.EOLMode = lineEndType
			self.assertEqual(self.ed.EOLMode, lineEndType)
			self.ed.ConvertEOLs(lineEndType)
			self.assertEqual(self.ed.Contents(), b"x" + lineEnds[lineEndType] + b"y")
			self.assertEqual(self.ed.LineLength(0), 1 + len(lineEnds[lineEndType]))

	# Several tests for unicode line ends U+2028 and U+2029

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testUnicodeLineEnds(self):
		# Add two lines separated with U+2028 and ensure it is seen as two lines
		# Then remove U+2028 and should be just 1 lines
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.ed.AddText(5, b"x\xe2\x80\xa8y")
		self.assertEqual(self.ed.LineCount, 2)
		self.assertEqual(self.ed.GetLineEndPosition(0), 1)
		self.assertEqual(self.ed.GetLineEndPosition(1), 5)
		self.assertEqual(self.ed.LineLength(0), 4)
		self.assertEqual(self.ed.LineLength(1), 1)
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 4
		self.ed.ReplaceTarget(0, b"")
		self.assertEqual(self.ed.LineCount, 1)
		self.assertEqual(self.ed.LineLength(0), 2)
		self.assertEqual(self.ed.GetLineEndPosition(0), 2)
		self.assertEqual(self.ed.LineEndTypesSupported, 1)

	def testUnicodeLineEndsWithCodePage0(self):
		# Try the Unicode line ends when not in Unicode mode -> should remain 1 line
		self.ed.SetCodePage(0)
		self.ed.AddText(5, b"x\xe2\x80\xa8y")
		self.assertEqual(self.ed.LineCount, 1)
		self.ed.AddText(4, b"x\xc2\x85y")
		self.assertEqual(self.ed.LineCount, 1)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testUnicodeLineEndsSwitchToUnicodeAndBack(self):
		# Add the Unicode line ends when not in Unicode mode
		self.ed.SetCodePage(0)
		self.ed.AddText(5, b"x\xe2\x80\xa8y")
		self.assertEqual(self.ed.LineCount, 1)
		# Into UTF-8 mode - should now be interpreting as two lines
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.assertEqual(self.ed.LineCount, 2)
		# Back to code page 0 and 1 line
		self.ed.SetCodePage(0)
		self.assertEqual(self.ed.LineCount, 1)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testUFragmentedEOLCompletion(self):
		# Add 2 starting bytes of UTF-8 line end then complete it
		self.ed.ClearAll()
		self.ed.AddText(4, b"x\xe2\x80y")
		self.assertEqual(self.ed.LineCount, 1)
		self.assertEqual(self.ed.GetLineEndPosition(0), 4)
		self.ed.SetSel(3,3)
		self.ed.AddText(1, b"\xa8")
		self.assertEqual(self.ed.Contents(), b"x\xe2\x80\xa8y")
		self.assertEqual(self.ed.LineCount, 2)

		# Add 1 starting bytes of UTF-8 line end then complete it
		self.ed.ClearAll()
		self.ed.AddText(3, b"x\xe2y")
		self.assertEqual(self.ed.LineCount, 1)
		self.assertEqual(self.ed.GetLineEndPosition(0), 3)
		self.ed.SetSel(2,2)
		self.ed.AddText(2, b"\x80\xa8")
		self.assertEqual(self.ed.Contents(), b"x\xe2\x80\xa8y")
		self.assertEqual(self.ed.LineCount, 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testUFragmentedEOLStart(self):
		# Add end of UTF-8 line end then insert start
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.assertEqual(self.ed.LineCount, 1)
		self.ed.AddText(4, b"x\x80\xa8y")
		self.assertEqual(self.ed.LineCount, 1)
		self.ed.SetSel(1,1)
		self.ed.AddText(1, b"\xe2")
		self.assertEqual(self.ed.LineCount, 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testUBreakApartEOL(self):
		# Add two lines separated by U+2029 then remove and add back each byte ensuring
		# only one line after each removal of any byte in line end and 2 lines after reinsertion
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		text = b"x\xe2\x80\xa9y"
		self.ed.AddText(5, text)
		self.assertEqual(self.ed.LineCount, 2)

		for i in range(len(text)):
			self.ed.TargetStart = i
			self.ed.TargetEnd = i + 1
			self.ed.ReplaceTarget(0, b"")
			if i in [0, 4]:
				# Removing text characters does not change number of lines
				self.assertEqual(self.ed.LineCount, 2)
			else:
				# Removing byte from line end, removes 1 line
				self.assertEqual(self.ed.LineCount, 1)

			self.ed.TargetEnd = i
			self.ed.ReplaceTarget(1, text[i:i+1])
			self.assertEqual(self.ed.LineCount, 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testURemoveEOLFragment(self):
		# Add UTF-8 line end then delete each byte causing line end to disappear
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		for i in range(3):
			self.ed.ClearAll()
			self.ed.AddText(5, b"x\xe2\x80\xa8y")
			self.assertEqual(self.ed.LineCount, 2)
			self.ed.TargetStart = i+1
			self.ed.TargetEnd = i+2
			self.ed.ReplaceTarget(0, b"")
			self.assertEqual(self.ed.LineCount, 1)

	# Several tests for unicode NEL line ends U+0085

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testNELLineEnds(self):
		# Add two lines separated with U+0085 and ensure it is seen as two lines
		# Then remove U+0085 and should be just 1 lines
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.ed.AddText(4, b"x\xc2\x85y")
		self.assertEqual(self.ed.LineCount, 2)
		self.assertEqual(self.ed.GetLineEndPosition(0), 1)
		self.assertEqual(self.ed.GetLineEndPosition(1), 4)
		self.assertEqual(self.ed.LineLength(0), 3)
		self.assertEqual(self.ed.LineLength(1), 1)
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.ed.ReplaceTarget(0, b"")
		self.assertEqual(self.ed.LineCount, 1)
		self.assertEqual(self.ed.LineLength(0), 2)
		self.assertEqual(self.ed.GetLineEndPosition(0), 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testNELFragmentedEOLCompletion(self):
		# Add starting byte of UTF-8 NEL then complete it
		self.ed.AddText(3, b"x\xc2y")
		self.assertEqual(self.ed.LineCount, 1)
		self.assertEqual(self.ed.GetLineEndPosition(0), 3)
		self.ed.SetSel(2,2)
		self.ed.AddText(1, b"\x85")
		self.assertEqual(self.ed.Contents(), b"x\xc2\x85y")
		self.assertEqual(self.ed.LineCount, 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testNELFragmentedEOLStart(self):
		# Add end of UTF-8 NEL then insert start
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.assertEqual(self.ed.LineCount, 1)
		self.ed.AddText(4, b"x\x85y")
		self.assertEqual(self.ed.LineCount, 1)
		self.ed.SetSel(1,1)
		self.ed.AddText(1, b"\xc2")
		self.assertEqual(self.ed.LineCount, 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testNELBreakApartEOL(self):
		# Add two lines separated by U+0085 then remove and add back each byte ensuring
		# only one line after each removal of any byte in line end and 2 lines after reinsertion
		self.xite.ChooseLexer(b"cpp")
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		text = b"x\xc2\x85y"
		self.ed.AddText(4, text)
		self.assertEqual(self.ed.LineCount, 2)

		for i in range(len(text)):
			self.ed.TargetStart = i
			self.ed.TargetEnd = i + 1
			self.ed.ReplaceTarget(0, b"")
			if i in [0, 3]:
				# Removing text characters does not change number of lines
				self.assertEqual(self.ed.LineCount, 2)
			else:
				# Removing byte from line end, removes 1 line
				self.assertEqual(self.ed.LineCount, 1)

			self.ed.TargetEnd = i
			self.ed.ReplaceTarget(1, text[i:i+1])
			self.assertEqual(self.ed.LineCount, 2)

	@unittest.skipUnless(unicodeLineEndsAvailable, "can not test Unicode line ends")
	def testNELRemoveEOLFragment(self):
		# Add UTF-8 NEL then delete each byte causing line end to disappear
		self.ed.SetCodePage(65001)
		for i in range(2):
			self.ed.ClearAll()
			self.ed.AddText(4, b"x\xc2\x85y")
			self.assertEqual(self.ed.LineCount, 2)
			self.ed.TargetStart = i+1
			self.ed.TargetEnd = i+2
			self.ed.ReplaceTarget(0, b"")
			self.assertEqual(self.ed.LineCount, 1)

	def testGoto(self):
		self.ed.AddText(5, b"a\nb\nc")
		self.assertEqual(self.ed.CurrentPos, 5)
		self.ed.GotoLine(1)
		self.assertEqual(self.ed.CurrentPos, 2)
		self.ed.GotoPos(4)
		self.assertEqual(self.ed.CurrentPos, 4)

	def testCutCopyPaste(self):
		self.ed.AddText(5, b"a1b2c")
		self.ed.SetSel(1,3)
		self.ed.Cut()
		# Clipboard = "1b"
		self.assertEqual(self.ed.Contents(), b"a2c")
		self.assertEqual(self.ed.CanPaste(), 1)
		self.ed.SetSel(0, 0)
		self.ed.Paste()
		self.assertEqual(self.ed.Contents(), b"1ba2c")
		self.ed.SetSel(4,5)
		self.ed.Copy()
		# Clipboard = "c"
		self.ed.SetSel(1,3)
		self.ed.Paste()
		self.assertEqual(self.ed.Contents(), b"1c2c")
		self.ed.SetSel(2,4)
		self.ed.Clear()
		self.assertEqual(self.ed.Contents(), b"1c")

	def testReplaceRectangular(self):
		self.ed.AddText(5, b"a\nb\nc")
		self.ed.SetSel(0,0)
		self.ed.ReplaceRectangular(3, b"1\n2")
		self.assertEqual(self.ed.Contents(), b"1a\n2b\nc")

	def testCopyAllowLine(self):
		lineEndType = self.ed.EOLMode
		self.ed.EOLMode = self.ed.SC_EOL_LF
		self.ed.AddText(5, b"a1\nb2")
		self.ed.SetSel(1,1)
		self.ed.CopyAllowLine()
		# Clipboard = "a1\n"
		self.assertEqual(self.ed.CanPaste(), 1)
		self.ed.SetSel(0, 0)
		self.ed.Paste()
		self.ed.EOLMode = lineEndType
		self.assertEqual(self.ed.Contents(), b"a1\na1\nb2")

	def testCutAllowLine(self):
		lineEndType = self.ed.EOLMode
		self.ed.EOLMode = self.ed.SC_EOL_LF
		self.ed.AddText(5, b"a1\nb2")
		self.ed.SetSel(1,1)
		self.ed.CutAllowLine()
		# Clipboard = "a1\n"
		self.assertEqual(self.ed.CanPaste(), 1)
		self.ed.SetSel(0, 0)
		self.ed.Paste()
		self.ed.EOLMode = lineEndType
		self.assertEqual(self.ed.Contents(), b"a1\nb2")

	def testDuplicate(self):
		self.ed.AddText(3, b"1b2")
		self.ed.SetSel(1,2)
		self.ed.SelectionDuplicate()
		self.assertEqual(self.ed.Contents(), b"1bb2")

	def testLineDuplicate(self):
		self.ed.SetContents(b"1\r\nb\r\n2")
		self.ed.SetSel(3, 3)
		# Duplicates the second line containing 'b'
		self.ed.LineDuplicate()
		self.assertEqual(self.ed.Contents(), b"1\r\nb\r\nb\r\n2")

	def testLineDuplicateDifferentLineEnd(self):
		self.ed.SetContents(b"1\nb\n2")
		self.ed.SetSel(3, 3)
		# Duplicates the second line containing 'b'
		self.ed.LineDuplicate()
		# Same as above but end of duplicated line is \r\n
		self.assertEqual(self.ed.Contents(), b"1\nb\r\nb\n2")

	def testTransposeLines(self):
		self.ed.AddText(8, b"a1\nb2\nc3")
		self.ed.SetSel(3,3)
		self.ed.LineTranspose()
		self.assertEqual(self.ed.Contents(), b"b2\na1\nc3")

	def testReverseLines(self):
		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(0, 8)
		self.ed.LineReverse()
		self.assertEqual(self.ed.Contents(), b"c3\nb2\na1")

		self.ed.SetContents(b"a1\nb2\nc3\n")
		self.ed.SetSel(0, 9)
		self.ed.LineReverse()
		self.assertEqual(self.ed.Contents(), b"c3\nb2\na1\n")

	def testMoveSelectedLines(self):
		lineEndType = self.ed.EOLMode
		self.ed.EOLMode = self.ed.SC_EOL_LF

		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(3, 6)
		self.ed.MoveSelectedLinesDown()
		self.assertEqual(self.ed.Contents(), b"a1\nc3\nb2")

		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(3, 6)
		self.ed.MoveSelectedLinesUp()
		self.assertEqual(self.ed.Contents(), b"b2\na1\nc3")

		# Exercise the 'appendEol' case as the last line has no EOL characters to copy
		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(4, 7)
		self.ed.MoveSelectedLinesUp()
		self.assertEqual(self.ed.Contents(), b"b2\nc3\na1")

		# Testing extreme lines
		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(6, 7)
		self.ed.MoveSelectedLinesDown()
		# No change as moving last line down
		self.assertEqual(self.ed.Contents(), b"a1\nb2\nc3")

		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(1, 2)
		self.ed.MoveSelectedLinesUp()
		# No change as moving first line up
		self.assertEqual(self.ed.Contents(), b"a1\nb2\nc3")

		# Moving line to end with different line end uses that line end
		self.ed.EOLMode = self.ed.SC_EOL_CRLF
		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(3, 6)
		self.ed.MoveSelectedLinesDown()
		self.assertEqual(self.ed.Contents(), b"a1\nc3\r\nb2")

		# Exercise 'appendEol'
		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(4, 7)
		self.ed.MoveSelectedLinesUp()
		self.assertEqual(self.ed.Contents(), b"b2\nc3\r\na1")

		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.SetSel(1, 2)
		self.ed.MoveSelectedLinesDown()
		self.assertEqual(self.ed.Contents(), b"b2\na1\nc3")
		
		# Exercise rectangular selection
		self.ed.EOLMode = self.ed.SC_EOL_LF
		self.ed.SetContents(b"a1\nb2\nc3")
		self.ed.RectangularSelectionAnchor = 1
		self.ed.RectangularSelectionCaret = 4
		# Check rectangular not stream
		self.assertEqual(self.ed.GetSelectionMode(), self.ed.SC_SEL_RECTANGLE)
		self.assertEqual(self.ed.Selections, 2)
		self.ed.MoveSelectedLinesDown()
		self.assertEqual(self.ed.Contents(), b"c3\na1\nb2")

		# Restore line end
		self.ed.EOLMode = lineEndType

	def testGetSet(self):
		self.ed.SetContents(b"abc")
		self.assertEqual(self.ed.TextLength, 3)
		# String buffer containing exactly 5 digits
		result = ctypes.create_string_buffer(b"12345", 5)
		self.assertEqual(result.raw, b"12345")
		length = self.ed.GetText(4, result)
		self.assertEqual(length, 3)
		self.assertEqual(result.value, b"abc")
		# GetText has written the 3 bytes of text and a terminating NUL but left the final digit 5
		self.assertEqual(result.raw, b"abc\x005")

	def testAppend(self):
		self.ed.SetContents(b"abc")
		self.assertEqual(self.ed.SelectionStart, 0)
		self.assertEqual(self.ed.SelectionEnd, 0)
		text = b"12"
		self.ed.AppendText(len(text), text)
		self.assertEqual(self.ed.SelectionStart, 0)
		self.assertEqual(self.ed.SelectionEnd, 0)
		self.assertEqual(self.ed.Contents(), b"abc12")

	def testTarget(self):
		self.ed.SetContents(b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.assertEqual(self.ed.TargetStart, 1)
		self.assertEqual(self.ed.TargetEnd, 3)
		rep = b"321"
		self.ed.ReplaceTarget(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a321d")
		self.ed.SearchFlags = self.ed.SCFIND_REGEXP
		self.assertEqual(self.ed.SearchFlags, self.ed.SCFIND_REGEXP)
		searchString = rb"\([1-9]+\)"
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(1, pos)
		tagString = self.ed.GetTag(1)
		self.assertEqual(tagString, b"321")
		rep = b"\\1"
		self.ed.TargetStart = 0
		self.ed.TargetEnd = 0
		self.ed.ReplaceTargetRE(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"321a321d")
		self.ed.SetSel(4,5)
		self.ed.TargetFromSelection()
		self.assertEqual(self.ed.TargetStart, 4)
		self.assertEqual(self.ed.TargetEnd, 5)

	def testReplaceTargetMinimal(self):
		# 1: No common characters
		self.ed.SetContents(b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.assertEqual(self.ed.TargetStart, 1)
		self.assertEqual(self.ed.TargetEnd, 3)
		rep = b"321"
		self.ed.ReplaceTargetMinimal(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a321d")

		# 2: New characters with common prefix and suffix
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 4
		rep = b"3<>1"
		self.ed.ReplaceTargetMinimal(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a3<>1d")

		# 3: Remove characters with common prefix and suffix
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 5
		rep = b"31"
		self.ed.ReplaceTargetMinimal(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a31d")

		# 4: Common prefix
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		rep = b"3bc"
		self.ed.ReplaceTargetMinimal(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a3bcd")

		# 5: Common suffix
		self.ed.TargetStart = 2
		self.ed.TargetEnd = 5
		rep = b"cd"
		self.ed.ReplaceTargetMinimal(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a3cd")

	def testTargetWhole(self):
		self.ed.SetContents(b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.ed.TargetWholeDocument()
		self.assertEqual(self.ed.TargetStart, 0)
		self.assertEqual(self.ed.TargetEnd, 4)

	def testTargetEscape(self):
		# Checks that a literal \ can be in the replacement. Bug #2959876
		self.ed.SetContents(b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		rep = b"\\\\n"
		self.ed.ReplaceTargetRE(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a\\nd")

	def testTargetVirtualSpace(self):
		self.ed.SetContents(b"a\nbcd")
		self.assertEqual(self.ed.TargetStart, 0)
		self.assertEqual(self.ed.TargetStartVirtualSpace, 0)
		self.assertEqual(self.ed.TargetEnd, 5)
		self.assertEqual(self.ed.TargetEndVirtualSpace, 0)
		self.ed.TargetStart = 1
		self.ed.TargetStartVirtualSpace = 2
		self.ed.TargetEnd = 3
		self.ed.TargetEndVirtualSpace = 4
		# Adds 2 spaces to first line due to virtual space, and replace 2 characters with 3
		rep = b"12\n"
		self.ed.ReplaceTarget(len(rep), rep)
		self.assertEqual(self.ed.Contents(), b"a  12\ncd")
		# 1+2v realized to 3
		self.assertEqual(self.ed.TargetStart, 3)
		self.assertEqual(self.ed.TargetStartVirtualSpace, 0)
		self.assertEqual(self.ed.TargetEnd, 6)
		self.assertEqual(self.ed.TargetEndVirtualSpace, 0)

	def testPointsAndPositions(self):
		self.ed.SetContents(b"xyz")

		# Inter-character positions
		# Start of text
		self.assertEqual(self.ed.PositionFromPoint(1,1), 0)
		# End of text
		self.assertEqual(self.ed.PositionFromPoint(100, 1), 3)
		self.assertEqual(self.ed.PositionFromPointClose(100, 1), -1)
		
		# Character positions
		# Start of text
		self.assertEqual(self.ed.CharPositionFromPoint(0,0), 0)
		# End of text
		self.assertEqual(self.ed.CharPositionFromPoint(100, 0), 3)
		self.assertEqual(self.ed.CharPositionFromPointClose(100, 0), -1)

	def testSelectionFromPoint(self):
		self.ed.SetContents(b"xxxxxx")
		self.ed.SetSelection(3, 2)
		self.ed.AddSelection(0, 1)
		self.ed.AddSelection(5, 5)	# Empty
		xStart = self.ed.PointXFromPosition(0, 0)
		xEnd = self.ed.PointXFromPosition(0, 5)
		width = xEnd-xStart
		charWidth = width // 5
		widthMid = charWidth // 2

		posMid1 = xStart+widthMid
		self.assertEqual(self.ed.SelectionFromPoint(posMid1, 1), 1)
		posMid2 = xStart+charWidth+widthMid
		self.assertEqual(self.ed.SelectionFromPoint(posMid2, 1), -1)
		posMid3 = xStart+charWidth*2+widthMid
		self.assertEqual(self.ed.SelectionFromPoint(posMid3, 1), 0)
		# Empty selection at 5. Exact and then a few pixels either side
		self.assertEqual(self.ed.SelectionFromPoint(xEnd, 1), 2)
		self.assertEqual(self.ed.SelectionFromPoint(xEnd-2, 1), 2)
		self.assertEqual(self.ed.SelectionFromPoint(xEnd+2, 1), 2)
		self.assertEqual(self.ed.SelectionFromPoint(100, 0), -1)

	def testLinePositions(self):
		text = b"ab\ncd\nef"
		nl = b"\n"
		if sys.version_info[0] == 3:
			nl = ord(b"\n")
		self.ed.AddText(len(text), text)
		self.assertEqual(self.ed.LineFromPosition(-1), 0)
		line = 0
		for pos in range(len(text)+1):
			self.assertEqual(self.ed.LineFromPosition(pos), line)
			if pos < len(text) and text[pos] == nl:
				line += 1

	def testWordPositions(self):
		text = b"ab cd\tef"
		self.ed.AddText(len(text), text)
		self.assertEqual(self.ed.WordStartPosition(3, 0), 2)
		self.assertEqual(self.ed.WordStartPosition(4, 0), 3)
		self.assertEqual(self.ed.WordStartPosition(5, 0), 3)
		self.assertEqual(self.ed.WordStartPosition(6, 0), 5)

		self.assertEqual(self.ed.WordEndPosition(2, 0), 3)
		self.assertEqual(self.ed.WordEndPosition(3, 0), 5)
		self.assertEqual(self.ed.WordEndPosition(4, 0), 5)
		self.assertEqual(self.ed.WordEndPosition(5, 0), 6)
		self.assertEqual(self.ed.WordEndPosition(6, 0), 8)

	def testWordRange(self):
		text = b"ab cd\t++"
		self.ed.AddText(len(text), text)
		self.assertEqual(self.ed.IsRangeWord(0, 0), 0)
		self.assertEqual(self.ed.IsRangeWord(0, 1), 0)
		self.assertEqual(self.ed.IsRangeWord(0, 2), 1)
		self.assertEqual(self.ed.IsRangeWord(0, 3), 0)
		self.assertEqual(self.ed.IsRangeWord(0, 4), 0)
		self.assertEqual(self.ed.IsRangeWord(0, 5), 1)
		self.assertEqual(self.ed.IsRangeWord(6, 7), 0)
		self.assertEqual(self.ed.IsRangeWord(6, 8), 1)

atInsertion = 0
atDeletion = 1
actMayCoalesce = 0x100

class TestUndoSaveRestore(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.data = b"xy"

	def testSave(self):
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Contents(), b"xy")
		self.ed.SetSavePoint()
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Contents(), b"xyxy")
		self.ed.DeleteRange(1, 2)
		self.assertEqual(self.ed.Contents(), b"xy")
		self.ed.DeleteRange(1, 1)
		self.assertEqual(self.ed.Contents(), b"x")

		self.assertEqual(self.ed.UndoActions, 4)
		self.assertEqual(self.ed.UndoCurrent, 4)
		self.assertEqual(self.ed.UndoSavePoint, 1)
		self.assertEqual(self.ed.UndoTentative, -1)
		self.assertEqual(self.ed.GetUndoActionType(0), atInsertion)
		self.assertEqual(self.ed.GetUndoActionPosition(0), 0)
		self.assertEqual(bytes(self.ed.GetUndoActionText(0)), self.data)
		self.assertEqual(self.ed.GetUndoActionType(1), atInsertion)
		self.assertEqual(self.ed.GetUndoActionPosition(1), 0)
		self.assertEqual(bytes(self.ed.GetUndoActionText(1)), self.data)
		self.assertEqual(self.ed.GetUndoActionType(2), atDeletion + actMayCoalesce)
		self.assertEqual(self.ed.GetUndoActionPosition(2), 1)
		self.assertEqual(bytes(self.ed.GetUndoActionText(2)), b'yx')
		self.assertEqual(self.ed.GetUndoActionType(3), atDeletion + actMayCoalesce)
		self.assertEqual(self.ed.GetUndoActionPosition(3), 1)
		self.assertEqual(bytes(self.ed.GetUndoActionText(3)), b'y')

	def testRestore(self):
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Contents(), b"xy")
		self.ed.EmptyUndoBuffer()

		self.ed.PushUndoActionType(atInsertion, 0)
		self.ed.ChangeLastUndoActionText(2, b'xy')
		self.ed.PushUndoActionType(atInsertion, 0)
		self.ed.ChangeLastUndoActionText(2, b'xy')
		self.ed.PushUndoActionType(atDeletion + actMayCoalesce, 1)
		self.ed.ChangeLastUndoActionText(2, b'yx')
		self.ed.PushUndoActionType(atDeletion + actMayCoalesce, 1)
		self.ed.ChangeLastUndoActionText(1, b'y')
		
		self.assertEqual(self.ed.UndoActions, 4)
		self.ed.SetUndoCurrent(1)
		self.ed.SetUndoSavePoint(1)
		self.ed.SetUndoTentative(-1)
		
		self.ed.Undo()
		self.assertEqual(self.ed.UndoCurrent, 0)
		self.assertEqual(self.ed.Contents(), b"")
		self.ed.Redo()
		self.assertEqual(self.ed.UndoCurrent, 1)
		self.assertEqual(self.ed.Contents(), b"xy")
		self.ed.Redo()
		self.assertEqual(self.ed.UndoCurrent, 2)
		self.assertEqual(self.ed.Contents(), b"xyxy")
		self.ed.Redo()	# Does 2 actions due to mayCoalesce
		self.assertEqual(self.ed.UndoCurrent, 4)
		self.assertEqual(self.ed.Contents(), b"x")

		# No more redo actions
		self.assertEqual(self.ed.CanRedo(), 0)

class TestChangeHistory(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.data = b"xy"

	def testChangeHistory(self):
		self.assertEqual(self.ed.ChangeHistory, 0)
		self.assertEqual(self.ed.UndoCollection, 1)
		self.ed.UndoCollection = 0
		self.assertEqual(self.ed.UndoCollection, 0)
		self.ed.InsertText(0, self.data)
		self.ed.UndoCollection = 1
		self.ed.ChangeHistory = 1
		self.assertEqual(self.ed.ChangeHistory, 1)
		self.ed.InsertText(0, self.data)
		self.ed.DeleteRange(0, 2)
		self.ed.ChangeHistory = 0
		self.assertEqual(self.ed.ChangeHistory, 0)
		self.ed.ChangeHistory = 1
		self.assertEqual(self.ed.ChangeHistory, 1)
		self.ed.Undo()

MODI = 1
UNDO = 2
REDO = 4

class TestContainerUndo(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.data = b"xy"

	def UndoState(self):
		return (MODI if self.ed.Modify else 0) | \
			(UNDO if self.ed.CanUndo() else 0) | \
			(REDO if self.ed.CanRedo() else 0)

	def testContainerActNoCoalesce(self):
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 0)
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO | REDO)
		self.ed.Redo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()

	def testContainerActCoalesce(self):
		self.ed.InsertText(0, self.data)
		self.ed.AddUndoAction(5, 1)
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)

	def testContainerMultiStage(self):
		self.ed.InsertText(0, self.data)
		self.ed.AddUndoAction(5, 1)
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), REDO)

	def testContainerMultiStageNoText(self):
		self.ed.AddUndoAction(5, 1)
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEqual(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEqual(self.UndoState(), REDO)

	def testContainerActCoalesceEnd(self):
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)

	def testContainerBetweenInsertAndInsert(self):
		self.assertEqual(self.ed.Length, 0)
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.InsertText(2, self.data)
		self.assertEqual(self.ed.Length, 4)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		# Undoes both insertions and the containerAction in the middle
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), REDO)

	def testContainerNoCoalesceBetweenInsertAndInsert(self):
		self.assertEqual(self.ed.Length, 0)
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 0)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.InsertText(2, self.data)
		self.assertEqual(self.ed.Length, 4)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		# Undo last insertion
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO | REDO)
		# Undo container
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO | REDO)
		# Undo first insertion
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 0)
		self.assertEqual(self.UndoState(), REDO)

	def testContainerBetweenDeleteAndDelete(self):
		self.ed.InsertText(0, self.data)
		self.ed.EmptyUndoBuffer()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), 0)
		self.ed.SetSel(2,2)
		self.ed.DeleteBack()
		self.assertEqual(self.ed.Length, 1)
		self.ed.AddUndoAction(5, 1)
		self.ed.DeleteBack()
		self.assertEqual(self.ed.Length, 0)
		# Undoes both deletions and the containerAction in the middle
		self.ed.Undo()
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), REDO)

	def testContainerBetweenInsertAndDelete(self):
		self.assertEqual(self.ed.Length, 0)
		self.ed.InsertText(0, self.data)
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.SetSel(0,1)
		self.ed.Cut()
		self.assertEqual(self.ed.Length, 1)
		self.assertEqual(self.UndoState(), MODI | UNDO)
		self.ed.Undo()	# Only undoes the deletion
		self.assertEqual(self.ed.Length, 2)
		self.assertEqual(self.UndoState(), MODI | UNDO | REDO)

class TestKeyCommands(unittest.TestCase):
	""" These commands are normally assigned to keys and take no arguments """

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def selRange(self):
		return self.ed.CurrentPos, self.ed.Anchor

	def testLineMove(self):
		self.ed.AddText(8, b"x1\ny2\nz3")
		self.ed.SetSel(0,0)
		self.ed.ChooseCaretX()
		self.ed.LineDown()
		self.ed.LineDown()
		self.assertEqual(self.selRange(), (6, 6))
		self.ed.LineUp()
		self.assertEqual(self.selRange(), (3, 3))
		self.ed.LineDownExtend()
		self.assertEqual(self.selRange(), (6, 3))
		self.ed.LineUpExtend()
		self.ed.LineUpExtend()
		self.assertEqual(self.selRange(), (0, 3))

	def testCharMove(self):
		self.ed.AddText(8, b"x1\ny2\nz3")
		self.ed.SetSel(0,0)
		self.ed.CharRight()
		self.ed.CharRight()
		self.assertEqual(self.selRange(), (2, 2))
		self.ed.CharLeft()
		self.assertEqual(self.selRange(), (1, 1))
		self.ed.CharRightExtend()
		self.assertEqual(self.selRange(), (2, 1))
		self.ed.CharLeftExtend()
		self.ed.CharLeftExtend()
		self.assertEqual(self.selRange(), (0, 1))

	def testWordMove(self):
		self.ed.AddText(10, b"a big boat")
		self.ed.SetSel(3,3)
		self.ed.WordRight()
		self.ed.WordRight()
		self.assertEqual(self.selRange(), (10, 10))
		self.ed.WordLeft()
		self.assertEqual(self.selRange(), (6, 6))
		self.ed.WordRightExtend()
		self.assertEqual(self.selRange(), (10, 6))
		self.ed.WordLeftExtend()
		self.ed.WordLeftExtend()
		self.assertEqual(self.selRange(), (2, 6))

	def testHomeEndMove(self):
		self.ed.AddText(10, b"a big boat")
		self.ed.SetSel(3,3)
		self.ed.Home()
		self.assertEqual(self.selRange(), (0, 0))
		self.ed.LineEnd()
		self.assertEqual(self.selRange(), (10, 10))
		self.ed.SetSel(3,3)
		self.ed.HomeExtend()
		self.assertEqual(self.selRange(), (0, 3))
		self.ed.LineEndExtend()
		self.assertEqual(self.selRange(), (10, 3))

	def testVCHome(self):
		# Goes to the indent unless there already when goes to origin
		self.ed.AddText(10, b"   a big boat")
		self.ed.SetSel(6,6)
		self.ed.VCHome()
		self.assertEqual(self.selRange(), (3, 3))
		self.ed.VCHome()
		self.assertEqual(self.selRange(), (0, 0))
		self.ed.VCHome()
		self.assertEqual(self.selRange(), (3, 3))

	def testStartEndMove(self):
		self.ed.AddText(10, b"a\nbig\nboat")
		self.ed.SetSel(3,3)
		self.ed.DocumentStart()
		self.assertEqual(self.selRange(), (0, 0))
		self.ed.DocumentEnd()
		self.assertEqual(self.selRange(), (10, 10))
		self.ed.SetSel(3,3)
		self.ed.DocumentStartExtend()
		self.assertEqual(self.selRange(), (0, 3))
		self.ed.DocumentEndExtend()
		self.assertEqual(self.selRange(), (10, 3))

	def testParagraphMove(self):
		example = b"a\n\nbig\n\n\n\nboat"
		self.ed.AddText(len(example), example)
		start1 = 0	# Before 'a'
		start2 = 3	# Before 'big'
		start3 = 10	# Before 'boat'

		# Paragraph 2 to 1
		self.ed.SetSel(start2, start2)
		self.ed.ParaUp()
		self.assertEqual(self.selRange(), (start1, start1))
		self.ed.ParaDown()
		self.assertEqual(self.selRange(), (start2, start2))
		self.ed.SetSel(start2, start2)
		self.ed.ParaUpExtend()
		self.assertEqual(self.selRange(), (start1, start2))
		self.ed.ParaDownExtend()
		self.assertEqual(self.selRange(), (start2, start2))

		# Inside paragraph 2 to start paragraph 2
		mid2 = start2+1
		self.ed.SetSel(mid2, mid2)
		# Next line behaved differently before change for bug #2363
		self.ed.ParaUp()
		self.assertEqual(self.selRange(), (start2, start2))
		self.ed.ParaDown()
		self.assertEqual(self.selRange(), (start3, start3))
		self.ed.SetSel(mid2, mid2)
		self.ed.ParaUpExtend()
		self.assertEqual(self.selRange(), (start2, mid2))
		self.ed.ParaDownExtend()
		self.assertEqual(self.selRange(), (start3, mid2))

		# Paragraph 3 to 2
		self.ed.SetSel(start3, start3)
		self.ed.ParaUp()
		self.assertEqual(self.selRange(), (start2, start2))
		self.ed.ParaDown()
		self.assertEqual(self.selRange(), (start3, start3))


class TestMarkers(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.AddText(5, b"x\ny\nz")

	def testMarker(self):
		handle = self.ed.MarkerAdd(1,1)
		self.assertEqual(self.ed.MarkerLineFromHandle(handle), 1)
		self.ed.MarkerDelete(1,1)
		self.assertEqual(self.ed.MarkerLineFromHandle(handle), -1)

	def testTwiceAddedDelete(self):
		handle = self.ed.MarkerAdd(1,1)
		self.assertNotEqual(handle, -1)
		self.assertEqual(self.ed.MarkerGet(1), 2)
		handle2 = self.ed.MarkerAdd(1,1)
		self.assertNotEqual(handle2, -1)
		self.assertEqual(self.ed.MarkerGet(1), 2)
		self.ed.MarkerDelete(1,1)
		self.assertEqual(self.ed.MarkerGet(1), 2)
		self.ed.MarkerDelete(1,1)
		self.assertEqual(self.ed.MarkerGet(1), 0)

	def testMarkerDeleteAll(self):
		h1 = self.ed.MarkerAdd(0,1)
		h2 = self.ed.MarkerAdd(1,2)
		self.assertEqual(self.ed.MarkerLineFromHandle(h1), 0)
		self.assertEqual(self.ed.MarkerLineFromHandle(h2), 1)
		self.ed.MarkerDeleteAll(1)
		self.assertEqual(self.ed.MarkerLineFromHandle(h1), -1)
		self.assertEqual(self.ed.MarkerLineFromHandle(h2), 1)
		self.ed.MarkerDeleteAll(-1)
		self.assertEqual(self.ed.MarkerLineFromHandle(h1), -1)
		self.assertEqual(self.ed.MarkerLineFromHandle(h2), -1)

	def testMarkerDeleteHandle(self):
		handle = self.ed.MarkerAdd(0,1)
		self.assertEqual(self.ed.MarkerLineFromHandle(handle), 0)
		self.ed.MarkerDeleteHandle(handle)
		self.assertEqual(self.ed.MarkerLineFromHandle(handle), -1)

	def testMarkerBits(self):
		self.assertEqual(self.ed.MarkerGet(0), 0)
		self.ed.MarkerAdd(0,1)
		self.assertEqual(self.ed.MarkerGet(0), 2)
		self.ed.MarkerAdd(0,2)
		self.assertEqual(self.ed.MarkerGet(0), 6)

	def testMarkerAddSet(self):
		self.assertEqual(self.ed.MarkerGet(0), 0)
		self.ed.MarkerAddSet(0,5)
		self.assertEqual(self.ed.MarkerGet(0), 5)
		self.ed.MarkerDeleteAll(-1)

	def testMarkerNext(self):
		self.assertEqual(self.ed.MarkerNext(0, 2), -1)
		h1 = self.ed.MarkerAdd(0,1)
		self.assertNotEqual(h1, -1)
		h2 = self.ed.MarkerAdd(2,1)
		self.assertNotEqual(h2, -1)
		self.assertEqual(self.ed.MarkerNext(0, 2), 0)
		self.assertEqual(self.ed.MarkerNext(1, 2), 2)
		self.assertEqual(self.ed.MarkerNext(2, 2), 2)
		self.assertEqual(self.ed.MarkerPrevious(0, 2), 0)
		self.assertEqual(self.ed.MarkerPrevious(1, 2), 0)
		self.assertEqual(self.ed.MarkerPrevious(2, 2), 2)

	def testMarkerNegative(self):
		self.assertEqual(self.ed.MarkerNext(-1, 2), -1)

	def testLineState(self):
		self.assertEqual(self.ed.MaxLineState, 0)
		self.assertEqual(self.ed.GetLineState(0), 0)
		self.assertEqual(self.ed.GetLineState(1), 0)
		self.assertEqual(self.ed.GetLineState(2), 0)
		self.ed.SetLineState(1, 100)
		self.assertNotEqual(self.ed.MaxLineState, 0)
		self.assertEqual(self.ed.GetLineState(0), 0)
		self.assertEqual(self.ed.GetLineState(1), 100)
		self.assertEqual(self.ed.GetLineState(2), 0)

	def testSymbolRetrieval(self):
		self.ed.MarkerDefine(1,3)
		self.assertEqual(self.ed.MarkerSymbolDefined(1), 3)

	def markerFromLineIndex(self, line, index):
		""" Helper that returns (handle, number) """
		return (self.ed.MarkerHandleFromLine(line, index), self.ed.MarkerNumberFromLine(line, index))

	def markerSetFromLine(self, line):
		""" Helper that returns set of (handle, number) """
		markerSet = set()
		index = 0
		while 1:
			marker = self.markerFromLineIndex(line, index)
			if marker[0] == -1:
				return markerSet
			markerSet.add(marker)
			index += 1

	def testMarkerFromLine(self):
		self.assertEqual(self.ed.MarkerHandleFromLine(1, 0), -1)
		self.assertEqual(self.ed.MarkerNumberFromLine(1, 0), -1)
		self.assertEqual(self.markerFromLineIndex(1, 0), (-1, -1))
		self.assertEqual(self.markerSetFromLine(1), set())

		handle = self.ed.MarkerAdd(1,10)
		self.assertEqual(self.markerFromLineIndex(1, 0), (handle, 10))
		self.assertEqual(self.markerFromLineIndex(1, 1), (-1, -1))
		self.assertEqual(self.markerSetFromLine(1), {(handle, 10)})

		handle2 = self.ed.MarkerAdd(1,11)
		# Don't want to depend on ordering so check as sets
		self.assertEqual(self.markerSetFromLine(1), {(handle, 10), (handle2, 11)})

		self.ed.MarkerDeleteHandle(handle2)
		self.assertEqual(self.markerSetFromLine(1), {(handle, 10)})

class TestIndicators(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def indicatorValueString(self, indicator):
		# create a string with -/X where indicator off/on to make checks simple
		return "".join("-X"[self.ed.IndicatorValueAt(indicator, index)] for index in range(self.ed.TextLength))

	def testSetIndicator(self):
		self.assertEqual(self.ed.IndicGetStyle(0), 1)
		self.assertEqual(self.ed.IndicGetFore(0), 0x007f00)
		self.ed.IndicSetStyle(0, 2)
		self.ed.IndicSetFore(0, 0xff0080)
		self.assertEqual(self.ed.IndicGetStyle(0), 2)
		self.assertEqual(self.ed.IndicGetFore(0), 0xff0080)

	def testIndicatorFill(self):
		self.ed.InsertText(0, b"abc")
		self.ed.IndicatorCurrent = 3
		self.ed.IndicatorFillRange(1,1)
		self.assertEqual(self.indicatorValueString(3), "-X-")
		self.assertEqual(self.ed.IndicatorStart(3, 0), 0)
		self.assertEqual(self.ed.IndicatorEnd(3, 0), 1)
		self.assertEqual(self.ed.IndicatorStart(3, 1), 1)
		self.assertEqual(self.ed.IndicatorEnd(3, 1), 2)
		self.assertEqual(self.ed.IndicatorStart(3, 2), 2)
		self.assertEqual(self.ed.IndicatorEnd(3, 2), 3)

	def testIndicatorAtEnd(self):
		self.ed.InsertText(0, b"ab")
		self.ed.IndicatorCurrent = 3
		self.ed.IndicatorFillRange(1,1)
		self.assertEqual(self.ed.IndicatorValueAt(3, 0), 0)
		self.assertEqual(self.ed.IndicatorValueAt(3, 1), 1)
		self.assertEqual(self.ed.IndicatorStart(3, 0), 0)
		self.assertEqual(self.ed.IndicatorEnd(3, 0), 1)
		self.assertEqual(self.ed.IndicatorStart(3, 1), 1)
		self.assertEqual(self.ed.IndicatorEnd(3, 1), 2)
		self.ed.DeleteRange(1, 1)
		# Now only one character left and does not have indicator so indicator 3 is null
		self.assertEqual(self.ed.IndicatorValueAt(3, 0), 0)
		# Since null, remaining calls return 0
		self.assertEqual(self.ed.IndicatorStart(3, 0), 0)
		self.assertEqual(self.ed.IndicatorEnd(3, 0), 0)
		self.assertEqual(self.ed.IndicatorStart(3, 1), 0)
		self.assertEqual(self.ed.IndicatorEnd(3, 1), 0)

	def testIndicatorMovement(self):
		# Create a two character indicator over "BC" in "aBCd" and ensure that it doesn't
		# leak onto surrounding characters when insertions made at either end but
		# insertion inside indicator does extend length
		self.ed.InsertText(0, b"aBCd")
		self.ed.IndicatorCurrent = 3
		self.ed.IndicatorFillRange(1,2)
		self.assertEqual(self.indicatorValueString(3), "-XX-")

		# Insertion 1 before
		self.ed.InsertText(0, b"1")
		self.assertEqual(b"1aBCd", self.ed.Contents())
		self.assertEqual(self.indicatorValueString(3), "--XX-")

		# Insertion at start of indicator
		self.ed.InsertText(2, b"2")
		self.assertEqual(b"1a2BCd", self.ed.Contents())
		self.assertEqual(self.indicatorValueString(3), "---XX-")

		# Insertion inside indicator
		self.ed.InsertText(4, b"3")
		self.assertEqual(b"1a2B3Cd", self.ed.Contents())
		self.assertEqual(self.indicatorValueString(3), "---XXX-")

		# Insertion at end of indicator
		self.ed.InsertText(6, b"4")
		self.assertEqual(b"1a2B3C4d", self.ed.Contents())
		self.assertEqual(self.indicatorValueString(3), "---XXX--")

		# Insertion 1 after
		self.ed.InsertText(8, b"5")
		self.assertEqual(self.indicatorValueString(3), "---XXX---")
		self.assertEqual(b"1a2B3C4d5", self.ed.Contents())


class TestScrolling(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 150 should be enough lines
		self.ed.InsertText(0, b"a" * 150 + b"\n" * 150)

	def testTop(self):
		self.ed.GotoLine(0)
		self.assertEqual(self.ed.FirstVisibleLine, 0)

	def testLineScroll(self):
		self.ed.GotoLine(0)
		self.ed.LineScroll(0, 3)
		self.assertEqual(self.ed.FirstVisibleLine, 3)
		self.ed.LineScroll(0, -2)
		self.assertEqual(self.ed.FirstVisibleLine, 1)
		self.assertEqual(self.ed.XOffset, 0)
		self.ed.LineScroll(10, 0)
		self.assertGreater(self.ed.XOffset, 0)
		scroll_width = float(self.ed.XOffset) / 10
		self.ed.LineScroll(-2, 0)
		self.assertEqual(self.ed.XOffset, scroll_width * 8)

	def testVisibleLine(self):
		self.ed.FirstVisibleLine = 7
		self.assertEqual(self.ed.FirstVisibleLine, 7)

class TestSearch(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.InsertText(0, b"a\tbig boat\t")

	def testFind(self):
		pos = self.ed.FindBytes(0, self.ed.Length, b"zzz", 0)
		self.assertEqual(pos, -1)
		pos = self.ed.FindBytes(0, self.ed.Length, b"big", 0)
		self.assertEqual(pos, 2)

	def testFindFull(self):
		pos = self.ed.FindBytesFull(0, self.ed.Length, b"zzz", 0)
		self.assertEqual(pos, -1)
		pos = self.ed.FindBytesFull(0, self.ed.Length, b"big", 0)
		self.assertEqual(pos, 2)

	def testFindEmpty(self):
		pos = self.ed.FindBytes(0, self.ed.Length, b"", 0)
		self.assertEqual(pos, 0)

	def testCaseFind(self):
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"big", 0), 2)
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"bIg", 0), 2)
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"bIg",
			self.ed.SCFIND_MATCHCASE), -1)

	def testWordFind(self):
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"bi", 0), 2)
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"bi",
			self.ed.SCFIND_WHOLEWORD), -1)

	def testWordStartFind(self):
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"bi", 0), 2)
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"bi",
			self.ed.SCFIND_WORDSTART), 2)
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"ig", 0), 3)
		self.assertEqual(self.ed.FindBytes(0, self.ed.Length, b"ig",
			self.ed.SCFIND_WORDSTART), -1)

	def testREFind(self):
		flags = self.ed.SCFIND_REGEXP
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"b.g", 0))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"b.g", flags))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, rb"\<b.g\>", flags))
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"b[A-Z]g",
			flags | self.ed.SCFIND_MATCHCASE))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]g", flags))
		self.assertEqual(6, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]*t", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"^a", flags))
		self.assertEqual(10, self.ed.FindBytes(0, self.ed.Length, b"\t$", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"\\([a]\\).*\0", flags))

	def testPosixREFind(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_POSIX
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"b.g", 0))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"b.g", flags))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, rb"\<b.g\>", flags))
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"b[A-Z]g",
			flags | self.ed.SCFIND_MATCHCASE))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]g", flags))
		self.assertEqual(6, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]*t", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"^a", flags))
		self.assertEqual(10, self.ed.FindBytes(0, self.ed.Length, b"\t$", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"([a]).*\0", flags))

	def testCxx11REFind(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_CXX11REGEX
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"b.g", 0))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"b.g", flags))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, rb"\bb.g\b", flags))
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"b[A-Z]g",
			flags | self.ed.SCFIND_MATCHCASE))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]g", flags))
		self.assertEqual(6, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]*t", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"^a", flags))
		self.assertEqual(10, self.ed.FindBytes(0, self.ed.Length, b"\t$", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"([a]).*\0", flags))

	def testCxx11RETooMany(self):
		# For bug #2281
		self.ed.InsertText(0, b"3ringsForTheElvenKing")
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_CXX11REGEX
		# Only MAXTAG (10) matches allocated, but doesn't modify a vulnerable address until 15
		pattern = b"(.)" * 15
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, pattern, flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, pattern, flags))

	def testPhilippeREFind(self):
		# Requires 1.,72
		flags = self.ed.SCFIND_REGEXP
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, rb"\w", flags))
		self.assertEqual(1, self.ed.FindBytes(0, self.ed.Length, rb"\W", flags))
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, rb"\d", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, rb"\D", flags))
		self.assertEqual(1, self.ed.FindBytes(0, self.ed.Length, rb"\s", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, rb"\S", flags))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"\x62", flags))

	def testRENonASCII(self):
		self.ed.InsertText(0, b"\xAD")
		flags = self.ed.SCFIND_REGEXP
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"\\x10", flags))
		self.assertEqual(2, self.ed.FindBytes(0, self.ed.Length, b"\\x09", flags))
		self.assertEqual(-1, self.ed.FindBytes(0, self.ed.Length, b"\\xAB", flags))
		self.assertEqual(0, self.ed.FindBytes(0, self.ed.Length, b"\\xAD", flags))

	def testMultipleAddSelection(self):
		# Find both 'a'
		self.assertEqual(self.ed.MultipleSelection, 0)
		self.ed.MultipleSelection = 1
		self.assertEqual(self.ed.MultipleSelection, 1)
		self.ed.TargetWholeDocument()
		self.ed.SearchFlags = 0
		self.ed.SetSelection(1, 0)
		self.assertEqual(self.ed.Selections, 1)
		self.ed.MultipleSelectAddNext()
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(1), 8)
		self.assertEqual(self.ed.GetSelectionNCaret(1), 9)
		self.ed.MultipleSelection = 0

	def testMultipleAddEachSelection(self):
		# Find each 'b'
		self.assertEqual(self.ed.MultipleSelection, 0)
		self.ed.MultipleSelection = 1
		self.assertEqual(self.ed.MultipleSelection, 1)
		self.ed.TargetWholeDocument()
		self.ed.SearchFlags = 0
		self.ed.SetSelection(3, 2)
		self.assertEqual(self.ed.Selections, 1)
		self.ed.MultipleSelectAddEach()
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 3)
		self.assertEqual(self.ed.GetSelectionNAnchor(1), 6)
		self.assertEqual(self.ed.GetSelectionNCaret(1), 7)
		self.ed.MultipleSelection = 0

class TestRepresentations(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testGetControl(self):
		result = self.ed.GetRepresentation(b"\001")
		self.assertEqual(result, b"SOH")

	def testClearControl(self):
		result = self.ed.GetRepresentation(b"\002")
		self.assertEqual(result, b"STX")
		self.ed.ClearRepresentation(b"\002")
		result = self.ed.GetRepresentation(b"\002")
		self.assertEqual(result, b"")

	def testSetOhm(self):
		ohmSign = b"\xe2\x84\xa6"
		ohmExplained = b"U+2126 \xe2\x84\xa6"
		self.ed.SetRepresentation(ohmSign, ohmExplained)
		result = self.ed.GetRepresentation(ohmSign)
		self.assertEqual(result, ohmExplained)

	def testNul(self):
		self.ed.SetRepresentation(b"", b"Nul")
		result = self.ed.GetRepresentation(b"")
		self.assertEqual(result, b"Nul")

	def testAppearance(self):
		ohmSign = b"\xe2\x84\xa6"
		ohmExplained = b"U+2126 \xe2\x84\xa6"
		self.ed.SetRepresentation(ohmSign, ohmExplained)
		result = self.ed.GetRepresentationAppearance(ohmSign)
		self.assertEqual(result, self.ed.SC_REPRESENTATION_BLOB)
		self.ed.SetRepresentationAppearance(ohmSign, self.ed.SC_REPRESENTATION_PLAIN)
		result = self.ed.GetRepresentationAppearance(ohmSign)
		self.assertEqual(result, self.ed.SC_REPRESENTATION_PLAIN)

	def testColour(self):
		ohmSign = b"\xe2\x84\xa6"
		ohmExplained = b"U+2126 \xe2\x84\xa6"
		self.ed.SetRepresentation(ohmSign, ohmExplained)
		result = self.ed.GetRepresentationColour(ohmSign)
		self.assertEqual(result, 0)
		self.ed.SetRepresentationColour(ohmSign, 0x10203040)
		result = self.ed.GetRepresentationColour(ohmSign)
		self.assertEqual(result, 0x10203040)

@unittest.skipUnless(lexersAvailable, "no lexers included")
class TestProperties(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testSet(self):
		self.xite.ChooseLexer(b"cpp")
		# For Lexilla, only known property names may work
		propName = b"lexer.cpp.allow.dollars"
		self.ed.SetProperty(propName, b"1")
		self.assertEqual(self.ed.GetPropertyInt(propName), 1)
		result = self.ed.GetProperty(propName)
		self.assertEqual(result, b"1")
		self.ed.SetProperty(propName, b"0")
		self.assertEqual(self.ed.GetPropertyInt(propName), 0)
		result = self.ed.GetProperty(propName)
		self.assertEqual(result, b"0")
		# No longer expands but should at least return the value
		result = self.ed.GetPropertyExpanded(propName)
		self.assertEqual(result, b"0")

class TestTextMargin(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.txt = b"abcd"
		self.ed.AddText(1, b"x")

	def testAscent(self):
		lineHeight = self.ed.TextHeight(0)
		self.assertEqual(self.ed.ExtraAscent, 0)
		self.assertEqual(self.ed.ExtraDescent, 0)
		self.ed.ExtraAscent = 1
		self.assertEqual(self.ed.ExtraAscent, 1)
		self.ed.ExtraDescent = 2
		self.assertEqual(self.ed.ExtraDescent, 2)
		lineHeightIncreased = self.ed.TextHeight(0)
		self.assertEqual(lineHeightIncreased, lineHeight + 2 + 1)

	def testTextMargin(self):
		self.ed.MarginSetText(0, self.txt)
		result = self.ed.MarginGetText(0)
		self.assertEqual(result, self.txt)
		self.ed.MarginTextClearAll()

	def testTextMarginStyle(self):
		self.ed.MarginSetText(0, self.txt)
		self.ed.MarginSetStyle(0, 33)
		self.assertEqual(self.ed.MarginGetStyle(0), 33)
		self.ed.MarginTextClearAll()

	def testTextMarginStyles(self):
		styles = b"\001\002\003\004"
		self.ed.MarginSetText(0, self.txt)
		self.ed.MarginSetStyles(0, styles)
		result = self.ed.MarginGetStyles(0)
		self.assertEqual(result, styles)
		self.ed.MarginTextClearAll()

	def testTextMarginStyleOffset(self):
		self.ed.MarginSetStyleOffset(300)
		self.assertEqual(self.ed.MarginGetStyleOffset(), 300)

class TestAnnotation(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.txt = b"abcd"
		self.ed.AddText(1, b"x")

	def testTextAnnotation(self):
		self.assertEqual(self.ed.AnnotationGetLines(), 0)
		self.ed.AnnotationSetText(0, self.txt)
		self.assertEqual(self.ed.AnnotationGetLines(), 1)
		result = self.ed.AnnotationGetText(0)
		self.assertEqual(len(result), 4)
		self.assertEqual(result, self.txt)
		self.ed.AnnotationClearAll()

	def testTextAnnotationStyle(self):
		self.ed.AnnotationSetText(0, self.txt)
		self.ed.AnnotationSetStyle(0, 33)
		self.assertEqual(self.ed.AnnotationGetStyle(0), 33)
		self.ed.AnnotationClearAll()

	def testTextAnnotationStyles(self):
		styles = b"\001\002\003\004"
		self.ed.AnnotationSetText(0, self.txt)
		self.ed.AnnotationSetStyles(0, styles)
		result = self.ed.AnnotationGetStyles(0)
		self.assertEqual(result, styles)
		self.ed.AnnotationClearAll()

	def testExtendedStyles(self):
		start0 = self.ed.AllocateExtendedStyles(0)
		self.assertEqual(start0, 256)
		start1 = self.ed.AllocateExtendedStyles(10)
		self.assertEqual(start1, 256)
		start2 = self.ed.AllocateExtendedStyles(20)
		self.assertEqual(start2, start1 + 10)
		# Reset by changing lexer
		self.ed.ReleaseAllExtendedStyles()
		start0 = self.ed.AllocateExtendedStyles(0)
		self.assertEqual(start0, 256)

	def testTextAnnotationStyleOffset(self):
		self.ed.AnnotationSetStyleOffset(300)
		self.assertEqual(self.ed.AnnotationGetStyleOffset(), 300)

	def testTextAnnotationVisible(self):
		self.assertEqual(self.ed.AnnotationGetVisible(), 0)
		self.ed.AnnotationSetVisible(2)
		self.assertEqual(self.ed.AnnotationGetVisible(), 2)
		self.ed.AnnotationSetVisible(0)

def selectionPositionRepresentation(selectionPosition):
	position, virtualSpace = selectionPosition
	representation = str(position)
	if virtualSpace > 0:
		representation += "+" + str(virtualSpace) + "v"
	return representation

def selectionRangeRepresentation(selectionRange):
	anchor, caret = selectionRange
	return selectionPositionRepresentation(anchor) + "-" + selectionPositionRepresentation(caret)

def selectionRepresentation(ed, n):
	anchor = (ed.GetSelectionNAnchor(n), ed.GetSelectionNAnchorVirtualSpace(n))
	caret = (ed.GetSelectionNCaret(n), ed.GetSelectionNCaretVirtualSpace(n))
	return selectionRangeRepresentation((anchor, caret))

def allSelectionsRepresentation(ed):
	reps = [selectionRepresentation(ed, i) for i in range(ed.Selections)]
	return ';'.join(reps)

class TestMultiSelection(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 3 lines of 3 characters
		self.ed.EOLMode = self.ed.SC_EOL_CRLF
		t = b"xxx\nxxx\nxxx"
		self.ed.AddText(len(t), t)

	def textOfSelection(self, n):
		self.ed.TargetStart = self.ed.GetSelectionNStart(n)
		self.ed.TargetEnd = self.ed.GetSelectionNEnd(n)
		return bytes(self.ed.GetTargetText())

	def testSelectionCleared(self):
		self.ed.ClearSelections()
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 0)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 0)

	def test1Selection(self):
		self.ed.SetSelection(1, 2)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEqual(self.ed.GetSelectionNStart(0), 1)
		self.assertEqual(self.ed.GetSelectionNEnd(0), 2)
		self.ed.SwapMainAnchorCaret()
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)

	def test1SelectionReversed(self):
		self.ed.SetSelection(2, 1)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.assertEqual(self.ed.GetSelectionNStart(0), 1)
		self.assertEqual(self.ed.GetSelectionNEnd(0), 2)

	def test1SelectionByStartEnd(self):
		self.ed.SetSelectionNStart(0, 2)
		self.ed.SetSelectionNEnd(0, 3)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 3)
		self.assertEqual(self.ed.GetSelectionNStart(0), 2)
		self.assertEqual(self.ed.GetSelectionNEnd(0), 3)

	def test2Selections(self):
		self.ed.SetSelection(1, 2)
		self.ed.AddSelection(4, 5)
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.ed.MainSelection, 1)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEqual(self.ed.GetSelectionNCaret(1), 4)
		self.assertEqual(self.ed.GetSelectionNAnchor(1), 5)
		self.assertEqual(self.ed.GetSelectionNStart(0), 1)
		self.assertEqual(self.ed.GetSelectionNEnd(0), 2)
		self.ed.MainSelection = 0
		self.assertEqual(self.ed.MainSelection, 0)
		self.ed.RotateSelection()
		self.assertEqual(self.ed.MainSelection, 1)

	def testRectangularSelection(self):
		self.ed.RectangularSelectionAnchor = 1
		self.assertEqual(self.ed.RectangularSelectionAnchor, 1)
		self.ed.RectangularSelectionCaret = 10
		self.assertEqual(self.ed.RectangularSelectionCaret, 10)
		self.assertEqual(self.ed.Selections, 3)
		self.assertEqual(self.ed.MainSelection, 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(1), 5)
		self.assertEqual(self.ed.GetSelectionNCaret(1), 6)
		self.assertEqual(self.ed.GetSelectionNAnchor(2), 9)
		self.assertEqual(self.ed.GetSelectionNCaret(2), 10)

	def testRectangularCopy(self):
		self.ed.RectangularSelectionAnchor = 1
		self.assertEqual(self.ed.RectangularSelectionAnchor, 1)
		self.ed.RectangularSelectionCaret = 10
		self.assertEqual(self.ed.RectangularSelectionCaret, 10)
		self.assertEqual(self.ed.Selections, 3)
		self.ed.Copy()
		self.ed.ClearAll()
		self.ed.Paste()
		# Single character slice with current line ends
		result = b"x\r\nx\r\nx"
		self.assertEqual(self.ed.Contents(), result)

	def testMultipleCopy(self):
		self.ed.SetContents(b"abc\n123\nxyz")
		self.ed.SetSelection(4, 5)	# 1
		self.ed.AddSelection(1, 3) 	# bc
		self.ed.AddSelection(10, 11)	# z
		self.ed.Copy()
		# 1,bc,z
		self.ed.ClearAll()
		self.ed.Paste()
		self.assertEqual(self.ed.Contents(), b"1bcz")

	def testCopySeparator(self):
		self.assertEqual(self.ed.GetCopySeparator(), b"")
		self.ed.CopySeparator = b"_"
		self.assertEqual(self.ed.GetCopySeparator(), b"_")
		self.ed.SetContents(b"abc\n123\nxyz")
		self.ed.SetSelection(4, 5)	# 1
		self.ed.AddSelection(1, 3) 	# bc
		self.ed.AddSelection(10, 11)	# z
		self.ed.Copy()
		self.ed.ClearAll()
		self.ed.Paste()
		# 1,bc,z separated by _
		self.assertEqual(self.ed.Contents(), b"1_bc_z")
		self.ed.CopySeparator = b""

	def testPasteConversion(self):
		# Test that line ends are converted to current mode
		self.ed.SetSelection(0, 11)
		self.ed.Copy()

		self.ed.ClearAll()
		self.ed.EOLMode = self.ed.SC_EOL_CRLF
		self.ed.Paste()
		self.assertEqual(self.ed.Contents(), b"xxx\r\nxxx\r\nxxx")

		self.ed.ClearAll()
		self.ed.EOLMode = self.ed.SC_EOL_CR
		self.ed.Paste()
		self.assertEqual(self.ed.Contents(), b"xxx\rxxx\rxxx")

		self.ed.ClearAll()
		self.ed.EOLMode = self.ed.SC_EOL_LF
		self.ed.Paste()
		self.assertEqual(self.ed.Contents(), b"xxx\nxxx\nxxx")

	def testVirtualSpace(self):
		self.ed.SetSelection(3, 7)
		self.ed.SetSelectionNCaretVirtualSpace(0, 3)
		self.assertEqual(self.ed.GetSelectionNCaretVirtualSpace(0), 3)
		self.ed.SetSelectionNAnchorVirtualSpace(0, 2)
		self.assertEqual(self.ed.GetSelectionNAnchorVirtualSpace(0), 2)
		self.assertEqual(self.ed.GetSelectionNStartVirtualSpace(0), 3)
		self.assertEqual(self.ed.GetSelectionNEndVirtualSpace(0), 2)
		# Does not check that virtual space is valid by being at end of line
		self.ed.SetSelection(1, 1)
		self.ed.SetSelectionNCaretVirtualSpace(0, 3)
		self.assertEqual(self.ed.GetSelectionNCaretVirtualSpace(0), 3)
		self.assertEqual(self.ed.GetSelectionNStartVirtualSpace(0), 0)
		self.assertEqual(self.ed.GetSelectionNEndVirtualSpace(0), 3)

	def testRectangularVirtualSpace(self):
		self.ed.VirtualSpaceOptions=1
		self.ed.RectangularSelectionAnchor = 3
		self.assertEqual(self.ed.RectangularSelectionAnchor, 3)
		self.ed.RectangularSelectionCaret = 7
		self.assertEqual(self.ed.RectangularSelectionCaret, 7)
		self.ed.RectangularSelectionAnchorVirtualSpace = 1
		self.assertEqual(self.ed.RectangularSelectionAnchorVirtualSpace, 1)
		self.ed.RectangularSelectionCaretVirtualSpace = 10
		self.assertEqual(self.ed.RectangularSelectionCaretVirtualSpace, 10)
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.ed.MainSelection, 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 3)
		self.assertEqual(self.ed.GetSelectionNAnchorVirtualSpace(0), 1)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 3)
		self.assertEqual(self.ed.GetSelectionNCaretVirtualSpace(0), 10)

	def testRectangularVirtualSpaceOptionOff(self):
		# Same as previous test but virtual space option off so no virtual space in result
		self.ed.VirtualSpaceOptions=0
		self.ed.RectangularSelectionAnchor = 3
		self.assertEqual(self.ed.RectangularSelectionAnchor, 3)
		self.ed.RectangularSelectionCaret = 7
		self.assertEqual(self.ed.RectangularSelectionCaret, 7)
		self.ed.RectangularSelectionAnchorVirtualSpace = 1
		self.assertEqual(self.ed.RectangularSelectionAnchorVirtualSpace, 1)
		self.ed.RectangularSelectionCaretVirtualSpace = 10
		self.assertEqual(self.ed.RectangularSelectionCaretVirtualSpace, 10)
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.ed.MainSelection, 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 3)
		self.assertEqual(self.ed.GetSelectionNAnchorVirtualSpace(0), 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 3)
		self.assertEqual(self.ed.GetSelectionNCaretVirtualSpace(0), 0)

	def testDropSelectionN(self):
		self.ed.SetSelection(1, 2)
		# Only one so dropping has no effect
		self.ed.DropSelectionN(0)
		self.assertEqual(self.ed.Selections, 1)
		self.ed.AddSelection(4, 5)
		self.assertEqual(self.ed.Selections, 2)
		# Outside bounds so no effect
		self.ed.DropSelectionN(2)
		self.assertEqual(self.ed.Selections, 2)
		# Dropping before main so main decreases
		self.ed.DropSelectionN(0)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 4)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 5)

		self.ed.AddSelection(10, 11)
		self.ed.AddSelection(20, 21)
		self.assertEqual(self.ed.Selections, 3)
		self.assertEqual(self.ed.MainSelection, 2)
		self.ed.MainSelection = 1
		# Dropping after main so main does not change
		self.ed.DropSelectionN(2)
		self.assertEqual(self.ed.MainSelection, 1)

		# Dropping first selection so wraps around to new last.
		self.ed.AddSelection(30, 31)
		self.ed.AddSelection(40, 41)
		self.assertEqual(self.ed.Selections, 4)
		self.ed.MainSelection = 0
		self.ed.DropSelectionN(0)
		self.assertEqual(self.ed.MainSelection, 2)

	def partFromSelection(self, n):
		# Return a tuple (order, text) from a selection part
		# order is a boolean whether the caret is before the anchor
		self.ed.TargetStart = self.ed.GetSelectionNStart(n)
		self.ed.TargetEnd = self.ed.GetSelectionNEnd(n)
		return (self.ed.GetSelectionNCaret(n) < self.ed.GetSelectionNAnchor(n), self.textOfSelection(n))

	def replacePart(self, n, part):
		startSelection = self.ed.GetSelectionNStart(n)
		self.ed.TargetStart = startSelection
		self.ed.TargetEnd = self.ed.GetSelectionNEnd(n)
		direction, text = part
		length = len(text)
		self.ed.ReplaceTarget(len(text), text)
		if direction:
			self.ed.SetSelectionNCaret(n, startSelection)
			self.ed.SetSelectionNAnchor(n, startSelection + length)
		else:
			self.ed.SetSelectionNAnchor(n, startSelection)
			self.ed.SetSelectionNCaret(n, startSelection + length)

	def swapSelections(self):
		# Swap the selections
		part0 = self.partFromSelection(0)
		part1 = self.partFromSelection(1)

		self.replacePart(1, part0)
		self.replacePart(0, part1)

	def checkAdjacentSelections(self, selections, invert):
		# Only called from testAdjacentSelections to try one permutation
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		t = b"ab"
		texts = (b'b', b'a') if invert else (b'a', b'b')
		self.ed.AddText(len(t), t)
		sel0, sel1 = selections
		self.ed.SetSelection(sel0[0], sel0[1])
		self.ed.AddSelection(sel1[0], sel1[1])
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.textOfSelection(0), texts[0])
		self.assertEqual(self.textOfSelection(1), texts[1])
		self.swapSelections()
		self.assertEqual(self.ed.Contents(), b'ba')
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.textOfSelection(0), texts[1])
		self.assertEqual(self.textOfSelection(1), texts[0])

	def testAdjacentSelections(self):
		# For various permutations of selections, try swapping the text and ensure that the
		# selections remain distinct
		self.checkAdjacentSelections(((1, 0),(1, 2)), False)
		self.checkAdjacentSelections(((0, 1),(1, 2)), False)
		self.checkAdjacentSelections(((1, 0),(2, 1)), False)
		self.checkAdjacentSelections(((0, 1),(2, 1)), False)

		# Reverse order, first selection is after second
		self.checkAdjacentSelections(((1, 2),(1, 0)), True)
		self.checkAdjacentSelections(((1, 2),(0, 1)), True)
		self.checkAdjacentSelections(((2, 1),(1, 0)), True)
		self.checkAdjacentSelections(((2, 1),(0, 1)), True)

	def testInsertBefore(self):
		self.ed.ClearAll()
		t = b"a"
		self.ed.AddText(len(t), t)
		self.ed.SetSelection(0, 1)
		self.assertEqual(self.textOfSelection(0), b'a')

		self.ed.SetTargetRange(0, 0)
		self.ed.ReplaceTarget(1, b'1')
		self.assertEqual(self.ed.Contents(), b'1a')
		self.assertEqual(self.textOfSelection(0), b'a')

	def testInsertAfter(self):
		self.ed.ClearAll()
		t = b"a"
		self.ed.AddText(len(t), t)
		self.ed.SetSelection(0, 1)
		self.assertEqual(self.textOfSelection(0), b'a')

		self.ed.SetTargetRange(1, 1)
		self.ed.ReplaceTarget(1, b'9')
		self.assertEqual(self.ed.Contents(), b'a9')
		self.assertEqual(self.textOfSelection(0), b'a')

	def testInsertBeforeVirtualSpace(self):
		self.ed.SetContents(b"a")
		self.ed.SetSelection(1, 1)
		self.ed.SetSelectionNAnchorVirtualSpace(0, 2)
		self.ed.SetSelectionNCaretVirtualSpace(0, 2)
		self.assertEqual(selectionRepresentation(self.ed, 0), "1+2v-1+2v")
		self.assertEqual(self.textOfSelection(0), b'')

		# Append '1'
		self.ed.SetTargetRange(1, 1)
		self.ed.ReplaceTarget(1, b'1')
		# Selection moved on 1, but still empty
		self.assertEqual(selectionRepresentation(self.ed, 0), "2+1v-2+1v")
		self.assertEqual(self.ed.Contents(), b'a1')
		self.assertEqual(self.textOfSelection(0), b'')

	def testInsertThroughVirtualSpace(self):
		self.ed.SetContents(b"a")
		self.ed.SetSelection(1, 1)
		self.ed.SetSelectionNAnchorVirtualSpace(0, 2)
		self.ed.SetSelectionNCaretVirtualSpace(0, 3)
		self.assertEqual(selectionRepresentation(self.ed, 0), "1+2v-1+3v")
		self.assertEqual(self.textOfSelection(0), b'')

		# Append '1' past current virtual space
		self.ed.SetTargetRange(1, 1)
		self.ed.SetTargetStartVirtualSpace(4)
		self.ed.SetTargetEndVirtualSpace(5)
		self.ed.ReplaceTarget(1, b'1')
		# Virtual space of selection all converted to real positions
		self.assertEqual(selectionRepresentation(self.ed, 0), "3-4")
		self.assertEqual(self.ed.Contents(), b'a    1')
		self.assertEqual(self.textOfSelection(0), b' ')

	def testSelectionSerialization(self):
		self.ed.SetContents(b"a")
		self.ed.SetSelection(0, 1)
		self.assertEqual(self.ed.GetSelectionSerialized(), b'1-0')
		self.ed.SetSelection(1, 1)
		self.assertEqual(self.ed.GetSelectionSerialized(), b'1')
		self.ed.SetSelectionNAnchorVirtualSpace(0, 2)
		self.ed.SetSelectionNCaretVirtualSpace(0, 3)
		self.assertEqual(selectionRepresentation(self.ed, 0), "1+2v-1+3v")
		self.assertEqual(self.textOfSelection(0), b'')
		self.assertEqual(self.ed.GetSelectionSerialized(), b'1v2-1v3')
		self.ed.SetSelectionSerialized(0, b'1-0')
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.Anchor, 1)
		self.assertEqual(self.ed.CurrentPos, 0)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 0)

	def testSelectionSerializationOutOfBounds(self):
		# Try setting selections that extend past document end through serialized form
		# and check that the selection is limited to the document.
		self.ed.SetContents(b"a")
		self.ed.SetSelectionSerialized(0, b'200-0')
		self.assertEqual(self.ed.GetSelectionSerialized(), b'1-0')
		
		# Retain virtual space
		self.ed.SetSelectionSerialized(0, b'0v1-200')
		self.assertEqual(self.ed.GetSelectionSerialized(), b'0v1-1')

		# Drop identical ranges, but touching empty range survives
		self.ed.SetSelectionSerialized(0, b'0-200,300-400,500-600')
		self.assertEqual(self.ed.GetSelectionSerialized(), b'0-1,1')


class TestModalSelection(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 3 lines of 3 characters
		t = b"xxx\nxxx\nxxx"
		self.ed.AddText(len(t), t)

	def testCharacterSelection(self):
		self.ed.SetSelection(1, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(allSelectionsRepresentation(self.ed), "1-1")
		self.ed.SelectionMode = self.ed.SC_SEL_STREAM
		self.assertEqual(self.ed.GetSelectionMode(), self.ed.SC_SEL_STREAM)
		self.assertEqual(self.ed.MoveExtendsSelection, True)
		self.assertEqual(allSelectionsRepresentation(self.ed), "1-1")
		self.ed.CharRight()
		self.assertEqual(allSelectionsRepresentation(self.ed), "1-2")
		self.ed.LineDown()
		self.assertEqual(allSelectionsRepresentation(self.ed), "1-6")
		self.ed.ClearSelections()

	def testChangeSelectionMode(self):
		# Like testCharacterSelection but calling ChangeSelectionMode instead of SetSelectionMode
		self.ed.SetSelection(1, 1)
		self.assertEqual(allSelectionsRepresentation(self.ed), "1-1")
		self.ed.ChangeSelectionMode(self.ed.SC_SEL_STREAM)
		self.assertEqual(self.ed.GetSelectionMode(), self.ed.SC_SEL_STREAM)
		self.assertEqual(self.ed.MoveExtendsSelection, False)
		self.assertEqual(allSelectionsRepresentation(self.ed), "1-1")
		self.ed.CharRight()
		self.assertEqual(allSelectionsRepresentation(self.ed), "2-2")
		self.ed.LineDown()
		self.assertEqual(allSelectionsRepresentation(self.ed), "6-6")
		self.ed.ClearSelections()

	def testTurningOffMoveExtendsSelection(self):
		self.ed.SetSelection(1, 1)
		self.ed.SelectionMode = self.ed.SC_SEL_STREAM
		self.ed.CharRight()
		self.ed.LineDown()
		self.assertEqual(self.ed.MoveExtendsSelection, True)
		self.ed.MoveExtendsSelection = False
		self.assertEqual(self.ed.MoveExtendsSelection, False)
		self.ed.CharRight()
		self.assertEqual(allSelectionsRepresentation(self.ed), "6-6")
		self.ed.CharRight()
		self.assertEqual(allSelectionsRepresentation(self.ed), "7-7")
		self.ed.ClearSelections()

	def testRectangleSelection(self):
		self.ed.SetSelection(1, 1)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.ed.SelectionMode = self.ed.SC_SEL_RECTANGLE
		self.assertEqual(self.ed.GetSelectionMode(), self.ed.SC_SEL_RECTANGLE)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.ed.CharRight()
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.ed.LineDown()
		self.assertEqual(self.ed.Selections, 2)
		self.assertEqual(self.ed.MainSelection, 1)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 2)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.assertEqual(self.ed.GetSelectionNCaret(1), 6)
		self.assertEqual(self.ed.GetSelectionNAnchor(1), 5)
		self.ed.ClearSelections()

	def testLinesSelection(self):
		self.ed.SetSelection(1, 1)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 1)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 1)
		self.ed.SelectionMode = self.ed.SC_SEL_LINES
		self.assertEqual(self.ed.GetSelectionMode(), self.ed.SC_SEL_LINES)
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 0)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 3)
		self.ed.CharRight()
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 0)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 3)
		self.ed.LineDown()
		self.assertEqual(self.ed.Selections, 1)
		self.assertEqual(self.ed.MainSelection, 0)
		self.assertEqual(self.ed.GetSelectionNCaret(0), 7)
		self.assertEqual(self.ed.GetSelectionNAnchor(0), 0)
		self.ed.ClearSelections()

class TestTechnology(unittest.TestCase):
	""" These tests are just to ensure that the calls set and retrieve values.
	They assume running on a Direct2D compatible version of Windows.
	They do not check visual appearance.
	"""
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testTechnologyAndBufferedDraw(self):
		self.ed.Technology = self.ed.SC_TECHNOLOGY_DEFAULT
		self.assertEqual(self.ed.GetTechnology(), self.ed.SC_TECHNOLOGY_DEFAULT)
		if sys.platform == "win32":
			self.ed.Technology = self.ed.SC_TECHNOLOGY_DIRECTWRITE
			self.assertEqual(self.ed.GetTechnology(), self.ed.SC_TECHNOLOGY_DIRECTWRITE)
			self.assertEqual(self.ed.BufferedDraw, False)
			self.ed.Technology = self.ed.SC_TECHNOLOGY_DEFAULT
			self.assertEqual(self.ed.GetTechnology(), self.ed.SC_TECHNOLOGY_DEFAULT)
			self.assertEqual(self.ed.BufferedDraw, True)

class TestStyleAttributes(unittest.TestCase):
	""" These tests are just to ensure that the calls set and retrieve values.
	They do not check the visual appearance of the style attributes.
	"""
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.testColour = 0x171615
		self.testFont = b"Georgia"
		self.testRepresentation = "\N{BULLET}".encode("utf-8")

	def tearDown(self):
		self.ed.StyleResetDefault()

	def testFont(self):
		self.ed.StyleSetFont(self.ed.STYLE_DEFAULT, self.testFont)
		self.assertEqual(self.ed.StyleGetFont(self.ed.STYLE_DEFAULT), self.testFont)

	def testSize(self):
		self.ed.StyleSetSize(self.ed.STYLE_DEFAULT, 12)
		self.assertEqual(self.ed.StyleGetSize(self.ed.STYLE_DEFAULT), 12)
		self.assertEqual(self.ed.StyleGetSizeFractional(self.ed.STYLE_DEFAULT), 12*self.ed.SC_FONT_SIZE_MULTIPLIER)
		self.ed.StyleSetSizeFractional(self.ed.STYLE_DEFAULT, 1234)
		self.assertEqual(self.ed.StyleGetSizeFractional(self.ed.STYLE_DEFAULT), 1234)

	def testInvisibleRepresentation(self):
		self.assertEqual(self.ed.StyleGetInvisibleRepresentation(self.ed.STYLE_DEFAULT), b"")
		self.ed.StyleSetInvisibleRepresentation(self.ed.STYLE_DEFAULT, self.testRepresentation)
		self.assertEqual(self.ed.StyleGetInvisibleRepresentation(self.ed.STYLE_DEFAULT), self.testRepresentation)
		self.ed.StyleSetInvisibleRepresentation(self.ed.STYLE_DEFAULT, b"\000")
		self.assertEqual(self.ed.StyleGetInvisibleRepresentation(self.ed.STYLE_DEFAULT), b"")

	def testBold(self):
		self.ed.StyleSetBold(self.ed.STYLE_DEFAULT, 1)
		self.assertEqual(self.ed.StyleGetBold(self.ed.STYLE_DEFAULT), 1)
		self.assertEqual(self.ed.StyleGetWeight(self.ed.STYLE_DEFAULT), self.ed.SC_WEIGHT_BOLD)
		self.ed.StyleSetWeight(self.ed.STYLE_DEFAULT, 530)
		self.assertEqual(self.ed.StyleGetWeight(self.ed.STYLE_DEFAULT), 530)

	def testItalic(self):
		self.ed.StyleSetItalic(self.ed.STYLE_DEFAULT, 1)
		self.assertEqual(self.ed.StyleGetItalic(self.ed.STYLE_DEFAULT), 1)

	def testUnderline(self):
		self.assertEqual(self.ed.StyleGetUnderline(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetUnderline(self.ed.STYLE_DEFAULT, 1)
		self.assertEqual(self.ed.StyleGetUnderline(self.ed.STYLE_DEFAULT), 1)

	def testFore(self):
		self.assertEqual(self.ed.StyleGetFore(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetFore(self.ed.STYLE_DEFAULT, self.testColour)
		self.assertEqual(self.ed.StyleGetFore(self.ed.STYLE_DEFAULT), self.testColour)

	def testBack(self):
		self.assertEqual(self.ed.StyleGetBack(self.ed.STYLE_DEFAULT), 0xffffff)
		self.ed.StyleSetBack(self.ed.STYLE_DEFAULT, self.testColour)
		self.assertEqual(self.ed.StyleGetBack(self.ed.STYLE_DEFAULT), self.testColour)

	def testEOLFilled(self):
		self.assertEqual(self.ed.StyleGetEOLFilled(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetEOLFilled(self.ed.STYLE_DEFAULT, 1)
		self.assertEqual(self.ed.StyleGetEOLFilled(self.ed.STYLE_DEFAULT), 1)

	def testCharacterSet(self):
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		self.assertEqual(self.ed.StyleGetCharacterSet(self.ed.STYLE_DEFAULT), self.ed.SC_CHARSET_RUSSIAN)

	def testCase(self):
		self.assertEqual(self.ed.StyleGetCase(self.ed.STYLE_DEFAULT), self.ed.SC_CASE_MIXED)
		self.ed.StyleSetCase(self.ed.STYLE_DEFAULT, self.ed.SC_CASE_UPPER)
		self.assertEqual(self.ed.StyleGetCase(self.ed.STYLE_DEFAULT), self.ed.SC_CASE_UPPER)
		self.ed.StyleSetCase(self.ed.STYLE_DEFAULT, self.ed.SC_CASE_LOWER)
		self.assertEqual(self.ed.StyleGetCase(self.ed.STYLE_DEFAULT), self.ed.SC_CASE_LOWER)

	def testVisible(self):
		self.assertEqual(self.ed.StyleGetVisible(self.ed.STYLE_DEFAULT), 1)
		self.ed.StyleSetVisible(self.ed.STYLE_DEFAULT, 0)
		self.assertEqual(self.ed.StyleGetVisible(self.ed.STYLE_DEFAULT), 0)

	def testChangeable(self):
		self.assertEqual(self.ed.StyleGetChangeable(self.ed.STYLE_DEFAULT), 1)
		self.ed.StyleSetChangeable(self.ed.STYLE_DEFAULT, 0)
		self.assertEqual(self.ed.StyleGetChangeable(self.ed.STYLE_DEFAULT), 0)

	def testHotSpot(self):
		self.assertEqual(self.ed.StyleGetHotSpot(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetHotSpot(self.ed.STYLE_DEFAULT, 1)
		self.assertEqual(self.ed.StyleGetHotSpot(self.ed.STYLE_DEFAULT), 1)

	def testFoldDisplayTextStyle(self):
		self.assertEqual(self.ed.FoldDisplayTextGetStyle(), 0)
		self.ed.FoldDisplayTextSetStyle(self.ed.SC_FOLDDISPLAYTEXT_BOXED)
		self.assertEqual(self.ed.FoldDisplayTextGetStyle(), self.ed.SC_FOLDDISPLAYTEXT_BOXED)

	def testDefaultFoldDisplayText(self):
		self.assertEqual(self.ed.GetDefaultFoldDisplayText(), b"")
		self.ed.SetDefaultFoldDisplayText(0, b"...")
		self.assertEqual(self.ed.GetDefaultFoldDisplayText(), b"...")

	def testFontQuality(self):
		self.assertEqual(self.ed.GetFontQuality(), self.ed.SC_EFF_QUALITY_DEFAULT)
		self.ed.SetFontQuality(self.ed.SC_EFF_QUALITY_LCD_OPTIMIZED)
		self.assertEqual(self.ed.GetFontQuality(), self.ed.SC_EFF_QUALITY_LCD_OPTIMIZED)

	def testFontLocale(self):
		initialLocale = "en-us".encode("UTF-8")
		testLocale = "zh-Hans".encode("UTF-8")
		self.assertEqual(self.ed.GetFontLocale(), initialLocale)
		self.ed.FontLocale = testLocale
		self.assertEqual(self.ed.GetFontLocale(), testLocale)

	def testCheckMonospaced(self):
		self.assertEqual(self.ed.StyleGetCheckMonospaced(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetCheckMonospaced(self.ed.STYLE_DEFAULT, 1)
		self.assertEqual(self.ed.StyleGetCheckMonospaced(self.ed.STYLE_DEFAULT), 1)

class TestElements(unittest.TestCase):
	""" These tests are just to ensure that the calls set and retrieve values.
	They do not check the visual appearance of the style attributes.
	"""
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.testColourAlpha = 0x18171615
		self.opaque = 0xff000000
		self.dropAlpha = 0x00ffffff

	def tearDown(self):
		pass

	def ElementColour(self, element):
		# & 0xffffffff prevents sign extension issues
		return self.ed.GetElementColour(element) & 0xffffffff

	def RestoreCaretLine(self):
		self.ed.CaretLineLayer = 0
		self.ed.CaretLineFrame = 0
		self.ed.ResetElementColour(self.ed.SC_ELEMENT_CARET_LINE_BACK)
		self.ed.CaretLineVisibleAlways = False

	def testIsSet(self):
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_SELECTION_TEXT))

	def testAllowsTranslucent(self):
		self.assertFalse(self.ed.GetElementAllowsTranslucent(self.ed.SC_ELEMENT_LIST))
		self.assertTrue(self.ed.GetElementAllowsTranslucent(self.ed.SC_ELEMENT_SELECTION_TEXT))

	def testChanging(self):
		self.ed.SetElementColour(self.ed.SC_ELEMENT_LIST_BACK, self.testColourAlpha)
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_LIST_BACK), self.testColourAlpha)
		self.assertTrue(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_LIST_BACK))

	def testSubline(self):
		# Default is false
		self.assertEqual(self.ed.CaretLineHighlightSubLine, False)
		self.ed.CaretLineHighlightSubLine = True
		self.assertEqual(self.ed.CaretLineHighlightSubLine, True)
		self.ed.CaretLineHighlightSubLine = False
		self.assertEqual(self.ed.CaretLineHighlightSubLine, False)

	def testReset(self):
		self.ed.SetElementColour(self.ed.SC_ELEMENT_SELECTION_ADDITIONAL_TEXT, self.testColourAlpha)
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_SELECTION_ADDITIONAL_TEXT), self.testColourAlpha)
		self.ed.ResetElementColour(self.ed.SC_ELEMENT_SELECTION_ADDITIONAL_TEXT)
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_SELECTION_ADDITIONAL_TEXT), 0)
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_SELECTION_ADDITIONAL_TEXT))

	def testBaseColour(self):
		if sys.platform == "win32":
			# SC_ELEMENT_LIST* base colours only currently implemented on Win32
			text = self.ed.GetElementBaseColour(self.ed.SC_ELEMENT_LIST)
			back = self.ed.GetElementBaseColour(self.ed.SC_ELEMENT_LIST_BACK)
			self.assertEqual(text & self.opaque, self.opaque)
			self.assertEqual(back & self.opaque, self.opaque)
			self.assertNotEqual(text & self.dropAlpha, back & self.dropAlpha)
			selText = self.ed.GetElementBaseColour(self.ed.SC_ELEMENT_LIST_SELECTED)
			selBack = self.ed.GetElementBaseColour(self.ed.SC_ELEMENT_LIST_SELECTED_BACK)
			self.assertEqual(selText & self.opaque, self.opaque)
			self.assertEqual(selBack & self.opaque, self.opaque)
			self.assertNotEqual(selText & self.dropAlpha, selBack & self.dropAlpha)

	def testSelectionLayer(self):
		self.ed.SelectionLayer = self.ed.SC_LAYER_OVER_TEXT
		self.assertEqual(self.ed.SelectionLayer, self.ed.SC_LAYER_OVER_TEXT)
		self.ed.SelectionLayer = self.ed.SC_LAYER_BASE
		self.assertEqual(self.ed.SelectionLayer, self.ed.SC_LAYER_BASE)

	def testCaretLine(self):
		# Newer Layer / ElementColour API
		self.assertEqual(self.ed.CaretLineLayer, 0)
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_CARET_LINE_BACK))
		self.assertEqual(self.ed.CaretLineFrame, 0)
		self.assertFalse(self.ed.CaretLineVisibleAlways)

		self.ed.CaretLineLayer = 2
		self.assertEqual(self.ed.CaretLineLayer, 2)
		self.ed.CaretLineFrame = 2
		self.assertEqual(self.ed.CaretLineFrame, 2)
		self.ed.CaretLineVisibleAlways = True
		self.assertTrue(self.ed.CaretLineVisibleAlways)
		self.ed.SetElementColour(self.ed.SC_ELEMENT_CARET_LINE_BACK, self.testColourAlpha)
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_CARET_LINE_BACK), self.testColourAlpha)

		self.RestoreCaretLine()

	def testCaretLineLayerDiscouraged(self):
		# Check old discouraged APIs
		# This is s bit tricky as there is no clean mapping: parts of the old state are distributed to
		# sometimes-multiple parts of the new state.
		backColour = 0x102030
		backColourOpaque = backColour | self.opaque
		self.assertEqual(self.ed.CaretLineVisible, 0)
		self.ed.CaretLineVisible = 1
		self.assertTrue(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_CARET_LINE_BACK))
		self.assertEqual(self.ed.CaretLineVisible, 1)
		self.ed.CaretLineBack = backColour
		self.assertEqual(self.ed.CaretLineBack, backColour)
		# Check with newer API
		self.assertTrue(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_CARET_LINE_BACK))
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_CARET_LINE_BACK), backColourOpaque)
		self.assertEqual(self.ed.CaretLineLayer, 0)

		alpha = 0x7f
		self.ed.CaretLineBackAlpha = alpha
		self.assertEqual(self.ed.CaretLineBackAlpha, alpha)
		backColourTranslucent = backColour | (alpha << 24)
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_CARET_LINE_BACK), backColourTranslucent)
		self.assertEqual(self.ed.CaretLineLayer, 2)

		self.ed.CaretLineBackAlpha = 0x100
		self.assertEqual(self.ed.CaretLineBackAlpha, 0x100)
		self.assertEqual(self.ed.CaretLineLayer, 0)	# SC_ALPHA_NOALPHA moved to base layer

		self.RestoreCaretLine()

		# Try other orders

		self.ed.CaretLineBackAlpha = 0x100
		self.assertEqual(self.ed.CaretLineBackAlpha, 0x100)
		self.assertEqual(self.ed.CaretLineLayer, 0)	# SC_ALPHA_NOALPHA moved to base layer
		self.ed.CaretLineBack = backColour
		self.assertTrue(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_CARET_LINE_BACK))
		self.ed.CaretLineVisible = 0
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_CARET_LINE_BACK))

		self.RestoreCaretLine()

	def testMarkerLayer(self):
		self.assertEqual(self.ed.MarkerGetLayer(1), 0)
		self.ed.MarkerSetAlpha(1, 23)
		self.assertEqual(self.ed.MarkerGetLayer(1), 2)
		self.ed.MarkerSetAlpha(1, 0x100)
		self.assertEqual(self.ed.MarkerGetLayer(1), 0)

	def testHotSpot(self):
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE))
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE_BACK))
		self.assertEqual(self.ed.HotspotActiveFore, 0)
		self.assertEqual(self.ed.HotspotActiveBack, 0)

		testColour = 0x804020
		resetColour = 0x112233	# Doesn't get set
		self.ed.SetHotspotActiveFore(1, testColour)
		self.assertEqual(self.ed.HotspotActiveFore, testColour)
		self.assertTrue(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE))
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE), testColour | self.opaque)
		self.ed.SetHotspotActiveFore(0, resetColour)
		self.assertEqual(self.ed.HotspotActiveFore, 0)
		self.assertFalse(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE))
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE), 0)

		translucentColour = 0x50403020
		self.ed.SetElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE, translucentColour)
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE), translucentColour)
		self.assertEqual(self.ed.HotspotActiveFore, translucentColour & self.dropAlpha)

		backColour = 0x204080
		self.ed.SetHotspotActiveBack(1, backColour)
		self.assertEqual(self.ed.HotspotActiveBack, backColour)
		self.assertTrue(self.ed.GetElementIsSet(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE_BACK))
		self.assertEqual(self.ElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE_BACK), backColour | self.opaque)

		# Restore
		self.ed.ResetElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE)
		self.ed.ResetElementColour(self.ed.SC_ELEMENT_HOT_SPOT_ACTIVE_BACK)

	def testHideSelection(self):
		self.assertEqual(self.ed.SelectionHidden, False)
		self.ed.HideSelection(True)
		self.assertEqual(self.ed.SelectionHidden, True)
		self.ed.HideSelection(False)	# Restore
		self.assertEqual(self.ed.SelectionHidden, False)

class TestIndices(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.SetCodePage(65001)
		# Text includes one non-BMP character
		t = "aå\U00010348ﬂﬔ-\n"
		self.tv = t.encode("UTF-8")

	def tearDown(self):
		self.ed.SetCodePage(0)

	def testAllocation(self):
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)
		self.ed.AllocateLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32)
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_UTF32)
		self.ed.ReleaseLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32)
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)

	def testUTF32(self):
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)
		self.ed.SetContents(self.tv)
		self.ed.AllocateLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32)
		self.assertEqual(self.ed.IndexPositionFromLine(0, self.ed.SC_LINECHARACTERINDEX_UTF32), 0)
		self.assertEqual(self.ed.IndexPositionFromLine(1, self.ed.SC_LINECHARACTERINDEX_UTF32), 7)
		self.ed.ReleaseLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32)
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)

	def testUTF16(self):
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)
		self.ed.SetContents(self.tv)
		self.ed.AllocateLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF16)
		self.assertEqual(self.ed.IndexPositionFromLine(0, self.ed.SC_LINECHARACTERINDEX_UTF16), 0)
		self.assertEqual(self.ed.IndexPositionFromLine(1, self.ed.SC_LINECHARACTERINDEX_UTF16), 8)
		self.ed.ReleaseLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF16)
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)

	def testBoth(self):
		# Set text before turning indices on
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)
		self.ed.SetContents(self.tv)
		self.ed.AllocateLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32+self.ed.SC_LINECHARACTERINDEX_UTF16)
		self.assertEqual(self.ed.IndexPositionFromLine(0, self.ed.SC_LINECHARACTERINDEX_UTF32), 0)
		self.assertEqual(self.ed.IndexPositionFromLine(1, self.ed.SC_LINECHARACTERINDEX_UTF32), 7)
		self.assertEqual(self.ed.IndexPositionFromLine(0, self.ed.SC_LINECHARACTERINDEX_UTF16), 0)
		self.assertEqual(self.ed.IndexPositionFromLine(1, self.ed.SC_LINECHARACTERINDEX_UTF16), 8)
		# Test the inverse: position->line
		self.assertEqual(self.ed.LineFromIndexPosition(0, self.ed.SC_LINECHARACTERINDEX_UTF32), 0)
		self.assertEqual(self.ed.LineFromIndexPosition(7, self.ed.SC_LINECHARACTERINDEX_UTF32), 1)
		self.assertEqual(self.ed.LineFromIndexPosition(0, self.ed.SC_LINECHARACTERINDEX_UTF16), 0)
		self.assertEqual(self.ed.LineFromIndexPosition(8, self.ed.SC_LINECHARACTERINDEX_UTF16), 1)
		self.ed.ReleaseLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32+self.ed.SC_LINECHARACTERINDEX_UTF16)
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)

	def testMaintenance(self):
		# Set text after turning indices on
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)
		self.ed.AllocateLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32+self.ed.SC_LINECHARACTERINDEX_UTF16)
		self.ed.SetContents(self.tv)
		self.assertEqual(self.ed.IndexPositionFromLine(0, self.ed.SC_LINECHARACTERINDEX_UTF32), 0)
		self.assertEqual(self.ed.IndexPositionFromLine(1, self.ed.SC_LINECHARACTERINDEX_UTF32), 7)
		self.assertEqual(self.ed.IndexPositionFromLine(0, self.ed.SC_LINECHARACTERINDEX_UTF16), 0)
		self.assertEqual(self.ed.IndexPositionFromLine(1, self.ed.SC_LINECHARACTERINDEX_UTF16), 8)
		self.ed.ReleaseLineCharacterIndex(self.ed.SC_LINECHARACTERINDEX_UTF32+self.ed.SC_LINECHARACTERINDEX_UTF16)
		self.assertEqual(self.ed.GetLineCharacterIndex(), self.ed.SC_LINECHARACTERINDEX_NONE)

class TestCharacterNavigation(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.SetCodePage(65001)

	def tearDown(self):
		self.ed.SetCodePage(0)

	def testBeforeAfter(self):
		t = "aåﬂﬔ-"
		tv = t.encode("UTF-8")
		self.ed.SetContents(tv)
		pos = 0
		for i in range(len(t)-1):
			after = self.ed.PositionAfter(pos)
			self.assertTrue(after > i)
			back = self.ed.PositionBefore(after)
			self.assertEqual(pos, back)
			pos = after

	def testRelative(self):
		# \x61  \xc3\xa5  \xef\xac\x82   \xef\xac\x94   \x2d
		t = "aåﬂﬔ-"
		tv = t.encode("UTF-8")
		self.ed.SetContents(tv)
		self.assertEqual(self.ed.PositionRelative(1, 2), 6)
		self.assertEqual(self.ed.CountCharacters(1, 6), 2)
		self.assertEqual(self.ed.PositionRelative(6, -2), 1)
		pos = 0
		previous = 0
		for i in range(1, len(t)):
			after = self.ed.PositionRelative(pos, i)
			self.assertTrue(after > pos)
			self.assertTrue(after > previous)
			previous = after
		pos = len(t)
		previous = pos
		for i in range(1, len(t)-1):
			after = self.ed.PositionRelative(pos, -i)
			self.assertTrue(after < pos)
			self.assertTrue(after < previous)
			previous = after

	def testRelativeNonBOM(self):
		# \x61  \xF0\x90\x8D\x88  \xef\xac\x82   \xef\xac\x94   \x2d
		t = "a\U00010348ﬂﬔ-"
		tv = t.encode("UTF-8")
		self.ed.SetContents(tv)
		self.assertEqual(self.ed.PositionRelative(1, 2), 8)
		self.assertEqual(self.ed.CountCharacters(1, 8), 2)
		self.assertEqual(self.ed.CountCodeUnits(1, 8), 3)
		self.assertEqual(self.ed.PositionRelative(8, -2), 1)
		self.assertEqual(self.ed.PositionRelativeCodeUnits(8, -3), 1)
		pos = 0
		previous = 0
		for i in range(1, len(t)):
			after = self.ed.PositionRelative(pos, i)
			self.assertTrue(after > pos)
			self.assertTrue(after > previous)
			previous = after
		pos = len(t)
		previous = pos
		for i in range(1, len(t)-1):
			after = self.ed.PositionRelative(pos, -i)
			self.assertTrue(after < pos)
			self.assertTrue(after <= previous)
			previous = after

	def testLineEnd(self):
		t = "a\r\nb\nc"
		tv = t.encode("UTF-8")
		self.ed.SetContents(tv)
		for i in range(0, len(t)):
			self.assertEqual(self.ed.CountCharacters(0, i), i)

class TestCaseMapping(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.SetCodePage(0)
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_DEFAULT)

	def testEmpty(self):
		# Trying to upper case an empty string caused a crash at one stage
		t = b"x"
		self.ed.SetContents(t)
		self.ed.UpperCase()
		self.assertEqual(self.ed.Contents(), b"x")

	def testASCII(self):
		t = b"x"
		self.ed.SetContents(t)
		self.ed.SetSel(0,1)
		self.ed.UpperCase()
		self.assertEqual(self.ed.Contents(), b"X")

	def testLatin1(self):
		t = "å".encode("Latin-1")
		r = "Å".encode("Latin-1")
		self.ed.SetContents(t)
		self.ed.SetSel(0,1)
		self.ed.UpperCase()
		self.assertEqual(self.ed.Contents(), r)

	def testRussian(self):
		if sys.platform == "win32":
			self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		else:
			self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_CYRILLIC)
		t = "Б".encode("Windows-1251")
		r = "б".encode("Windows-1251")
		self.ed.SetContents(t)
		self.ed.SetSel(0,1)
		self.ed.LowerCase()
		self.assertEqual(self.ed.Contents(), r)

	def testUTF(self):
		self.ed.SetCodePage(65001)
		t = "å".encode("UTF-8")
		r = "Å".encode("UTF-8")
		self.ed.SetContents(t)
		self.ed.SetSel(0,2)
		self.ed.UpperCase()
		self.assertEqual(self.ed.Contents(), r)

	def testUTFDifferentLength(self):
		self.ed.SetCodePage(65001)
		t = "ı".encode("UTF-8")
		r = "I".encode("UTF-8")
		self.ed.SetContents(t)
		self.assertEqual(self.ed.Length, 2)
		self.ed.SetSel(0,2)
		self.ed.UpperCase()
		self.assertEqual(self.ed.Length, 1)
		self.assertEqual(self.ed.Contents(), r)

	def testUTFGrows(self):
		# This crashed at one point in debug builds due to looking past end of shorter string
		self.ed.SetCodePage(65001)
		# ﬖ is a single character ligature taking 3 bytes in UTF8: EF AC 96
		t = 'ﬖﬖ'.encode("UTF-8")
		self.ed.SetContents(t)
		self.assertEqual(self.ed.Length, 6)
		self.ed.SetSel(0,self.ed.Length)
		self.ed.UpperCase()
		# To convert to upper case the ligature is separated into վ and ն then uppercased to Վ and Ն
		# each of which takes 2 bytes in UTF-8: D5 8E D5 86
		r = 'ՎՆՎՆ'.encode("UTF-8")
		self.assertEqual(self.ed.Length, 8)
		self.assertEqual(self.ed.Contents(), r)
		self.assertEqual(self.ed.SelectionEnd, self.ed.Length)

	def testUTFShrinks(self):
		self.ed.SetCodePage(65001)
		# ﬁ is a single character ligature taking 3 bytes in UTF8: EF AC 81
		t = 'ﬁﬁ'.encode("UTF-8")
		self.ed.SetContents(t)
		self.assertEqual(self.ed.Length, 6)
		self.ed.SetSel(0,self.ed.Length)
		self.ed.UpperCase()
		# To convert to upper case the ligature is separated into f and i then uppercased to F and I
		# each of which takes 1 byte in UTF-8: 46 49
		r = 'FIFI'.encode("UTF-8")
		self.assertEqual(self.ed.Length, 4)
		self.assertEqual(self.ed.Contents(), r)
		self.assertEqual(self.ed.SelectionEnd, self.ed.Length)

class TestCaseInsensitiveSearch(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.SetCodePage(0)
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_DEFAULT)

	def testEmpty(self):
		text = b" x X"
		searchString = b""
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(0, pos)

	def testASCII(self):
		text = b" x X"
		searchString = b"X"
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(1, pos)

	def testLatin1(self):
		text = "Frånd Åå".encode("Latin-1")
		searchString = "Å".encode("Latin-1")
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(2, pos)

	def testRussian(self):
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		text = "=(Б tex б)".encode("Windows-1251")
		searchString = "б".encode("Windows-1251")
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(2, pos)

	def testUTF(self):
		self.ed.SetCodePage(65001)
		text = "Frånd Åå".encode("UTF-8")
		searchString = "Å".encode("UTF-8")
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(2, pos)

	def testUTFDifferentLength(self):
		# Searching for a two byte string finds a single byte
		self.ed.SetCodePage(65001)
		# two byte string "ſ" single byte "s"
		text = "Frånds Ååſ $".encode("UTF-8")
		searchString = "ſ".encode("UTF-8")
		firstPosition = len("Frånd".encode("UTF-8"))
		self.assertEqual(len(searchString), 2)
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEqual(firstPosition, pos)
		self.assertEqual(firstPosition+1, self.ed.TargetEnd)

@unittest.skipUnless(lexersAvailable, "no lexers included")
class TestLexer(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testLexerNumber(self):
		self.xite.ChooseLexer(b"cpp")
		self.assertEqual(self.ed.GetLexer(), self.ed.SCLEX_CPP)

	def testLexerName(self):
		self.xite.ChooseLexer(b"cpp")
		self.assertEqual(self.ed.GetLexer(), self.ed.SCLEX_CPP)
		name = self.ed.GetLexerLanguage(0)
		self.assertEqual(name, b"cpp")

	def testPropertyNames(self):
		propertyNames = self.ed.PropertyNames()
		self.assertNotEqual(propertyNames, b"")
		# The cpp lexer has a boolean property named lexer.cpp.allow.dollars
		propNameDollars = b"lexer.cpp.allow.dollars"
		propertyType = self.ed.PropertyType(propNameDollars)
		self.assertEqual(propertyType, self.ed.SC_TYPE_BOOLEAN)
		propertyDescription = self.ed.DescribeProperty(propNameDollars)
		self.assertNotEqual(propertyDescription, b"")

	def testWordListDescriptions(self):
		wordSet = self.ed.DescribeKeyWordSets()
		self.assertNotEqual(wordSet, b"")

@unittest.skipUnless(lexersAvailable, "no lexers included")
class TestSubStyles(unittest.TestCase):
	''' These tests include knowledge of the current implementation in the cpp lexer
	and may have to change when that implementation changes.
	Currently supports subStyles for IDENTIFIER 11 and COMMENTDOCKEYWORD 17 '''
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testInfo(self):
		self.xite.ChooseLexer(b"cpp")
		bases = self.ed.GetSubStyleBases()
		self.assertEqual(bases, b"\x0b\x11")	# 11, 17
		self.assertEqual(self.ed.DistanceToSecondaryStyles(), 0x40)

	def testAllocate(self):
		firstSubStyle = 0x80	# Current implementation
		self.xite.ChooseLexer(b"cpp")
		self.assertEqual(self.ed.GetStyleFromSubStyle(firstSubStyle), firstSubStyle)
		self.assertEqual(self.ed.GetSubStylesStart(self.ed.SCE_C_IDENTIFIER), 0)
		self.assertEqual(self.ed.GetSubStylesLength(self.ed.SCE_C_IDENTIFIER), 0)
		numSubStyles = 5
		subs = self.ed.AllocateSubStyles(self.ed.SCE_C_IDENTIFIER, numSubStyles)
		self.assertEqual(subs, firstSubStyle)
		self.assertEqual(self.ed.GetSubStylesStart(self.ed.SCE_C_IDENTIFIER), firstSubStyle)
		self.assertEqual(self.ed.GetSubStylesLength(self.ed.SCE_C_IDENTIFIER), numSubStyles)
		self.assertEqual(self.ed.GetStyleFromSubStyle(subs), self.ed.SCE_C_IDENTIFIER)
		self.assertEqual(self.ed.GetStyleFromSubStyle(subs+numSubStyles-1), self.ed.SCE_C_IDENTIFIER)
		self.assertEqual(self.ed.GetStyleFromSubStyle(self.ed.SCE_C_IDENTIFIER), self.ed.SCE_C_IDENTIFIER)
		# Now free and check same as start
		self.ed.FreeSubStyles()
		self.assertEqual(self.ed.GetStyleFromSubStyle(subs), subs)
		self.assertEqual(self.ed.GetSubStylesStart(self.ed.SCE_C_IDENTIFIER), 0)
		self.assertEqual(self.ed.GetSubStylesLength(self.ed.SCE_C_IDENTIFIER), 0)

	def testInactive(self):
		firstSubStyle = 0x80	# Current implementation
		inactiveDistance = self.ed.DistanceToSecondaryStyles()
		self.xite.ChooseLexer(b"cpp")
		numSubStyles = 5
		subs = self.ed.AllocateSubStyles(self.ed.SCE_C_IDENTIFIER, numSubStyles)
		self.assertEqual(subs, firstSubStyle)
		self.assertEqual(self.ed.GetStyleFromSubStyle(subs), self.ed.SCE_C_IDENTIFIER)
		self.assertEqual(self.ed.GetStyleFromSubStyle(subs+inactiveDistance), self.ed.SCE_C_IDENTIFIER+inactiveDistance)
		self.ed.FreeSubStyles()

	def testSecondary(self):
		inactiveDistance = self.ed.DistanceToSecondaryStyles()
		inactiveIdentifier = self.ed.SCE_C_IDENTIFIER+inactiveDistance
		self.assertEqual(self.ed.GetPrimaryStyleFromStyle(inactiveIdentifier), self.ed.SCE_C_IDENTIFIER)

class TestCallTip(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 1 line of 4 characters
		t = b"fun("
		self.ed.AddText(len(t), t)

	def testBasics(self):
		self.assertEqual(self.ed.CallTipActive(), 0)
		self.ed.CallTipShow(1, "fun(int x)")
		self.assertEqual(self.ed.CallTipActive(), 1)
		self.assertEqual(self.ed.CallTipPosStart(), 4)
		self.ed.CallTipSetPosStart(1)
		self.assertEqual(self.ed.CallTipPosStart(), 1)
		self.ed.CallTipCancel()
		self.assertEqual(self.ed.CallTipActive(), 0)

class TestEdge(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()

	def testBasics(self):
		self.ed.EdgeColumn = 3
		self.assertEqual(self.ed.EdgeColumn, 3)
		self.ed.SetEdgeColour(0xA0)
		self.assertEqual(self.ed.GetEdgeColour(), 0xA0)

	def testMulti(self):
		self.assertEqual(self.ed.GetMultiEdgeColumn(-1), -1)
		self.assertEqual(self.ed.GetMultiEdgeColumn(0), -1)
		self.ed.MultiEdgeAddLine(5, 0x50)
		self.assertEqual(self.ed.GetMultiEdgeColumn(0), 5)
		self.assertEqual(self.ed.GetMultiEdgeColumn(1), -1)
		self.ed.MultiEdgeAddLine(6, 0x60)
		self.assertEqual(self.ed.GetMultiEdgeColumn(0), 5)
		self.assertEqual(self.ed.GetMultiEdgeColumn(1), 6)
		self.assertEqual(self.ed.GetMultiEdgeColumn(2), -1)
		self.ed.MultiEdgeAddLine(4, 0x40)
		self.assertEqual(self.ed.GetMultiEdgeColumn(0), 4)
		self.assertEqual(self.ed.GetMultiEdgeColumn(1), 5)
		self.assertEqual(self.ed.GetMultiEdgeColumn(2), 6)
		self.assertEqual(self.ed.GetMultiEdgeColumn(3), -1)
		self.ed.MultiEdgeClearAll()
		self.assertEqual(self.ed.GetMultiEdgeColumn(0), -1)

	def testSameTwice(self):
		# Tests that adding a column twice retains both
		self.ed.MultiEdgeAddLine(5, 0x50)
		self.ed.MultiEdgeAddLine(5, 0x55)
		self.assertEqual(self.ed.GetMultiEdgeColumn(0), 5)
		self.assertEqual(self.ed.GetMultiEdgeColumn(1), 5)
		self.assertEqual(self.ed.GetMultiEdgeColumn(2), -1)
		self.ed.MultiEdgeClearAll()

class TestAutoComplete(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 1 line of 3 characters
		t = b"xxx\n"
		self.ed.AddText(len(t), t)

	def testDefaults(self):
		self.assertEqual(self.ed.AutoCGetSeparator(), ord(' '))
		self.assertEqual(self.ed.AutoCGetMaxHeight(), 5)
		self.assertEqual(self.ed.AutoCGetMaxWidth(), 0)
		self.assertEqual(self.ed.AutoCGetTypeSeparator(), ord('?'))
		self.assertEqual(self.ed.AutoCGetIgnoreCase(), 0)
		self.assertEqual(self.ed.AutoCGetAutoHide(), 1)
		self.assertEqual(self.ed.AutoCGetDropRestOfWord(), 0)

	def testChangeDefaults(self):
		self.ed.AutoCSetSeparator(ord('-'))
		self.assertEqual(self.ed.AutoCGetSeparator(), ord('-'))
		self.ed.AutoCSetSeparator(ord(' '))

		self.ed.AutoCSetMaxHeight(100)
		self.assertEqual(self.ed.AutoCGetMaxHeight(), 100)
		self.ed.AutoCSetMaxHeight(5)

		self.ed.AutoCSetMaxWidth(100)
		self.assertEqual(self.ed.AutoCGetMaxWidth(), 100)
		self.ed.AutoCSetMaxWidth(0)

		self.ed.AutoCSetTypeSeparator(ord('@'))
		self.assertEqual(self.ed.AutoCGetTypeSeparator(), ord('@'))
		self.ed.AutoCSetTypeSeparator(ord('?'))

		self.ed.AutoCSetIgnoreCase(1)
		self.assertEqual(self.ed.AutoCGetIgnoreCase(), 1)
		self.ed.AutoCSetIgnoreCase(0)

		self.ed.AutoCSetAutoHide(0)
		self.assertEqual(self.ed.AutoCGetAutoHide(), 0)
		self.ed.AutoCSetAutoHide(1)

		self.ed.AutoCSetDropRestOfWord(1)
		self.assertEqual(self.ed.AutoCGetDropRestOfWord(), 1)
		self.ed.AutoCSetDropRestOfWord(0)

		self.ed.AutoCSetStyle(13)
		self.assertEqual(self.ed.AutoCGetStyle(), 13)
		self.ed.AutoCSetStyle(self.ed.STYLE_DEFAULT)

	def testAutoShow(self):
		self.assertEqual(self.ed.AutoCActive(), 0)
		self.ed.SetSel(0, 0)

		self.ed.AutoCShow(0, b"za defn ghi")
		self.assertEqual(self.ed.AutoCActive(), 1)
		#~ time.sleep(2)
		self.assertEqual(self.ed.AutoCPosStart(), 0)
		self.assertEqual(self.ed.AutoCGetCurrent(), 0)
		t = self.ed.AutoCGetCurrentText(5)
		#~ self.assertEqual(l, 3)
		self.assertEqual(t, b"za")
		self.ed.AutoCCancel()
		self.assertEqual(self.ed.AutoCActive(), 0)

	def testAutoShowComplete(self):
		self.assertEqual(self.ed.AutoCActive(), 0)
		self.ed.SetSel(0, 0)

		self.ed.AutoCShow(0, b"za defn ghi")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"zaxxx\n")

		self.assertEqual(self.ed.AutoCActive(), 0)

	def testAutoShowSelect(self):
		self.assertEqual(self.ed.AutoCActive(), 0)
		self.ed.SetSel(0, 0)

		self.ed.AutoCShow(0, b"za defn ghi")
		self.ed.AutoCSelect(0, b"d")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"defnxxx\n")

		self.assertEqual(self.ed.AutoCActive(), 0)

	def testAutoSelectFirstItem(self):
		self.assertEqual(self.ed.AutoCActive(), 0)

		self.ed.AutoCSetOrder(self.ed.SC_ORDER_CUSTOM)

		# without SC_AUTOCOMPLETE_SELECT_FIRST_ITEM option
		self.ed.SetSel(3, 3)
		self.ed.AutoCShow(3, b"aaa1 bbb1 xxx1")
		# automatically selects the item with the entered prefix xxx
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"xxx1\n")

		# with SC_AUTOCOMPLETE_SELECT_FIRST_ITEM option
		self.ed.AutoCSetOptions(2, 0)
		self.ed.SetSel(3, 3)
		self.ed.AutoCShow(3, b"aaa1 bbb1 xxx1")
		# selects the first item regardless of the entered prefix and replaces the entered xxx
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"aaa11\n")

		self.assertEqual(self.ed.AutoCActive(), 0)

	def testAutoCustomSort(self):
		# Checks bug #2294 where SC_ORDER_CUSTOM with an empty list asserts
		# https://sourceforge.net/p/scintilla/bugs/2294/
		self.assertEqual(self.ed.AutoCGetOrder(), self.ed.SC_ORDER_PRESORTED)

		self.ed.AutoCSetOrder(self.ed.SC_ORDER_CUSTOM)
		self.assertEqual(self.ed.AutoCGetOrder(), self.ed.SC_ORDER_CUSTOM)

		#~ self.ed.AutoCShow(0, b"")
		#~ self.ed.AutoCComplete()
		#~ self.assertEqual(self.ed.Contents(), b"xxx\n")

		self.ed.AutoCShow(0, b"a")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"xxx\na")

		self.ed.AutoCSetOrder(self.ed.SC_ORDER_PERFORMSORT)
		self.assertEqual(self.ed.AutoCGetOrder(), self.ed.SC_ORDER_PERFORMSORT)

		self.ed.AutoCShow(0, b"")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"xxx\na")

		self.ed.AutoCShow(0, b"b a")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"xxx\naa")

		self.ed.AutoCSetOrder(self.ed.SC_ORDER_PRESORTED)
		self.assertEqual(self.ed.AutoCGetOrder(), self.ed.SC_ORDER_PRESORTED)

		self.ed.AutoCShow(0, b"")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"xxx\naa")

		self.ed.AutoCShow(0, b"a b")
		self.ed.AutoCComplete()
		self.assertEqual(self.ed.Contents(), b"xxx\naaa")

		self.assertEqual(self.ed.AutoCActive(), 0)

	def testWriteOnly(self):
		""" Checks that setting attributes doesn't crash or change tested behaviour
		but does not check that the changed attributes are effective. """
		self.ed.AutoCStops(0, b"abcde")
		self.ed.AutoCSetFillUps(0, b"1234")

class TestDirectAccess(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testGapPosition(self):
		text = b"abcd"
		self.ed.SetContents(text)
		self.assertEqual(self.ed.GapPosition, 4)
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 1
		rep = b"-"
		self.ed.ReplaceTarget(len(rep), rep)
		self.assertEqual(self.ed.GapPosition, 2)

	def testCharacterPointerAndRangePointer(self):
		text = b"abcd"
		self.ed.SetContents(text)
		characterPointer = self.ed.CharacterPointer
		rangePointer = self.ed.GetRangePointer(0,3)
		self.assertEqual(characterPointer, rangePointer)
		cpBuffer = ctypes.c_char_p(characterPointer)
		self.assertEqual(cpBuffer.value, text)
		# Gap will not be moved as already moved for CharacterPointer call
		rangePointer = self.ed.GetRangePointer(1,3)
		cpBuffer = ctypes.c_char_p(rangePointer)
		self.assertEqual(cpBuffer.value, text[1:])

class TestJoin(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testJoin(self):
		text = b"a\r\nb\r\nc"
		self.ed.SetContents(text)
		self.ed.TargetWholeDocument()
		self.ed.LinesJoin()
		self.assertEqual(self.ed.Contents(), b"a b c")

	def testJoinEndLine(self):
		text = b"a\r\nb\r\nc"
		self.ed.SetContents(text)
		# Select a..b
		self.ed.SetTargetRange(0, 4)
		self.ed.LinesJoin()
		self.assertEqual(self.ed.Contents(), b"a b\r\nc")

	def testJoinSpace(self):
		# Demonstration of bug #2372 which produced b"a \r"
		text = b"a \r\n\r\n"
		self.ed.SetContents(text)
		self.ed.TargetWholeDocument()
		self.ed.LinesJoin()
		self.assertEqual(self.ed.Contents(), b"a ")

	def testJoinOutOfBounds(self):
		text = b"a\r\nb\r\nc"
		self.ed.SetContents(text)
		# Setting end of target after document end encourages non-termination.
		self.ed.SetTargetRange(-10, 16)
		self.ed.LinesJoin()
		# Out-of-bounds leaves extra space as 'c' is processed
		self.assertEqual(self.ed.Contents(), b"a b c ")

class TestWordChars(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.SetCharsDefault()

	def _setChars(self, charClass, chars):
		""" Wrapper to call self.ed.Set*Chars with the right type
		@param charClass {str} the character class, "word", "space", etc.
		@param chars {iterable of int} characters to set
		"""
		if sys.version_info.major == 2:
			# Python 2, use latin-1 encoded str
			unichars = (unichr(x) for x in chars if x != 0)
			# can't use literal u"", that's a syntax error in Py3k
			# uncode() doesn't exist in Py3k, but we never run it there
			result = unicode("").join(unichars).encode("latin-1")
		else:
			# Python 3, use bytes()
			result = bytes(x for x in chars if x != 0)
		meth = getattr(self.ed, "Set%sChars" % (charClass.capitalize()))
		return meth(None, result)

	def assertCharSetsEqual(self, first, second, *args, **kwargs):
		""" Assert that the two character sets are equal.
		If either set are an iterable of numbers, convert them to chars
		first. """
		first_set = set()
		for c in first:
			first_set.add(chr(c) if isinstance(c, int) else c)
		second_set = set()
		for c in second:
			second_set.add(chr(c) if isinstance(c, int) else c)
		return self.assertEqual(first_set, second_set, *args, **kwargs)

	def testDefaultWordChars(self):
		# check that the default word chars are as expected
		data = self.ed.GetWordChars(None)
		expected = set(string.digits + string.ascii_letters + '_') | \
			set(chr(x) for x in range(0x80, 0x100))
		self.assertCharSetsEqual(data, expected)

	def testDefaultWhitespaceChars(self):
		# check that the default whitespace chars are as expected
		data = self.ed.GetWhitespaceChars(None)
		expected = (set(chr(x) for x in (range(0, 0x20))) | set(' ') | set('\x7f')) - \
			set(['\r', '\n'])
		self.assertCharSetsEqual(data, expected)

	def testDefaultPunctuationChars(self):
		# check that the default punctuation chars are as expected
		data = self.ed.GetPunctuationChars(None)
		expected = set(chr(x) for x in range(0x20, 0x80)) - \
			set(string.ascii_letters + string.digits + "\r\n\x7f_ ")
		self.assertCharSetsEqual(data, expected)

	def testCustomWordChars(self):
		# check that setting things to whitespace chars makes them not words
		self._setChars("whitespace", range(1, 0x100))
		data = self.ed.GetWordChars(None)
		expected = set()
		self.assertCharSetsEqual(data, expected)
		# and now set something to make sure that works too
		expected = set(range(1, 0x100, 2))
		self._setChars("word", expected)
		data = self.ed.GetWordChars(None)
		self.assertCharSetsEqual(data, expected)

	def testCustomWhitespaceChars(self):
		# check setting whitespace chars to non-default values
		self._setChars("word", range(1, 0x100))
		# we can't change chr(0) from being anything but whitespace
		expected = set([0])
		data = self.ed.GetWhitespaceChars(None)
		self.assertCharSetsEqual(data, expected)
		# now try to set it to something custom
		expected = set(range(1, 0x100, 2)) | set([0])
		self._setChars("whitespace", expected)
		data = self.ed.GetWhitespaceChars(None)
		self.assertCharSetsEqual(data, expected)

	def testCustomPunctuationChars(self):
		# check setting punctuation chars to non-default values
		self._setChars("word", range(1, 0x100))
		expected = set()
		data = self.ed.GetPunctuationChars(0)
		self.assertEqual(set(data), expected)
		# now try to set it to something custom
		expected = set(range(1, 0x100, 1))
		self._setChars("punctuation", expected)
		data = self.ed.GetPunctuationChars(None)
		self.assertCharSetsEqual(data, expected)

	def testCharacterCategoryOptimization(self):
		self.assertEqual(self.ed.CharacterCategoryOptimization, 0x100)
		self.ed.CharacterCategoryOptimization = 0x1000
		self.assertEqual(self.ed.CharacterCategoryOptimization, 0x1000)

class TestExplicitTabStops(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 2 lines of 4 characters
		self.t = b"fun(\nint)"
		self.ed.AddText(len(self.t), self.t)

	def testAddingAndClearing(self):
		self.assertEqual(self.ed.GetNextTabStop(0,0), 0)

		# Add a tab stop at 7
		self.ed.AddTabStop(0, 7)
		# Check added
		self.assertEqual(self.ed.GetNextTabStop(0,0), 7)
		# Check does not affect line 1
		self.assertEqual(self.ed.GetNextTabStop(1,0), 0)

		# Add a tab stop at 18
		self.ed.AddTabStop(0, 18)
		# Check added
		self.assertEqual(self.ed.GetNextTabStop(0,0), 7)
		self.assertEqual(self.ed.GetNextTabStop(0,7), 18)
		# Check does not affect line 1
		self.assertEqual(self.ed.GetNextTabStop(1,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(1,7), 0)

		# Add a tab stop between others at 13
		self.ed.AddTabStop(0, 13)
		# Check added
		self.assertEqual(self.ed.GetNextTabStop(0,0), 7)
		self.assertEqual(self.ed.GetNextTabStop(0,7), 13)
		self.assertEqual(self.ed.GetNextTabStop(0,13), 18)
		# Check does not affect line 1
		self.assertEqual(self.ed.GetNextTabStop(1,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(1,7), 0)

		self.ed.ClearTabStops(0)
		# Check back to original state
		self.assertEqual(self.ed.GetNextTabStop(0,0), 0)

	def testLineInsertionDeletion(self):
		# Add a tab stop at 7 on line 1
		self.ed.AddTabStop(1, 7)
		# Check added
		self.assertEqual(self.ed.GetNextTabStop(1,0), 7)

		# More text at end
		self.ed.AddText(len(self.t), self.t)
		self.assertEqual(self.ed.GetNextTabStop(0,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(1,0), 7)
		self.assertEqual(self.ed.GetNextTabStop(2,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(3,0), 0)

		# Another 2 lines before explicit line moves the explicit tab stop
		data = b"x\ny\n"
		self.ed.InsertText(4, data)
		self.assertEqual(self.ed.GetNextTabStop(0,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(1,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(2,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(3,0), 7)
		self.assertEqual(self.ed.GetNextTabStop(4,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(5,0), 0)

		# Undo moves the explicit tab stop back
		self.ed.Undo()
		self.assertEqual(self.ed.GetNextTabStop(0,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(1,0), 7)
		self.assertEqual(self.ed.GetNextTabStop(2,0), 0)
		self.assertEqual(self.ed.GetNextTabStop(3,0), 0)

if __name__ == '__main__':
	uu = Xite.main("simpleTests")
	#~ for x in sorted(uu.keys()):
		#~ print(x, uu[x])
	#~ print()
