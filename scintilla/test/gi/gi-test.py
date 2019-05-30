#!/usr/bin/python
import gi
gi.require_version('Scintilla', '0.1')

# Scintilla is imported before because it loads Gtk with a specified version
# this avoids a warning when Gtk is imported without version such as below (where
# it is imported without because this script works with gtk2 and gtk3)
from gi.repository import Scintilla
from gi.repository import Gtk

def on_notify(sci, id, scn):
    if (scn.nmhdr.code == 2001): # SCN_CHARADDED
        print ("sci-notify: id: %d, char added: %d" % (id, scn.ch))
    elif (scn.nmhdr.code == 2008): # SCN_MODIFIED
        print ("sci-notify: id: %d, pos: %d, mod type: %d" % (id, scn.position, scn.modificationType))
    else:
        print ("sci-notify: id: %d, scn.nmhdr.code: %d" % (id, scn.nmhdr.code))

win = Gtk.Window()
win.connect("delete-event", Gtk.main_quit)
sci = Scintilla.Object()
sci.connect("sci-notify", on_notify)
win.add(sci)
win.show_all()
win.resize(400,300)
Gtk.main()
