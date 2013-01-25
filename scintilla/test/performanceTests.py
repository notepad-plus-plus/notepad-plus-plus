# -*- coding: utf-8 -*-

from __future__ import with_statement

import os, string, time, unittest

import XiteWin

class TestPerformance(unittest.TestCase):

	def setUp(self):
		self.xite = XiteWin.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def testAddLine(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		start = time.time()
		for i in range(1000):
			self.ed.AddText(len(data), data)
			self.assertEquals(self.ed.LineCount, i + 2)
		end = time.time()
		duration = end - start
		print("%6.3f testAddLine" % duration)
		self.xite.DoEvents()
		self.assert_(self.ed.Length > 0)

	def testAddLineMiddle(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		start = time.time()
		for i in range(1000):
			self.ed.AddText(len(data), data)
			self.assertEquals(self.ed.LineCount, i + 2)
		end = time.time()
		duration = end - start
		print("%6.3f testAddLineMiddle" % duration)
		self.xite.DoEvents()
		self.assert_(self.ed.Length > 0)

	def testHuge(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		data = data * 100000
		start = time.time()
		self.ed.AddText(len(data), data)
		end = time.time()
		duration = end - start
		print("%6.3f testHuge" % duration)
		self.xite.DoEvents()
		self.assert_(self.ed.Length > 0)

	def testHugeInserts(self):
		data = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		data = data * 100000
		insert = (string.digits + "\n").encode('utf-8')
		self.ed.AddText(len(data), data)
		start = time.time()
		for i in range(1000):
			self.ed.InsertText(0, insert)
		end = time.time()
		duration = end - start
		print("%6.3f testHugeInserts" % duration)
		self.xite.DoEvents()
		self.assert_(self.ed.Length > 0)

	def testHugeReplace(self):
		oneLine = (string.ascii_letters + string.digits + "\n").encode('utf-8')
		data = oneLine * 100000
		insert = (string.digits + "\n").encode('utf-8')
		self.ed.AddText(len(data), data)
		start = time.time()
		for i in range(1000):
			self.ed.TargetStart = i * len(insert)
			self.ed.TargetEnd = self.ed.TargetStart + len(oneLine)
			self.ed.ReplaceTarget(len(insert), insert)
		end = time.time()
		duration = end - start
		print("%6.3f testHugeReplace" % duration)
		self.xite.DoEvents()
		self.assert_(self.ed.Length > 0)

if __name__ == '__main__':
	XiteWin.main("performanceTests")
