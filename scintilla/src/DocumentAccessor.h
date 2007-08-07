// Scintilla source code edit control
/** @file DocumentAccessor.h
 ** Implementation of BufferAccess and StylingAccess on a Scintilla
 ** rapid easy access to contents of a Scintilla.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

class Document;

/**
 */
class DocumentAccessor : public Accessor {
	// Private so DocumentAccessor objects can not be copied
	DocumentAccessor(const DocumentAccessor &source) : Accessor(), props(source.props) {}
	DocumentAccessor &operator=(const DocumentAccessor &) { return *this; }

protected:
	Document *pdoc;
	PropSet &props;
	WindowID id;
	int lenDoc;

	char styleBuf[bufferSize];
	int validLen;
	char chFlags;
	char chWhile;
	unsigned int startSeg;
	int startPosStyling;
	int mask;

	bool InternalIsLeadByte(char ch);
	void Fill(int position);

public:
	DocumentAccessor(Document *pdoc_, PropSet &props_, WindowID id_=0) : 
		Accessor(), pdoc(pdoc_), props(props_), id(id_),
		lenDoc(-1), validLen(0), chFlags(0), chWhile(0), 
		startSeg(0), startPosStyling(0),
		mask(127) { // Initialize the mask to be big enough for any lexer.
	}
	~DocumentAccessor();
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
	WindowID GetWindow() { return id; }

	void StartAt(unsigned int start, char chMask=31);
	void SetFlags(char chFlags_, char chWhile_) {chFlags = chFlags_; chWhile = chWhile_; };
	unsigned int GetStartSegment() { return startSeg; }
	void StartSegment(unsigned int pos);
	void ColourTo(unsigned int pos, int chAttr);
	void SetLevel(int line, int level);
	int IndentAmount(int line, int *flags, PFNIsCommentLeader pfnIsCommentLeader = 0);
};
