// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// List commands and native Return handling, added 2026-11-01.
@implementation NXApp (Editing)
- (BOOL)insertEditorNewline:(ScintillaView *)view {
    if (Send(view, SCI_GETREADONLY)) return YES;
    if (Send(view, SCI_AUTOCACTIVE)) return NO;
    struct Edit { sptr_t start, end, caret; std::string value; };
    std::string text(static_cast<const char *>(Text(view).bytes), Text(view).length);
    std::vector<Edit> edits;
    const auto mode = Send(view, SCI_GETEOLMODE);
    const std::string eol = mode == SC_EOL_CRLF ? "\r\n" : mode == SC_EOL_CR ? "\r" : "\n";
    for (sptr_t i = 0; i < Send(view, SCI_GETSELECTIONS); ++i) {
        auto a = Send(view, SCI_GETSELECTIONNSTART, i), b = Send(view, SCI_GETSELECTIONNEND, i);
        auto line = Send(view, SCI_LINEFROMPOSITION, a), start = Send(view, SCI_POSITIONFROMLINE, line);
        auto end = Send(view, SCI_GETLINEENDPOSITION, line);
        std::string contents = text.substr(start, end - start);
        auto prefix = NXParseList(contents);
        std::string indent = contents.substr(0, prefix.indent);
        std::string replacement = eol;
        if (prefix.end && a >= start + static_cast<sptr_t>(prefix.end)) {
            if (b <= end && contents.find_first_not_of(" \t", prefix.end) == std::string::npos) {
                edits.push_back({start + static_cast<sptr_t>(prefix.indent), end, 0, ""}); continue;
            }
            replacement += indent + NXNextListMarker(prefix) + " ";
        } else if ([self.preferences[@"autoIndent"] boolValue]) replacement += indent;
        edits.push_back({a, b, 0, replacement});
    }
    std::sort(edits.begin(), edits.end(), [](const Edit &a, const Edit &b) { return a.start < b.start; });
    // Scintilla usually merges overlaps. Do not apply ambiguous overlapping edits twice.
    for (size_t i = 1; i < edits.size(); ++i) if (edits[i].start <= edits[i-1].end) return NO;
    Send(view, SCI_BEGINUNDOACTION);
    for (auto it = edits.rbegin(); it != edits.rend(); ++it) {
        Send(view, SCI_SETTARGETRANGE, it->start, it->end);
        Send(view, SCI_REPLACETARGET, it->value.size(), reinterpret_cast<sptr_t>(it->value.data()));
    }
    Send(view, SCI_ENDUNDOACTION);
    sptr_t delta = 0;
    for (auto &edit : edits) { edit.caret = edit.start + delta + edit.value.size(); delta += edit.value.size() - (edit.end - edit.start); }
    if (!edits.empty()) {
        Send(view, SCI_SETSELECTION, edits[0].caret, edits[0].caret);
        for (size_t i = 1; i < edits.size(); ++i) Send(view, SCI_ADDSELECTION, edits[i].caret, edits[i].caret);
    }
    Send(view, SCI_SCROLLCARET); [self updateControls]; return YES;
}
- (void)toggleList:(BOOL)numbered {
    ScintillaView *view = self.current.editor;
    if (Send(view, SCI_GETREADONLY)) return;
    auto start = Send(view, SCI_GETSELECTIONSTART), end = Send(view, SCI_GETSELECTIONEND);
    auto first = Send(view, SCI_LINEFROMPOSITION, start), last = Send(view, SCI_LINEFROMPOSITION, end);
    if (end > start && last > first && end == Send(view, SCI_POSITIONFROMLINE, last)) --last;
    std::string text(static_cast<const char *>(Text(view).bytes), Text(view).length);
    bool remove = true;
    for (auto line = first; line <= last; ++line) {
        auto p = NXParseList(std::string_view(text).substr(Send(view, SCI_POSITIONFROMLINE, line)));
        if (!p.end || p.numbered != numbered) remove = false;
    }
    Send(view, SCI_BEGINUNDOACTION);
    for (auto line = last; line >= first; --line) {
        auto pos = Send(view, SCI_POSITIONFROMLINE, line);
        auto p = NXParseList(std::string_view(text).substr(pos));
        std::string value = remove ? "" : numbered ? std::to_string(line - first + 1) + ". " : "\xe2\x80\xa2 ";
        Send(view, SCI_SETTARGETRANGE, pos + p.indent, pos + (p.end ? p.end : p.indent));
        Send(view, SCI_REPLACETARGET, value.size(), reinterpret_cast<sptr_t>(value.data()));
    }
    Send(view, SCI_ENDUNDOACTION);
    Send(view, SCI_SETSEL, Send(view, SCI_POSITIONFROMLINE, first), Send(view, SCI_GETLINEENDPOSITION, last));
    [self.window makeFirstResponder:view]; [self tick:nil];
}
@end
