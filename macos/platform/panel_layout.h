// panel_layout.h — Central panel layout orchestrator
// Positions all panels, editor, and dividers.

#pragma once

#import <Cocoa/Cocoa.h>

void relayoutPanels();

static const CGFloat kPanelCollapseThreshold = 60.0;
static const CGFloat kMinEditorWidth = 120.0;
static const CGFloat kDividerThickness = 3.0;

static const int kDefaultLeftPanelWidth = 200;
static const int kDefaultRightPanelWidth = 220;
static const int kDefaultDocumentMapWidth = 140;
