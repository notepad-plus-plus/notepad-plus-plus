# -*- coding: utf-8 -*-

from __future__ import with_statement
from __future__ import unicode_literals

import os, sys, unittest

import ctypes
from ctypes import wintypes
from ctypes import c_int, c_ulong, c_char_p, c_wchar_p, c_ushort
user32=ctypes.windll.user32
gdi32=ctypes.windll.gdi32
kernel32=ctypes.windll.kernel32
from MessageNumbers import msgs, sgsm

import XiteMenu

scintillaDirectory = ".."
scintillaIncludeDirectory = os.path.join(scintillaDirectory, "include")
sys.path.append(scintillaIncludeDirectory)
import Face

scintillaBinDirectory = os.path.join(scintillaDirectory, "bin")
os.environ['PATH'] = os.environ['PATH']  + ";" + scintillaBinDirectory
#print(os.environ['PATH'])

WFUNC = ctypes.WINFUNCTYPE(c_int, c_int, c_int, c_int, c_int)

WS_CHILD = 0x40000000
WS_CLIPCHILDREN = 0x2000000
WS_OVERLAPPEDWINDOW = 0xcf0000
WS_VISIBLE = 0x10000000
WS_HSCROLL = 0x100000
WS_VSCROLL = 0x200000
WA_INACTIVE = 0
MF_POPUP = 16
MF_SEPARATOR = 0x800
IDYES = 6
OFN_HIDEREADONLY = 4
MB_OK = 0
MB_YESNOCANCEL = 3
MF_CHECKED = 8
MF_UNCHECKED = 0
SW_SHOW = 5
PM_REMOVE = 1

VK_SHIFT = 16
VK_CONTROL = 17
VK_MENU = 18

class OPENFILENAME(ctypes.Structure):
	_fields_ = (("lStructSize", c_int),
		("hwndOwner", c_int),
		("hInstance", c_int),
		("lpstrFilter", c_wchar_p),
		("lpstrCustomFilter", c_char_p),
		("nMaxCustFilter", c_int),
		("nFilterIndex", c_int),
		("lpstrFile", c_wchar_p),
		("nMaxFile", c_int),
		("lpstrFileTitle", c_wchar_p),
		("nMaxFileTitle", c_int),
		("lpstrInitialDir", c_wchar_p),
		("lpstrTitle", c_wchar_p),
		("flags", c_int),
		("nFileOffset", c_ushort),
		("nFileExtension", c_ushort),
		("lpstrDefExt", c_char_p),
		("lCustData", c_int),
		("lpfnHook", c_char_p),
		("lpTemplateName", c_char_p),
		("pvReserved", c_char_p),
		("dwReserved", c_int),
		("flagsEx", c_int))

	def __init__(self, win, title):
		ctypes.Structure.__init__(self)
		self.lStructSize = ctypes.sizeof(OPENFILENAME)
		self.nMaxFile = 1024
		self.hwndOwner = win
		self.lpstrTitle = title
		self.Flags = OFN_HIDEREADONLY

trace = False
#~ trace = True

def WindowSize(w):
	rc = ctypes.wintypes.RECT()
	user32.GetClientRect(w, ctypes.byref(rc))
	return rc.right - rc.left, rc.bottom - rc.top

def IsKeyDown(key):
	return (user32.GetKeyState(key) & 0x8000) != 0

def KeyTranslate(w):
	tr = { 9: "Tab", 0xD:"Enter", 0x1B: "Esc" }
	if w in tr:
		return tr[w]
	elif ord("A") <= w <= ord("Z"):
		return chr(w)
	elif 0x70 <= w <= 0x7b:
		return "F" + str(w-0x70+1)
	else:
		return "Unknown_" + hex(w)

class WNDCLASS(ctypes.Structure):
	_fields_= (\
		('style', c_int),
		('lpfnWndProc', WFUNC),
		('cls_extra', c_int),
		('wnd_extra', c_int),
		('hInst', c_int),
		('hIcon', c_int),
		('hCursor', c_int),
		('hbrBackground', c_int),
		('menu_name', c_wchar_p),
		('lpzClassName', c_wchar_p),
	)

class XTEXTRANGE(ctypes.Structure):
	_fields_= (\
		('cpMin', c_int),
		('cpMax', c_int),
		('lpstrText', c_char_p),
	)

class TEXTRANGE(ctypes.Structure):
	_fields_= (\
		('cpMin', c_int),
		('cpMax', c_int),
		('lpstrText', ctypes.POINTER(ctypes.c_char)),
	)

class FINDTEXT(ctypes.Structure):
	_fields_= (\
		('cpMin', c_int),
		('cpMax', c_int),
		('lpstrText', c_char_p),
		('cpMinText', c_int),
		('cpMaxText', c_int),
	)

hinst = ctypes.windll.kernel32.GetModuleHandleW(0)

def RegisterClass(name, func, background = 0):
	# register a window class for toplevel windows.
	wc = WNDCLASS()
	wc.style = 0
	wc.lpfnWndProc = func
	wc.cls_extra = 0
	wc.wnd_extra = 0
	wc.hInst = hinst
	wc.hIcon = 0
	wc.hCursor = 0
	wc.hbrBackground = background
	wc.menu_name = 0
	wc.lpzClassName = name
	user32.RegisterClassW(ctypes.byref(wc))

class SciCall:
	def __init__(self, fn, ptr, msg):
		self._fn = fn
		self._ptr = ptr
		self._msg = msg
	def __call__(self, w=0, l=0):
		return self._fn(self._ptr, self._msg, w, l)

class Scintilla:
	def __init__(self, face, hwndParent, hinstance):
		self.__dict__["face"] = face
		self.__dict__["used"] = set()
		self.__dict__["all"] = set()
		# The k member is for accessing constants as a dictionary
		self.__dict__["k"] = {}
		for f in face.features:
			self.all.add(f)
			if face.features[f]["FeatureType"] == "val":
				self.k[f] = int(self.face.features[f]["Value"], 0)
			elif face.features[f]["FeatureType"] == "evt":
				self.k["SCN_"+f] = int(self.face.features[f]["Value"], 0)
		# Get the function first as that also loads the DLL
		import ctypes.util
		print >> sys.stderr, "SciLexer path: ",ctypes.util.find_library("scilexer")
		self.__dict__["_scifn"] = ctypes.windll.SciLexer.Scintilla_DirectFunction
		self.__dict__["_hwnd"] = user32.CreateWindowExW(0,
			"Scintilla", "Source",
			WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
			0, 0, 100, 100, hwndParent, 0, hinstance, 0)
		self.__dict__["_sciptr"] = user32.SendMessageW(self._hwnd,
			int(self.face.features["GetDirectPointer"]["Value"], 0), 0,0)
		user32.ShowWindow(self._hwnd, SW_SHOW)
	def __getattr__(self, name):
		if name in self.face.features:
			self.used.add(name)
			feature = self.face.features[name]
			value = int(feature["Value"], 0)
			#~ print("Feature", name, feature)
			if feature["FeatureType"] == "val":
				self.__dict__[name] = value
				return value
			else:
				return SciCall(self._scifn, self._sciptr, value)
		elif ("Get" + name) in self.face.features:
			self.used.add("Get" + name)
			feature = self.face.features["Get" + name]
			value = int(feature["Value"], 0)
			if feature["FeatureType"] == "get" and \
				not name.startswith("Get") and \
				not feature["Param1Type"] and \
				not feature["Param2Type"] and \
				feature["ReturnType"] in ["bool", "int", "position"]:
				#~ print("property", feature)
				return self._scifn(self._sciptr, value, 0, 0)
		elif name.startswith("SCN_") and name in self.k:
			self.used.add(name)
			feature = self.face.features[name[4:]]
			value = int(feature["Value"], 0)
			#~ print("Feature", name, feature)
			if feature["FeatureType"] == "val":
				return value
		raise AttributeError(name)
	def __setattr__(self, name, val):
		if ("Set" + name) in self.face.features:
			self.used.add("Set" + name)
			feature = self.face.features["Set" + name]
			value = int(feature["Value"], 0)
			#~ print("setproperty", feature)
			if feature["FeatureType"] == "set" and not name.startswith("Set"):
				if feature["Param1Type"] in ["bool", "int", "position"]:
					return self._scifn(self._sciptr, value, val, 0)
				elif feature["Param2Type"] in ["string"]:
					return self._scifn(self._sciptr, value, 0, val)
				raise AttributeError(name)
		raise AttributeError(name)
	def getvalue(self, name):
		if name in self.face.features:
			feature = self.face.features[name]
			if feature["FeatureType"] != "evt":
				try:
					return int(feature["Value"], 0)
				except ValueError:
					return -1
		return -1


	def ByteRange(self, start, end):
		tr = TEXTRANGE()
		tr.cpMin = start
		tr.cpMax = end
		length = end - start
		tr.lpstrText = ctypes.create_string_buffer(length + 1)
		self.GetTextRange(0, ctypes.byref(tr))
		text = tr.lpstrText[:length]
		text += b"\0" * (length - len(text))
		return text
	def StyledTextRange(self, start, end):
		tr = TEXTRANGE()
		tr.cpMin = start
		tr.cpMax = end
		length = 2 * (end - start)
		tr.lpstrText = ctypes.create_string_buffer(length + 2)
		self.GetStyledText(0, ctypes.byref(tr))
		styledText = tr.lpstrText[:length]
		styledText += b"\0" * (length - len(styledText))
		return styledText
	def FindBytes(self, start, end, s, flags):
		ft = FINDTEXT()
		ft.cpMin = start
		ft.cpMax = end
		ft.lpstrText = s
		ft.cpMinText = 0
		ft.cpMaxText = 0
		pos = self.FindText(flags, ctypes.byref(ft))
		#~ print(start, end, ft.cpMinText, ft.cpMaxText)
		return pos
	def FindBytes2(self, start, end, s, flags):
		ft = FINDTEXT()
		ft.cpMin = start
		ft.cpMax = end
		ft.lpstrText = s
		ft.cpMinText = 0
		ft.cpMaxText = 0
		pos = self.FindText(flags, ctypes.byref(ft))
		#~ print(start, end, ft.cpMinText, ft.cpMaxText)
		return (pos, ft.cpMinText, ft.cpMaxText)

	def Contents(self):
		return self.ByteRange(0, self.Length)
	def SizeTo(self, width, height):
		user32.SetWindowPos(self._hwnd, 0, 0, 0, width, height, 0)
	def FocusOn(self):
		user32.SetFocus(self._hwnd)

class XiteWin():
	def __init__(self, test=""):
		self.face = Face.Face()
		self.face.ReadFromFile(os.path.join(scintillaIncludeDirectory, "Scintilla.iface"))

		self.titleDirty = True
		self.fullPath = ""
		self.test = test

		self.appName = "xite"

		self.cmds = {}
		self.windowName = "XiteWindow"
		self.wfunc = WFUNC(self.WndProc)
		RegisterClass(self.windowName, self.wfunc)
		user32.CreateWindowExW(0, self.windowName, self.appName, \
			WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, \
			0, 0, 500, 700, 0, 0, hinst, 0)

		args = sys.argv[1:]
		self.SetMenus()
		if args:
			self.GrabFile(args[0])
			self.ed.FocusOn()
			self.ed.GotoPos(self.ed.Length)

		if self.test:
			print(self.test)
			for k in self.cmds:
				if self.cmds[k] == "Test":
					user32.PostMessageW(self.win, msgs["WM_COMMAND"], k, 0)

	def OnSize(self):
		width, height = WindowSize(self.win)
		self.ed.SizeTo(width, height)
		user32.InvalidateRect(self.win, 0, 0)

	def OnCreate(self, hwnd):
		self.win = hwnd
		self.ed = Scintilla(self.face, hwnd, hinst)
		self.ed.FocusOn()


	def Invalidate(self):
		user32.InvalidateRect(self.win, 0, 0)

	def WndProc(self, h, m, w, l):
		ms = sgsm.get(m, "XXX")
		if trace:
			print("%s %s %s %s" % (hex(h)[2:],ms,w,l))
		if ms == "WM_CLOSE":
			user32.PostQuitMessage(0)
		elif ms == "WM_CREATE":
			self.OnCreate(h)
			return 0
		elif ms == "WM_SIZE":
			# Work out size
			if w != 1:
				self.OnSize()
			return 0
		elif ms == "WM_COMMAND":
			cmdCode = w & 0xffff
			if cmdCode in self.cmds:
				self.Command(self.cmds[cmdCode])
			return 0
		elif ms == "WM_ACTIVATE":
			if w != WA_INACTIVE:
				self.ed.FocusOn()
			return 0
		else:
			return user32.DefWindowProcW(h, m, w, l)
		return 0

	def Command(self, name):
		name = name.replace(" ", "")
		method = "Cmd" + name
		cmd = None
		try:
			cmd = getattr(self, method)
		except AttributeError:
			return
		if cmd:
			cmd()

	def KeyDown(self, w, prefix = ""):
		keyName = prefix
		if IsKeyDown(VK_CONTROL):
			keyName += "<control>"
		if IsKeyDown(VK_SHIFT):
			keyName += "<shift>"
		keyName += KeyTranslate(w)
		if trace:
			print("Key:", keyName)
		if keyName in self.keys:
			method = "Cmd" + self.keys[keyName]
			getattr(self, method)()
			return True
		#~ print("UKey:", keyName)
		return False

	def Accelerator(self, msg):
		ms = sgsm.get(msg.message, "XXX")
		if ms == "WM_KEYDOWN":
			return self.KeyDown(msg.wParam)
		elif ms == "WM_SYSKEYDOWN":
			return self.KeyDown(msg.wParam, "<alt>")
		return False

	def AppLoop(self):
		msg = ctypes.wintypes.MSG()
		lpmsg = ctypes.byref(msg)
		while user32.GetMessageW(lpmsg, 0, 0, 0):
			if trace and msg.message != msgs["WM_TIMER"]:
				print('mm', hex(msg.hWnd)[2:],sgsm.get(msg.message, "XXX"))
			if not self.Accelerator(msg):
				user32.TranslateMessage(lpmsg)
				user32.DispatchMessageW(lpmsg)

	def DoEvents(self):
		msg = ctypes.wintypes.MSG()
		lpmsg = ctypes.byref(msg)
		cont = True
		while cont:
			cont = user32.PeekMessageW(lpmsg, 0, 0, 0, PM_REMOVE)
			if cont:
				if not self.Accelerator(msg):
					user32.TranslateMessage(lpmsg)
					user32.DispatchMessageW(lpmsg)

	def SetTitle(self, changePath):
		if changePath or self.titleDirty != self.ed.Modify:
			self.titleDirty = self.ed.Modify
			self.title = self.fullPath
			if self.titleDirty:
				self.title += " * "
			else:
				self.title += " - "
			self.title += self.appName
			if self.win:
				user32.SetWindowTextW(self.win, self.title)

	def Open(self):
		ofx = OPENFILENAME(self.win, "Open File")
		opath = "\0" * 1024
		ofx.lpstrFile = opath
		filters = ["Python (.py;.pyw)|*.py;*.pyw|All|*.*"]
		filterText = "\0".join([f.replace("|", "\0") for f in filters])+"\0\0"
		ofx.lpstrFilter = filterText
		if ctypes.windll.comdlg32.GetOpenFileNameW(ctypes.byref(ofx)):
			absPath = opath.replace("\0", "")
			self.GrabFile(absPath)
			self.ed.FocusOn()
			self.ed.LexerLanguage = "python"
			self.ed.Lexer = self.ed.SCLEX_PYTHON
			self.ed.SetKeyWords(0, b"class def else for from if import print return while")
			for style in [k for k in self.ed.k if k.startswith("SCE_P_")]:
				self.ed.StyleSetFont(self.ed.k[style], b"Verdana")
				if "COMMENT" in style:
					self.ed.StyleSetFore(self.ed.k[style], 127 * 256)
					self.ed.StyleSetFont(self.ed.k[style], b"Comic Sans MS")
				elif "OPERATOR" in style:
					self.ed.StyleSetBold(self.ed.k[style], 1)
					self.ed.StyleSetFore(self.ed.k[style], 127 * 256 * 256)
				elif "WORD" in style:
					self.ed.StyleSetItalic(self.ed.k[style], 255)
					self.ed.StyleSetFore(self.ed.k[style], 255 * 256 * 256)
				elif "TRIPLE" in style:
					self.ed.StyleSetFore(self.ed.k[style], 0xA0A0)
				elif "STRING" in style or "CHARACTER" in style:
					self.ed.StyleSetFore(self.ed.k[style], 0xA000A0)
				else:
					self.ed.StyleSetFore(self.ed.k[style], 0)

	def SaveAs(self):
		ofx = OPENFILENAME(self.win, "Save File")
		opath = "\0" * 1024
		ofx.lpstrFile = opath
		if ctypes.windll.comdlg32.GetSaveFileNameW(ctypes.byref(ofx)):
			self.fullPath = opath.replace("\0", "")
			self.Save()
			self.SetTitle(1)
			self.ed.FocusOn()

	def SetMenus(self):
		ui = XiteMenu.MenuStructure
		self.cmds = {}
		self.keys = {}

		cmdId = 0
		self.menuBar = user32.CreateMenu()
		for name, contents in ui:
			cmdId += 1
			menu = user32.CreateMenu()
			for item in contents:
				text, key = item
				cmdText = text.replace("&", "")
				cmdText = cmdText.replace("...", "")
				cmdText = cmdText.replace(" ", "")
				cmdId += 1
				if key:
					keyText = key.replace("<control>", "Ctrl+")
					keyText = keyText.replace("<shift>", "Shift+")
					text += "\t" + keyText
				if text == "-":
					user32.AppendMenuW(menu, MF_SEPARATOR, cmdId, text)
				else:
					user32.AppendMenuW(menu, 0, cmdId, text)
				self.cmds[cmdId] = cmdText
				self.keys[key] = cmdText
				#~ print(cmdId, item)
			user32.AppendMenuW(self.menuBar, MF_POPUP, menu, name)
		user32.SetMenu(self.win, self.menuBar)
		self.CheckMenuItem("Wrap", True)
		user32.ShowWindow(self.win, SW_SHOW)

	def CheckMenuItem(self, name, val):
		#~ print(name, val)
		if self.cmds:
			for k,v in self.cmds.items():
				if v == name:
					#~ print(name, k)
					user32.CheckMenuItem(user32.GetMenu(self.win), \
						k, [MF_UNCHECKED, MF_CHECKED][val])

	def Exit(self):
		sys.exit(0)

	def DisplayMessage(self, msg, ask):
		return IDYES == user32.MessageBoxW(self.win, \
			msg, self.appName, [MB_OK, MB_YESNOCANCEL][ask])

	def NewDocument(self):
		self.ed.ClearAll()
		self.ed.EmptyUndoBuffer()
		self.ed.SetSavePoint()

	def SaveIfUnsure(self):
		if self.ed.Modify:
			msg = "Save changes to \"" + self.fullPath + "\"?"
			print(msg)
			decision = self.DisplayMessage(msg, True)
			if decision:
				self.CmdSave()
			return decision
		return True

	def New(self):
		if self.SaveIfUnsure():
			self.fullPath = ""
			self.overrideMode = None
			self.NewDocument()
			self.SetTitle(1)
		self.Invalidate()

	def CheckMenus(self):
		pass

	def MoveSelection(self, caret, anchor=-1):
		if anchor == -1:
			anchor = caret
		self.ed.SetSelectionStart(caret)
		self.ed.SetSelectionEnd(anchor)
		self.ed.ScrollCaret()
		self.Invalidate()

	def GrabFile(self, name):
		self.fullPath = name
		self.overrideMode = None
		self.NewDocument()
		fsr = open(name, "rb")
		data = fsr.read()
		fsr.close()
		self.ed.AddText(len(data), data)
		self.ed.EmptyUndoBuffer()
		self.MoveSelection(0)
		self.SetTitle(1)

	def Save(self):
		fos = open(self.fullPath, "wb")
		blockSize = 1024
		length = self.ed.Length
		i = 0
		while i < length:
			grabSize = length - i
			if grabSize > blockSize:
				grabSize = blockSize
			#~ print(i, grabSize, length)
			data = self.ed.ByteRange(i, i + grabSize)
			fos.write(data)
			i += grabSize
		fos.close()
		self.ed.SetSavePoint()
		self.SetTitle(0)

	# Command handlers are called by menu actions

	def CmdNew(self):
		self.New()

	def CmdOpen(self):
		self.Open()

	def CmdSave(self):
		if (self.fullPath == None) or (len(self.fullPath) == 0):
			self.SaveAs()
		else:
			self.Save()

	def CmdSaveAs(self):
		self.SaveAs()

	def CmdTest(self):
		runner = unittest.TextTestRunner()
		if self.test:
			tests = unittest.defaultTestLoader.loadTestsFromName(self.test)
		else:
			tests = unittest.defaultTestLoader.loadTestsFromName("simpleTests")
		results = runner.run(tests)
		#~ print(results)
		if self.test:
			user32.PostQuitMessage(0)

	def CmdExercised(self):
		print()
		unused = sorted(self.ed.all.difference(self.ed.used))
		print("Unused", len(unused))
		print()
		print("\n".join(unused))
		print()
		print("Used", len(self.ed.used))
		print()
		print("\n".join(sorted(self.ed.used)))

	def Uncalled(self):
		print("")
		unused = sorted(self.ed.all.difference(self.ed.used))
		uu = {}
		for u in unused:
			v = self.ed.getvalue(u)
			if v > 2000:
				uu[v] = u
		#~ for x in sorted(uu.keys())[150:]:
		return uu

	def CmdExit(self):
		self.Exit()

	def CmdUndo(self):
		self.ed.Undo()

	def CmdRedo(self):
		self.ed.Redo()

	def CmdCut(self):
		self.ed.Cut()

	def CmdCopy(self):
		self.ed.Copy()

	def CmdPaste(self):
		self.ed.Paste()

	def CmdDelete(self):
		self.ed.Clear()

xiteFrame = None

def main(test):
	global xiteFrame
	xiteFrame = XiteWin(test)
	xiteFrame.AppLoop()
	#~ xiteFrame.CmdExercised()
	return xiteFrame.Uncalled()
