#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Requires Python 2.7 or later

from __future__ import with_statement
from __future__ import unicode_literals

import string, time, unittest

try:
	start = time.perf_counter()
	timer = time.perf_counter
except AttributeError:
	timer = time.time

import XiteWin as Xite

class TestPerformance(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testAddLine(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		start = timer()
		for i in range(2000):
			self.ed.AddText(len(data), data)
			self.assertEqual(self.ed.LineCount, i + 2)
		end = timer()
		duration = end - start
		print("%6.3f testAddLine" % duration)
		self.xite.DoEvents()
		self.assertTrue(self.ed.Length > 0)

	def testAddLineMiddle(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		start = timer()
		for i in range(2000):
			self.ed.AddText(len(data), data)
			self.assertEqual(self.ed.LineCount, i + 2)
		end = timer()
		duration = end - start
		print("%6.3f testAddLineMiddle" % duration)
		self.xite.DoEvents()
		self.assertTrue(self.ed.Length > 0)

	def testHuge(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		data = data * 100000
		start = timer()
		self.ed.AddText(len(data), data)
		end = timer()
		duration = end - start
		print("%6.3f testHuge" % duration)
		self.xite.DoEvents()
		self.assertTrue(self.ed.Length > 0)

	def testHugeInserts(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		data = data * 100000
		insert = (string.digits + "\n").encode('utf-8')
		self.ed.AddText(len(data), data)
		start = timer()
		for i in range(2000):
			self.ed.InsertText(0, insert)
		end = timer()
		duration = end - start
		print("%6.3f testHugeInserts" % duration)
		self.xite.DoEvents()
		self.assertTrue(self.ed.Length > 0)

	def testHugeReplace(self):
		oneLine = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		data = oneLine * 100000
		insert = (string.digits + "\n").encode('utf-8')
		self.ed.AddText(len(data), data)
		start = timer()
		for i in range(1000):
			self.ed.TargetStart = i * len(insert)
			self.ed.TargetEnd = self.ed.TargetStart + len(oneLine)
			self.ed.ReplaceTarget(len(insert), insert)
		end = timer()
		duration = end - start
		print("%6.3f testHugeReplace" % duration)
		self.xite.DoEvents()
		self.assertTrue(self.ed.Length > 0)

	def testUTF8CaseSearches(self):
		self.ed.SetCodePage(65001)
		oneLine = "Fold Margin=折りたたみ表示用の余白(&F)\n".encode('utf-8')
		manyLines = oneLine * 100000
		manyLines = manyLines + "φ\n".encode('utf-8')
		self.ed.AddText(len(manyLines), manyLines)
		searchString = "φ".encode('utf-8')
		start = timer()
		for i in range(1000):
			self.ed.TargetStart = 0
			self.ed.TargetEnd = self.ed.Length-1
			self.ed.SearchFlags = self.ed.SCFIND_MATCHCASE
			pos = self.ed.SearchInTarget(len(searchString), searchString)
			self.assertTrue(pos > 0)
		end = timer()
		duration = end - start
		print("%6.3f testUTF8CaseSearches" % duration)
		self.xite.DoEvents()

	def testUTF8Searches(self):
		self.ed.SetCodePage(65001)
		oneLine = "Fold Margin=折りたたみ表示用の余白(&F)\n".encode('utf-8')
		manyLines = oneLine * 100000
		manyLines = manyLines + "φ\n".encode('utf-8')
		self.ed.AddText(len(manyLines), manyLines)
		searchString = "φ".encode('utf-8')
		start = timer()
		for i in range(20):
			self.ed.TargetStart = 0
			self.ed.TargetEnd = self.ed.Length-1
			self.ed.SearchFlags = 0
			pos = self.ed.SearchInTarget(len(searchString), searchString)
			self.assertTrue(pos > 0)
		end = timer()
		duration = end - start
		print("%6.3f testUTF8Searches" % duration)
		self.xite.DoEvents()

	def testUTF8AsciiSearches(self):
		self.ed.SetCodePage(65001)
		oneLine = "Fold Margin=NagasakiOsakaHiroshimaHanedaKyoto(&F)\n".encode('utf-8')
		manyLines = oneLine * 100000
		manyLines = manyLines + "φ\n".encode('utf-8')
		self.ed.AddText(len(manyLines), manyLines)
		searchString = "φ".encode('utf-8')
		start = timer()
		for i in range(20):
			self.ed.TargetStart = 0
			self.ed.TargetEnd = self.ed.Length-1
			self.ed.SearchFlags = 0
			pos = self.ed.SearchInTarget(len(searchString), searchString)
			self.assertTrue(pos > 0)
		end = timer()
		duration = end - start
		print("%6.3f testUTF8AsciiSearches" % duration)
		self.xite.DoEvents()

	def testShiftJISSearches(self):
		self.ed.SetCodePage(932)
		self.ed.StyleSetCharacterSet(32, 128)
		oneLine = "Fold Margin=折りたたみ表示用の余白(&F)\n".encode('cp932')
		manyLines = oneLine * 100000
		manyLines = manyLines + "φ\n".encode('cp932')
		#~ with open("932.txt", "wb") as fp:
			#~ fp.write(manyLines)
		self.ed.AddText(len(manyLines), manyLines)
		searchString = "φ".encode('cp932')
		start = timer()
		for i in range(20):
			self.ed.TargetStart = 0
			self.ed.TargetEnd = self.ed.Length-1
			self.ed.SearchFlags = 0
			pos = self.ed.SearchInTarget(len(searchString), searchString)
			self.assertTrue(pos > 0)
		end = timer()
		duration = end - start
		print("%6.3f testShiftJISSearches" % duration)
		self.xite.DoEvents()

if __name__ == '__main__':
	Xite.main("performanceTests")
