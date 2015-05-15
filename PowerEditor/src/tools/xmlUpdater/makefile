# this file is part of notepad++
# Copyright (C)2003 Don HO ( donho@altern.org )
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  
.SUFFIXES: .cpp
CPP = g++
CFLAGS = -Wall -Os -DNDEBUG

MAINOBJS = xmlUpdater.o
TINYXMLOBJS = tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o

TINYXMLDIR = ../../TinyXml

OBJS = $(MAINOBJS) $(TINYXMLOBJS)

PROG = xmlUpdater.exe

INCLUDEDIR = $(TINYXMLDIR)
INCLUDEFLAGS = -I$(INCLUDEDIR)
LDFLAGS = 
#-mwindows  -lshlwapi -Os -s

ALL : $(PROG)


$(PROG) : $(OBJS)
	$(CPP) -o $@ $(OBJS) $(LDFLAGS)

xmlUpdater.o : $(INCLUDEDIR)/tinyxml.h
	$(CPP) -c xmlUpdater.cpp -o $@ $(INCLUDEFLAGS)

tinystr.o: $(TINYXMLDIR)/tinystr.h $(TINYXMLDIR)/tinyxml.h 
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinystr.cpp -o $@ $(INCLUDEFLAGS)

tinyxml.o: $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinyxml.cpp -o $@ $(INCLUDEFLAGS)
	
tinyxmlerror.o: $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinyxmlerror.cpp -o $@ $(INCLUDEFLAGS)

tinyxmlparser.o: $(TINYXMLDIR)/tinyxml.h
	$(CPP) $(CFLAGS) -c $(TINYXMLDIR)/tinyxmlparser.cpp -o $@ $(INCLUDEFLAGS)

