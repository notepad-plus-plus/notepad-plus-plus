# -*- coding: utf-8 -*-

from __future__ import with_statement
from __future__ import unicode_literals

import codecs, ctypes, os, sys, unittest

import XiteWin

class TestSimple(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testLength(self):
		self.assertEquals(self.ed.Length, 0)

	def testAddText(self):
		self.ed.AddText(1, "x")
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.ed.GetCharAt(0), ord("x"))
		self.assertEquals(self.ed.GetStyleAt(0), 0)
		self.ed.ClearAll()
		self.assertEquals(self.ed.Length, 0)

	# def testDeleteRange(self):
		# self.ed.AddText(5, b"abcde")
		# self.assertEquals(self.ed.Length, 5)
		# self.ed.DeleteRange(1, 2)
		# self.assertEquals(self.ed.Length, 3)
		# self.assertEquals(self.ed.Contents(), b"ade")

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
		# Set the mask so 0 bit changed but not 2 bit
		self.ed.StartStyling(0,0x1)
		self.ed.SetStyling(1, 0)
		self.assertEquals(self.ed.StyledTextRange(0, 2), b"x\004y\003")

		self.ed.StartStyling(0,0xff)
		self.ed.SetStylingEx(2, b"\100\101")
		self.assertEquals(self.ed.StyledTextRange(0, 2), b"x\100y\101")

	def testPosition(self):
		self.assertEquals(self.ed.CurrentPos, 0)
		self.assertEquals(self.ed.Anchor, 0)
		self.ed.AddText(1, "x")
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

	def testSelection(self):
		self.assertEquals(self.ed.CurrentPos, 0)
		self.assertEquals(self.ed.Anchor, 0)
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 0)
		self.ed.AddText(1, "x")
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
		result = b"\0" * 5
		length = self.ed.GetSelText(0, result)
		self.assertEquals(length, 3)
		self.assertEquals(result[:length], b"bc\0")
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
			self.xite.DoEvents()
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
			# else:
				# self.assertEquals(self.ed.FindColumn(0, col), 1)
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
		data = b"\0" * 100
		caret = self.ed.GetCurLine(len(data), data)
		data = data.rstrip(b"\0")
		self.assertEquals(caret, 1)
		self.assertEquals(data, b"x")

	def testGetLine(self):
		self.ed.AddText(1, b"x")
		data = b"\0" * 100
		length = self.ed.GetLine(0, data)
		self.assertEquals(length, 1)
		data = data[:length]
		self.assertEquals(data, b"x")

	def testLineEnds(self):
		self.ed.AddText(3, b"x\ny")
		self.assertEquals(self.ed.GetLineEndPosition(0), 1)
		self.assertEquals(self.ed.GetLineEndPosition(1), 3)
		self.assertEquals(self.ed.LineLength(0), 2)
		self.assertEquals(self.ed.LineLength(1), 1)
		self.assertEquals(self.ed.EOLMode, self.ed.SC_EOL_CRLF)
		lineEnds = [b"\r\n", b"\r", b"\n"]
		for lineEndType in [self.ed.SC_EOL_CR, self.ed.SC_EOL_LF, self.ed.SC_EOL_CRLF]:
			self.ed.EOLMode = lineEndType
			self.assertEquals(self.ed.EOLMode, lineEndType)
			self.ed.ConvertEOLs(lineEndType)
			self.assertEquals(self.ed.Contents(), b"x" + lineEnds[lineEndType] + b"y")
			self.assertEquals(self.ed.LineLength(0), 1 + len(lineEnds[lineEndType]))

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

	def testGetSet(self):
		self.ed.SetText(0, b"abc")
		self.assertEquals(self.ed.TextLength, 3)
		result = b"\0" * 5
		length = self.ed.GetText(4, result)
		result = result[:length]
		self.assertEquals(result, b"abc")

	def testAppend(self):
		self.ed.SetText(0, b"abc")
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 0)
		text = b"12"
		self.ed.AppendText(len(text), text)
		self.assertEquals(self.ed.SelectionStart, 0)
		self.assertEquals(self.ed.SelectionEnd, 0)
		self.assertEquals(self.ed.Contents(), b"abc12")

	def testTarget(self):
		self.ed.SetText(0, b"abcd")
		self.ed.TargetStart = 1
		self.ed.TargetEnd = 3
		self.assertEquals(self.ed.TargetStart, 1)
		self.assertEquals(self.ed.TargetEnd, 3)
		rep = b"321"
		self.ed.ReplaceTarget(len(rep), rep)
		self.assertEquals(self.ed.Contents(), b"a321d")
		self.ed.SearchFlags = self.ed.SCFIND_REGEXP
		self.assertEquals(self.ed.SearchFlags, self.ed.SCFIND_REGEXP)
		searchString = b"([1-9]+)" # were \( \) is original regex algorithm.
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(1, pos)
		tagString = b"abcdefghijklmnop"
		lenTag = self.ed.GetTag(1, tagString)
		tagString = tagString[:lenTag]
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
		self.ed.SetText(0, b"abcd")
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
		self.xite = XiteWin.xiteFrame
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
		self.xite = XiteWin.xiteFrame
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
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.AddText(5, b"x\ny\nz")

	def testMarker(self):
		handle = self.ed.MarkerAdd(1,1)
		self.assertEquals(self.ed.MarkerLineFromHandle(handle), 1)
		self.ed.MarkerDelete(1,1)
		self.assertEquals(self.ed.MarkerLineFromHandle(handle), -1)

	# def testTwiceAddedDelete(self):
		# handle = self.ed.MarkerAdd(1,1)
		# self.assertEquals(self.ed.MarkerGet(1), 2)
		# handle2 = self.ed.MarkerAdd(1,1)
		# self.assertEquals(self.ed.MarkerGet(1), 2)
		# self.ed.MarkerDelete(1,1)
		# self.assertEquals(self.ed.MarkerGet(1), 2)
		# self.ed.MarkerDelete(1,1)
		# self.assertEquals(self.ed.MarkerGet(1), 0)

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
		self.xite = XiteWin.xiteFrame
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

	# def testIndicatorAtEnd(self):
		# self.ed.InsertText(0, b"ab")
		# self.ed.IndicatorCurrent = 3
		# self.ed.IndicatorFillRange(1,1)
		# self.assertEquals(self.ed.IndicatorValueAt(3, 0), 0)
		# self.assertEquals(self.ed.IndicatorValueAt(3, 1), 1)
		# self.assertEquals(self.ed.IndicatorStart(3, 0), 0)
		# self.assertEquals(self.ed.IndicatorEnd(3, 0), 1)
		# self.assertEquals(self.ed.IndicatorStart(3, 1), 1)
		# self.assertEquals(self.ed.IndicatorEnd(3, 1), 2)
		# self.ed.DeleteRange(1, 1)
		# # Now only one character left and does not have indicator so indicator 3 is null
		# self.assertEquals(self.ed.IndicatorValueAt(3, 0), 0)
		# # Since null, remaining calls return 0
		# self.assertEquals(self.ed.IndicatorStart(3, 0), 0)
		# self.assertEquals(self.ed.IndicatorEnd(3, 0), 0)
		# self.assertEquals(self.ed.IndicatorStart(3, 1), 0)
		# self.assertEquals(self.ed.IndicatorEnd(3, 1), 0)

class TestScrolling(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		# 150 should be enough lines
		self.ed.InsertText(0, b"a\n" * 150)

	def testTop(self):
		self.ed.GotoLine(0)
		self.assertEquals(self.ed.FirstVisibleLine, 0)

	def testLineScroll(self):
		self.ed.GotoLine(0)
		self.ed.LineScroll(0, 3)
		self.assertEquals(self.ed.FirstVisibleLine, 3)

	def testVisibleLine(self):
		self.ed.FirstVisibleLine = 7
		self.assertEquals(self.ed.FirstVisibleLine, 7)


class TestBoostRegExSearch(unittest.TestCase):
	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		
	def testDotStarRegEx(self):
		toFind = b".*"
		direction = self.SearchDirectionForward(self.ed)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE
		self.doSearchTests(b"[aaAaaaBaCD]\r\n\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[aaAaaaBaCD]\r\n[]\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[aaAaaaBaCD][]\r\n[]\r\n[aaa][]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

		flags |= self.ed.SCFIND_REGEXP_DOTMATCHESNL
		self.doSearchTests(b"[aaAaaaBaCD\r\n\r\naaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[aaAaaaBaCD\r\n\r\naaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[aaAaaaBaCD\r\n\r\naaa][]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

		flags = self.ed.SCFIND_REGEXP
		self.doSearchTests(b"[aaAaaaBaCD]\r[]\n[]\r[]\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)

	def testDotStarRegExBackward(self):
		toFind = b".*"
		direction = self.SearchDirectionBackward(self.ed)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE
		self.doSearchTests(b"[aaAaaaBaCD]\r\n\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[aaAaaaBaCD]\r\n[]\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[][aaAaaaBaCD]\r\n[]\r\n[][aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

		flags |= self.ed.SCFIND_REGEXP_DOTMATCHESNL
		self.doSearchTests(b"[aaAaaaBaCD\r\n\r\naaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[aaAaaaBaCD\r\n\r\naaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[][aaAaaaBaCD\r\n\r\naaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

		flags = self.ed.SCFIND_REGEXP
		self.doSearchTests(b"[aaAaaaBaCD]\r[]\n[]\r[]\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)

	def testAStarRegEx(self):
		toFind = b"a*"
		direction = self.SearchDirectionForward(self.ed)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE | self.ed.SCFIND_MATCHCASE
		self.doSearchTests(b"X[aa]A[aaa]B[a]CD\r\n\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[]X[aa]A[aaa]B[a]C[]D[]\r\n[]\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[]X[aa][]A[aaa][]B[a][]C[]D[]\r\n[]\r\n[aaa][]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)
		
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_MATCHCASE
		self.doSearchTests(b"[]X[aa]A[aaa]B[a]C[]D[]\r[]\n[]\r[]\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE
		self.doSearchTests(b"X[aaAaaa]B[a]CD\r\n\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[]X[aaAaaa]B[a]C[]D[]\r\n[]\r\n[aaa]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[]X[aaAaaa][]B[a][]C[]D[]\r\n[]\r\n[aaa][]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

	def testAStarRegExBackward(self):
		toFind = b"a*"
		direction = self.SearchDirectionBackward(self.ed)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE | self.ed.SCFIND_MATCHCASE
		self.doSearchTests(b"[aa]A[aaa]B[a]CD\r\n\r\n[aaa]X", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[aa]A[aaa]B[a]C[]D[]\r\n[]\r\n[aaa]X[]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[][aa]A[][aaa]B[][a]C[]D[]\r\n[]\r\n[][aaa]X[]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_MATCHCASE
		self.doSearchTests(b"[aa]A[aaa]B[a]C[]D[]\r[]\n[]\r[]\n[aaa]X[]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)

		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE
		self.doSearchTests(b"[aaAaaa]B[a]CD\r\n\r\n[aaa]X", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NONE, direction)
		self.doSearchTests(b"[aaAaaa]B[a]C[]D[]\r\n[]\r\n[aaa]X[]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, direction)
		self.doSearchTests(b"[][aaAaaa]B[][a]C[]D[]\r\n[]\r\n[][aaa]X[]", toFind, flags | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL, direction)

	def testLineBeginEndRegEx(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL
		direction = self.SearchDirectionForward(self.ed)
		self.doSearchTests(b"[]ab\n[]ab\r[]ab\r\n[]ab\n[]\n[]ab\r[]\r[]ab\r\n[]\r\n[]ab", b"^", flags, direction)
		self.doSearchTests(b"ab[]\nab[]\rab[]\r\nab[]\n[]\nab[]\r[]\rab[]\r\n[]\r\nab[]", b"$", flags, direction)
		self.doSearchTests(b"[]", b"^", flags, direction)
		self.doSearchTests(b"[]", b"$", flags, direction)

		direction = self.SearchDirectionBackwardExceptFirstAndLastCharacter(self.ed)
		self.doSearchTests(b"ab\n[]ab\r[]ab\r\n[]ab\n[]\n[]ab\r[]\r[]ab\r\n[]\r\n[]ab", b"^", flags, direction)
		self.doSearchTests(b"ab[]\nab[]\rab[]\r\nab[]\n[]\nab[]\r[]\rab[]\r\n[]\r\nab", b"$", flags, direction)

	def testLineBeginEndRegExBackward(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL
		direction = self.SearchDirectionBackward(self.ed)
		self.doSearchTests(b"[]ab\n[]ab\r[]ab\r\n[]ab\n[]\n[]ab\r[]\r[]ab\r\n[]\r\n[]ab", b"^", flags, direction)
		self.doSearchTests(b"ab[]\nab[]\rab[]\r\nab[]\n[]\nab[]\r[]\rab[]\r\n[]\r\nab[]", b"$", flags, direction)
		self.doSearchTests(b"[]", b"^", flags, direction)
		self.doSearchTests(b"[]", b"$", flags, direction)

		direction = self.SearchDirectionBackwardExceptFirstAndLastCharacter(self.ed)
		self.doSearchTests(b"ab\n[]ab\r[]ab\r\n[]ab\n[]\n[]ab\r[]\r[]ab\r\n[]\r\n[]ab", b"^", flags, direction)
		self.doSearchTests(b"ab[]\nab[]\rab[]\r\nab[]\n[]\nab[]\r[]\rab[]\r\n[]\r\nab", b"$", flags, direction)

	def testUtf8RegEx(self):
		self.ed.SetCodePage(self.ed.SC_CP_UTF8)
		self.doSearchTests(b"[aa]b[éé]b[aé]b[éÉ]ááá[Éaéé]b", b"[aéÉ]+", self.ed.SCFIND_REGEXP, self.SearchDirectionForward(self.ed))
		self.doSearchTests(b"[áéíóú]\r\n[]\r\n[áé]", b".*", self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, self.SearchDirectionForward(self.ed))

	def testUtf8RegExBackward(self):
		self.ed.SetCodePage(self.ed.SC_CP_UTF8)
		self.doSearchTests(b"[aa]b[éé]b[aé]b[éÉ]ááá[Éaéé]b", b"[aéÉ]+", self.ed.SCFIND_REGEXP, self.SearchDirectionBackward(self.ed))
		self.doSearchTests(b"[áéíóú]\r\n[]\r\n[áé]", b".*", self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_SKIPCRLFASONE | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH, self.SearchDirectionBackward(self.ed))

	def testOverlapRegEx(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_MATCHCASE
		toFind = b"abc|bcde"
		self.doSearchTests(b"[abc]def[abc]ab[bcde]f", toFind, flags, self.SearchDirectionForward(self.ed))
		self.doSearchTests(b"a[bcde]f[abc]ab[bcde]f", toFind, flags, self.SearchDirectionBackward(self.ed))
		toFind = b"abcde|bc"
		self.doSearchTests(b"[abcde]fa[bc]ab[bc]def", toFind, flags, self.SearchDirectionForward(self.ed))
		self.doSearchTests(b"[abcde]fa[bc]ab[bc]def", toFind, flags, self.SearchDirectionBackward(self.ed))

	def testDocumentModificationBetweenSearch(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH
		toFind = b"a*"
		self.setText(b"BaaC") ; firstMatch = (0,0,0) ; secondMatch = (1,1,3)
		def searchFromStart(): return self.ed.FindBytes2(0, self.ed.Length, toFind, flags)
		self.assertEquals(firstMatch, searchFromStart())
		self.assertEquals(secondMatch, searchFromStart()) # This search should find second match, even though it is from same start position, as it should be considered a continuation of previous search.
		self.assertEquals(firstMatch, searchFromStart()) # A third search from same start should find first match, as it is not a continuation of previous search.
		self.setText(b"Baa")
		self.assertEquals(firstMatch, searchFromStart()) # This search should find first match, as it should not be considered a continuation since text has been modified.


	def testAStarAllowEmptyAtStartRegEx(self):
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALLOWATSTART
		toFind = b"a*"
		self.setText(b"BaaC") 
		firstMatch = (0,0,0)
		secondMatch = (1,1,3)
		def searchFromStart(): return self.ed.FindBytes2(0, self.ed.Length, toFind, flags)
		self.assertEquals(firstMatch, searchFromStart())
		self.assertEquals(firstMatch, searchFromStart()) # This search would normally find second match, even though it is from same start position,
		                                                 # as it should be considered a continuation of previous search, but we have set SCFIND_REGEXP_EMPTYMATCH_ALLOWATSTART
		                                                 # which will force it to allow the first match to be found again.


	# 'textWithBrackets' should contain the text with [] brackets around all strings that should be found by successive search of 'whatToFind', each search starting at end of previous match.
	def doSearchTests(self, textWithBrackets, whatToFind, flags, direction):
		expectedResults = self.setupTest(textWithBrackets, direction)
		currentPosition = direction.startPosition(); i = 1
		for expected in direction.orderResults(expectedResults):
			result = self.ed.FindBytes2(currentPosition, direction.endPosition(), whatToFind, flags)
			self.assertEquals(expected, result, self.failureString(expected, result, i))
			currentPosition = direction.nextPositionFromResult(expected); i += 1
	
	def failureString(self, expected, result, i):
		class MapCrLf:
			def __getitem__(self, c):
				if c == ord('\r'): return u'«'
				if c == ord('\n'): return u'¬'
				return c
		return ("%s != %s  (at search #%d)\n"%(str(expected), str(result), i) \
			+ unicode(self.text, 'utf-8').translate(MapCrLf()) + "\n" \
			+ self.markCharRange(expected) + "\n" \
			+ self.markCharRange(result) \
			).encode('utf-8')
	
	def markCharRange(self, searchResult):
		if searchResult[0] < 0: return "(No match)"
		left = searchResult[1]; right = searchResult[2]
		self.assertTrue(left <= right, "Invalid range (%d,%d)"%(left,right))
		leftChar = self.bytePositionToCharacterPosition(left)
		returnValue = ' '*leftChar
		if left == right:
			if not self.positionError: returnValue += '^'
			else:                      returnValue += '~'
		else:
			if not self.positionError: returnValue += '['
			else:                      returnValue += '~'
			rightChar = self.bytePositionToCharacterPosition(right)
			returnValue += ' '*(rightChar-leftChar-1)
			if not self.positionError: returnValue += ']'
			else:                      returnValue += '~'
		return returnValue
	
	def bytePositionToCharacterPosition(self, bytePosition):
		self.positionError = False
		while True:
			try:
				return len(unicode(self.text[:bytePosition], 'utf-8'))
			except UnicodeDecodeError:
				self.positionError = True
				bytePosition -= 1
	
	class SearchDirectionForward:
		def __init__(self, editorObject):               self.ed = editorObject
		def startPosition(self):                        return 0
		def endPosition(self):                          return self.ed.Length
		def nextPositionFromResult(self, searchResult): return searchResult[2]
		def orderResults(self, expectedResults):        return expectedResults
		def atStart(self):                              return []
		def atEnd(self):                                return [(-1, 0, 0)]  # Last search should fail.
	
	class SearchDirectionBackward:
		def __init__(self, editorObject):               self.ed = editorObject
		def startPosition(self):                        return self.ed.Length
		def endPosition(self):                          return 0
		def nextPositionFromResult(self, searchResult): return searchResult[1]
		def orderResults(self, expectedResults):        return reversed(expectedResults)
		def atStart(self):                              return [(-1, 0, 0)]  # Last search should fail.
		def atEnd(self):                                return []
	
	class SearchDirectionForwardExceptFirstAndLastCharacter(SearchDirectionForward):
		def startPosition(self):                        return 1
		def endPosition(self):                          return self.ed.Length-1
	
	class SearchDirectionBackwardExceptFirstAndLastCharacter(SearchDirectionBackward):
		def startPosition(self):                        return self.ed.Length-1
		def endPosition(self):                          return 1
	
	def setupTest(self, textWithBrackets, direction):
		expectedResults = self.getExpectedResults(textWithBrackets, direction)
		text = self.removeBrackets(textWithBrackets)
		self.setText(text)
		return expectedResults
		
	def getExpectedResults(self, textWithBrackets, direction):
		textSplit = textWithBrackets.split(b'[')
		currentPosition = 0
		expectedResults = []
		expectedResults += direction.atStart()
		for i in range(0, len(textSplit)-1):
			currentPosition += len(self.removeBrackets(textSplit[i]))
			expectedResults += [(currentPosition, currentPosition, currentPosition+textSplit[i+1].find(b']'))]
		expectedResults += direction.atEnd()
		return expectedResults
	
	def removeBrackets(self, textWithBrackets):
		return textWithBrackets.translate(None, b"[]")
	
	def setText(self, text):
		self.text = text
		self.ed.SetText(len(text), text)

class TestBoostRegExReplace(unittest.TestCase):
	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testReplace(self):
		self.ed.SetCodePage(0)
		self.setText(b"abcd1234XYZef5678Wgh")
		self.doReplace(b"([0-9]+)([A-Z]+)", b"[\\2;\\1]", self.ed.SCFIND_REGEXP | self.ed.SCFIND_MATCHCASE)
		self.assertEquals(self.ed.Contents(), b"abcd[XYZ;1234]ef5678Wgh")

	def testReplaceAStar(self):
		self.setText(b"aaXYaZaaa")
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH
		position = 0
		position = self.doReplace(b"a*", b"b", flags, position)
		self.assertEquals(self.ed.Contents(), b"bXYaZaaa")
		self.assertEquals(position, 1)
		position = self.doReplace(b"a*", b"c", flags, position)
		self.assertEquals(self.ed.Contents(), b"bXcYaZaaa")
		self.assertEquals(position, 3)
		position = self.doReplace(b"a*", b"d", flags, position)
		self.assertEquals(self.ed.Contents(), b"bXcYdZaaa")
		self.assertEquals(position, 5)
		position = self.doReplace(b"a*", b"e", flags, position)
		self.assertEquals(self.ed.Contents(), b"bXcYdZe")
		self.assertEquals(position, 7)
		position = self.doReplace(b"a*", b"f", flags, position)
		self.assertEquals(self.ed.Contents(), b"bXcYdZe")
		self.assertEquals(position, -1)

		self.setText(b"aaXYa")
		flags = self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL
		position = self.doReplace(b"a*", b"b", flags, position)
		self.assertEquals(self.ed.Contents(), b"bXYa")
		self.assertEquals(position, 1)
		position = self.doReplace(b"a*", b"c", flags, position)
		self.assertEquals(self.ed.Contents(), b"bcXYa")
		self.assertEquals(position, 2)
		position = self.doReplace(b"a*", b"d", flags, position)
		self.assertEquals(self.ed.Contents(), b"bcXdYa")
		self.assertEquals(position, 4)
		position = self.doReplace(b"a*", b"e", flags, position)
		self.assertEquals(self.ed.Contents(), b"bcXdYe")
		self.assertEquals(position, 6)
		position = self.doReplace(b"a*", b"f", flags, position)
		self.assertEquals(self.ed.Contents(), b"bcXdYef")
		self.assertEquals(position, 7)
		position = self.doReplace(b"a*", b"g", flags, position)
		self.assertEquals(self.ed.Contents(), b"bcXdYef")
		self.assertEquals(position, -1)

	def testReplaceEOL(self):
		self.doTestReplaceEOL(self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_EMPTYMATCH_NOTAFTERMATCH)
		self.doTestReplaceEOL(self.ed.SCFIND_REGEXP | self.ed.SCFIND_REGEXP_EMPTYMATCH_ALL)

	def doTestReplaceEOL(self, flags):
		self.setText(b"ab\ncde\n\nfg")
		position = 0
		position = self.doReplace(b"$", b"A", flags, position)
		self.assertEquals(self.ed.Contents(), b"abA\ncde\n\nfg")
		self.assertEquals(position, 3)
		position = self.doReplace(b"$", b"B", flags, position)
		self.assertEquals(self.ed.Contents(), b"abA\ncdeB\n\nfg")
		self.assertEquals(position, 8)
		position = self.doReplace(b"$", b"C", flags, position)
		self.assertEquals(self.ed.Contents(), b"abA\ncdeB\nC\nfg")
		self.assertEquals(position, 10)
		position = self.doReplace(b"$", b"D", flags, position)
		self.assertEquals(self.ed.Contents(), b"abA\ncdeB\nC\nfgD")
		self.assertEquals(position, 14)
		position = self.doReplace(b"$", b"E", flags, position)
		self.assertEquals(self.ed.Contents(), b"abA\ncdeB\nC\nfgD")
		self.assertEquals(position, -1)

	def testReplaceUtf8(self):
		self.ed.SetCodePage(self.ed.SC_CP_UTF8)
		self.setText(b"áéíó1234ÁÉÍáé5678Áae")
		self.doReplace(b"([0-9]+)([ÁÉÍ]+)", b"[\\2;\\1]", self.ed.SCFIND_REGEXP | self.ed.SCFIND_MATCHCASE)
		self.assertEquals(self.ed.Contents(), b"áéíó[ÁÉÍ;1234]áé5678Áae")

	def doReplace(self, whatToFind, replaceBy, flags, startSearchPosition = 0):
		searchResult = self.ed.FindBytes2(startSearchPosition, self.ed.Length, whatToFind, flags)
		if searchResult[0] < 0: return -1
		self.ed.TargetStart = searchResult[1]
		self.ed.TargetEnd = searchResult[2]
		replacementLength = self.ed.ReplaceTargetRE(len(replaceBy), replaceBy)
		return searchResult[1] + replacementLength

	def setText(self, text):
		self.ed.SetText(len(text), text)

class TestSearch(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
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
		self.assertEquals(0, self.ed.FindBytes(0, self.ed.Length, b"([a]).*\0", flags))  # were \( \) in original regex algorithm.

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

class TestProperties(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testSet(self):
		self.ed.SetProperty(b"test", b"12")
		self.assertEquals(self.ed.GetPropertyInt(b"test"), 12)
		result = b"\0" * 10
		length = self.ed.GetProperty(b"test", result)
		result = result[:length]
		self.assertEquals(result, b"12")
		self.ed.SetProperty(b"test.plus", b"[$(test)]")
		result = b"\0" * 10
		length = self.ed.GetPropertyExpanded(b"test.plus", result)
		result = result[:length]
		self.assertEquals(result, b"[12]")

class TestTextMargin(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.txt = b"abcd"
		self.ed.AddText(1, b"x")

	def testAscent(self):
		lineHeight = self.ed.TextHeight(0)
		self.ed.ExtraAscent
		self.assertEquals(self.ed.ExtraAscent, 0)
		self.assertEquals(self.ed.ExtraDescent, 0)
		self.ed.ExtraAscent = 1
		self.assertEquals(self.ed.ExtraAscent, 1)
		self.ed.ExtraDescent = 2
		self.assertEquals(self.ed.ExtraDescent, 2)
		# Allow line height to recalculate
		self.xite.DoEvents()
		lineHeightIncreased = self.ed.TextHeight(0)
		self.assertEquals(lineHeightIncreased, lineHeight + 2 + 1)

	def testTextMargin(self):
		self.ed.MarginSetText(0, self.txt)
		result = b"\0" * 10
		length = self.ed.MarginGetText(0, result)
		result = result[:length]
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
		result = b"\0" * 10
		length = self.ed.MarginGetStyles(0, result)
		result = result[:length]
		self.assertEquals(result, styles)
		self.ed.MarginTextClearAll()

	def testTextMarginStyleOffset(self):
		self.ed.MarginSetStyleOffset(300)
		self.assertEquals(self.ed.MarginGetStyleOffset(), 300)

class TestAnnotation(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.txt = b"abcd"
		self.ed.AddText(1, b"x")

	def testTextAnnotation(self):
		self.assertEquals(self.ed.AnnotationGetLines(), 0)
		self.ed.AnnotationSetText(0, self.txt)
		self.assertEquals(self.ed.AnnotationGetLines(), 1)
		result = b"\0" * 10
		length = self.ed.AnnotationGetText(0, result)
		self.assertEquals(length, 4)
		result = result[:length]
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
		result = b"\0" * 10
		length = self.ed.AnnotationGetStyles(0, result)
		result = result[:length]
		self.assertEquals(result, styles)
		self.ed.AnnotationClearAll()

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
		self.xite = XiteWin.xiteFrame
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

class TestCaseMapping(unittest.TestCase):
	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.SetCodePage(0)
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_DEFAULT)

	def testEmpty(self):
		# Trying to upper case an empty string caused a crash at one stage
		t = b"x"
		self.ed.SetText(len(t), t)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), b"x")

	def testASCII(self):
		t = b"x"
		self.ed.SetText(len(t), t)
		self.ed.SetSel(0,1)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), b"X")

	def testLatin1(self):
		t = "å".encode("Latin-1")
		r = "Å".encode("Latin-1")
		self.ed.SetText(len(t), t)
		self.ed.SetSel(0,1)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), r)

	def testRussian(self):
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		t = "Б".encode("Windows-1251")
		r = "б".encode("Windows-1251")
		self.ed.SetText(len(t), t)
		self.ed.SetSel(0,1)
		self.ed.LowerCase()
		self.assertEquals(self.ed.Contents(), r)

	def testUTF(self):
		self.ed.SetCodePage(65001)
		t = "å".encode("UTF-8")
		r = "Å".encode("UTF-8")
		self.ed.SetText(len(t), t)
		self.ed.SetSel(0,2)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Contents(), r)

	def testUTFDifferentLength(self):
		self.ed.SetCodePage(65001)
		t = "ı".encode("UTF-8")
		r = "I".encode("UTF-8")
		self.ed.SetText(len(t), t)
		self.assertEquals(self.ed.Length, 2)
		self.ed.SetSel(0,2)
		self.ed.UpperCase()
		self.assertEquals(self.ed.Length, 1)
		self.assertEquals(self.ed.Contents(), r)

class TestCaseInsensitiveSearch(unittest.TestCase):
	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def tearDown(self):
		self.ed.SetCodePage(0)
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_DEFAULT)

	def testEmpty(self):
		text = b" x X"
		searchString = b""
		self.ed.SetText(len(text), text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(0, pos)

	def testASCII(self):
		text = b" x X"
		searchString = b"X"
		self.ed.SetText(len(text), text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(1, pos)

	def testLatin1(self):
		text = "Frånd Åå".encode("Latin-1")
		searchString = "Å".encode("Latin-1")
		self.ed.SetText(len(text), text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(2, pos)

	def testRussian(self):
		self.ed.StyleSetCharacterSet(self.ed.STYLE_DEFAULT, self.ed.SC_CHARSET_RUSSIAN)
		text = "=(Б tex б)".encode("Windows-1251")
		searchString = "б".encode("Windows-1251")
		self.ed.SetText(len(text), text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(2, pos)

	def testUTF(self):
		self.ed.SetCodePage(65001)
		text = "Frånd Åå".encode("UTF-8")
		searchString = "Å".encode("UTF-8")
		self.ed.SetText(len(text), text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(2, pos)

	def testUTFDifferentLength(self):
		# Searching for a two byte string "ı" finds a single byte "I"
		self.ed.SetCodePage(65001)
		text = "Fråndi Ååİ $".encode("UTF-8")
		firstPosition = len("Frånd".encode("UTF-8"))
		searchString = "İ".encode("UTF-8")
		self.assertEquals(len(searchString), 2)
		self.ed.SetText(len(text), text)
		self.ed.TargetStart = 0
		self.ed.TargetEnd = self.ed.Length-1
		self.ed.SearchFlags = 0
		pos = self.ed.SearchInTarget(len(searchString), searchString)
		self.assertEquals(firstPosition, pos)
		self.assertEquals(firstPosition+1, self.ed.TargetEnd)

class TestLexer(unittest.TestCase):
	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testLexerNumber(self):
		self.ed.Lexer = self.ed.SCLEX_CPP
		self.assertEquals(self.ed.GetLexer(), self.ed.SCLEX_CPP)

	def testLexerName(self):
		self.ed.LexerLanguage = b"cpp"
		self.assertEquals(self.ed.GetLexer(), self.ed.SCLEX_CPP)
		name = b"-" * 100
		length = self.ed.GetLexerLanguage(0, name)
		name = name[:length]
		self.assertEquals(name, b"cpp")

class TestAutoComplete(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
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
		t = b"xxx"
		l = self.ed.AutoCGetCurrentText(5, t)
		#~ self.assertEquals(l, 3)
		self.assertEquals(t, b"za\0")
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

class TestDirectAccess(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	# def testGapPosition(self):
		# text = b"abcd"
		# self.ed.SetText(len(text), text)
		# self.assertEquals(self.ed.GapPosition, 4)
		# self.ed.TargetStart = 1
		# self.ed.TargetEnd = 1
		# rep = b"-"
		# self.ed.ReplaceTarget(len(rep), rep)
		# self.assertEquals(self.ed.GapPosition, 2)

	def testCharacterPointerAndRangePointer(self):
		text = b"abcd"
		self.ed.SetText(len(text), text)
		characterPointer = self.ed.CharacterPointer
		# rangePointer = self.ed.GetRangePointer(0,3)
		# self.assertEquals(characterPointer, rangePointer)
		cpBuffer = ctypes.c_char_p(characterPointer)
		self.assertEquals(cpBuffer.value, text)
		# Gap will not be moved as already moved for CharacterPointer call
		# rangePointer = self.ed.GetRangePointer(1,3)
		# cpBuffer = ctypes.c_char_p(rangePointer)
		# self.assertEquals(cpBuffer.value, text[1:])

class TestWordChars(unittest.TestCase):
	def setUp(self):
		self.xite = XiteWin.xiteFrame
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

	# def testDefaultWordChars(self):
		# # check that the default word chars are as expected
		# import string
		# dataLen = self.ed.GetWordChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetWordChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# expected = set(string.digits + string.ascii_letters + '_') | \
			# set(chr(x) for x in range(0x80, 0x100))
		# self.assertCharSetsEqual(data, expected)

	# def testDefaultWhitespaceChars(self):
		# # check that the default whitespace chars are as expected
		# import string
		# dataLen = self.ed.GetWhitespaceChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetWhitespaceChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# expected = (set(chr(x) for x in (range(0, 0x20))) | set(' ')) - \
			# set(['\r', '\n'])
		# self.assertCharSetsEqual(data, expected)

	# def testDefaultPunctuationChars(self):
		# # check that the default punctuation chars are as expected
		# import string
		# dataLen = self.ed.GetPunctuationChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetPunctuationChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# expected = set(chr(x) for x in range(0x20, 0x80)) - \
			# set(string.ascii_letters + string.digits + "\r\n_ ")
		# self.assertCharSetsEqual(data, expected)

	# def testCustomWordChars(self):
		# # check that setting things to whitespace chars makes them not words
		# self._setChars("whitespace", range(1, 0x100))
		# dataLen = self.ed.GetWordChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetWordChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# expected = set()
		# self.assertCharSetsEqual(data, expected)
		# # and now set something to make sure that works too
		# expected = set(range(1, 0x100, 2))
		# self._setChars("word", expected)
		# dataLen = self.ed.GetWordChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetWordChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# self.assertCharSetsEqual(data, expected)

	# def testCustomWhitespaceChars(self):
		# # check setting whitespace chars to non-default values
		# self._setChars("word", range(1, 0x100))
		# # we can't change chr(0) from being anything but whitespace
		# expected = set([0])
		# dataLen = self.ed.GetWhitespaceChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetWhitespaceChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# self.assertCharSetsEqual(data, expected)
		# # now try to set it to something custom
		# expected = set(range(1, 0x100, 2)) | set([0])
		# self._setChars("whitespace", expected)
		# dataLen = self.ed.GetWhitespaceChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetWhitespaceChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# self.assertCharSetsEqual(data, expected)

	# def testCustomPunctuationChars(self):
		# # check setting punctuation chars to non-default values
		# self._setChars("word", range(1, 0x100))
		# expected = set()
		# dataLen = self.ed.GetPunctuationChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetPunctuationChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# self.assertEquals(set(data), expected)
		# # now try to set it to something custom
		# expected = set(range(1, 0x100, 1))
		# self._setChars("punctuation", expected)
		# dataLen = self.ed.GetPunctuationChars(None, None)
		# data = b"\0" * dataLen
		# self.ed.GetPunctuationChars(None, data)
		# self.assertEquals(dataLen, len(data))
		# self.assertCharSetsEqual(data, expected)

#~ import os
#~ for x in os.getenv("PATH").split(";"):
	#~ n = "scilexer.dll"
	#~ nf = x + "\\" + n
	#~ print os.access(nf, os.R_OK), nf
if __name__ == '__main__':
	uu = XiteWin.main("simpleTests")
	#~ for x in sorted(uu.keys()):
		#~ print(x, uu[x])
	#~ print()
