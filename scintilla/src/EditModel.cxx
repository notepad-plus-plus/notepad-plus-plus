// Scintilla source code edit control
/** @file EditModel.cxx
 ** Defines the editor state that must be visible to EditorView.
 **/
// Copyright 1998-2014 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include <stdexcept>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include "Platform.h"

#include "ILexer.h"
#include "Scintilla.h"

#include "StringCopy.h"
#include "Position.h"
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
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"

#ifdef SCI_NAMESPACE
using namespace Scintilla;
#endif

Caret::Caret() :
	active(false), on(false), period(500) {}

EditModel::EditModel() {
	inOverstrike = false;
	xOffset = 0;
	trackLineWidth = false;
	posDrag = SelectionPosition(invalidPosition);
	braces[0] = invalidPosition;
	braces[1] = invalidPosition;
	bracesMatchStyle = STYLE_BRACEBAD;
	highlightGuideColumn = 0;
	primarySelection = true;
	imeInteraction = imeWindowed;
	foldFlags = 0;
	hotspot = Range(invalidPosition);
	hoverIndicatorPos = invalidPosition;
	wrapWidth = LineLayout::wrapWidthInfinite;
	pdoc = new Document();
	pdoc->AddRef();
}

EditModel::~EditModel() {
	pdoc->Release();
	pdoc = 0;
}
