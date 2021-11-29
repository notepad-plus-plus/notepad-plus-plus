// Scintilla source code edit control
/** @file TestDocument.h
 ** Lexer testing.
 **/
// Copyright 2019 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

class TestDocument : public Scintilla::IDocument {
	std::string text;
	std::string textStyles;
	std::vector<Sci_Position> lineStarts;
	std::vector<int> lineStates;
	Sci_Position endStyled=0;
public:
	void Set(std::string_view sv);
	virtual ~TestDocument() = default;

	int SCI_METHOD Version() const override;
	void SCI_METHOD SetErrorStatus(int status) override;
	Sci_Position SCI_METHOD Length() const override;
	void SCI_METHOD GetCharRange(char *buffer, Sci_Position position, Sci_Position lengthRetrieve) const override;
	char SCI_METHOD StyleAt(Sci_Position position) const override;
	Sci_Position SCI_METHOD LineFromPosition(Sci_Position position) const override;
	Sci_Position SCI_METHOD LineStart(Sci_Position line) const override;
	int SCI_METHOD GetLevel(Sci_Position line) const override;
	int SCI_METHOD SetLevel(Sci_Position line, int level) override;
	int SCI_METHOD GetLineState(Sci_Position line) const override;
	int SCI_METHOD SetLineState(Sci_Position line, int state) override;
	void SCI_METHOD StartStyling(Sci_Position position) override;
	bool SCI_METHOD SetStyleFor(Sci_Position length, char style) override;
	bool SCI_METHOD SetStyles(Sci_Position length, const char *styles) override;
	void SCI_METHOD DecorationSetCurrentIndicator(int indicator) override;
	void SCI_METHOD DecorationFillRange(Sci_Position position, int value, Sci_Position fillLength) override;
	void SCI_METHOD ChangeLexerState(Sci_Position start, Sci_Position end) override;
	int SCI_METHOD CodePage() const override;
	bool SCI_METHOD IsDBCSLeadByte(char ch) const override;
	const char *SCI_METHOD BufferPointer() override;
	int SCI_METHOD GetLineIndentation(Sci_Position line) override;
	Sci_Position SCI_METHOD LineEnd(Sci_Position line) const override;
	Sci_Position SCI_METHOD GetRelativePosition(Sci_Position positionStart, Sci_Position characterOffset) const override;
	int SCI_METHOD GetCharacterAndWidth(Sci_Position position, Sci_Position *pWidth) const override;
};

