// tab_context_menu.h — Tab bar context menu (right-click)
// Part of the Notepad++ macOS port.

#pragma once

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
void showTabContextMenu(int viewIndex, int tabIndex, NSPoint screenPoint);
#endif
