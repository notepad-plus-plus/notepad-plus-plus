# Build gir and test
# For Ubuntu, these may be needed:
#     apt-get install gobject-introspection
#     apt-get install libgirepository1.0-dev
all: Scintilla-0.1.typelib

ifdef GTK3
GTKVERSION=3.0
else
GTKVERSION=2.0
endif

GI_SCANNER = g-ir-scanner
GI_COMPILER = g-ir-compiler
GTK_LIBS = $(shell pkg-config --libs gtk+-$(GTKVERSION))
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-$(GTKVERSION))
PWD = $(shell pwd)

.PHONY: test clean FORCE

../../bin/scintilla.a: FORCE
	$(MAKE) -C ../../gtk all

Scintilla-filtered.h: ../../include/Scintilla.h
	python filter-scintilla-h.py $< > $@

libscintilla.so: ../../bin/scintilla.a
	$(CXX) -shared -o $@ -Wl,--whole-archive $^ -Wl,--no-whole-archive $(GTK_LIBS)

Scintilla-0.1.gir: libscintilla.so Scintilla-filtered.h
	LDFLAGS=-Wl,-rpath=$(shell pwd) \
		$(GI_SCANNER) --no-libtool --warn-all -i Gtk-$(GTKVERSION) -DG_IR_SCANNING -DGTK \
		--cflags-begin $(GTK_CFLAGS) -include gtk/gtk.h \
		-include Scintilla-filtered.h -I../../include --cflags-end \
		--accept-unprefixed \
		--c-include Scintilla.h --c-include ScintillaWidget.h \
		-n Scintilla --nsversion 0.1 --library scintilla -L$(PWD) \
		../../include/Sci_Position.h ../../include/ScintillaWidget.h Scintilla-filtered.h \
		-o $@

Scintilla-0.1.typelib: Scintilla-0.1.gir
	$(GI_COMPILER) $^ -o $@

clean:
	rm -f libscintilla.so Scintilla-0.1.gir Scintilla-0.1.typelib Scintilla-filtered.h
	$(MAKE) -C ../../gtk clean

test: Scintilla-0.1.gir Scintilla-0.1.typelib
	@echo Verifying Scintilla-0.1.gir file
	@diff $<.good $< || (echo "GIR FILE MISMATCH!"; exit 1)
	@echo Launching gi-test.py python program
	GI_TYPELIB_PATH=$(PWD) LD_LIBRARY_PATH=$(PWD) \
		python $(PWD)/gi-test.py
