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

keywordsPerl = [
b"NULL __FILE__ __LINE__ __PACKAGE__ __DATA__ __END__ AUTOLOAD "
b"BEGIN CORE DESTROY END EQ GE GT INIT LE LT NE CHECK abs accept "
b"alarm and atan2 bind binmode bless caller chdir chmod chomp chop "
b"chown chr chroot close closedir cmp connect continue cos crypt "
b"dbmclose dbmopen defined delete die do dump each else elsif endgrent "
b"endhostent endnetent endprotoent endpwent endservent eof eq eval "
b"exec exists exit exp fcntl fileno flock for foreach fork format "
b"formline ge getc getgrent getgrgid getgrnam gethostbyaddr gethostbyname "
b"gethostent getlogin getnetbyaddr getnetbyname getnetent getpeername "
b"getpgrp getppid getpriority getprotobyname getprotobynumber getprotoent "
b"getpwent getpwnam getpwuid getservbyname getservbyport getservent "
b"getsockname getsockopt glob gmtime goto grep gt hex if index "
b"int ioctl join keys kill last lc lcfirst le length link listen "
b"local localtime lock log lstat lt map mkdir msgctl msgget msgrcv "
b"msgsnd my ne next no not oct open opendir or ord our pack package "
b"pipe pop pos print printf prototype push quotemeta qu "
b"rand read readdir readline readlink readpipe recv redo "
b"ref rename require reset return reverse rewinddir rindex rmdir "
b"scalar seek seekdir select semctl semget semop send setgrent "
b"sethostent setnetent setpgrp setpriority setprotoent setpwent "
b"setservent setsockopt shift shmctl shmget shmread shmwrite shutdown "
b"sin sleep socket socketpair sort splice split sprintf sqrt srand "
b"stat study sub substr symlink syscall sysopen sysread sysseek "
b"system syswrite tell telldir tie tied time times truncate "
b"uc ucfirst umask undef unless unlink unpack unshift untie until "
b"use utime values vec wait waitpid wantarray warn while write "
b"xor "
b"given when default break say state UNITCHECK __SUB__ fc"
]

class TestLexers(unittest.TestCase):

	def setUp(self):
		self.xite = Xite.xiteFrame
		self.ed = self.xite.ed
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()

	def AsStyled(self, withWindowsLineEnds):
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
		if withWindowsLineEnds:
			return data.getvalue().replace(b"\n", b"\r\n")
		else:
			return data.getvalue()

	def LexExample(self, name, lexerName, keywords, fileMode="b"):
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.SetCodePage(65001)
		self.ed.LexerLanguage = lexerName
		mask = 0xff
		for i in range(len(keywords)):
			self.ed.SetKeyWords(i, keywords[i])

		nameExample = os.path.join("examples", name)
		namePrevious = nameExample +".styled"
		nameNew = nameExample +".new"
		with open(nameExample, "rb") as f:
			prog = f.read()
		if fileMode == "t" and sys.platform == "win32":
			prog = prog.replace(b"\r\n", b"\n")
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
			if fileMode == "t" and sys.platform == "win32":
				prog = prog.replace(b"\r\n", b"\n")
		except EnvironmentError:
			prevStyled = ""
		progStyled = self.AsStyled(fileMode == "t" and sys.platform == "win32")
		if progStyled != prevStyled:
			with open(nameNew, "wb") as f:
				f.write(progStyled)
			print("Incorrect lex for " + name)
			print(progStyled)
			print(prevStyled)
			self.assertEquals(progStyled, prevStyled)
			# The whole file doesn't parse like it did before so don't try line by line
			# as that is likely to fail many times.
			return

		if fileMode == "b":	# "t" files are large and this is a quadratic check
			# Try partial lexes from the start of every line which should all be identical.
			for line in range(self.ed.LineCount):
				lineStart = self.ed.PositionFromLine(line)
				self.ed.StartStyling(lineStart, mask)
				self.assertEquals(self.ed.EndStyled, lineStart)
				self.ed.Colourise(lineStart, lenDocument)
				progStyled = self.AsStyled(fileMode == "t" and sys.platform == "win32")
				if progStyled != prevStyled:
					print("Incorrect partial lex for " + name + " at line " + line)
					with open(nameNew, "wb") as f:
						f.write(progStyled)
					self.assertEquals(progStyled, prevStyled)
					# Give up after one failure
					return

	# Test lexing just once from beginning to end in text form.
	# This is used for test cases that are too long to be exhaustively tested by lines and
	# may be sensitive to line ends so are tested as if using Unix LF line ends.
	def LexLongCase(self, name, lexerName, keywords, fileMode="b"):
		self.LexExample(name, lexerName, keywords, "t")

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

	def testNim(self):
		self.LexExample("x.nim", b"nim", [b"else end if let proc"])

	def testRuby(self):
		self.LexExample("x.rb", b"ruby", [b"class def end"])

	def testPerl(self):
		self.LexExample("x.pl", b"perl", keywordsPerl)

	def testPerl52(self):
		self.LexLongCase("perl-test-5220delta.pl", b"perl", keywordsPerl)

	def testPerlPrototypes(self):
		self.LexLongCase("perl-test-sub-prototypes.pl", b"perl", keywordsPerl)

	def testD(self):
		self.LexExample("x.d", b"d",
			[b"keyword1", b"keyword2", b"", b"keyword4", b"keyword5",
			b"keyword6", b"keyword7"])

	def testTCL(self):
		self.LexExample("x.tcl", b"tcl", [b"proc set socket vwait"])

if __name__ == '__main__':
	Xite.main("lexTests")
