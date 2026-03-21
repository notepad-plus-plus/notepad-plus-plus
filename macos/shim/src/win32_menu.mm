// Win32 Menu/Dialog/Shell/Theme shim aggregator for macOS.
//
// This file previously contained all menu, dialog, clipboard, shell,
// and theme implementations. These have been split into focused modules:
//
//   win32_menu_impl.mm       - Menu API, ObjC targets, accelerator stubs
//   win32_file_dialogs.mm    - GetOpenFileNameW, GetSaveFileNameW
//   win32_dialog_items.mm    - ChooseColorW, common dialog stubs
//   win32_dialog_impl.mm     - Clipboard, Shell API, Theme/DWM stubs
//
// Shared string helpers live in win32_string_helpers.h.
//
// This file is intentionally kept as a placeholder to preserve
// the git history of the original monolithic implementation.
