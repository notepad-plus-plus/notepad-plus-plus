// @file ScintillaDocument.cpp
// Wrapper for Scintilla document object so it can be manipulated independently.
// Copyright (c) 2011 Archaeopteryx Software, Inc. d/b/a Wingware

#include <stdexcept>
#include <string_view>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <memory>

#include "ScintillaTypes.h"
#include "ScintillaMessages.h"
#include "ScintillaStructures.h"
#include "ScintillaDocument.h"

#include "Debugging.h"
#include "Geometry.h"
#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "CharacterCategoryMap.h"
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"

using namespace Scintilla;
using namespace Scintilla::Internal;

class WatcherHelper : public DocWatcher {
    ScintillaDocument *owner;
public:
    explicit WatcherHelper(ScintillaDocument *owner_);

    void NotifyModifyAttempt(Document *doc, void *userData) override;
    void NotifySavePoint(Document *doc, void *userData, bool atSavePoint) override;
    void NotifyModified(Document *doc, DocModification mh, void *userData) override;
    void NotifyDeleted(Document *doc, void *userData) noexcept override;
    void NotifyStyleNeeded(Document *doc, void *userData, Sci::Position endPos) override;
    void NotifyLexerChanged(Document *doc, void *userData) override;
    void NotifyErrorOccurred(Document *doc, void *userData, Status status) override;
};

WatcherHelper::WatcherHelper(ScintillaDocument *owner_) : owner(owner_) {
}

void WatcherHelper::NotifyModifyAttempt(Document *, void *) {
    emit owner->modify_attempt();
}

void WatcherHelper::NotifySavePoint(Document *, void *, bool atSavePoint) {
    emit owner->save_point(atSavePoint);
}

void WatcherHelper::NotifyModified(Document *, DocModification mh, void *) {
    int length = mh.length;
    if (!mh.text)
        length = 0;
    QByteArray ba = QByteArray::fromRawData(mh.text, length);
    emit owner->modified(mh.position, static_cast<int>(mh.modificationType), ba, length,
			 mh.linesAdded, mh.line, static_cast<int>(mh.foldLevelNow), static_cast<int>(mh.foldLevelPrev));
}

void WatcherHelper::NotifyDeleted(Document *, void *) noexcept {
}

void WatcherHelper::NotifyStyleNeeded(Document *, void *, Sci::Position endPos) {
    emit owner->style_needed(endPos);
}

void WatcherHelper::NotifyLexerChanged(Document *, void *) {
    emit owner->lexer_changed();
}

void WatcherHelper::NotifyErrorOccurred(Document *, void *, Status status) {
    emit owner->error_occurred(static_cast<int>(status));
}

ScintillaDocument::ScintillaDocument(QObject *parent, void *pdoc_) :
    QObject(parent), pdoc(pdoc_), docWatcher(nullptr) {
    if (!pdoc) {
	pdoc = new Document(DocumentOption::Default);
    }
    docWatcher = new WatcherHelper(this);
    (static_cast<Document *>(pdoc))->AddRef();
    (static_cast<Document *>(pdoc))->AddWatcher(docWatcher, pdoc);
}

ScintillaDocument::~ScintillaDocument() {
    Document *doc = static_cast<Document *>(pdoc);
    if (doc) {
        doc->RemoveWatcher(docWatcher, doc);
        doc->Release();
    }
    pdoc = nullptr;
    delete docWatcher;
    docWatcher = nullptr;
}

void *ScintillaDocument::pointer() {
    return pdoc;
}

int ScintillaDocument::line_from_position(int pos) {
    return (static_cast<Document *>(pdoc))->LineFromPosition(pos);
}

bool ScintillaDocument::is_cr_lf(int pos) {
    return (static_cast<Document *>(pdoc))->IsCrLf(pos);
}

bool ScintillaDocument::delete_chars(int pos, int len) {
    return (static_cast<Document *>(pdoc))->DeleteChars(pos, len);
}

int ScintillaDocument::undo() {
    return (static_cast<Document *>(pdoc))->Undo();
}

int ScintillaDocument::redo() {
    return (static_cast<Document *>(pdoc))->Redo();
}

bool ScintillaDocument::can_undo() {
    return (static_cast<Document *>(pdoc))->CanUndo();
}

bool ScintillaDocument::can_redo() {
    return (static_cast<Document *>(pdoc))->CanRedo();
}

void ScintillaDocument::delete_undo_history() {
    (static_cast<Document *>(pdoc))->DeleteUndoHistory();
}

bool ScintillaDocument::set_undo_collection(bool collect_undo) {
    return (static_cast<Document *>(pdoc))->SetUndoCollection(collect_undo);
}

bool ScintillaDocument::is_collecting_undo() {
    return (static_cast<Document *>(pdoc))->IsCollectingUndo();
}

void ScintillaDocument::begin_undo_action() {
    (static_cast<Document *>(pdoc))->BeginUndoAction();
}

void ScintillaDocument::end_undo_action() {
    (static_cast<Document *>(pdoc))->EndUndoAction();
}

void ScintillaDocument::set_save_point() {
    (static_cast<Document *>(pdoc))->SetSavePoint();
}

bool ScintillaDocument::is_save_point() {
    return (static_cast<Document *>(pdoc))->IsSavePoint();
}

void ScintillaDocument::set_read_only(bool read_only) {
    (static_cast<Document *>(pdoc))->SetReadOnly(read_only);
}

bool ScintillaDocument::is_read_only() {
    return (static_cast<Document *>(pdoc))->IsReadOnly();
}

void ScintillaDocument::insert_string(int position, QByteArray &str) {
    (static_cast<Document *>(pdoc))->InsertString(position, str.data(), str.size());
}

QByteArray ScintillaDocument::get_char_range(int position, int length) {
    Document *doc = static_cast<Document *>(pdoc);

    if (position < 0 || length <= 0 || position + length > doc->Length())
        return QByteArray();

    QByteArray ba(length, '\0');
    doc->GetCharRange(ba.data(), position, length);
    return ba;
}

char ScintillaDocument::style_at(int position) {
    return (static_cast<Document *>(pdoc))->StyleAt(position);
}

int ScintillaDocument::line_start(int lineno) {
    return (static_cast<Document *>(pdoc))->LineStart(lineno);
}

int ScintillaDocument::line_end(int lineno) {
    return (static_cast<Document *>(pdoc))->LineEnd(lineno);
}

int ScintillaDocument::line_end_position(int pos) {
    return (static_cast<Document *>(pdoc))->LineEndPosition(pos);
}

int ScintillaDocument::length() {
    return (static_cast<Document *>(pdoc))->Length();
}

int ScintillaDocument::lines_total() {
    return (static_cast<Document *>(pdoc))->LinesTotal();
}

void ScintillaDocument::start_styling(int position) {
    (static_cast<Document *>(pdoc))->StartStyling(position);
}

bool ScintillaDocument::set_style_for(int length, char style) {
    return (static_cast<Document *>(pdoc))->SetStyleFor(length, style);
}

int ScintillaDocument::get_end_styled() {
    return (static_cast<Document *>(pdoc))->GetEndStyled();
}

void ScintillaDocument::ensure_styled_to(int position) {
    (static_cast<Document *>(pdoc))->EnsureStyledTo(position);
}

void ScintillaDocument::set_current_indicator(int indic) {
    (static_cast<Document *>(pdoc))->decorations->SetCurrentIndicator(indic);
}

void ScintillaDocument::decoration_fill_range(int position, int value, int fillLength) {
    (static_cast<Document *>(pdoc))->DecorationFillRange(position, value, fillLength);
}

int ScintillaDocument::decorations_value_at(int indic, int position) {
    return (static_cast<Document *>(pdoc))->decorations->ValueAt(indic, position);
}

int ScintillaDocument::decorations_start(int indic, int position) {
    return (static_cast<Document *>(pdoc))->decorations->Start(indic, position);
}

int ScintillaDocument::decorations_end(int indic, int position) {
    return (static_cast<Document *>(pdoc))->decorations->End(indic, position);
}

int ScintillaDocument::get_code_page() {
    return (static_cast<Document *>(pdoc))->CodePage();
}

void ScintillaDocument::set_code_page(int code_page) {
    (static_cast<Document *>(pdoc))->dbcsCodePage = code_page;
}

int ScintillaDocument::get_eol_mode() {
    return static_cast<int>((static_cast<Document *>(pdoc))->eolMode);
}

void ScintillaDocument::set_eol_mode(int eol_mode) {
    (static_cast<Document *>(pdoc))->eolMode = static_cast<EndOfLine>(eol_mode);
}

int ScintillaDocument::move_position_outside_char(int pos, int move_dir, bool check_line_end) {
    return (static_cast<Document *>(pdoc))->MovePositionOutsideChar(pos, move_dir, check_line_end);
}

int ScintillaDocument::get_character(int pos) {
    return (static_cast<Document *>(pdoc))->GetCharacterAndWidth(pos, nullptr);
}
