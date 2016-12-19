# -*- coding: utf-8 -*-
# Requires Python 2.7 or later

from __future__ import with_statement
from __future__ import unicode_literals

import codecs, ctypes, os, sys, unittest

if sys.platform == "win32":
	import XiteWin as Xite
else:
	import XiteQt as Xite

class TestSimple(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testLength(self):
		self.assertEquals(self.ed.Length, 0)

	def testAddText(self):
		self.ed.AddText(1, b"x")
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.ed.GetCharAt(0), ord("x"))
		self.assertEquals(self.ed.GetStyleAt(0), 0)
		self.ed.ClearAll()
		self.assertEquals(self.ed.Length, 0)

	def testDeleteRange(self):
		self.ed.AddText(5, b"abcde")
		self.assertEquals(self.ed.Length, 5)
		self.ed.DeleteRange(1, 2)
		self.assertEquals(self.ed.Length, 3)
		self.assertEquals(self.ed.Contents(), b"ade")

	def testAddStyledText(self):
		self.assertEquals(self.ed.EndStyled, 0)
		self.ed.AddStyledText(2, b"x\002")
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.ed.GetCharAt(0), ord("x"))
		self.assertEquals(self.ed.GetStyleAt(0), 2)
		self.assertEquals(self.ed.StyledTextRange(0, 1), b"x\002")
		self.ed.ClearDocumentStyle()
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.ed.GetCharAt(0), ord("x"))
		self.assertEquals(self.ed.GetStyleAt(0), 0)
		self.assertEquals(self.ed.StyledTextRange(0, 1), b"x\0")

	def testStyling(self):
		self.assertEquals(self.ed.EndStyled, 0)
		self.ed.AddStyledText(4, b"x\002y\003")
		self.assertEquals(self.ed.StyledTextRange(0, 2), b"x\002y\003")
		self.ed.StartStyling(0,0xf)
		self.ed.SetStyling(1, 5)
		self.assertEquals(self.ed.StyledTextRange(0, 2), b"x\005y\003")

		self.ed.StartStyling(0,0xff)
		self.ed.SetStylingEx(2, b"\100\101")
		self.assertEquals(self.ed.StyledTextRange(0, 2), b"x\100y\101")

	def testPosition(self):
		self.assertEquals(self.ed.CurrentPos, 0)
		self.assertEquals(self.ed.Anchor, 0)
		self.ed.AddText(1, b"x")
		# Caret has automatically moved
		self.assertEquals(self.ed.CurrentPos, 1)
		self.assertEquals(self.ed.Anchor, 1)
		self.ed.SelectAll()
		self.assertEquals(self.ed.CurrentPos, 0)
		self.assertEquals(self.ed.Anchor, 1)
		self.ed.Anchor = 0
		self.assertEquals(self.ed.Anchor, 0)
		# Check line positions
		self.assertEquals(self.ed.PositionFromLine(0), 0)
		self.assertEquals(self.ed.GetLineEndPosition(0), 1)
		self.assertEquals(self.ed.PositionFromLine(1), 1)

		self.ed.CurrentPos = 1
		self.assertEquals(self.ed.Anchor, 0)
		self.assertEquals(self.ed.CurrentPos, 1)

	def testBeyonEnd(self):
		self.ed.AddText(1, b"x")
		self.assertEquals(self.ed.GetLineEndPosition(0), 1)
		self.assertEquals(self.ed.GetLineEndPosition(1), 1)
		self.assertEquals(self.ed.GetLineEndPosition(2), 1)

	def testSelection(self):
		self.assertEquals(self.ed.CurrentPos, 0)
		self.assertEquals(self.ed.Anchor, 0)
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 0)
		self.ed.AddText(1, b"x")
		self.ed.SelectionStart = 0
		self.assertEquals(self.ed.CurrentPos, 1)
		self.assertEquals(self.ed.Anchor, 0)
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 1)
		self.ed.SelectionStart = 1
		self.assertEquals(self.ed.CurrentPos, 1)
		self.assertEquals(self.ed.Anchor, 1)
		self.assertEquals(self.ed.SelectionStart, 1)
		self.assertEquals(self.ed.SelectionEnd, 1)

		self.ed.SelectionEnd = 0
		self.assertEquals(self.ed.CurrentPos, 0)
		self.assertEquals(self.ed.Anchor, 0)

	def testSetSelection(self):
		self.ed.AddText(4, b"abcd")
		self.ed.SetSel(1, 3)
		self.assertEquals(self.ed.SelectionStart, 1)
		self.assertEquals(self.ed.SelectionEnd, 3)
		result = self.ed.GetSelText(0)
		self.assertEquals(result, b"bc\0")
		self.ed.ReplaceSel(0, b"1234")
		self.assertEquals(self.ed.Length, 6)
		self.assertEquals(self.ed.Contents(), b"a1234d")

	def testReadOnly(self):
		self.ed.AddText(1, b"x")
		self.assertEquals(self.ed.ReadOnly, 0)
		self.assertEquals(self.ed.Contents(), b"x")
		self.ed.ReadOnly = 1
		self.assertEquals(self.ed.ReadOnly, 1)
		self.ed.AddText(1, b"x")
		self.assertEquals(self.ed.Contents(), b"x")
		self.ed.ReadOnly = 0
		self.ed.AddText(1, b"x")
		self.assertEquals(self.ed.Contents(), b"xx")
		self.ed.Null()
		self.assertEquals(self.ed.Contents(), b"xx")

	def testAddLine(self):
		data = b"x" * 70 + b"\n"
		for i in range(5):
			self.ed.AddText(len(data), data)
			self.assertEquals(self.ed.LineCount, i + 2)
		self.assert_(self.ed.Length > 0)

	def testInsertText(self):
		data = b"xy"
		self.ed.InsertText(0, data)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(data, self.ed.ByteRange(0,2))

		self.ed.InsertText(1, data)
		# Should now be "xxyy"
		self.assertEquals(self.ed.Length, 4)
		self.assertEquals(b"xxyy", self.ed.ByteRange(0,4))

	def testInsertNul(self):
		data = b"\0"
		self.ed.AddText(1, data)
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(data, self.ed.ByteRange(0,1))

	def testUndoRedo(self):
		data = b"xy"
		self.assertEquals(self.ed.Modify, 0)
		self.assertEquals(self.ed.UndoCollection, 1)
		self.assertEquals(self.ed.CanRedo(), 0)
		self.assertEquals(self.ed.CanUndo(), 0)
		self.ed.InsertText(0, data)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.ed.Modify, 1)
		self.assertEquals(self.ed.CanRedo(), 0)
		self.assertEquals(self.ed.CanUndo(), 1)
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.ed.Modify, 0)
		self.assertEquals(self.ed.CanRedo(), 1)
		self.assertEquals(self.ed.CanUndo(), 0)
		self.ed.Redo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.ed.Modify, 1)
		self.assertEquals(data, self.ed.Contents())
		self.assertEquals(self.ed.CanRedo(), 0)
		self.assertEquals(self.ed.CanUndo(), 1)

	def testUndoSavePoint(self):
		data = b"xy"
		self.assertEquals(self.ed.Modify, 0)
		self.ed.InsertText(0, data)
		self.assertEquals(self.ed.Modify, 1)
		self.ed.SetSavePoint()
		self.assertEquals(self.ed.Modify, 0)
		self.ed.InsertText(0, data)
		self.assertEquals(self.ed.Modify, 1)

	def testUndoCollection(self):
		data = b"xy"
		self.assertEquals(self.ed.UndoCollection, 1)
		self.ed.UndoCollection = 0
		self.assertEquals(self.ed.UndoCollection, 0)
		self.ed.InsertText(0, data)
		self.assertEquals(self.ed.CanRedo(), 0)
		self.assertEquals(self.ed.CanUndo(), 0)
		self.ed.UndoCollection = 1

	def testGetColumn(self):
		self.ed.AddText(1, b"x")
		self.assertEquals(self.ed.GetColumn(0), 0)
		self.assertEquals(self.ed.GetColumn(1), 1)
		# Next line caused infinite loop in 1.71
		self.assertEquals(self.ed.GetColumn(2), 1)
		self.assertEquals(self.ed.GetColumn(3), 1)

	def testTabWidth(self):
		self.assertEquals(self.ed.TabWidth, 8)
		self.ed.AddText(3, b"x\tb")
		self.assertEquals(self.ed.GetColumn(0), 0)
		self.assertEquals(self.ed.GetColumn(1), 1)
		self.assertEquals(self.ed.GetColumn(2), 8)
		for col in range(10):
			if col == 0:
				self.assertEquals(self.ed.FindColumn(0, col), 0)
			elif col == 1:
				self.assertEquals(self.ed.FindColumn(0, col), 1)
			elif col == 8:
				self.assertEquals(self.ed.FindColumn(0, col), 2)
			elif col == 9:
				self.assertEquals(self.ed.FindColumn(0, col), 3)
			else:
				self.assertEquals(self.ed.FindColumn(0, col), 1)
		self.ed.TabWidth = 4
		self.assertEquals(self.ed.TabWidth, 4)
		self.assertEquals(self.ed.GetColumn(0), 0)
		self.assertEquals(self.ed.GetColumn(1), 1)
		self.assertEquals(self.ed.GetColumn(2), 4)

	def testIndent(self):
		self.assertEquals(self.ed.Indent, 0)
		self.assertEquals(self.ed.UseTabs, 1)
		self.ed.Indent = 8
		self.ed.UseTabs = 0
		self.assertEquals(self.ed.Indent, 8)
		self.assertEquals(self.ed.UseTabs, 0)
		self.ed.AddText(3, b"x\tb")
		self.assertEquals(self.ed.GetLineIndentation(0), 0)
		self.ed.InsertText(0, b" ")
		self.assertEquals(self.ed.GetLineIndentation(0), 1)
		self.assertEquals(self.ed.GetLineIndentPosition(0), 1)
		self.assertEquals(self.ed.Contents(), b" x\tb")
		self.ed.SetLineIndentation(0,2)
		self.assertEquals(self.ed.Contents(), b"  x\tb")
		self.assertEquals(self.ed.GetLineIndentPosition(0), 2)
		self.ed.UseTabs = 1
		self.ed.SetLineIndentation(0,8)
		self.assertEquals(self.ed.Contents(), b"\tx\tb")
		self.assertEquals(self.ed.GetLineIndentPosition(0), 1)

	def testGetCurLine(self):
		self.ed.AddText(1, b"x")
		data = ctypes.create_string_buffer(b"\0" * 100)
		caret = self.ed.GetCurLine(len(data), data)
		self.assertEquals(caret, 1)
		self.assertEquals(data.value, b"x")

	def testGetLine(self):
		self.ed.AddText(1, b"x")
		data = ctypes.create_string_buffer(b"\0" * 100)
		self.ed.GetLine(0, data)
		self.assertEquals(data.value, b"x")

	def testLineEnds(self):
		self.ed.AddText(3, b"x\ny")
		self.assertEquals(self.ed.GetLineEndPosition(0), 1)
		self.assertEquals(self.ed.GetLineEndPosition(1), 3)
		self.assertEquals(self.ed.LineLength(0), 2)
		self.assertEquals(self.ed.LineLength(1), 1)
		if sys.platform == "win32":
			self.assertEquals(self.ed.EOLMode, self.ed.SC_EOL_CRLF)
		else:
			self.assertEquals(self.ed.EOLMode, self.ed.SC_EOL_LF)
		lineEnds = [b"\r\n", b"\r", b"\n"]
		for lineEndType in [self.ed.SC_EOL_CR, self.ed.SC_EOL_LF, self.ed.SC_EOL_CRLF]:
			self.ed.EOLMode = lineEndType
			self.assertEquals(self.ed.EOLMode, lineEndType)
			self.ed.ConvertEOLs(lineEndType)
			self.assertEquals(self.ed.Contents(), b"x" + lineEnds[lineEndType] + b"y")
			self.assertEquals(self.ed.LineLength(0), 1 + len(lineEnds[lineEndType]))

	# Several tests for unicode line ends U+2028 and U+2029

	def testUnicodeLineEnds(self):
		# Add two lines separated with U+2028 and ensure it is seen as two lines
		# Then remove U+2028 and should be just 1 lines
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.ed.AddText(5, b"x\xe2\x80\xa8y")
		self.assertEquals(self.ed.LineCount, 2)
		self.assertEquals(self.ed.GetLineEndPosition(0), 1)
		self.assertEquals(self.ed.GetLineEndPosition(1), 5)
		self.assertEquals(self.ed.LineLength(0), 4)
		self.assertEquals(self.ed.LineLength(1), 1)
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 4
		self.ed.ReplaceTarget(0, b"")
		self.assertEquals(self.ed.LineCount, 1)
		self.assertEquals(self.ed.LineLength(0), 2)
		self.assertEquals(self.ed.GetLineEndPosition(0), 2)
		self.assertEquals(self.ed.LineEndTypesSupported, 1)

	def testUnicodeLineEndsWithCodePage0(self):
		# Try the Unicode line ends when not in Unicode mode -> should remain 1 line
		self.ed.SetCodePage(0)
		self.ed.AddText(5, b"x\xe2\x80\xa8y")
		self.assertEquals(self.ed.LineCount, 1)
		self.ed.AddText(4, b"x\xc2\x85y")
		self.assertEquals(self.ed.LineCount, 1)

	def testUnicodeLineEndsSwitchToUnicodeAndBack(self):
		# Add the Unicode line ends when not in Unicode mode
		self.ed.SetCodePage(0)
		self.ed.AddText(5, b"x\xe2\x80\xa8y")
		self.assertEquals(self.ed.LineCount, 1)
		# Into UTF-8 mode - should now be interpreting as two lines
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.assertEquals(self.ed.LineCount, 2)
		# Back to code page 0 and 1 line
		self.ed.SetCodePage(0)
		self.assertEquals(self.ed.LineCount, 1)

	def testUFragmentedEOLCompletion(self):
		# Add 2 starting bytes of UTF-8 line end then complete it
		self.ed.ClearAll()
		self.ed.AddText(4, b"x\xe2\x80y")
		self.assertEquals(self.ed.LineCount, 1)
		self.assertEquals(self.ed.GetLineEndPosition(0), 4)
		self.ed.SetSel(3,3)
		self.ed.AddText(1, b"\xa8")
		self.assertEquals(self.ed.Contents(), b"x\xe2\x80\xa8y")
		self.assertEquals(self.ed.LineCount, 2)

		# Add 1 starting bytes of UTF-8 line end then complete it
		self.ed.ClearAll()
		self.ed.AddText(3, b"x\xe2y")
		self.assertEquals(self.ed.LineCount, 1)
		self.assertEquals(self.ed.GetLineEndPosition(0), 3)
		self.ed.SetSel(2,2)
		self.ed.AddText(2, b"\x80\xa8")
		self.assertEquals(self.ed.Contents(), b"x\xe2\x80\xa8y")
		self.assertEquals(self.ed.LineCount, 2)

	def testUFragmentedEOLStart(self):
		# Add end of UTF-8 line end then insert start
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.assertEquals(self.ed.LineCount, 1)
		self.ed.AddText(4, b"x\x80\xa8y")
		self.assertEquals(self.ed.LineCount, 1)
		self.ed.SetSel(1,1)
		self.ed.AddText(1, b"\xe2")
		self.assertEquals(self.ed.LineCount, 2)

	def testUBreakApartEOL(self):
		# Add two lines separated by U+2029 then remove and add back each byte ensuring
		# only one line after each removal of any byte in line end and 2 lines after reinsertion
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		text = b"x\xe2\x80\xa9y";
		self.ed.AddText(5, text)
		self.assertEquals(self.ed.LineCount, 2)

		for i in range(len(text)):
			self.ed.TargetStart = i
			self.ed.TargetEnd = i + 1
			self.ed.ReplaceTarget(0, b"")
			if i in [0, 4]:
				# Removing text characters does not change number of lines
				self.assertEquals(self.ed.LineCount, 2)
			else:
				# Removing byte from line end, removes 1 line
				self.assertEquals(self.ed.LineCount, 1)

			self.ed.TargetEnd = i
			self.ed.ReplaceTarget(1, text[i:i+1])
			self.assertEquals(self.ed.LineCount, 2)

	def testURemoveEOLFragment(self):
		# Add UTF-8 line end then delete each byte causing line end to disappear
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		for i in range(3):
			self.ed.ClearAll()
			self.ed.AddText(5, b"x\xe2\x80\xa8y")
			self.assertEquals(self.ed.LineCount, 2)
			self.ed.TargetStart = i+1
			self.ed.TargetEnd = i+2
			self.ed.ReplaceTarget(0, b"")
			self.assertEquals(self.ed.LineCount, 1)

	# Several tests for unicode NEL line ends U+0085

	def testNELLineEnds(self):
		# Add two lines separated with U+0085 and ensure it is seen as two lines
		# Then remove U+0085 and should be just 1 lines
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.ed.AddText(4, b"x\xc2\x85y")
		self.assertEquals(self.ed.LineCount, 2)
		self.assertEquals(self.ed.GetLineEndPosition(0), 1)
		self.assertEquals(self.ed.GetLineEndPosition(1), 4)
		self.assertEquals(self.ed.LineLength(0), 3)
		self.assertEquals(self.ed.LineLength(1), 1)
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.ed.ReplaceTarget(0, b"")
		self.assertEquals(self.ed.LineCount, 1)
		self.assertEquals(self.ed.LineLength(0), 2)
		self.assertEquals(self.ed.GetLineEndPosition(0), 2)

	def testNELFragmentedEOLCompletion(self):
		# Add starting byte of UTF-8 NEL then complete it
		self.ed.AddText(3, b"x\xc2y")
		self.assertEquals(self.ed.LineCount, 1)
		self.assertEquals(self.ed.GetLineEndPosition(0), 3)
		self.ed.SetSel(2,2)
		self.ed.AddText(1, b"\x85")
		self.assertEquals(self.ed.Contents(), b"x\xc2\x85y")
		self.assertEquals(self.ed.LineCount, 2)

	def testNELFragmentedEOLStart(self):
		# Add end of UTF-8 NEL then insert start
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		self.assertEquals(self.ed.LineCount, 1)
		self.ed.AddText(4, b"x\x85y")
		self.assertEquals(self.ed.LineCount, 1)
		self.ed.SetSel(1,1)
		self.ed.AddText(1, b"\xc2")
		self.assertEquals(self.ed.LineCount, 2)

	def testNELBreakApartEOL(self):
		# Add two lines separated by U+0085 then remove and add back each byte ensuring
		# only one line after each removal of any byte in line end and 2 lines after reinsertion
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.ed.SetCodePage(65001)
		self.ed.SetLineEndTypesAllowed(1)
		text = b"x\xc2\x85y";
		self.ed.AddText(4, text)
		self.assertEquals(self.ed.LineCount, 2)

		for i in range(len(text)):
			self.ed.TargetStart = i
			self.ed.TargetEnd = i + 1
			self.ed.ReplaceTarget(0, b"")
			if i in [0, 3]:
				# Removing text characters does not change number of lines
				self.assertEquals(self.ed.LineCount, 2)
			else:
				# Removing byte from line end, removes 1 line
				self.assertEquals(self.ed.LineCount, 1)

			self.ed.TargetEnd = i
			self.ed.ReplaceTarget(1, text[i:i+1])
			self.assertEquals(self.ed.LineCount, 2)

	def testNELRemoveEOLFragment(self):
		# Add UTF-8 NEL then delete each byte causing line end to disappear
		self.ed.SetCodePage(65001)
		for i in range(2):
			self.ed.ClearAll()
			self.ed.AddText(4, b"x\xc2\x85y")
			self.assertEquals(self.ed.LineCount, 2)
			self.ed.TargetStart = i+1
			self.ed.TargetEnd = i+2
			self.ed.ReplaceTarget(0, b"")
			self.assertEquals(self.ed.LineCount, 1)

	def testGoto(self):
		self.ed.AddText(5, b"a\nb\nc")
		self.assertEquals(self.ed.CurrentPos, 5)
		self.ed.GotoLine(1)
		self.assertEquals(self.ed.CurrentPos, 2)
		self.ed.GotoPos(4)
		self.assertEquals(self.ed.CurrentPos, 4)

	def testCutCopyPaste(self):
		self.ed.AddText(5, b"a1b2c")
		self.ed.SetSel(1,3)
		self.ed.Cut()
		self.xite.DoEvents()
		self.assertEquals(self.ed.CanPaste(), 1)
		self.ed.SetSel(0, 0)
		self.ed.Paste()
		self.assertEquals(self.ed.Contents(), b"1ba2c")
		self.ed.SetSel(4,5)
		self.ed.Copy()
		self.ed.SetSel(1,3)
		self.ed.Paste()
		self.assertEquals(self.ed.Contents(), b"1c2c")
		self.ed.SetSel(2,4)
		self.ed.Clear()
		self.assertEquals(self.ed.Contents(), b"1c")

	def testCopyAllowLine(self):
		self.xite.DoEvents()
		lineEndType = self.ed.EOLMode
		self.ed.EOLMode = self.ed.SC_EOL_LF
		self.ed.AddText(5, b"a1\nb2")
		self.ed.SetSel(1,1)
		self.ed.CopyAllowLine()
		self.xite.DoEvents()
		self.assertEquals(self.ed.CanPaste(), 1)
		self.ed.SetSel(0, 0)
		self.ed.Paste()
		self.ed.EOLMode = lineEndType
		self.assertEquals(self.ed.Contents(), b"a1\na1\nb2")

	def testDuplicate(self):
		self.ed.AddText(3, b"1b2")
		self.ed.SetSel(1,2)
		self.ed.SelectionDuplicate()
		self.assertEquals(self.ed.Contents(), b"1bb2")

	def testTransposeLines(self):
		self.ed.AddText(8, b"a1\nb2\nc3")
		self.ed.SetSel(3,3)
		self.ed.LineTranspose()
		self.assertEquals(self.ed.Contents(), b"b2\na1\nc3")

	def testGetSet(self):
		self.ed.SetContents(b"abc")
		self.assertEquals(self.ed.TextLength, 3)
		result = ctypes.create_string_buffer(b"\0" * 5)
		length = self.ed.GetText(4, result)
		self.assertEquals(result.value, b"abc")

	def testAppend(self):
		self.ed.SetContents(b"abc")
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 0)
		text = b"12"
		self.ed.AppendText(len(text), text)
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 0)
		self.assertEquals(self.ed.Contents(), b"abc12")

	def testTarget(self):
		self.ed.SetContents(b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.assertEquals(self.ed.TargetStart, 1)
		self.assertEquals(self.ed.TargetEnd, 3)
		rep = b"321"
		self.ed.ReplaceTarget(len(rep), rep)
		self.assertEquals(self.ed.Contents(), b"a321d")
		self.ed.SearchFlags = self.ed.SCFIND_REGEXP
		self.assertEquals(self.ed.SearchFlags, self.ed.SCFIND_REGEXP)
		searchString = b"\([1-9]+\)"
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(1, pos)
		tagString = self.ed.GetTag(1)
		self.assertEquals(tagString, b"321")
		rep = b"\\1"
		self.ed.TargetStart = 0
		self.ed.TargetEnd = 0
		self.ed.ReplaceTargetRE(len(rep), rep)
		self.assertEquals(self.ed.Contents(), b"321a321d")
		self.ed.SetSel(4,5)
		self.ed.TargetFromSelection()
		self.assertEquals(self.ed.TargetStart, 4)
		self.assertEquals(self.ed.TargetEnd, 5)

	def testTargetEscape(self):
		# Checks that a literal \ can be in the replacement. Bug #2959876
		self.ed.SetContents(b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		rep = b"\\\\n"
		self.ed.ReplaceTargetRE(len(rep), rep)
		self.assertEquals(self.ed.Contents(), b"a\\nd")

	def testPointsAndPositions(self):
		self.ed.AddText(1, b"x")
		# Start of text
		self.assertEquals(self.ed.PositionFromPoint(0,0), 0)
		# End of text
		self.assertEquals(self.ed.PositionFromPoint(0,100), 1)

	def testLinePositions(self):
		text = b"ab\ncd\nef"
		nl = b"\n"
		if sys.version_info[0] == 3:
			nl = ord(b"\n")
		self.ed.AddText(len(text), text)
		self.assertEquals(self.ed.LineFromPosition(-1), 0)
		line = 0
		for pos in range(len(text)+1):
			self.assertEquals(self.ed.LineFromPosition(pos), line)
			if pos < len(text) and text[pos] == nl:
				line += 1

	def testWordPositions(self):
		text = b"ab cd\tef"
		self.ed.AddText(len(text), text)
		self.assertEquals(self.ed.WordStartPosition(3, 0), 2)
		self.assertEquals(self.ed.WordStartPosition(4, 0), 3)
		self.assertEquals(self.ed.WordStartPosition(5, 0), 3)
		self.assertEquals(self.ed.WordStartPosition(6, 0), 5)

		self.assertEquals(self.ed.WordEndPosition(2, 0), 3)
		self.assertEquals(self.ed.WordEndPosition(3, 0), 5)
		self.assertEquals(self.ed.WordEndPosition(4, 0), 5)
		self.assertEquals(self.ed.WordEndPosition(5, 0), 6)
		self.assertEquals(self.ed.WordEndPosition(6, 0), 8)

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
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 0)
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO | REDO)
		self.ed.Redo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()

	def testContainerActCoalesce(self):
		self.ed.InsertText(0, self.data)
		self.ed.AddUndoAction(5, 1)
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)

	def testContainerMultiStage(self):
		self.ed.InsertText(0, self.data)
		self.ed.AddUndoAction(5, 1)
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), REDO)

	def testContainerMultiStageNoText(self):
		self.ed.AddUndoAction(5, 1)
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEquals(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEquals(self.UndoState(), REDO)

	def testContainerActCoalesceEnd(self):
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.InsertText(0, self.data)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), REDO)
		self.ed.Redo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)

	def testContainerBetweenInsertAndInsert(self):
		self.assertEquals(self.ed.Length, 0)
		self.ed.InsertText(0, self.data)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.InsertText(2, self.data)
		self.assertEquals(self.ed.Length, 4)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		# Undoes both insertions and the containerAction in the middle
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), REDO)

	def testContainerNoCoalesceBetweenInsertAndInsert(self):
		self.assertEquals(self.ed.Length, 0)
		self.ed.InsertText(0, self.data)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 0)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.InsertText(2, self.data)
		self.assertEquals(self.ed.Length, 4)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		# Undo last insertion
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO | REDO)
		# Undo container
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO | REDO)
		# Undo first insertion
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 0)
		self.assertEquals(self.UndoState(), REDO)

	def testContainerBetweenDeleteAndDelete(self):
		self.ed.InsertText(0, self.data)
		self.ed.EmptyUndoBuffer()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), 0)
		self.ed.SetSel(2,2)
		self.ed.DeleteBack()
		self.assertEquals(self.ed.Length, 1)
		self.ed.AddUndoAction(5, 1)
		self.ed.DeleteBack()
		self.assertEquals(self.ed.Length, 0)
		# Undoes both deletions and the containerAction in the middle
		self.ed.Undo()
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), REDO)

	def testContainerBetweenInsertAndDelete(self):
		self.assertEquals(self.ed.Length, 0)
		self.ed.InsertText(0, self.data)
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.AddUndoAction(5, 1)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.SetSel(0,1)
		self.ed.Cut()
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.UndoState(), MODI | UNDO)
		self.ed.Undo()	# Only undoes the deletion
		self.assertEquals(self.ed.Length, 2)
		self.assertEquals(self.UndoState(), MODI | UNDO | REDO)

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
		self.assertEquals(self.selRange(), (6, 6))
		self.ed.LineUp()
		self.assertEquals(self.selRange(), (3, 3))
		self.ed.LineDownExtend()
		self.assertEquals(self.selRange(), (6, 3))
		self.ed.LineUpExtend()
		self.ed.LineUpExtend()
		self.assertEquals(self.selRange(), (0, 3))

	def testCharMove(self):
		self.ed.AddText(8, b"x1\ny2\nz3")
		self.ed.SetSel(0,0)
		self.ed.CharRight()
		self.ed.CharRight()
		self.assertEquals(self.selRange(), (2, 2))
		self.ed.CharLeft()
		self.assertEquals(self.selRange(), (1, 1))
		self.ed.CharRightExtend()
		self.assertEquals(self.selRange(), (2, 1))
		self.ed.CharLeftExtend()
		self.ed.CharLeftExtend()
		self.assertEquals(self.selRange(), (0, 1))

	def testWordMove(self):
		self.ed.AddText(10, b"a big boat")
		self.ed.SetSel(3,3)
		self.ed.WordRight()
		self.ed.WordRight()
		self.assertEquals(self.selRange(), (10, 10))
		self.ed.WordLeft()
		self.assertEquals(self.selRange(), (6, 6))
		self.ed.WordRightExtend()
		self.assertEquals(self.selRange(), (10, 6))
		self.ed.WordLeftExtend()
		self.ed.WordLeftExtend()
		self.assertEquals(self.selRange(), (2, 6))

	def testHomeEndMove(self):
		self.ed.AddText(10, b"a big boat")
		self.ed.SetSel(3,3)
		self.ed.Home()
		self.assertEquals(self.selRange(), (0, 0))
		self.ed.LineEnd()
		self.assertEquals(self.selRange(), (10, 10))
		self.ed.SetSel(3,3)
		self.ed.HomeExtend()
		self.assertEquals(self.selRange(), (0, 3))
		self.ed.LineEndExtend()
		self.assertEquals(self.selRange(), (10, 3))

	def testStartEndMove(self):
		self.ed.AddText(10, b"a\nbig\nboat")
		self.ed.SetSel(3,3)
		self.ed.DocumentStart()
		self.assertEquals(self.selRange(), (0, 0))
		self.ed.DocumentEnd()
		self.assertEquals(self.selRange(), (10, 10))
		self.ed.SetSel(3,3)
		self.ed.DocumentStartExtend()
		self.assertEquals(self.selRange(), (0, 3))
		self.ed.DocumentEndExtend()
		self.assertEquals(self.selRange(), (10, 3))


class TestMarkers(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.AddText(5, b"x\ny\nz")

	def testMarker(self):
		handle = self.ed.MarkerAdd(1,1)
		self.assertEquals(self.ed.MarkerLineFromHandle(handle), 1)
		self.ed.MarkerDelete(1,1)
		self.assertEquals(self.ed.MarkerLineFromHandle(handle), -1)

	def testTwiceAddedDelete(self):
		handle = self.ed.MarkerAdd(1,1)
		self.assertEquals(self.ed.MarkerGet(1), 2)
		handle2 = self.ed.MarkerAdd(1,1)
		self.assertEquals(self.ed.MarkerGet(1), 2)
		self.ed.MarkerDelete(1,1)
		self.assertEquals(self.ed.MarkerGet(1), 2)
		self.ed.MarkerDelete(1,1)
		self.assertEquals(self.ed.MarkerGet(1), 0)

	def testMarkerDeleteAll(self):
		h1 = self.ed.MarkerAdd(0,1)
		h2 = self.ed.MarkerAdd(1,2)
		self.assertEquals(self.ed.MarkerLineFromHandle(h1), 0)
		self.assertEquals(self.ed.MarkerLineFromHandle(h2), 1)
		self.ed.MarkerDeleteAll(1)
		self.assertEquals(self.ed.MarkerLineFromHandle(h1), -1)
		self.assertEquals(self.ed.MarkerLineFromHandle(h2), 1)
		self.ed.MarkerDeleteAll(-1)
		self.assertEquals(self.ed.MarkerLineFromHandle(h1), -1)
		self.assertEquals(self.ed.MarkerLineFromHandle(h2), -1)

	def testMarkerDeleteHandle(self):
		handle = self.ed.MarkerAdd(0,1)
		self.assertEquals(self.ed.MarkerLineFromHandle(handle), 0)
		self.ed.MarkerDeleteHandle(handle)
		self.assertEquals(self.ed.MarkerLineFromHandle(handle), -1)

	def testMarkerBits(self):
		self.assertEquals(self.ed.MarkerGet(0), 0)
		self.ed.MarkerAdd(0,1)
		self.assertEquals(self.ed.MarkerGet(0), 2)
		self.ed.MarkerAdd(0,2)
		self.assertEquals(self.ed.MarkerGet(0), 6)

	def testMarkerAddSet(self):
		self.assertEquals(self.ed.MarkerGet(0), 0)
		self.ed.MarkerAddSet(0,5)
		self.assertEquals(self.ed.MarkerGet(0), 5)
		self.ed.MarkerDeleteAll(-1)

	def testMarkerNext(self):
		self.assertEquals(self.ed.MarkerNext(0, 2), -1)
		h1 = self.ed.MarkerAdd(0,1)
		h2 = self.ed.MarkerAdd(2,1)
		self.assertEquals(self.ed.MarkerNext(0, 2), 0)
		self.assertEquals(self.ed.MarkerNext(1, 2), 2)
		self.assertEquals(self.ed.MarkerNext(2, 2), 2)
		self.assertEquals(self.ed.MarkerPrevious(0, 2), 0)
		self.assertEquals(self.ed.MarkerPrevious(1, 2), 0)
		self.assertEquals(self.ed.MarkerPrevious(2, 2), 2)

	def testMarkerNegative(self):
		self.assertEquals(self.ed.MarkerNext(-1, 2), -1)

	def testLineState(self):
		self.assertEquals(self.ed.MaxLineState, 0)
		self.assertEquals(self.ed.GetLineState(0), 0)
		self.assertEquals(self.ed.GetLineState(1), 0)
		self.assertEquals(self.ed.GetLineState(2), 0)
		self.ed.SetLineState(1, 100)
		self.assertNotEquals(self.ed.MaxLineState, 0)
		self.assertEquals(self.ed.GetLineState(0), 0)
		self.assertEquals(self.ed.GetLineState(1), 100)
		self.assertEquals(self.ed.GetLineState(2), 0)

	def testSymbolRetrieval(self):
		self.ed.MarkerDefine(1,3)
		self.assertEquals(self.ed.MarkerSymbolDefined(1), 3)

class TestIndicators(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testSetIndicator(self):
		self.assertEquals(self.ed.IndicGetStyle(0), 1)
		self.assertEquals(self.ed.IndicGetFore(0), 0x007f00)
		self.ed.IndicSetStyle(0, 2)
		self.ed.IndicSetFore(0, 0xff0080)
		self.assertEquals(self.ed.IndicGetStyle(0), 2)
		self.assertEquals(self.ed.IndicGetFore(0), 0xff0080)

	def testIndicatorFill(self):
		self.ed.InsertText(0, b"abc")
		self.ed.IndicatorCurrent = 3
		self.ed.IndicatorFillRange(1,1)
		self.assertEquals(self.ed.IndicatorValueAt(3, 0), 0)
		self.assertEquals(self.ed.IndicatorValueAt(3, 1), 1)
		self.assertEquals(self.ed.IndicatorValueAt(3, 2), 0)
		self.assertEquals(self.ed.IndicatorStart(3, 0), 0)
		self.assertEquals(self.ed.IndicatorEnd(3, 0), 1)
		self.assertEquals(self.ed.IndicatorStart(3, 1), 1)
		self.assertEquals(self.ed.IndicatorEnd(3, 1), 2)
		self.assertEquals(self.ed.IndicatorStart(3, 2), 2)
		self.assertEquals(self.ed.IndicatorEnd(3, 2), 3)

	def testIndicatorAtEnd(self):
		self.ed.InsertText(0, b"ab")
		self.ed.IndicatorCurrent = 3
		self.ed.IndicatorFillRange(1,1)
		self.assertEquals(self.ed.IndicatorValueAt(3, 0), 0)
		self.assertEquals(self.ed.IndicatorValueAt(3, 1), 1)
		self.assertEquals(self.ed.IndicatorStart(3, 0), 0)
		self.assertEquals(self.ed.IndicatorEnd(3, 0), 1)
		self.assertEquals(self.ed.IndicatorStart(3, 1), 1)
		self.assertEquals(self.ed.IndicatorEnd(3, 1), 2)
		self.ed.DeleteRange(1, 1)
		# Now only one character left and does not have indicator so indicator 3 is null
		self.assertEquals(self.ed.IndicatorValueAt(3, 0), 0)
		# Since null, remaining calls return 0
		self.assertEquals(self.ed.IndicatorStart(3, 0), 0)
		self.assertEquals(self.ed.IndicatorEnd(3, 0), 0)
		self.assertEquals(self.ed.IndicatorStart(3, 1), 0)
		self.assertEquals(self.ed.IndicatorEnd(3, 1), 0)

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
		self.assertEquals(self.ed.FirstVisibleLine, 0)

	def testLineScroll(self):
		self.ed.GotoLine(0)
		self.ed.LineScroll(0, 3)
		self.assertEquals(self.ed.FirstVisibleLine, 3)
		self.ed.LineScroll(0, -2)
		self.assertEquals(self.ed.FirstVisibleLine, 1)
		self.assertEquals(self.ed.XOffset, 0)
		self.ed.LineScroll(10, 0)
		self.assertGreater(self.ed.XOffset, 0)
		scroll_width = float(self.ed.XOffset) / 10
		self.ed.LineScroll(-2, 0)
		self.assertEquals(self.ed.XOffset, scroll_width * 8)

	def testVisibleLine(self):
		self.ed.FirstVisibleLine = 7
		self.assertEquals(self.ed.FirstVisibleLine, 7)

class TestSearch(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.InsertText(0, b"a\tbig boat\t")

	def testFind(self):
		pos = self.ed.FindBytes(0, self.ed.Length, b"zzz", 0)
		self.assertEquals(pos, -1)
		pos = self.ed.FindBytes(0, self.ed.Length, b"big", 0)
		self.assertEquals(pos, 2)

	def testFindEmpty(self):
		pos = self.ed.FindBytes(0, self.ed.Length, b"", 0)
		self.assertEquals(pos, 0)

	def testCaseFind(self):
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"big", 0), 2)
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"bIg", 0), 2)
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"bIg",
			self.ed.SCFIND_MATCHCASE), -1)

	def testWordFind(self):
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"bi", 0), 2)
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"bi",
			self.ed.SCFIND_WHOLEWORD), -1)

	def testWordStartFind(self):
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"bi", 0), 2)
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"bi",
			self.ed.SCFIND_WORDSTART), 2)
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"ig", 0), 3)
		self.assertEquals(self.ed.FindBytes(0, self.ed.Length, b"ig",
			self.ed.SCFIND_WORDSTART), -1)

	def testREFind(self):
		flags = self.ed.SCFIND_REGEXP
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"b.g", 0))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"b.g", flags))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"\<b.g\>", flags))
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"b[A-Z]g",
			flags | self.ed.SCFIND_MATCHCASE))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]g", flags))
		self.assertEquals(6, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]*t", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"^a", flags))
		self.assertEquals(10, self.ed.FindBytes(0, self.ed.Length, b"\t$", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"\([a]\).*\0", flags))

	def testPosixREFind(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_POSIX
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"b.g", 0))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"b.g", flags))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"\<b.g\>", flags))
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"b[A-Z]g",
			flags | self.ed.SCFIND_MATCHCASE))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]g", flags))
		self.assertEquals(6, self.ed.FindBytes(0, self.ed.Length, b"b[a-z]*t", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"^a", flags))
		self.assertEquals(10, self.ed.FindBytes(0, self.ed.Length, b"\t$", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"([a]).*\0", flags))

	def testPhilippeREFind(self):
		# Requires 1.,72
		flags = self.ed.SCFIND_REGEXP
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"\w", flags))
		self.assertEquals(1, self.ed.FindBytes(0, self.ed.Length, b"\W", flags))
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"\d", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"\D", flags))
		self.assertEquals(1, self.ed.FindBytes(0, self.ed.Length, b"\s", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"\S", flags))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"\x62", flags))

	def testRENonASCII(self):
		self.ed.InsertText(0, b"\xAD")
		flags = self.ed.SCFIND_REGEXP
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"\\x10", flags))
		self.assertEquals(2, self.ed.FindBytes(0, self.ed.Length, b"\\x09", flags))
		self.assertEquals(-1, self.ed.FindBytes(0, self.ed.Length, b"\\xAB", flags))
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"\\xAD", flags))

class TestRepresentations(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testGetControl(self):
		result = self.ed.GetRepresentation(b"\001")
		self.assertEquals(result, b"SOH")

	def testClearControl(self):
		result = self.ed.GetRepresentation(b"\002")
		self.assertEquals(result, b"STX")
		self.ed.ClearRepresentation(b"\002")
		result = self.ed.GetRepresentation(b"\002")
		self.assertEquals(result, b"")

	def testSetOhm(self):
		ohmSign = b"\xe2\x84\xa6"
		ohmExplained = b"U+2126 \xe2\x84\xa6"
		self.ed.SetRepresentation(ohmSign, ohmExplained)
		result = self.ed.GetRepresentation(ohmSign)
		self.assertEquals(result, ohmExplained)

class TestProperties(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testSet(self):
		self.ed.SetProperty(b"test", b"12")
		self.assertEquals(self.ed.GetPropertyInt(b"test"), 12)
		result = self.ed.GetProperty(b"test")
		self.assertEquals(result, b"12")
		self.ed.SetProperty(b"test.plus", b"[$(test)]")
		result = self.ed.GetPropertyExpanded(b"test.plus")
		self.assertEquals(result, b"[12]")

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
		self.assertEquals(self.ed.ExtraAscent, 0)
		self.assertEquals(self.ed.ExtraDescent, 0)
		self.ed.ExtraAscent = 1
		self.assertEquals(self.ed.ExtraAscent, 1)
		self.ed.ExtraDescent = 2
		self.assertEquals(self.ed.ExtraDescent, 2)
		lineHeightIncreased = self.ed.TextHeight(0)
		self.assertEquals(lineHeightIncreased, lineHeight + 2 + 1)

	def testTextMargin(self):
		self.ed.MarginSetText(0, self.txt)
		result = self.ed.MarginGetText(0)
		self.assertEquals(result, self.txt)
		self.ed.MarginTextClearAll()

	def testTextMarginStyle(self):
		self.ed.MarginSetText(0, self.txt)
		self.ed.MarginSetStyle(0, 33)
		self.assertEquals(self.ed.MarginGetStyle(0), 33)
		self.ed.MarginTextClearAll()

	def testTextMarginStyles(self):
		styles = b"\001\002\003\004"
		self.ed.MarginSetText(0, self.txt)
		self.ed.MarginSetStyles(0, styles)
		result = self.ed.MarginGetStyles(0)
		self.assertEquals(result, styles)
		self.ed.MarginTextClearAll()

	def testTextMarginStyleOffset(self):
		self.ed.MarginSetStyleOffset(300)
		self.assertEquals(self.ed.MarginGetStyleOffset(), 300)

class TestAnnotation(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.txt = b"abcd"
		self.ed.AddText(1, b"x")

	def testTextAnnotation(self):
		self.assertEquals(self.ed.AnnotationGetLines(), 0)
		self.ed.AnnotationSetText(0, self.txt)
		self.assertEquals(self.ed.AnnotationGetLines(), 1)
		result = self.ed.AnnotationGetText(0)
		self.assertEquals(len(result), 4)
		self.assertEquals(result, self.txt)
		self.ed.AnnotationClearAll()

	def testTextAnnotationStyle(self):
		self.ed.AnnotationSetText(0, self.txt)
		self.ed.AnnotationSetStyle(0, 33)
		self.assertEquals(self.ed.AnnotationGetStyle(0), 33)
		self.ed.AnnotationClearAll()

	def testTextAnnotationStyles(self):
		styles = b"\001\002\003\004"
		self.ed.AnnotationSetText(0, self.txt)
		self.ed.AnnotationSetStyles(0, styles)
		result = self.ed.AnnotationGetStyles(0)
		self.assertEquals(result, styles)
		self.ed.AnnotationClearAll()

	def testExtendedStyles(self):
		start0 = self.ed.AllocateExtendedStyles(0)
		self.assertEquals(start0, 256)
		start1 = self.ed.AllocateExtendedStyles(10)
		self.assertEquals(start1, 256)
		start2 = self.ed.AllocateExtendedStyles(20)
		self.assertEquals(start2, start1 + 10)
		# Reset by changing lexer
		self.ed.ReleaseAllExtendedStyles()
		start0 = self.ed.AllocateExtendedStyles(0)
		self.assertEquals(start0, 256)

	def testTextAnnotationStyleOffset(self):
		self.ed.AnnotationSetStyleOffset(300)
		self.assertEquals(self.ed.AnnotationGetStyleOffset(), 300)

	def testTextAnnotationVisible(self):
		self.assertEquals(self.ed.AnnotationGetVisible(), 0)
		self.ed.AnnotationSetVisible(2)
		self.assertEquals(self.ed.AnnotationGetVisible(), 2)
		self.ed.AnnotationSetVisible(0)

class TestMultiSelection(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 3 lines of 3 characters
		t = b"xxx\nxxx\nxxx"
		self.ed.AddText(len(t), t)

	def testSelectionCleared(self):
		self.ed.ClearSelections()
		self.assertEquals(self.ed.Selections, 1)
		self.assertEquals(self.ed.MainSelection, 0)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 0)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 0)

	def test1Selection(self):
		self.ed.SetSelection(1, 2)
		self.assertEquals(self.ed.Selections, 1)
		self.assertEquals(self.ed.MainSelection, 0)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 1)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEquals(self.ed.GetSelectionNStart(0), 1)
		self.assertEquals(self.ed.GetSelectionNEnd(0), 2)
		self.ed.SwapMainAnchorCaret()
		self.assertEquals(self.ed.Selections, 1)
		self.assertEquals(self.ed.MainSelection, 0)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 2)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 1)

	def test1SelectionReversed(self):
		self.ed.SetSelection(2, 1)
		self.assertEquals(self.ed.Selections, 1)
		self.assertEquals(self.ed.MainSelection, 0)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 2)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 1)
		self.assertEquals(self.ed.GetSelectionNStart(0), 1)
		self.assertEquals(self.ed.GetSelectionNEnd(0), 2)

	def test1SelectionByStartEnd(self):
		self.ed.SetSelectionNStart(0, 2)
		self.ed.SetSelectionNEnd(0, 3)
		self.assertEquals(self.ed.Selections, 1)
		self.assertEquals(self.ed.MainSelection, 0)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 3)
		self.assertEquals(self.ed.GetSelectionNStart(0), 2)
		self.assertEquals(self.ed.GetSelectionNEnd(0), 3)

	def test2Selections(self):
		self.ed.SetSelection(1, 2)
		self.ed.AddSelection(4, 5)
		self.assertEquals(self.ed.Selections, 2)
		self.assertEquals(self.ed.MainSelection, 1)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 1)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 2)
		self.assertEquals(self.ed.GetSelectionNCaret(1), 4)
		self.assertEquals(self.ed.GetSelectionNAnchor(1), 5)
		self.assertEquals(self.ed.GetSelectionNStart(0), 1)
		self.assertEquals(self.ed.GetSelectionNEnd(0), 2)
		self.ed.MainSelection = 0
		self.assertEquals(self.ed.MainSelection, 0)
		self.ed.RotateSelection()
		self.assertEquals(self.ed.MainSelection, 1)

	def testRectangularSelection(self):
		self.ed.RectangularSelectionAnchor = 1
		self.assertEquals(self.ed.RectangularSelectionAnchor, 1)
		self.ed.RectangularSelectionCaret = 10
		self.assertEquals(self.ed.RectangularSelectionCaret, 10)
		self.assertEquals(self.ed.Selections, 3)
		self.assertEquals(self.ed.MainSelection, 2)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 1)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 2)
		self.assertEquals(self.ed.GetSelectionNAnchor(1), 5)
		self.assertEquals(self.ed.GetSelectionNCaret(1), 6)
		self.assertEquals(self.ed.GetSelectionNAnchor(2), 9)
		self.assertEquals(self.ed.GetSelectionNCaret(2), 10)

	def testVirtualSpace(self):
		self.ed.SetSelection(3, 7)
		self.ed.SetSelectionNCaretVirtualSpace(0, 3)
		self.assertEquals(self.ed.GetSelectionNCaretVirtualSpace(0), 3)
		self.ed.SetSelectionNAnchorVirtualSpace(0, 2)
		self.assertEquals(self.ed.GetSelectionNAnchorVirtualSpace(0), 2)
		# Does not check that virtual space is valid by being at end of line
		self.ed.SetSelection(1, 1)
		self.ed.SetSelectionNCaretVirtualSpace(0, 3)
		self.assertEquals(self.ed.GetSelectionNCaretVirtualSpace(0), 3)

	def testRectangularVirtualSpace(self):
		self.ed.VirtualSpaceOptions=1
		self.ed.RectangularSelectionAnchor = 3
		self.assertEquals(self.ed.RectangularSelectionAnchor, 3)
		self.ed.RectangularSelectionCaret = 7
		self.assertEquals(self.ed.RectangularSelectionCaret, 7)
		self.ed.RectangularSelectionAnchorVirtualSpace = 1
		self.assertEquals(self.ed.RectangularSelectionAnchorVirtualSpace, 1)
		self.ed.RectangularSelectionCaretVirtualSpace = 10
		self.assertEquals(self.ed.RectangularSelectionCaretVirtualSpace, 10)
		self.assertEquals(self.ed.Selections, 2)
		self.assertEquals(self.ed.MainSelection, 1)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 3)
		self.assertEquals(self.ed.GetSelectionNAnchorVirtualSpace(0), 1)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 3)
		self.assertEquals(self.ed.GetSelectionNCaretVirtualSpace(0), 10)

	def testRectangularVirtualSpaceOptionOff(self):
		# Same as previous test but virtual space option off so no virtual space in result
		self.ed.VirtualSpaceOptions=0
		self.ed.RectangularSelectionAnchor = 3
		self.assertEquals(self.ed.RectangularSelectionAnchor, 3)
		self.ed.RectangularSelectionCaret = 7
		self.assertEquals(self.ed.RectangularSelectionCaret, 7)
		self.ed.RectangularSelectionAnchorVirtualSpace = 1
		self.assertEquals(self.ed.RectangularSelectionAnchorVirtualSpace, 1)
		self.ed.RectangularSelectionCaretVirtualSpace = 10
		self.assertEquals(self.ed.RectangularSelectionCaretVirtualSpace, 10)
		self.assertEquals(self.ed.Selections, 2)
		self.assertEquals(self.ed.MainSelection, 1)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 3)
		self.assertEquals(self.ed.GetSelectionNAnchorVirtualSpace(0), 0)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 3)
		self.assertEquals(self.ed.GetSelectionNCaretVirtualSpace(0), 0)

	def testDropSelectionN(self):
		self.ed.SetSelection(1, 2)
		# Only one so dropping has no effect
		self.ed.DropSelectionN(0)
		self.assertEquals(self.ed.Selections, 1)
		self.ed.AddSelection(4, 5)
		self.assertEquals(self.ed.Selections, 2)
		# Outside bounds so no effect
		self.ed.DropSelectionN(2)
		self.assertEquals(self.ed.Selections, 2)
		# Dropping before main so main decreases
		self.ed.DropSelectionN(0)
		self.assertEquals(self.ed.Selections, 1)
		self.assertEquals(self.ed.MainSelection, 0)
		self.assertEquals(self.ed.GetSelectionNCaret(0), 4)
		self.assertEquals(self.ed.GetSelectionNAnchor(0), 5)

		self.ed.AddSelection(10, 11)
		self.ed.AddSelection(20, 21)
		self.assertEquals(self.ed.Selections, 3)
		self.assertEquals(self.ed.MainSelection, 2)
		self.ed.MainSelection = 1
		# Dropping after main so main does not change
		self.ed.DropSelectionN(2)
		self.assertEquals(self.ed.MainSelection, 1)

		# Dropping first selection so wraps around to new last.
		self.ed.AddSelection(30, 31)
		self.ed.AddSelection(40, 41)
		self.assertEquals(self.ed.Selections, 4)
		self.ed.MainSelection = 0
		self.ed.DropSelectionN(0)
		self.assertEquals(self.ed.MainSelection, 2)

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

	def tearDown(self):
		self.ed.StyleResetDefault()

	def testFont(self):
		self.ed.StyleSetFont(self.ed.STYLE_DEFAULT, self.testFont)
		self.assertEquals(self.ed.StyleGetFont(self.ed.STYLE_DEFAULT), self.testFont)

	def testSize(self):
		self.ed.StyleSetSize(self.ed.STYLE_DEFAULT, 12)
		self.assertEquals(self.ed.StyleGetSize(self.ed.STYLE_DEFAULT), 12)
		self.assertEquals(self.ed.StyleGetSizeFractional(self.ed.STYLE_DEFAULT), 12*self.ed.SC_FONT_SIZE_MULTIPLIER)
		self.ed.StyleSetSizeFractional(self.ed.STYLE_DEFAULT, 1234)
		self.assertEquals(self.ed.StyleGetSizeFractional(self.ed.STYLE_DEFAULT), 1234)

	def testBold(self):
		self.ed.StyleSetBold(self.ed.STYLE_DEFAULT, 1)
		self.assertEquals(self.ed.StyleGetBold(self.ed.STYLE_DEFAULT), 1)
		self.assertEquals(self.ed.StyleGetWeight(self.ed.STYLE_DEFAULT), self.ed.SC_WEIGHT_BOLD)
		self.ed.StyleSetWeight(self.ed.STYLE_DEFAULT, 530)
		self.assertEquals(self.ed.StyleGetWeight(self.ed.STYLE_DEFAULT), 530)

	def testItalic(self):
		self.ed.StyleSetItalic(self.ed.STYLE_DEFAULT, 1)
		self.assertEquals(self.ed.StyleGetItalic(self.ed.STYLE_DEFAULT), 1)

	def testUnderline(self):
		self.assertEquals(self.ed.StyleGetUnderline(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetUnderline(self.ed.STYLE_DEFAULT, 1)
		self.assertEquals(self.ed.StyleGetUnderline(self.ed.STYLE_DEFAULT), 1)

	def testFore(self):
		self.assertEquals(self.ed.StyleGetFore(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetFore(self.ed.STYLE_DEFAULT, self.testColour)
		self.assertEquals(self.ed.StyleGetFore(self.ed.STYLE_DEFAULT), self.testColour)

	def testBack(self):
		self.assertEquals(self.ed.StyleGetBack(self.ed.STYLE_DEFAULT), 0xffffff)
		self.ed.StyleSetBack(self.ed.STYLE_DEFAULT, self.testColour)
		self.assertEquals(self.ed.StyleGetBack(self.ed.STYLE_DEFAULT), self.testColour)

	def testEOLFilled(self):
		self.assertEquals(self.ed.StyleGetEOLFilled(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetEOLFilled(self.ed.STYLE_DEFAULT, 1)
		self.assertEquals(self.ed.StyleGetEOLFilled(self.ed.STYLE_DEFAULT), 1)

	def testCharacterSet(self):
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		self.assertEquals(self.ed.StyleGetCharacterSet(self.ed.STYLE_DEFAULT), self.ed.SC_CHARSET_RUSSIAN)

	def testCase(self):
		self.assertEquals(self.ed.StyleGetCase(self.ed.STYLE_DEFAULT), self.ed.SC_CASE_MIXED)
		self.ed.StyleSetCase(self.ed.STYLE_DEFAULT, self.ed.SC_CASE_UPPER)
		self.assertEquals(self.ed.StyleGetCase(self.ed.STYLE_DEFAULT), self.ed.SC_CASE_UPPER)
		self.ed.StyleSetCase(self.ed.STYLE_DEFAULT, self.ed.SC_CASE_LOWER)
		self.assertEquals(self.ed.StyleGetCase(self.ed.STYLE_DEFAULT), self.ed.SC_CASE_LOWER)

	def testVisible(self):
		self.assertEquals(self.ed.StyleGetVisible(self.ed.STYLE_DEFAULT), 1)
		self.ed.StyleSetVisible(self.ed.STYLE_DEFAULT, 0)
		self.assertEquals(self.ed.StyleGetVisible(self.ed.STYLE_DEFAULT), 0)

	def testChangeable(self):
		self.assertEquals(self.ed.StyleGetChangeable(self.ed.STYLE_DEFAULT), 1)
		self.ed.StyleSetChangeable(self.ed.STYLE_DEFAULT, 0)
		self.assertEquals(self.ed.StyleGetChangeable(self.ed.STYLE_DEFAULT), 0)

	def testHotSpot(self):
		self.assertEquals(self.ed.StyleGetHotSpot(self.ed.STYLE_DEFAULT), 0)
		self.ed.StyleSetHotSpot(self.ed.STYLE_DEFAULT, 1)
		self.assertEquals(self.ed.StyleGetHotSpot(self.ed.STYLE_DEFAULT), 1)

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
			self.assert_(after > i)
			back = self.ed.PositionBefore(after)
			self.assertEquals(pos, back)
			pos = after

	def testRelative(self):
		# \x61  \xc3\xa5  \xef\xac\x82   \xef\xac\x94   \x2d
		t = "aåﬂﬔ-"
		tv = t.encode("UTF-8")
		self.ed.SetContents(tv)
		self.assertEquals(self.ed.PositionRelative(1, 2), 6)
		self.assertEquals(self.ed.PositionRelative(6, -2), 1)
		pos = 0
		previous = 0
		for i in range(1, len(t)):
			after = self.ed.PositionRelative(pos, i)
			self.assert_(after > pos)
			self.assert_(after > previous)
			previous = after
		pos = len(t)
		previous = pos
		for i in range(1, len(t)-1):
			after = self.ed.PositionRelative(pos, -i)
			self.assert_(after < pos)
			self.assert_(after < previous)
			previous = after

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
		self.assertEquals(self.ed.Contents(), b"x")

	def testASCII(self):
		t = b"x"
		self.ed.SetContents(t)
		self.ed.SetSel(0,1)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), b"X")

	def testLatin1(self):
		t = "å".encode("Latin-1")
		r = "Å".encode("Latin-1")
		self.ed.SetContents(t)
		self.ed.SetSel(0,1)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), r)

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
		self.assertEquals(self.ed.Contents(), r)

	def testUTF(self):
		self.ed.SetCodePage(65001)
		t = "å".encode("UTF-8")
		r = "Å".encode("UTF-8")
		self.ed.SetContents(t)
		self.ed.SetSel(0,2)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), r)

	def testUTFDifferentLength(self):
		self.ed.SetCodePage(65001)
		t = "ı".encode("UTF-8")
		r = "I".encode("UTF-8")
		self.ed.SetContents(t)
		self.assertEquals(self.ed.Length, 2)
		self.ed.SetSel(0,2)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.ed.Contents(), r)

	def testUTFGrows(self):
		# This crashed at one point in debug builds due to looking past end of shorter string
		self.ed.SetCodePage(65001)
		# ﬖ is a single character ligature taking 3 bytes in UTF8: EF AC 96
		t = 'ﬖﬖ'.encode("UTF-8")
		self.ed.SetContents(t)
		self.assertEquals(self.ed.Length, 6)
		self.ed.SetSel(0,self.ed.Length)
		self.ed.UpperCase()
		# To convert to upper case the ligature is separated into վ and ն then uppercased to Վ and Ն
		# each of which takes 2 bytes in UTF-8: D5 8E D5 86
		r = 'ՎՆՎՆ'.encode("UTF-8")
		self.assertEquals(self.ed.Length, 8)
		self.assertEquals(self.ed.Contents(), r)
		self.assertEquals(self.ed.SelectionEnd, self.ed.Length)

	def testUTFShrinks(self):
		self.ed.SetCodePage(65001)
		# ﬁ is a single character ligature taking 3 bytes in UTF8: EF AC 81
		t = 'ﬁﬁ'.encode("UTF-8")
		self.ed.SetContents(t)
		self.assertEquals(self.ed.Length, 6)
		self.ed.SetSel(0,self.ed.Length)
		self.ed.UpperCase()
		# To convert to upper case the ligature is separated into f and i then uppercased to F and I
		# each of which takes 1 byte in UTF-8: 46 49
		r = 'FIFI'.encode("UTF-8")
		self.assertEquals(self.ed.Length, 4)
		self.assertEquals(self.ed.Contents(), r)
		self.assertEquals(self.ed.SelectionEnd, self.ed.Length)

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
		self.assertEquals(0, pos)

	def testASCII(self):
		text = b" x X"
		searchString = b"X"
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(1, pos)

	def testLatin1(self):
		text = "Frånd Åå".encode("Latin-1")
		searchString = "Å".encode("Latin-1")
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(2, pos)

	def testRussian(self):
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		text = "=(Б tex б)".encode("Windows-1251")
		searchString = "б".encode("Windows-1251")
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(2, pos)

	def testUTF(self):
		self.ed.SetCodePage(65001)
		text = "Frånd Åå".encode("UTF-8")
		searchString = "Å".encode("UTF-8")
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(2, pos)

	def testUTFDifferentLength(self):
		# Searching for a two byte string finds a single byte
		self.ed.SetCodePage(65001)
		# two byte string "ſ" single byte "s"
		text = "Frånds Ååſ $".encode("UTF-8")
		searchString = "ſ".encode("UTF-8")
		firstPosition = len("Frånd".encode("UTF-8"))
		self.assertEquals(len(searchString), 2)
		self.ed.SetContents(text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(firstPosition, pos)
		self.assertEquals(firstPosition+1, self.ed.TargetEnd)

class TestLexer(unittest.TestCase):
	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testLexerNumber(self):
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.assertEquals(self.ed.GetLexer(), self.ed.SCLEX_CPP)

	def testLexerName(self):
		self.ed.LexerLanguage = b"cpp"
		self.assertEquals(self.ed.GetLexer(), self.ed.SCLEX_CPP)
		name = self.ed.GetLexerLanguage(0)
		self.assertEquals(name, b"cpp")

	def testPropertyNames(self):
		propertyNames = self.ed.PropertyNames()
		self.assertNotEquals(propertyNames, b"")
		# The cpp lexer has a boolean property named lexer.cpp.allow.dollars
		propNameDollars = b"lexer.cpp.allow.dollars"
		propertyType = self.ed.PropertyType(propNameDollars)
		self.assertEquals(propertyType, self.ed.SC_TYPE_BOOLEAN)
		propertyDescription = self.ed.DescribeProperty(propNameDollars)
		self.assertNotEquals(propertyDescription, b"")

	def testWordListDescriptions(self):
		wordSet = self.ed.DescribeKeyWordSets()
		self.assertNotEquals(wordSet, b"")

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
		self.ed.Lexer = self.ed.SCLEX_CPP
		bases = self.ed.GetSubStyleBases()
		self.assertEquals(bases, b"\x0b\x11")	# 11, 17
		self.assertEquals(self.ed.DistanceToSecondaryStyles(), 0x40)

	def testAllocate(self):
		firstSubStyle = 0x80	# Current implementation
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.assertEquals(self.ed.GetStyleFromSubStyle(firstSubStyle), firstSubStyle)
		self.assertEquals(self.ed.GetSubStylesStart(self.ed.SCE_C_IDENTIFIER), 0)
		self.assertEquals(self.ed.GetSubStylesLength(self.ed.SCE_C_IDENTIFIER), 0)
		numSubStyles = 5
		subs = self.ed.AllocateSubStyles(self.ed.SCE_C_IDENTIFIER, numSubStyles)
		self.assertEquals(subs, firstSubStyle)
		self.assertEquals(self.ed.GetSubStylesStart(self.ed.SCE_C_IDENTIFIER), firstSubStyle)
		self.assertEquals(self.ed.GetSubStylesLength(self.ed.SCE_C_IDENTIFIER), numSubStyles)
		self.assertEquals(self.ed.GetStyleFromSubStyle(subs), self.ed.SCE_C_IDENTIFIER)
		self.assertEquals(self.ed.GetStyleFromSubStyle(subs+numSubStyles-1), self.ed.SCE_C_IDENTIFIER)
		self.assertEquals(self.ed.GetStyleFromSubStyle(self.ed.SCE_C_IDENTIFIER), self.ed.SCE_C_IDENTIFIER)
		# Now free and check same as start
		self.ed.FreeSubStyles()
		self.assertEquals(self.ed.GetStyleFromSubStyle(subs), subs)
		self.assertEquals(self.ed.GetSubStylesStart(self.ed.SCE_C_IDENTIFIER), 0)
		self.assertEquals(self.ed.GetSubStylesLength(self.ed.SCE_C_IDENTIFIER), 0)

	def testInactive(self):
		firstSubStyle = 0x80	# Current implementation
		inactiveDistance = self.ed.DistanceToSecondaryStyles()
		self.ed.Lexer = self.ed.SCLEX_CPP
		numSubStyles = 5
		subs = self.ed.AllocateSubStyles(self.ed.SCE_C_IDENTIFIER, numSubStyles)
		self.assertEquals(subs, firstSubStyle)
		self.assertEquals(self.ed.GetStyleFromSubStyle(subs), self.ed.SCE_C_IDENTIFIER)
		self.assertEquals(self.ed.GetStyleFromSubStyle(subs+inactiveDistance), self.ed.SCE_C_IDENTIFIER+inactiveDistance)
		self.ed.FreeSubStyles()

	def testSecondary(self):
		inactiveDistance = self.ed.DistanceToSecondaryStyles()
		self.assertEquals(self.ed.GetPrimaryStyleFromStyle(self.ed.SCE_C_IDENTIFIER+inactiveDistance), self.ed.SCE_C_IDENTIFIER)

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
		self.assertEquals(self.ed.CallTipActive(), 0)
		self.ed.CallTipShow(1, "fun(int x)")
		self.assertEquals(self.ed.CallTipActive(), 1)
		self.assertEquals(self.ed.CallTipPosStart(), 4)
		self.ed.CallTipSetPosStart(1)
		self.assertEquals(self.ed.CallTipPosStart(), 1)
		self.ed.CallTipCancel()
		self.assertEquals(self.ed.CallTipActive(), 0)

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
		self.assertEquals(self.ed.AutoCGetSeparator(), ord(' '))
		self.assertEquals(self.ed.AutoCGetMaxHeight(), 5)
		self.assertEquals(self.ed.AutoCGetMaxWidth(), 0)
		self.assertEquals(self.ed.AutoCGetTypeSeparator(), ord('?'))
		self.assertEquals(self.ed.AutoCGetIgnoreCase(), 0)
		self.assertEquals(self.ed.AutoCGetAutoHide(), 1)
		self.assertEquals(self.ed.AutoCGetDropRestOfWord(), 0)

	def testChangeDefaults(self):
		self.ed.AutoCSetSeparator(ord('-'))
		self.assertEquals(self.ed.AutoCGetSeparator(), ord('-'))
		self.ed.AutoCSetSeparator(ord(' '))

		self.ed.AutoCSetMaxHeight(100)
		self.assertEquals(self.ed.AutoCGetMaxHeight(), 100)
		self.ed.AutoCSetMaxHeight(5)

		self.ed.AutoCSetMaxWidth(100)
		self.assertEquals(self.ed.AutoCGetMaxWidth(), 100)
		self.ed.AutoCSetMaxWidth(0)

		self.ed.AutoCSetTypeSeparator(ord('@'))
		self.assertEquals(self.ed.AutoCGetTypeSeparator(), ord('@'))
		self.ed.AutoCSetTypeSeparator(ord('?'))

		self.ed.AutoCSetIgnoreCase(1)
		self.assertEquals(self.ed.AutoCGetIgnoreCase(), 1)
		self.ed.AutoCSetIgnoreCase(0)

		self.ed.AutoCSetAutoHide(0)
		self.assertEquals(self.ed.AutoCGetAutoHide(), 0)
		self.ed.AutoCSetAutoHide(1)

		self.ed.AutoCSetDropRestOfWord(1)
		self.assertEquals(self.ed.AutoCGetDropRestOfWord(), 1)
		self.ed.AutoCSetDropRestOfWord(0)

	def testAutoShow(self):
		self.assertEquals(self.ed.AutoCActive(), 0)
		self.ed.SetSel(0, 0)

		self.ed.AutoCShow(0, b"za defn ghi")
		self.assertEquals(self.ed.AutoCActive(), 1)
		#~ time.sleep(2)
		self.assertEquals(self.ed.AutoCPosStart(), 0)
		self.assertEquals(self.ed.AutoCGetCurrent(), 0)
		t = self.ed.AutoCGetCurrentText(5)
		#~ self.assertEquals(l, 3)
		self.assertEquals(t, b"za")
		self.ed.AutoCCancel()
		self.assertEquals(self.ed.AutoCActive(), 0)

	def testAutoShowComplete(self):
		self.assertEquals(self.ed.AutoCActive(), 0)
		self.ed.SetSel(0, 0)

		self.ed.AutoCShow(0, b"za defn ghi")
		self.ed.AutoCComplete()
		self.assertEquals(self.ed.Contents(), b"zaxxx\n")

		self.assertEquals(self.ed.AutoCActive(), 0)

	def testAutoShowSelect(self):
		self.assertEquals(self.ed.AutoCActive(), 0)
		self.ed.SetSel(0, 0)

		self.ed.AutoCShow(0, b"za defn ghi")
		self.ed.AutoCSelect(0, b"d")
		self.ed.AutoCComplete()
		self.assertEquals(self.ed.Contents(), b"defnxxx\n")

		self.assertEquals(self.ed.AutoCActive(), 0)

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
		self.assertEquals(self.ed.GapPosition, 4)
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 1
		rep = b"-"
		self.ed.ReplaceTarget(len(rep), rep)
		self.assertEquals(self.ed.GapPosition, 2)

	def testCharacterPointerAndRangePointer(self):
		text = b"abcd"
		self.ed.SetContents(text)
		characterPointer = self.ed.CharacterPointer
		rangePointer = self.ed.GetRangePointer(0,3)
		self.assertEquals(characterPointer, rangePointer)
		cpBuffer = ctypes.c_char_p(characterPointer)
		self.assertEquals(cpBuffer.value, text)
		# Gap will not be moved as already moved for CharacterPointer call
		rangePointer = self.ed.GetRangePointer(1,3)
		cpBuffer = ctypes.c_char_p(rangePointer)
		self.assertEquals(cpBuffer.value, text[1:])

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
		import string
		data = self.ed.GetWordChars(None)
		expected = set(string.digits + string.ascii_letters + '_') | \
			set(chr(x) for x in range(0x80, 0x100))
		self.assertCharSetsEqual(data, expected)

	def testDefaultWhitespaceChars(self):
		# check that the default whitespace chars are as expected
		import string
		data = self.ed.GetWhitespaceChars(None)
		expected = (set(chr(x) for x in (range(0, 0x20))) | set(' ')) - \
			set(['\r', '\n'])
		self.assertCharSetsEqual(data, expected)

	def testDefaultPunctuationChars(self):
		# check that the default punctuation chars are as expected
		import string
		data = self.ed.GetPunctuationChars(None)
		expected = set(chr(x) for x in range(0x20, 0x80)) - \
			set(string.ascii_letters + string.digits + "\r\n_ ")
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
		self.assertEquals(set(data), expected)
		# now try to set it to something custom
		expected = set(range(1, 0x100, 1))
		self._setChars("punctuation", expected)
		data = self.ed.GetPunctuationChars(None)
		self.assertCharSetsEqual(data, expected)

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
		self.assertEquals(self.ed.GetNextTabStop(0,0), 0)

		# Add a tab stop at 7
		self.ed.AddTabStop(0, 7)
		# Check added
		self.assertEquals(self.ed.GetNextTabStop(0,0), 7)
		# Check does not affect line 1
		self.assertEquals(self.ed.GetNextTabStop(1,0), 0)

		# Add a tab stop at 18
		self.ed.AddTabStop(0, 18)
		# Check added
		self.assertEquals(self.ed.GetNextTabStop(0,0), 7)
		self.assertEquals(self.ed.GetNextTabStop(0,7), 18)
		# Check does not affect line 1
		self.assertEquals(self.ed.GetNextTabStop(1,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(1,7), 0)

		# Add a tab stop between others at 13
		self.ed.AddTabStop(0, 13)
		# Check added
		self.assertEquals(self.ed.GetNextTabStop(0,0), 7)
		self.assertEquals(self.ed.GetNextTabStop(0,7), 13)
		self.assertEquals(self.ed.GetNextTabStop(0,13), 18)
		# Check does not affect line 1
		self.assertEquals(self.ed.GetNextTabStop(1,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(1,7), 0)

		self.ed.ClearTabStops(0)
		# Check back to original state
		self.assertEquals(self.ed.GetNextTabStop(0,0), 0)

	def testLineInsertionDeletion(self):
		# Add a tab stop at 7 on line 1
		self.ed.AddTabStop(1, 7)
		# Check added
		self.assertEquals(self.ed.GetNextTabStop(1,0), 7)

		# More text at end
		self.ed.AddText(len(self.t), self.t)
		self.assertEquals(self.ed.GetNextTabStop(0,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(1,0), 7)
		self.assertEquals(self.ed.GetNextTabStop(2,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(3,0), 0)

		# Another 2 lines before explicit line moves the explicit tab stop
		data = b"x\ny\n"
		self.ed.InsertText(4, data)
		self.assertEquals(self.ed.GetNextTabStop(0,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(1,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(2,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(3,0), 7)
		self.assertEquals(self.ed.GetNextTabStop(4,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(5,0), 0)

		# Undo moves the explicit tab stop back
		self.ed.Undo()
		self.assertEquals(self.ed.GetNextTabStop(0,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(1,0), 7)
		self.assertEquals(self.ed.GetNextTabStop(2,0), 0)
		self.assertEquals(self.ed.GetNextTabStop(3,0), 0)

if __name__ == '__main__':
	uu = Xite.main("simpleTests")
	#~ for x in sorted(uu.keys()):
		#~ print(x, uu[x])
	#~ print()
