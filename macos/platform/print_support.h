// print_support.h — Printing via macOS print system
// Part of the Notepad++ macOS port modular refactor.

#pragma once

#import <Cocoa/Cocoa.h>

// Show the macOS print dialog for the active document.
void showPrintDialog(NSWindow* parentWindow);
