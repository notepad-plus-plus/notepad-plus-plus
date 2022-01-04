#!/usr/bin/env python3
# ScintillaData.py - implemented 2013 by Neil Hodgson neilh@scintilla.org
# Released to the public domain.

# Common code used by Scintilla and SciTE for source file regeneration.
# The ScintillaData object exposes information about Scintilla as properties:
# Version properties
#     version
#     versionDotted
#     versionCommad
#
# Date last modified
#     dateModified
#     yearModified
#     mdyModified
#     dmyModified
#     myModified

# This file can be run to see the data it provides.
# Requires Python 3.6 or later

import datetime, pathlib, sys

def FindCredits(historyFile, removeLinks=True):
    credits = []
    stage = 0
    with historyFile.open(encoding="utf-8") as f:
        for l in f.readlines():
            l = l.strip()
            if stage == 0 and l == "<table>":
                stage = 1
            elif stage == 1 and l == "</table>":
                stage = 2
            if stage == 1 and l.startswith("<td>"):
                credit = l[4:-5]
                if removeLinks and "<a" in l:
                    title, _a, rest = credit.partition("<a href=")
                    urlplus, _bracket, end = rest.partition(">")
                    name = end.split("<")[0]
                    url = urlplus[1:-1]
                    credit = title.strip()
                    if credit:
                        credit += " "
                    credit += name + " " + url
                credits.append(credit)
    return credits

class ScintillaData:
    def __init__(self, scintillaRoot):
        # Discover version information
        self.version = (scintillaRoot / "version.txt").read_text().strip()
        self.versionDotted = self.version[0] + '.' + self.version[1] + '.' + \
            self.version[2]
        self.versionCommad = self.versionDotted.replace(".", ", ") + ', 0'

        with (scintillaRoot / "doc" / "index.html").open() as f:
            self.dateModified = [l for l in f.readlines() if "Date.Modified" in l]\
                [0].split('\"')[3]
            # 20130602
            # index.html, SciTE.html
            dtModified = datetime.datetime.strptime(self.dateModified, "%Y%m%d")
            self.yearModified = self.dateModified[0:4]
            monthModified = dtModified.strftime("%B")
            dayModified = "%d" % dtModified.day
            self.mdyModified = monthModified + " " + dayModified + " " + self.yearModified
            # May 22 2013
            # index.html, SciTE.html
            self.dmyModified = dayModified + " " + monthModified + " " + self.yearModified
            # 22 May 2013
            # ScintillaHistory.html -- only first should change
            self.myModified = monthModified + " " + self.yearModified

        self.credits = FindCredits(scintillaRoot / "doc" / "ScintillaHistory.html")

if __name__=="__main__":
    sci = ScintillaData(pathlib.Path(__file__).resolve().parent.parent)
    print("Version   %s   %s   %s" % (sci.version, sci.versionDotted, sci.versionCommad))
    print("Date last modified    %s   %s   %s   %s   %s" % (
        sci.dateModified, sci.yearModified, sci.mdyModified, sci.dmyModified, sci.myModified))
    print("Credits:")
    for c in sci.credits:
        sys.stdout.buffer.write(b"    " + c.encode("utf-8") + b"\n")
