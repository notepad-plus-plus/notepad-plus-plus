// Scintilla source code edit control
/** @file Document.h
 ** Text document that handles notifications, DBCS, styling, words and end of line.
 **/
// Copyright 1998-2011 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef DOCUMENT_H
#define DOCUMENT_H

namespace Scintilla::Internal {

class DocWatcher;
class DocModification;
class Document;
class LineMarkers;
class LineLevels;
class LineState;
class LineAnnotation;

enum class EncodingFamily { eightBit, unicode, dbcs };

/**
 * The range class represents a range of text in a document.
 * The two values are not sorted as one end may be more significant than the other
 * as is the case for the selection where the end position is the position of the caret.
 * If either position is invalidPosition then the range is invalid and most operations will fail.
 */
class Range {
public:
	Sci::Position start;
	Sci::Position end;

	explicit Range(Sci::Position pos=0) noexcept :
		start(pos), end(pos) {
	}
	Range(Sci::Position start_, Sci::Position end_) noexcept :
		start(start_), end(end_) {
	}

	bool operator==(const Range &other) const noexcept {
		return (start == other.start) && (end == other.end);
	}

	bool Valid() const noexcept {
		return (start != Sci::invalidPosition) && (end != Sci::invalidPosition);
	}

	Sci::Position First() const noexcept {
		return (start <= end) ? start : end;
	}

	Sci::Position Last() const noexcept {
		return (start > end) ? start : end;
	}

	// Is the position within the range?
	bool Contains(Sci::Position pos) const noexcept {
		if (start < end) {
			return (pos >= start && pos <= end);
		} else {
			return (pos <= start && pos >= end);
		}
	}

	// Is the character after pos within the range?
	bool ContainsCharacter(Sci::Position pos) const noexcept {
		if (start < end) {
			return (pos >= start && pos < end);
		} else {
			return (pos < start && pos >= end);
		}
	}

	bool Contains(Range other) const noexcept {
		return Contains(other.start) && Contains(other.end);
	}

	bool Overlaps(Range other) const noexcept {
		return
		Contains(other.start) ||
		Contains(other.end) ||
		other.Contains(start) ||
		other.Contains(end);
	}
};

/**
 * Interface class for regular expression searching
 */
class RegexSearchBase {
public:
	virtual ~RegexSearchBase() = default;

	virtual Sci::Position FindText(Document *doc, Sci::Position minPos, Sci::Position maxPos, const char *s,
                        bool caseSensitive, bool word, bool wordStart, Scintilla::FindOption flags, Sci::Position *length) = 0;

	///@return String with the substitutions, must remain valid until the next call or destruction
	virtual const char *SubstituteByPosition(Document *doc, const char *text, Sci::Position *length) = 0;
};

/// Factory function for RegexSearchBase
extern RegexSearchBase *CreateRegexSearch(CharClassify *charClassTable);

struct StyledText {
	size_t length;
	const char *text;
	bool multipleStyles;
	size_t style;
	const unsigned char *styles;
	StyledText(size_t length_, const char *text_, bool multipleStyles_, int style_, const unsigned char *styles_) noexcept :
		length(length_), text(text_), multipleStyles(multipleStyles_), style(style_), styles(styles_) {
	}
	// Return number of bytes from start to before '\n' or end of text.
	// Return 1 when start is outside text
	size_t LineLength(size_t start) const noexcept {
		size_t cur = start;
		while ((cur < length) && (text[cur] != '\n'))
			cur++;
		return cur-start;
	}
	size_t StyleAt(size_t i) const noexcept {
		return multipleStyles ? styles[i] : style;
	}
};

class HighlightDelimiter {
public:
	HighlightDelimiter() noexcept : isEnabled(false) {
		Clear();
	}

	void Clear() noexcept {
		beginFoldBlock = -1;
		endFoldBlock = -1;
		firstChangeableLineBefore = -1;
		firstChangeableLineAfter = -1;
	}

	bool NeedsDrawing(Sci::Line line) const noexcept {
		return isEnabled && (line <= firstChangeableLineBefore || line >= firstChangeableLineAfter);
	}

	bool IsFoldBlockHighlighted(Sci::Line line) const noexcept {
		return isEnabled && beginFoldBlock != -1 && beginFoldBlock <= line && line <= endFoldBlock;
	}

	bool IsHeadOfFoldBlock(Sci::Line line) const noexcept {
		return beginFoldBlock == line && line < endFoldBlock;
	}

	bool IsBodyOfFoldBlock(Sci::Line line) const noexcept {
		return beginFoldBlock != -1 && beginFoldBlock < line && line < endFoldBlock;
	}

	bool IsTailOfFoldBlock(Sci::Line line) const noexcept {
		return beginFoldBlock != -1 && beginFoldBlock < line && line == endFoldBlock;
	}

	Sci::Line beginFoldBlock;	// Begin of current fold block
	Sci::Line endFoldBlock;	// End of current fold block
	Sci::Line firstChangeableLineBefore;	// First line that triggers repaint before starting line that determined current fold block
	Sci::Line firstChangeableLineAfter;	// First line that triggers repaint after starting line that determined current fold block
	bool isEnabled;
};

struct LexerReleaser {
	// Called by unique_ptr to destroy/free the Resource
	void operator()(Scintilla::ILexer5 *pLexer) noexcept {
		if (pLexer) {
			try {
				pLexer->Release();
			} catch (...) {
				// ILexer5::Release must not throw, ignore if it does.
			}
		}
	}
};

using LexerInstance = std::unique_ptr<Scintilla::ILexer5, LexerReleaser>;

// LexInterface defines the interface to ILexer used in Document.
// The LexState subclass is actually created and that is used within ScintillaBase
// to provide more methods that are exposed through Scintilla's external API.
class LexInterface {
protected:
	Document *pdoc;
	LexerInstance instance;
	bool performingStyle;	///< Prevent reentrance
public:
	explicit LexInterface(Document *pdoc_) noexcept;
	// Deleted so LexInterface objects can not be copied.
	LexInterface(const LexInterface &) = delete;
	LexInterface(LexInterface &&) = delete;
	LexInterface &operator=(const LexInterface &) = delete;
	LexInterface &operator=(LexInterface &&) = delete;
	virtual ~LexInterface() noexcept;
	void SetInstance(ILexer5 *instance_);
	void Colourise(Sci::Position start, Sci::Position end);
	virtual Scintilla::LineEndType LineEndTypesSupported();
	bool UseContainerLexing() const noexcept;
};

struct RegexError : public std::runtime_error {
	RegexError() : std::runtime_error("regex failure") {}
};

/**
 * The ActionDuration class stores the average time taken for some action such as styling or
 * wrapping a line. It is used to decide how many repetitions of that action can be performed
 * on idle to maximize efficiency without affecting application responsiveness.
 * The duration changes if the time for the action changes. For example, if a simple lexer is
 * changed to a complex lexer. Changes are damped and clamped to avoid short periods of easy
 * or difficult processing moving the value too far leading to inefficiency or poor user
 * experience.
 */

class ActionDuration {
	double duration;
	const double minDuration;
	const double maxDuration;
public:
	ActionDuration(double duration_, double minDuration_, double maxDuration_) noexcept;
	void AddSample(size_t numberActions, double durationOfActions) noexcept;
	double Duration() const noexcept;
	size_t ActionsInAllowedTime(double secondsAllowed) const noexcept;
};

/**
 */
class Document : PerLine, public Scintilla::IDocument, public Scintilla::ILoader {

public:
	/** Used to pair watcher pointer with user data. */
	struct WatcherWithUserData {
		DocWatcher *watcher;
		void *userData;
		WatcherWithUserData(DocWatcher *watcher_=nullptr, void *userData_=nullptr) noexcept :
			watcher(watcher_), userData(userData_) {
		}
		bool operator==(const WatcherWithUserData &other) const noexcept {
			return (watcher == other.watcher) && (userData == other.userData);
		}
	};

private:
	int refCount;
	CellBuffer cb;
	CharClassify charClass;
	CharacterCategoryMap charMap;
	std::unique_ptr<CaseFolder> pcf;
	Sci::Position endStyled;
	int styleClock;
	int enteredModification;
	int enteredStyling;
	int enteredReadOnlyCount;

	bool insertionSet;
	std::string insertion;

	std::vector<WatcherWithUserData> watchers;

	// ldSize is not real data - it is for dimensions and loops
	enum lineData { ldMarkers, ldLevels, ldState, ldMargin, ldAnnotation, ldEOLAnnotation, ldSize };
	std::unique_ptr<PerLine> perLineData[ldSize];
	LineMarkers *Markers() const noexcept;
	LineLevels *Levels() const noexcept;
	LineState *States() const noexcept;
	LineAnnotation *Margins() const noexcept;
	LineAnnotation *Annotations() const noexcept;
	LineAnnotation *EOLAnnotations() const noexcept;

	bool matchesValid;
	std::unique_ptr<RegexSearchBase> regex;
	std::unique_ptr<LexInterface> pli;

public:

	struct CharacterExtracted {
		unsigned int character;
		unsigned int widthBytes;
		CharacterExtracted(unsigned int character_, unsigned int widthBytes_) noexcept :
			character(character_), widthBytes(widthBytes_) {
		}
		// For DBCS characters turn 2 bytes into an int
		static CharacterExtracted DBCS(unsigned char lead, unsigned char trail) noexcept {
			return CharacterExtracted((lead << 8) | trail, 2);
		}
	};

	Scintilla::EndOfLine eolMode;
	/// Can also be SC_CP_UTF8 to enable UTF-8 mode
	int dbcsCodePage;
	Scintilla::LineEndType lineEndBitSet;
	int tabInChars;
	int indentInChars;
	int actualIndentInChars;
	bool useTabs;
	bool tabIndents;
	bool backspaceUnindents;
	ActionDuration durationStyleOneByte;

	std::unique_ptr<IDecorationList> decorations;

	Document(Scintilla::DocumentOption options);
	// Deleted so Document objects can not be copied.
	Document(const Document &) = delete;
	Document(Document &&) = delete;
	void operator=(const Document &) = delete;
	Document &operator=(Document &&) = delete;
	~Document() override;

	int AddRef();
	int SCI_METHOD Release() override;

	// From PerLine
	void Init() override;
	void InsertLine(Sci::Line line) override;
	void InsertLines(Sci::Line line, Sci::Line lines) override;
	void RemoveLine(Sci::Line line) override;

	Scintilla::LineEndType LineEndTypesSupported() const;
	bool SetDBCSCodePage(int dbcsCodePage_);
	Scintilla::LineEndType GetLineEndTypesAllowed() const noexcept { return cb.GetLineEndTypes(); }
	bool SetLineEndTypesAllowed(Scintilla::LineEndType lineEndBitSet_);
	Scintilla::LineEndType GetLineEndTypesActive() const noexcept { return cb.GetLineEndTypes(); }

	int SCI_METHOD Version() const override {
		return Scintilla::dvRelease4;
	}

	void SCI_METHOD SetErrorStatus(int status) override;

	Sci_Position SCI_METHOD LineFromPosition(Sci_Position pos) const override;
	Sci::Line SciLineFromPosition(Sci::Position pos) const noexcept;	// Avoids casting LineFromPosition
	Sci::Position ClampPositionIntoDocument(Sci::Position pos) const noexcept;
	bool ContainsLineEnd(const char *s, Sci::Position length) const noexcept { return cb.ContainsLineEnd(s, length); }
	bool IsCrLf(Sci::Position pos) const noexcept;
	int LenChar(Sci::Position pos) const noexcept;
	bool InGoodUTF8(Sci::Position pos, Sci::Position &start, Sci::Position &end) const noexcept;
	Sci::Position MovePositionOutsideChar(Sci::Position pos, Sci::Position moveDir, bool checkLineEnd=true) const noexcept;
	Sci::Position NextPosition(Sci::Position pos, int moveDir) const noexcept;
	bool NextCharacter(Sci::Position &pos, int moveDir) const noexcept;	// Returns true if pos changed
	Document::CharacterExtracted CharacterAfter(Sci::Position position) const noexcept;
	Document::CharacterExtracted CharacterBefore(Sci::Position position) const noexcept;
	Sci_Position SCI_METHOD GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const override;
	Sci::Position GetRelativePositionUTF16(Sci::Position positionStart, Sci::Position characterOffset) const noexcept;
	int SCI_METHOD GetCharacterAndWidth(Sci_Position position, Sci_Position *pWidth) const override;
	int SCI_METHOD CodePage() const override;
	bool SCI_METHOD IsDBCSLeadByte(char ch) const override;
	bool IsDBCSLeadByteNoExcept(char ch) const noexcept;
	bool IsDBCSTrailByteNoExcept(char ch) const noexcept;
	int DBCSDrawBytes(std::string_view text) const noexcept;
	bool IsDBCSDualByteAt(Sci::Position pos) const noexcept;
	size_t SafeSegment(std::string_view text) const noexcept;
	EncodingFamily CodePageFamily() const noexcept;

	// Gateways to modifying document
	void ModifiedAt(Sci::Position pos) noexcept;
	void CheckReadOnly();
	bool DeleteChars(Sci::Position pos, Sci::Position len);
	Sci::Position InsertString(Sci::Position position, const char *s, Sci::Position insertLength);
	void ChangeInsertion(const char *s, Sci::Position length);
	int SCI_METHOD AddData(const char *data, Sci_Position length) override;
	void * SCI_METHOD ConvertToDocument() override;
	Sci::Position Undo();
	Sci::Position Redo();
	bool CanUndo() const noexcept { return cb.CanUndo(); }
	bool CanRedo() const noexcept { return cb.CanRedo(); }
	void DeleteUndoHistory() { cb.DeleteUndoHistory(); }
	bool SetUndoCollection(bool collectUndo) {
		return cb.SetUndoCollection(collectUndo);
	}
	bool IsCollectingUndo() const noexcept { return cb.IsCollectingUndo(); }
	void BeginUndoAction() { cb.BeginUndoAction(); }
	void EndUndoAction() { cb.EndUndoAction(); }
	void AddUndoAction(Sci::Position token, bool mayCoalesce) { cb.AddUndoAction(token, mayCoalesce); }
	void SetSavePoint();
	bool IsSavePoint() const noexcept { return cb.IsSavePoint(); }

	void TentativeStart() { cb.TentativeStart(); }
	void TentativeCommit() { cb.TentativeCommit(); }
	void TentativeUndo();
	bool TentativeActive() const noexcept { return cb.TentativeActive(); }

	const char * SCI_METHOD BufferPointer() override { return cb.BufferPointer(); }
	const char *RangePointer(Sci::Position position, Sci::Position rangeLength) noexcept { return cb.RangePointer(position, rangeLength); }
	Sci::Position GapPosition() const noexcept { return cb.GapPosition(); }

	int SCI_METHOD GetLineIndentation(Sci_Position line) override;
	Sci::Position SetLineIndentation(Sci::Line line, Sci::Position indent);
	Sci::Position GetLineIndentPosition(Sci::Line line) const;
	Sci::Position GetColumn(Sci::Position pos);
	Sci::Position CountCharacters(Sci::Position startPos, Sci::Position endPos) const noexcept;
	Sci::Position CountUTF16(Sci::Position startPos, Sci::Position endPos) const noexcept;
	Sci::Position FindColumn(Sci::Line line, Sci::Position column);
	void Indent(bool forwards, Sci::Line lineBottom, Sci::Line lineTop);
	static std::string TransformLineEnds(const char *s, size_t len, Scintilla::EndOfLine eolModeWanted);
	void ConvertLineEnds(Scintilla::EndOfLine eolModeSet);
	void SetReadOnly(bool set) { cb.SetReadOnly(set); }
	bool IsReadOnly() const noexcept { return cb.IsReadOnly(); }
	bool IsLarge() const noexcept { return cb.IsLarge(); }
	Scintilla::DocumentOption Options() const noexcept;

	void DelChar(Sci::Position pos);
	void DelCharBack(Sci::Position pos);

	char CharAt(Sci::Position position) const noexcept { return cb.CharAt(position); }
	void SCI_METHOD GetCharRange(char *buffer, Sci_Position position, Sci_Position lengthRetrieve) const override {
		cb.GetCharRange(buffer, position, lengthRetrieve);
	}
	char SCI_METHOD StyleAt(Sci_Position position) const override { return cb.StyleAt(position); }
	int StyleIndexAt(Sci_Position position) const noexcept { return static_cast<unsigned char>(cb.StyleAt(position)); }
	void GetStyleRange(unsigned char *buffer, Sci::Position position, Sci::Position lengthRetrieve) const {
		cb.GetStyleRange(buffer, position, lengthRetrieve);
	}
	int GetMark(Sci::Line line) const noexcept;
	Sci::Line MarkerNext(Sci::Line lineStart, int mask) const noexcept;
	int AddMark(Sci::Line line, int markerNum);
	void AddMarkSet(Sci::Line line, int valueSet);
	void DeleteMark(Sci::Line line, int markerNum);
	void DeleteMarkFromHandle(int markerHandle);
	void DeleteAllMarks(int markerNum);
	Sci::Line LineFromHandle(int markerHandle) const noexcept;
	int MarkerNumberFromLine(Sci::Line line, int which) const noexcept;
	int MarkerHandleFromLine(Sci::Line line, int which) const noexcept;
	Sci_Position SCI_METHOD LineStart(Sci_Position line) const override;
	bool IsLineStartPosition(Sci::Position position) const;
	Sci_Position SCI_METHOD LineEnd(Sci_Position line) const override;
	Sci::Position LineEndPosition(Sci::Position position) const;
	bool IsLineEndPosition(Sci::Position position) const;
	bool IsPositionInLineEnd(Sci::Position position) const;
	Sci::Position VCHomePosition(Sci::Position position) const;
	Sci::Position IndexLineStart(Sci::Line line, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept;
	Sci::Line LineFromPositionIndex(Sci::Position pos, Scintilla::LineCharacterIndexType lineCharacterIndex) const noexcept;
	Sci::Line LineFromPositionAfter(Sci::Line line, Sci::Position length) const noexcept;

	int SCI_METHOD SetLevel(Sci_Position line, int level) override;
	int SCI_METHOD GetLevel(Sci_Position line) const override;
	Scintilla::FoldLevel GetFoldLevel(Sci_Position line) const;
	void ClearLevels();
	Sci::Line GetLastChild(Sci::Line lineParent, std::optional<Scintilla::FoldLevel> level = {}, Sci::Line lastLine = -1);
	Sci::Line GetFoldParent(Sci::Line line) const;
	void GetHighlightDelimiters(HighlightDelimiter &highlightDelimiter, Sci::Line line, Sci::Line lastLine);

	Sci::Position ExtendWordSelect(Sci::Position pos, int delta, bool onlyWordCharacters=false) const;
	Sci::Position NextWordStart(Sci::Position pos, int delta) const;
	Sci::Position NextWordEnd(Sci::Position pos, int delta) const;
	Sci_Position SCI_METHOD Length() const override { return cb.Length(); }
	Sci::Position LengthNoExcept() const noexcept { return cb.Length(); }
	void Allocate(Sci::Position newSize) { cb.Allocate(newSize); }

	CharacterExtracted ExtractCharacter(Sci::Position position) const noexcept;

	bool IsWordStartAt(Sci::Position pos) const;
	bool IsWordEndAt(Sci::Position pos) const;
	bool IsWordAt(Sci::Position start, Sci::Position end) const;

	bool MatchesWordOptions(bool word, bool wordStart, Sci::Position pos, Sci::Position length) const;
	bool HasCaseFolder() const noexcept;
	void SetCaseFolder(std::unique_ptr<CaseFolder> pcf_) noexcept;
	Sci::Position FindText(Sci::Position minPos, Sci::Position maxPos, const char *search, Scintilla::FindOption flags, Sci::Position *length);
	const char *SubstituteByPosition(const char *text, Sci::Position *length);
	Scintilla::LineCharacterIndexType LineCharacterIndex() const noexcept;
	void AllocateLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex);
	void ReleaseLineCharacterIndex(Scintilla::LineCharacterIndexType lineCharacterIndex);
	Sci::Line LinesTotal() const noexcept;
	void AllocateLines(Sci::Line lines);

	void SetDefaultCharClasses(bool includeWordClass);
	void SetCharClasses(const unsigned char *chars, CharacterClass newCharClass);
	int GetCharsOfClass(CharacterClass characterClass, unsigned char *buffer) const;
	void SetCharacterCategoryOptimization(int countCharacters);
	int CharacterCategoryOptimization() const noexcept;
	void SCI_METHOD StartStyling(Sci_Position position) override;
	bool SCI_METHOD SetStyleFor(Sci_Position length, char style) override;
	bool SCI_METHOD SetStyles(Sci_Position length, const char *styles) override;
	Sci::Position GetEndStyled() const noexcept { return endStyled; }
	void EnsureStyledTo(Sci::Position pos);
	void StyleToAdjustingLineDuration(Sci::Position pos);
	void LexerChanged();
	int GetStyleClock() const noexcept { return styleClock; }
	void IncrementStyleClock() noexcept;
	void SCI_METHOD DecorationSetCurrentIndicator(int indicator) override;
	void SCI_METHOD DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength) override;
	LexInterface *GetLexInterface() const noexcept;
	void SetLexInterface(std::unique_ptr<LexInterface> pLexInterface) noexcept;

	int SCI_METHOD SetLineState(Sci_Position line, int state) override;
	int SCI_METHOD GetLineState(Sci_Position line) const override;
	Sci::Line GetMaxLineState() const noexcept;
	void SCI_METHOD ChangeLexerState(Sci_Position start, Sci_Position end) override;

	StyledText MarginStyledText(Sci::Line line) const noexcept;
	void MarginSetStyle(Sci::Line line, int style);
	void MarginSetStyles(Sci::Line line, const unsigned char *styles);
	void MarginSetText(Sci::Line line, const char *text);
	void MarginClearAll();

	StyledText AnnotationStyledText(Sci::Line line) const noexcept;
	void AnnotationSetText(Sci::Line line, const char *text);
	void AnnotationSetStyle(Sci::Line line, int style);
	void AnnotationSetStyles(Sci::Line line, const unsigned char *styles);
	int AnnotationLines(Sci::Line line) const noexcept;
	void AnnotationClearAll();

	StyledText EOLAnnotationStyledText(Sci::Line line) const noexcept;
	void EOLAnnotationSetStyle(Sci::Line line, int style);
	void EOLAnnotationSetText(Sci::Line line, const char *text);
	void EOLAnnotationClearAll();

	bool AddWatcher(DocWatcher *watcher, void *userData);
	bool RemoveWatcher(DocWatcher *watcher, void *userData) noexcept;

	CharacterClass WordCharacterClass(unsigned int ch) const;
	bool IsWordPartSeparator(unsigned int ch) const;
	Sci::Position WordPartLeft(Sci::Position pos) const;
	Sci::Position WordPartRight(Sci::Position pos) const;
	Sci::Position ExtendStyleRange(Sci::Position pos, int delta, bool singleLine) noexcept;
	bool IsWhiteLine(Sci::Line line) const;
	Sci::Position ParaUp(Sci::Position pos) const;
	Sci::Position ParaDown(Sci::Position pos) const;
	int IndentSize() const noexcept { return actualIndentInChars; }
	Sci::Position BraceMatch(Sci::Position position, Sci::Position maxReStyle, Sci::Position startPos, bool useStartPos) noexcept;

private:
	void NotifyModifyAttempt();
	void NotifySavePoint(bool atSavePoint);
	void NotifyModified(DocModification mh);
};

class UndoGroup {
	Document *pdoc;
	bool groupNeeded;
public:
	UndoGroup(Document *pdoc_, bool groupNeeded_=true) :
		pdoc(pdoc_), groupNeeded(groupNeeded_) {
		if (groupNeeded) {
			pdoc->BeginUndoAction();
		}
	}
	// Deleted so UndoGroup objects can not be copied.
	UndoGroup(const UndoGroup &) = delete;
	UndoGroup(UndoGroup &&) = delete;
	void operator=(const UndoGroup &) = delete;
	UndoGroup &operator=(UndoGroup &&) = delete;
	~UndoGroup() {
		if (groupNeeded) {
			// EndUndoAction can throw as it allocates but throw in destructor is fatal.
			// To fix this UndoHistory should allocate any memory needed by EndUndoAction
			// beforehand or change EndUndoAction to not require allocation.
			pdoc->EndUndoAction();
		}
	}
	bool Needed() const noexcept {
		return groupNeeded;
	}
};


/**
 * To optimise processing of document modifications by DocWatchers, a hint is passed indicating the
 * scope of the change.
 * If the DocWatcher is a document view then this can be used to optimise screen updating.
 */
class DocModification {
public:
	Scintilla::ModificationFlags modificationType;
	Sci::Position position;
	Sci::Position length;
	Sci::Line linesAdded;	/**< Negative if lines deleted. */
	const char *text;	/**< Only valid for changes to text, not for changes to style. */
	Sci::Line line;
	Scintilla::FoldLevel foldLevelNow;
	Scintilla::FoldLevel foldLevelPrev;
	Sci::Line annotationLinesAdded;
	Sci::Position token;

	DocModification(Scintilla::ModificationFlags modificationType_, Sci::Position position_=0, Sci::Position length_=0,
		Sci::Line linesAdded_=0, const char *text_=nullptr, Sci::Line line_=0) noexcept :
		modificationType(modificationType_),
		position(position_),
		length(length_),
		linesAdded(linesAdded_),
		text(text_),
		line(line_),
		foldLevelNow(Scintilla::FoldLevel::None),
		foldLevelPrev(Scintilla::FoldLevel::None),
		annotationLinesAdded(0),
		token(0) {}

	DocModification(Scintilla::ModificationFlags modificationType_, const Action &act, Sci::Line linesAdded_=0) noexcept :
		modificationType(modificationType_),
		position(act.position),
		length(act.lenData),
		linesAdded(linesAdded_),
		text(act.data.get()),
		line(0),
		foldLevelNow(Scintilla::FoldLevel::None),
		foldLevelPrev(Scintilla::FoldLevel::None),
		annotationLinesAdded(0),
		token(0) {}
};

/**
 * A class that wants to receive notifications from a Document must be derived from DocWatcher
 * and implement the notification methods. It can then be added to the watcher list with AddWatcher.
 */
class DocWatcher {
public:
	virtual ~DocWatcher() {}

	virtual void NotifyModifyAttempt(Document *doc, void *userData) = 0;
	virtual void NotifySavePoint(Document *doc, void *userData, bool atSavePoint) = 0;
	virtual void NotifyModified(Document *doc, DocModification mh, void *userData) = 0;
	virtual void NotifyDeleted(Document *doc, void *userData) noexcept = 0;
	virtual void NotifyStyleNeeded(Document *doc, void *userData, Sci::Position endPos) = 0;
	virtual void NotifyLexerChanged(Document *doc, void *userData) = 0;
	virtual void NotifyErrorOccurred(Document *doc, void *userData, Scintilla::Status status) = 0;
};

}

#endif
