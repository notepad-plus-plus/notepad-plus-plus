// Scintilla source code edit control
/** @file EditModel.cxx
 ** Defines the editor state that must be visible to EditorView.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cmath>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"

#include "CharacterCategory.h"

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
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"

using namespace Scintilla;

Caret::Caret() noexcept :
	active(false), on(false), period(500) {}

EditModel::EditModel() : braces{} {
	inOverstrike = false;
	xOffset = 0;
	trackLineWidth = false;
	posDrag = SelectionPosition(Sci::invalidPosition);
	braces[0] = Sci::invalidPosition;
	braces[1] = Sci::invalidPosition;
	bracesMatchStyle = STYLE_BRACEBAD;
	highlightGuideColumn = 0;
	primarySelection = true;
	imeInteraction = imeWindowed;
	bidirectional = Bidirectional::bidiDisabled;
	foldFlags = 0;
	foldDisplayTextStyle = SC_FOLDDISPLAYTEXT_HIDDEN;
	hotspot = Range(Sci::invalidPosition);
	hoverIndicatorPos = Sci::invalidPosition;
	wrapWidth = LineLayout::wrapWidthInfinite;
	pdoc = new Document(SC_DOCUMENTOPTION_DEFAULT);
	pdoc->AddRef();
	pcs = ContractionStateCreate(pdoc->IsLarge());
}

EditModel::~EditModel() {
	pdoc->Release();
	pdoc = nullptr;
}

bool EditModel::BidirectionalEnabled() const noexcept {
	return (bidirectional != Bidirectional::bidiDisabled) &&
		(SC_CP_UTF8 == pdoc->dbcsCodePage);
}

bool EditModel::BidirectionalR2L() const noexcept {
	return bidirectional == Bidirectional::bidiR2L;
}

void EditModel::SetDefaultFoldDisplayText(const char *text) {
	defaultFoldDisplayText = IsNullOrEmpty(text) ? UniqueString() : UniqueStringCopy(text);
}

const char *EditModel::GetDefaultFoldDisplayText() const noexcept {
	return defaultFoldDisplayText.get();
}

const char *EditModel::GetFoldDisplayText(Sci::Line lineDoc) const noexcept {
	if (foldDisplayTextStyle == SC_FOLDDISPLAYTEXT_HIDDEN || pcs->GetExpanded(lineDoc)) {
		return nullptr;
	}

	const char *text = pcs->GetFoldDisplayText(lineDoc);
	return text ? text : defaultFoldDisplayText.get();
}
