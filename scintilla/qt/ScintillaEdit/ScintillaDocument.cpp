// ScintillaDocument.cpp
// Wrapper for Scintilla document object so it can be manipulated independently.
// Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware

#include <stdexcept>
#include <vector>
#include <map>

#include "ScintillaDocument.h"

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "XPM.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

class WatcherHelper : public DocWatcher {
	ScintillaDocument *owner;
public:
	explicit WatcherHelper(ScintillaDocument *owner_);
	virtual ~WatcherHelper();

	void NotifyModifyAttempt(Document *doc, void *userData);
	void NotifySavePoint(Document *doc, void *userData, bool atSavePoint);
	void NotifyModified(Document *doc, DocModification mh, void *userData);
	void NotifyDeleted(Document *doc, void *userData);
	void NotifyStyleNeeded(Document *doc, void *userData, int endPos);
	void NotifyLexerChanged(Document *doc, void *userData);
	void NotifyErrorOccurred(Document *doc, void *userData, int status);
};

WatcherHelper::WatcherHelper(ScintillaDocument *owner_) : owner(owner_) {
}

WatcherHelper::~WatcherHelper() {
}

void WatcherHelper::NotifyModifyAttempt(Document *, void *) {
	owner->emit_modify_attempt();
}

void WatcherHelper::NotifySavePoint(Document *, void *, bool atSavePoint) {
	owner->emit_save_point(atSavePoint);
}

void WatcherHelper::NotifyModified(Document *, DocModification mh, void *) {
    int length = mh.length;
    if (!mh.text)
        length = 0;
    QByteArray ba = QByteArray::fromRawData(mh.text, length);
    owner->emit_modified(mh.position, mh.modificationType, ba, length,
        mh.linesAdded, mh.line, mh.foldLevelNow, mh.foldLevelPrev);
}

void WatcherHelper::NotifyDeleted(Document *, void *) {
}

void WatcherHelper::NotifyStyleNeeded(Document *, void *, int endPos) {
	owner->emit_style_needed(endPos);
}

void WatcherHelper::NotifyLexerChanged(Document *, void *) {
    owner->emit_lexer_changed();
}

void WatcherHelper::NotifyErrorOccurred(Document *, void *, int status) {
    owner->emit_error_occurred(status);
}

ScintillaDocument::ScintillaDocument(QObject *parent, void *pdoc_) :
    QObject(parent), pdoc(pdoc_), docWatcher(0) {
    if (!pdoc) {
        pdoc = new Document();
    }
    docWatcher = new WatcherHelper(this);
    ((Document *)pdoc)->AddRef();
    ((Document *)pdoc)->AddWatcher(docWatcher, pdoc);
}

ScintillaDocument::~ScintillaDocument() {
    Document *doc = static_cast<Document *>(pdoc);
    if (doc) {
        doc->RemoveWatcher(docWatcher, doc);
        doc->Release();
    }
    pdoc = NULL;
    delete docWatcher;
    docWatcher = NULL;
}

void *ScintillaDocument::pointer() {
	return pdoc;
}

int ScintillaDocument::line_from_position(int pos) {
    return ((Document *)pdoc)->LineFromPosition(pos);
}

bool ScintillaDocument::is_cr_lf(int pos) {
    return ((Document *)pdoc)->IsCrLf(pos);
}

bool ScintillaDocument::delete_chars(int pos, int len) {
    return ((Document *)pdoc)->DeleteChars(pos, len);
}

int ScintillaDocument::undo() {
    return ((Document *)pdoc)->Undo();
}

int ScintillaDocument::redo() {
    return ((Document *)pdoc)->Redo();
}

bool ScintillaDocument::can_undo() {
    return ((Document *)pdoc)->CanUndo();
}

bool ScintillaDocument::can_redo() {
    return ((Document *)pdoc)->CanRedo();
}

void ScintillaDocument::delete_undo_history() {
    ((Document *)pdoc)->DeleteUndoHistory();
}

bool ScintillaDocument::set_undo_collection(bool collect_undo) {
    return ((Document *)pdoc)->SetUndoCollection(collect_undo);
}

bool ScintillaDocument::is_collecting_undo() {
    return ((Document *)pdoc)->IsCollectingUndo();
}

void ScintillaDocument::begin_undo_action() {
    ((Document *)pdoc)->BeginUndoAction();
}

void ScintillaDocument::end_undo_action() {
    ((Document *)pdoc)->EndUndoAction();
}

void ScintillaDocument::set_save_point() {
    ((Document *)pdoc)->SetSavePoint();
}

bool ScintillaDocument::is_save_point() {
    return ((Document *)pdoc)->IsSavePoint();
}

void ScintillaDocument::set_read_only(bool read_only) {
    ((Document *)pdoc)->SetReadOnly(read_only);
}

bool ScintillaDocument::is_read_only() {
    return ((Document *)pdoc)->IsReadOnly();
}

void ScintillaDocument::insert_string(int position, QByteArray &str) {
    ((Document *)pdoc)->InsertString(position, str.data(), str.size());
}

QByteArray ScintillaDocument::get_char_range(int position, int length) {
    Document *doc = (Document *)pdoc;

    if (position < 0 || length <= 0 || position + length > doc->Length())
        return QByteArray();

    QByteArray ba(length, '\0');
    doc->GetCharRange(ba.data(), position, length);
    return ba;
}

char ScintillaDocument::style_at(int position) {
    return ((Document *)pdoc)->StyleAt(position);
}

int ScintillaDocument::line_start(int lineno) {
    return ((Document *)pdoc)->LineStart(lineno);
}

int ScintillaDocument::line_end(int lineno) {
    return ((Document *)pdoc)->LineEnd(lineno);
}

int ScintillaDocument::line_end_position(int pos) {
    return ((Document *)pdoc)->LineEndPosition(pos);
}

int ScintillaDocument::length() {
    return ((Document *)pdoc)->Length();
}

int ScintillaDocument::lines_total() {
    return ((Document *)pdoc)->LinesTotal();
}

void ScintillaDocument::start_styling(int position, char flags) {
    ((Document *)pdoc)->StartStyling(position, flags);
}

bool ScintillaDocument::set_style_for(int length, char style) {
    return ((Document *)pdoc)->SetStyleFor(length, style);
}

int ScintillaDocument::get_end_styled() {
    return ((Document *)pdoc)->GetEndStyled();
}

void ScintillaDocument::ensure_styled_to(int position) {
    ((Document *)pdoc)->EnsureStyledTo(position);
}

void ScintillaDocument::set_current_indicator(int indic) {
    ((Document *)pdoc)->decorations.SetCurrentIndicator(indic);
}

void ScintillaDocument::decoration_fill_range(int position, int value, int fillLength) {
    ((Document *)pdoc)->DecorationFillRange(position, value, fillLength);
}

int ScintillaDocument::decorations_value_at(int indic, int position) {
    return ((Document *)pdoc)->decorations.ValueAt(indic, position);
}

int ScintillaDocument::decorations_start(int indic, int position) {
    return ((Document *)pdoc)->decorations.Start(indic, position);
}

int ScintillaDocument::decorations_end(int indic, int position) {
    return ((Document *)pdoc)->decorations.End(indic, position);
}

int ScintillaDocument::get_code_page() {
    return ((Document *)pdoc)->CodePage();
}

void ScintillaDocument::set_code_page(int code_page) {
    ((Document *)pdoc)->dbcsCodePage = code_page;
}

int ScintillaDocument::get_eol_mode() {
    return ((Document *)pdoc)->eolMode;
}

void ScintillaDocument::set_eol_mode(int eol_mode) {
    ((Document *)pdoc)->eolMode = eol_mode;
}

int ScintillaDocument::move_position_outside_char(int pos, int move_dir, bool check_line_end) {
    return ((Document *)pdoc)->MovePositionOutsideChar(pos, move_dir, check_line_end);
}

int ScintillaDocument::get_character(int pos) {
    return ((Document *)pdoc)->GetCharacterAndWidth(pos, NULL);
}

// Signal emitters

void ScintillaDocument::emit_modify_attempt() {
    emit modify_attempt();
}

void ScintillaDocument::emit_save_point(bool atSavePoint) {
    emit save_point(atSavePoint);
}

void ScintillaDocument::emit_modified(int position, int modification_type, const QByteArray& text, int length,
    int linesAdded, int line, int foldLevelNow, int foldLevelPrev) {
    emit modified(position, modification_type, text, length,
        linesAdded, line, foldLevelNow, foldLevelPrev);
}

void ScintillaDocument::emit_style_needed(int pos) {
    emit style_needed(pos);
}

void ScintillaDocument::emit_lexer_changed() {
    emit lexer_changed();
}

void ScintillaDocument::emit_error_occurred(int status) {
    emit error_occurred(status);
}

