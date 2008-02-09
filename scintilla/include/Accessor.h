// Scintilla source code edit control
/** @file Accessor.h
 ** Rapid easy access to contents of a Scintilla.
 **/
// Copyright 1998-2001 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

enum { wsSpace = 1, wsTab = 2, wsSpaceTab = 4, wsInconsistent=8};

class Accessor;

typedef bool (*PFNIsCommentLeader)(Accessor &styler, int pos, int len);

/**
 * Interface to data in a Scintilla.
 */
class Accessor {
protected:
	enum {extremePosition=0x7FFFFFFF};
	/** @a bufferSize is a trade off between time taken to copy the characters
	 * and retrieval overhead.
	 * @a slopSize positions the buffer before the desired position
	 * in case there is some backtracking. */
	enum {bufferSize=4000, slopSize=bufferSize/8};
	char buf[bufferSize+1];
	int startPos;
	int endPos;
	int codePage;	

	virtual bool InternalIsLeadByte(char ch)=0;
	virtual void Fill(int position)=0;

public:
	Accessor() : startPos(extremePosition), endPos(0), codePage(0) {}
	virtual ~Accessor() {}
	char operator[](int position) {
		if (position < startPos || position >= endPos) {
			Fill(position);
		}
		return buf[position - startPos];
	}
	/** Safe version of operator[], returning a defined value for invalid position. */
	char SafeGetCharAt(int position, char chDefault=' ') {
		if (position < startPos || position >= endPos) {
			Fill(position);
			if (position < startPos || position >= endPos) {
				// Position is outside range of document 
				return chDefault;
			}
		}
		return buf[position - startPos];
	}
	bool IsLeadByte(char ch) {
		return codePage && InternalIsLeadByte(ch);
	}
	void SetCodePage(int codePage_) { codePage = codePage_; }

	virtual bool Match(int pos, const char *s)=0;
	virtual char StyleAt(int position)=0;
	virtual int GetLine(int position)=0;
	virtual int LineStart(int line)=0;
	virtual int LevelAt(int line)=0;
	virtual int Length()=0;
	virtual void Flush()=0;
	virtual int GetLineState(int line)=0;
	virtual int SetLineState(int line, int state)=0;
	virtual int GetPropertyInt(const char *key, int defaultValue=0)=0;
	virtual char *GetProperties()=0;

	// Style setting
	virtual void StartAt(unsigned int start, char chMask=31)=0;
	virtual void SetFlags(char chFlags_, char chWhile_)=0;
	virtual unsigned int GetStartSegment()=0;
	virtual void StartSegment(unsigned int pos)=0;
	virtual void ColourTo(unsigned int pos, int chAttr)=0;
	virtual void SetLevel(int line, int level)=0;
	virtual int IndentAmount(int line, int *flags, PFNIsCommentLeader pfnIsCommentLeader = 0)=0;
	virtual void IndicatorFill(int start, int end, int indicator, int value)=0;
};
