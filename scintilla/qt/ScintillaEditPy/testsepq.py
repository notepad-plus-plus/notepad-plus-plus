#!/usr/bin/python
# -*- coding: utf-8 -*-

import sys

from PySide.QtCore import *
from PySide.QtGui import *

import ScintillaConstants as sci

sys.path.append("../..")
from bin import ScintillaEditPy

txtInit =	"int main(int argc, char **argv) {\n" \
		"    // Start up the gnome\n" \
		"    gnome_init(\"stest\", \"1.0\", argc, argv);\n}\n";

keywords = \
		"and and_eq asm auto bitand bitor bool break " \
		"case catch char class compl const const_cast continue " \
		"default delete do double dynamic_cast else enum explicit export extern false float for " \
		"friend goto if inline int long mutable namespace new not not_eq " \
		"operator or or_eq private protected public " \
		"register reinterpret_cast return short signed sizeof static static_cast struct switch " \
		"template this throw true try typedef typeid typename union unsigned using " \
		"virtual void volatile wchar_t while xor xor_eq";

def uriDropped():
	print "uriDropped"

class Form(QDialog):

	def __init__(self, parent=None):
		super(Form, self).__init__(parent)
		self.resize(460,300)
		# Create widgets
		self.edit = ScintillaEditPy.ScintillaEdit(self)
		self.edit.uriDropped.connect(uriDropped)
		self.edit.command.connect(self.receive_command)
		self.edit.notify.connect(self.receive_notification)

		self.edit.styleClearAll()
		self.edit.setMarginWidthN(0, 35)
		self.edit.setScrollWidth(200)
		self.edit.setScrollWidthTracking(1)
		self.edit.setLexer(sci.SCLEX_CPP)
		self.edit.styleSetFore(sci.SCE_C_COMMENT, 0x008000)
		self.edit.styleSetFore(sci.SCE_C_COMMENTLINE, 0x008000)
		self.edit.styleSetFore(sci.SCE_C_COMMENTDOC, 0x008040)
		self.edit.styleSetItalic(sci.SCE_C_COMMENTDOC, 1)
		self.edit.styleSetFore(sci.SCE_C_NUMBER, 0x808000)
		self.edit.styleSetFore(sci.SCE_C_WORD, 0x800000)
		self.edit.styleSetBold(sci.SCE_C_WORD, True)
		self.edit.styleSetFore(sci.SCE_C_STRING, 0x800080)
		self.edit.styleSetFore(sci.SCE_C_PREPROCESSOR, 0x008080)
		self.edit.styleSetBold(sci.SCE_C_OPERATOR, True)
		self.edit.setMultipleSelection(1)
		self.edit.setVirtualSpaceOptions(
			sci.SCVS_RECTANGULARSELECTION | sci.SCVS_USERACCESSIBLE)
		self.edit.setAdditionalSelectionTyping(1)

		self.edit.styleSetFore(sci.STYLE_INDENTGUIDE, 0x808080)
		self.edit.setIndentationGuides(sci.SC_IV_LOOKBOTH)

		self.edit.setKeyWords(0, keywords)
		self.edit.addText(len(txtInit), txtInit)
		self.edit.setSel(1,10)
		retriever = str(self.edit.getLine(1))
		print(type(retriever), len(retriever))
		print('[' + retriever + ']')
		someText = str(self.edit.textRange(2,5))
		print(len(someText), '[' + someText + ']')
		someText = self.edit.getCurLine(100)
		print(len(someText), '[' + someText + ']')
		someText = self.edit.styleFont(1)
		print(len(someText), '[' + someText + ']')
		someText = self.edit.getSelText()
		print(len(someText), '[' + someText + ']')
		someText = self.edit.tag(1)
		print(len(someText), '[' + someText + ']')
		someText = self.edit.autoCCurrentText()
		print(len(someText), '[' + someText + ']')
		someText = self.edit.annotationText(1)
		print(len(someText), '[' + someText + ']')
		someText = self.edit.annotationStyles(1)
		print(len(someText), '[' + someText + ']')
		someText = self.edit.describeKeyWordSets()
		print(len(someText), '[' + someText + ']')
		someText = self.edit.propertyNames()
		print(len(someText), '[' + someText + ']')
		self.edit.setProperty("fold", "1")
		someText = self.edit.property("fold")
		print(len(someText), '[' + someText + ']')
		someText = self.edit.propertyExpanded("fold")
		print(len(someText), '[' + someText + ']')
		someText = self.edit.lexerLanguage()
		print(len(someText), '[' + someText + ']')
		someText = self.edit.describeProperty("styling.within.preprocessor")
		print(len(someText), '[' + someText + ']')

		xx = self.edit.findText(0, "main", 0, 25)
		print(type(xx), xx)
		print("isBold", self.edit.styleBold(sci.SCE_C_WORD))

		# Retrieve the document and write into it
		doc = self.edit.get_doc()
		doc.insert_string(40, "***")
		stars = doc.get_char_range(40,3)
		assert stars == "***"

		# Create a new independent document and attach it to the editor
		doc = ScintillaEditPy.ScintillaDocument()
		doc.insert_string(0, "/***/\nif(a)\n")
		self.edit.set_doc(doc)
		self.edit.setLexer(sci.SCLEX_CPP)

	def Call(self, message, wParam=0, lParam=0):
		return self.edit.send(message, wParam, lParam)

	def resizeEvent(self, e):
		self.edit.resize(e.size().width(), e.size().height())

	def receive_command(self, wParam, lParam):
		# Show underline at start when focussed
		notifyCode = wParam >> 16
		if (notifyCode == sci.SCEN_SETFOCUS) or (notifyCode == sci.SCEN_KILLFOCUS):
			self.edit.setIndicatorCurrent(sci.INDIC_CONTAINER);
			self.edit.indicatorClearRange(0, self.edit.length())
			if notifyCode == sci.SCEN_SETFOCUS:
				self.edit.indicatorFillRange(0, 2);

	def receive_notification(self, scn):
		if scn.nmhdr.code == sci.SCN_CHARADDED:
			print "Char %02X" % scn.ch
		elif scn.nmhdr.code == sci.SCN_SAVEPOINTREACHED:
			print "Saved"
		elif scn.nmhdr.code == sci.SCN_SAVEPOINTLEFT:
			print "Unsaved"
		elif scn.nmhdr.code == sci.SCN_MODIFIED:
			print "Modified"
		elif scn.nmhdr.code == sci.SCN_UPDATEUI:
			print "Update UI"
		elif scn.nmhdr.code == sci.SCN_PAINTED:
			#print "Painted"
			pass
		else:
			print "Notification", scn.nmhdr.code
			pass

if __name__ == '__main__':
    # Create the Qt Application
    app = QApplication(sys.argv)
    # Create and show the form
    form = Form()
    form.show()
    # Run the main Qt loop
    sys.exit(app.exec_())
