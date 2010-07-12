// Scintilla source code edit control
/** @file WindowAccessor.h
 ** Implementation of BufferAccess and StylingAccess on a Scintilla
 ** rapid easy access to contents of a Scintilla.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifdef SCI_NAMESPACE
namespace Scintilla {
#endif

/**
 */

class WindowAccessor : public Accessor {
	// Private so WindowAccessor objects can not be copied
	WindowAccessor(const WindowAccessor &source) : Accessor(), props(source.props) {}
	WindowAccessor &operator=(const WindowAccessor &) { return *this; }
protected:
	WindowID id;
	PropertyGet &props;
	int lenDoc;

	char styleBuf[bufferSize];
	int validLen;
	char chFlags;
	char chWhile;
	unsigned int startSeg;

	bool InternalIsLeadByte(char ch);
	void Fill(int position);
public:
	WindowAccessor(WindowID id_, PropertyGet &props_) : 
		Accessor(), id(id_), props(props_), 
		lenDoc(-1), validLen(0), chFlags(0), chWhile(0) {
	}
	~WindowAccessor();
	bool Match(int pos, const char *s);
	char StyleAt(int position);
	int GetLine(int position);
	int LineStart(int line);
	int LevelAt(int line);
	int Length();
	void Flush();
	int GetLineState(int line);
	int SetLineState(int line, int state);
	int GetPropertyInt(const char *key, int defaultValue=0) { 
		return props.GetInt(key, defaultValue); 
	}
	char *GetProperties() {
		return props.ToString();
	}

	void StartAt(unsigned int start, char chMask=31);
	void SetFlags(char chFlags_, char chWhile_) {chFlags = chFlags_; chWhile = chWhile_; }
	unsigned int GetStartSegment() { return startSeg; }
	void StartSegment(unsigned int pos);
	void ColourTo(unsigned int pos, int chAttr);
	void SetLevel(int line, int level);
	int IndentAmount(int line, int *flags, PFNIsCommentLeader pfnIsCommentLeader = 0);
	void IndicatorFill(int start, int end, int indicator, int value);
};

#ifdef SCI_NAMESPACE
}
#endif
