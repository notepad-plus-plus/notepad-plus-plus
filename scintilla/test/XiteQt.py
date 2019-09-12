# -*- coding: utf-8 -*-
# Requires Python 2.7 or later

import ctypes, os, sys, unittest

from PySide.QtCore import *
from PySide.QtGui import *

import ScintillaCallable

sys.path.append("..")
from bin import ScintillaEditPy

scintillaDirectory = ".."
scintillaIncludeDirectory = os.path.join(scintillaDirectory, "include")
scintillaScriptsDirectory = os.path.join(scintillaDirectory, "scripts")
sys.path.append(scintillaScriptsDirectory)
import Face

class Form(QDialog):

	def __init__(self, parent=None):
		super(Form, self).__init__(parent)
		self.resize(460,300)
		# Create widget
		self.edit = ScintillaEditPy.ScintillaEdit(self)

class XiteWin():
	def __init__(self, test=""):
		self.face = Face.Face()
		self.face.ReadFromFile(os.path.join(scintillaIncludeDirectory, "Scintilla.iface"))

		self.test = test

		self.form = Form()

		scifn = self.form.edit.send(int(self.face.features["GetDirectFunction"]["Value"]), 0, 0)
		sciptr = ctypes.c_char_p(self.form.edit.send(
			int(self.face.features["GetDirectPointer"]["Value"]), 0,0))

		self.ed = ScintillaCallable.ScintillaCallable(self.face, scifn, sciptr)
		self.form.show()

	def DoStuff(self):
		print(self.test)
		self.CmdTest()

	def DoEvents(self):
		QApplication.processEvents()

	def CmdTest(self):
		runner = unittest.TextTestRunner()
		tests = unittest.defaultTestLoader.loadTestsFromName(self.test)
		results = runner.run(tests)
		print(results)
		sys.exit(0)

xiteFrame = None

def main(test):
	global xiteFrame
	app = QApplication(sys.argv)
	xiteFrame = XiteWin(test)
	xiteFrame.DoStuff()
	sys.exit(app.exec_())
