// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Shared user-language lexer configuration, added 2026-11-01.
#import "ScintillaView.h"
NSDictionary *NXAdvancedLanguageDefaults(void);
BOOL NXValidateAdvancedLanguage(NSDictionary *language);
void NXConfigureUserLanguage(ScintillaView *view, NSDictionary *language, NSString *documentID);
void NXApplyUserLanguageTheme(ScintillaView *view, NSDictionary *theme);
