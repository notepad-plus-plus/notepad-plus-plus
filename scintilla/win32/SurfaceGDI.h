// Scintilla source code edit control
/** @file SurfaceGDI.h
 ** Definitions for drawing to GDI on Windows.
 **/
// Copyright 2025 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef SURFACEGDI_H
#define SURFACEGDI_H

namespace Scintilla::Internal {

std::shared_ptr<Font> FontGDI_Allocate(const FontParameters &fp);
std::unique_ptr<Surface> SurfaceGDI_Allocate();

}

#endif
