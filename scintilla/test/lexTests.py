# -*- coding: utf-8 -*-
# Requires Python 2.7 or later

import io, os, sys, unittest

if sys.platform == "win32":
	import XiteWin as Xite
else:
	import XiteQt as Xite

keywordsHTML = [
b"b body content head href html link meta "
	b"name rel script strong title type xmlns",
b"function",
b"sub"
]

class TestLexers(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def AsStyled(self):
		text = self.ed.Contents()
		data = io.BytesIO()
		prevStyle = -1
		for o in range(self.ed.Length):
			styleNow = self.ed.GetStyleAt(o)
			if styleNow != prevStyle:
				styleBuf = "{%0d}" % styleNow
				data.write(styleBuf.encode('utf-8'))
				prevStyle = styleNow
			data.write(text[o:o+1])
		return data.getvalue()

	def LexExample(self, name, lexerName, keywords=None):
		if keywords is None:
			keywords = []
		self.ed.SetCodePage(65001)
		self.ed.LexerLanguage = lexerName
		bits = self.ed.StyleBitsNeeded
		mask = 2 << bits - 1
		self.ed.StyleBits = bits
		for i in range(len(keywords)):
			self.ed.SetKeyWords(i, keywords[i])

		nameExample = os.path.join("examples", name)
		namePrevious = nameExample +".styled"
		nameNew = nameExample +".new"
		with open(nameExample, "rb") as f:
			prog = f.read()
		BOM = b"\xEF\xBB\xBF"
		if prog.startswith(BOM):
			prog = prog[len(BOM):]
		lenDocument = len(prog)
		self.ed.AddText(lenDocument, prog)
		self.ed.Colourise(0, lenDocument)
		self.assertEquals(self.ed.EndStyled, lenDocument)
		try:
			with open(namePrevious, "rb") as f:
				prevStyled = f.read()
		except FileNotFoundError:
			prevStyled = ""
		progStyled = self.AsStyled()
		if progStyled != prevStyled:
			with open(nameNew, "wb") as f:
				f.write(progStyled)
			print(progStyled)
			print(prevStyled)
			self.assertEquals(progStyled, prevStyled)
			# The whole file doesn't parse like it did before so don't try line by line
			# as that is likely to fail many times.
			return

		# Try partial lexes from the start of every line which should all be identical.
		for line in range(self.ed.LineCount):
			lineStart = self.ed.PositionFromLine(line)
			self.ed.StartStyling(lineStart, mask)
			self.assertEquals(self.ed.EndStyled, lineStart)
			self.ed.Colourise(lineStart, lenDocument)
			progStyled = self.AsStyled()
			if progStyled != prevStyled:
				with open(nameNew, "wb") as f:
					f.write(progStyled)
				self.assertEquals(progStyled, prevStyled)
				# Give up after one failure
				return

	def testCXX(self):
		self.LexExample("x.cxx", b"cpp", [b"int"])

	def testPython(self):
		self.LexExample("x.py", b"python",
			[b"class def else for if import in print return while"])

	def testHTML(self):
		self.LexExample("x.html", b"hypertext", keywordsHTML)

	def testASP(self):
		self.LexExample("x.asp", b"hypertext", keywordsHTML)

	def testPHP(self):
		self.LexExample("x.php", b"hypertext", keywordsHTML)

	def testVB(self):
		self.LexExample("x.vb", b"vb", [b"as dim or string"])

	def testLua(self):
		self.LexExample("x.lua", b"lua", [b"function end"])

	def testRuby(self):
		self.LexExample("x.rb", b"ruby", [b"class def end"])

	def testPerl(self):
		self.LexExample("x.pl", b"perl", [b"printf sleep use while"])

	def testD(self):
		self.LexExample("x.d", b"d",
			[b"keyword1", b"keyword2", b"", b"keyword4", b"keyword5",
			b"keyword6", b"keyword7"])

if __name__ == '__main__':
	Xite.main("lexTests")
