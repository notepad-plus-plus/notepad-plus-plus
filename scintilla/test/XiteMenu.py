# -*- coding: utf-8 -*-

from __future__ import unicode_literals

""" Define the menu structure used by the Pentacle applications """

MenuStructure = [
	["&File", [
		["&New", "<control>N"],
		["&Open...", "<control>O"],
		["&Save", "<control>S"],
		["Save &As...", "<control><shift>S"],
		["Test", ""],
		["Exercised", ""],
		["Uncalled", ""],
		["-", ""],
		["&Exit", ""]]],
	[ "&Edit", [
		["&Undo", "<control>Z"],
		["&Redo", "<control>Y"],
		["-", ""],
		["Cu&t", "<control>X"],
		["&Copy", "<control>C"],
		["&Paste", "<control>V"],
		["&Delete", "Del"],
		["Select &All", "<control>A"],
		]],
]
