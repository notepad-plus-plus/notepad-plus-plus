#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Requires Python 3.6 or later

from __future__ import with_statement
from __future__ import unicode_literals

import os, platform, sys, unittest

import ctypes
from ctypes import wintypes
from ctypes import c_int, c_ulong, c_char_p, c_wchar_p, c_ushort, c_uint, c_long
from ctypes.wintypes import HWND, WPARAM, LPARAM, HANDLE, HBRUSH, LPCWSTR
user32=ctypes.windll.user32
gdi32=ctypes.windll.gdi32
kernel32=ctypes.windll.kernel32
from MessageNumbers import msgs, sgsm

import ScintillaCallable
import XiteMenu

scintillaIncludesLexers = False
# Lexilla may optionally be tested it is built and can be loaded
lexillaAvailable = False

scintillaDirectory = ".."
scintillaIncludeDirectory = os.path.join(scintillaDirectory, "include")
scintillaScriptsDirectory = os.path.join(scintillaDirectory, "scripts")
sys.path.append(scintillaScriptsDirectory)
import Face

scintillaBinDirectory = os.path.join(scintillaDirectory, "bin")

lexillaDirectory = os.path.join(scintillaDirectory, "..", "lexilla")
lexillaBinDirectory = os.path.join(lexillaDirectory, "bin")
lexillaIncludeDirectory = os.path.join(lexillaDirectory, "include")

lexName = "Lexilla.DLL"
try:
	lexillaDLLPath = os.path.join(lexillaBinDirectory, lexName)
	lexillaLibrary = ctypes.cdll.LoadLibrary(lexillaDLLPath)
	createLexer = lexillaLibrary.CreateLexer
	createLexer.restype = ctypes.c_void_p
	lexillaAvailable = True
	print("Found Lexilla")
except OSError:
	print("Can't find " + lexName)
	print("Python is built for " + " ".join(platform.architecture()))

WFUNC = ctypes.WINFUNCTYPE(c_int, HWND, c_uint, WPARAM, LPARAM)

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
		('hInst', HANDLE),
		('hIcon', HANDLE),
		('hCursor', HANDLE),
		('hbrBackground', HBRUSH),
		('menu_name', LPCWSTR),
		('lpzClassName', LPCWSTR),
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
	wc.menu_name = None
	wc.lpzClassName = name
	user32.RegisterClassW(ctypes.byref(wc))


class XiteWin():
	def __init__(self, test=""):
		self.face = Face.Face()
		self.face.ReadFromFile(os.path.join(scintillaIncludeDirectory, "Scintilla.iface"))
		try:
			faceLex = Face.Face()
			faceLex.ReadFromFile(os.path.join(lexillaIncludeDirectory, "LexicalStyles.iface"))
			self.face.features.update(faceLex.features)
		except FileNotFoundError:
			print("Can't find " + "LexicalStyles.iface")

		self.titleDirty = True
		self.fullPath = ""
		self.test = test

		self.appName = "xite"

		self.large = "-large" in sys.argv

		self.cmds = {}
		self.windowName = "XiteWindow"
		self.wfunc = WFUNC(self.WndProc)
		RegisterClass(self.windowName, self.wfunc)
		user32.CreateWindowExW(0, self.windowName, self.appName, \
			WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, \
			0, 0, 500, 700, 0, 0, hinst, 0)

		args = [a for a in sys.argv[1:] if not a.startswith("-")]
		self.SetMenus()
		if args:
			self.GrabFile(args[0])
			self.FocusOnEditor()
			self.ed.GotoPos(self.ed.Length)

		if self.test:
			print(self.test)
			for k in self.cmds:
				if self.cmds[k] == "Test":
					user32.PostMessageW(self.win, msgs["WM_COMMAND"], k, 0)

	def FocusOnEditor(self):
		user32.SetFocus(self.sciHwnd)

	def OnSize(self):
		width, height = WindowSize(self.win)
		user32.SetWindowPos(self.sciHwnd, 0, 0, 0, width, height, 0)
		user32.InvalidateRect(self.win, 0, 0)

	def OnCreate(self, hwnd):
		self.win = hwnd
		if scintillaIncludesLexers:
			sciName = "SciLexer.DLL"
		else:
			sciName = "Scintilla.DLL"
		try:
			scintillaDLLPath = os.path.join(scintillaBinDirectory, sciName)
			ctypes.cdll.LoadLibrary(scintillaDLLPath)
		except OSError:
			print("Can't find " + sciName)
			print("Python is built for " + " ".join(platform.architecture()))
			sys.exit()
		self.sciHwnd = user32.CreateWindowExW(0,
			"Scintilla", "Source",
			WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
			0, 0, 100, 100, self.win, 0, hinst, 0)
		user32.ShowWindow(self.sciHwnd, SW_SHOW)
		user32.SendMessageW.restype = WPARAM
		scifn = user32.SendMessageW(self.sciHwnd,
			int(self.face.features["GetDirectFunction"]["Value"], 0), 0,0)
		sciptr = c_char_p(user32.SendMessageW(self.sciHwnd,
			int(self.face.features["GetDirectPointer"]["Value"], 0), 0,0))
		self.ed = ScintillaCallable.ScintillaCallable(self.face, scifn, sciptr)
		if self.large:
			doc = self.ed.CreateDocument(10, 0x100)
			self.ed.SetDocPointer(0, doc)

		self.FocusOnEditor()

	def ChooseLexer(self, lexer):
		if scintillaIncludesLexers:
			self.ed.LexerLanguage = lexer
		elif lexillaAvailable:
			pLexilla = createLexer(lexer)
			self.ed.SetILexer(0, pLexilla)
		else:	# No lexers available
			pass

	def Invalidate(self):
		user32.InvalidateRect(self.win, 0, 0)

	def WndProc(self, h, m, w, l):
		user32.DefWindowProcW.argtypes = [HWND, c_uint, WPARAM, LPARAM]
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
				self.FocusOnEditor()
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
		opath = ctypes.create_unicode_buffer(1024)
		ofx.lpstrFile = ctypes.addressof(opath)
		filters = ["Python (.py;.pyw)|*.py;*.pyw|All|*.*"]
		filterText = "\0".join([f.replace("|", "\0") for f in filters])+"\0\0"
		ofx.lpstrFilter = filterText
		if ctypes.windll.comdlg32.GetOpenFileNameW(ctypes.byref(ofx)):
			absPath = opath.value.replace("\0", "")
			self.GrabFile(absPath)
			self.FocusOnEditor()
			self.ed.LexerLanguage = b"python"
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
			self.FocusOnEditor()

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
