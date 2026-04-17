# Lazy Session Load — Risk Register

Scope: every behavior that might regress from the lazy-session-load changes
on branch `feature/lazy-session-load`.

Work order: verify → if real, fix → test the fix → mark DONE.
Keep notes inline (verification method, reproduction, fix summary).

Legend: **UNVERIFIED** / **NOT A BUG** / **CONFIRMED** / **FIXED** / **TESTED**

---

## R1. Tab ordering broken for active tab
**Status:** CONFIRMED — accepted tradeoff, documented

**Verified:** Session had active `new 3` at index 341/342. After startup
active appears at tab-bar index 0 instead of 341. Tab-bar order is
`[new 3, new 5, new 18, …, new 2]` instead of session's
`[new 5, new 18, …, new 2, new 3]`.

**Attempted fix — sync insert all:** makes loadSession take ~2.6 s
(window stays blank the whole time). Regresses user perception far more
than the ordering issue.

**Attempted fix — pump active in order:** makes user wait
(activeIndex × tickInterval) ms for their active content. For active at
index 341 of 342 that is ~2 s of blank content — same UX penalty as
sync insert.

**Decision:** keep option (b) — active tab sync-inserted at tab-bar
index 0, others fill in session order behind. Document as known
limitation in the PR. Windows dialog / Ctrl+Tab navigation still work
(they use buffer order, not strict visual order).

A proper fix (reorder after pump drains, or insert at arbitrary index)
requires either a tab-control reorder API we do not currently use, or a
tab-bar redesign. Out of scope for the initial PR.

---

## R2. Data loss if user exits during background pump
**Status:** FIXED + TESTED

**Verified:** With 342-tab session, closing at 1 s saved only 16 entries
to session.xml — **336 entries lost**. `getCurrentOpenedFiles`
enumerates `_mainDocTab` which only contains actually-inserted tabs;
pending queue entries were dropped silently.

**Two bugs compounding the first:**
- UNTITLED lazy buffers (backup-restored untitled tabs) also dropped
  even after drain because `getCurrentOpenedFiles` has an
  `isUntitled() && docLength()==0` filter — and our lazy docs are empty
  until content resolve.
- `SCI_SETDOCPOINTER` was dispatched unconditionally to buf->getDocument()
  which is null for lazy buffers — would crash or invisible-view gets
  hosed.

**Fix (in getCurrentOpenedFiles):**
1. Synchronously drain `_pendingSessionInserts` at the top of the
   function. Each pending entry becomes a real Buffer + tab.
2. Then resolve content for every still-lazy UNTITLED buffer so their
   backup content is readable by session serializer.
3. Guard `SCI_SETDOCPOINTER` + `SCI_MARKERNEXT` enumeration behind
   `if (buf->getDocument())`; fall back to the buffer's stashed
   `_lazyPendingMarks` for the session's bookmark list.

**Test result:** 342 entries preserved in both "close at 1 s" and
"close at 10 s" scenarios. Close cost ~8–13 s (content resolve for
~326 backup-restored untitled tabs). One-time shutdown price, no
startup impact.

---

## R3. Null Scintilla Document on lazy buffer access before resolve
**Status:** UNVERIFIED (high severity)

**Hypothesis:** I deferred `SCI_CREATEDOCUMENT` for lazy buffers; their
`_doc = 0` until resolveLazyBuffer runs. Any caller of
`buf->getDocument()` that dispatches `SCI_*` to that null doc will
crash or no-op silently.

**Verification plan:** grep for `getDocument()` callers on non-active
buffers. High-risk sites: plugin APIs `NPPM_GETSCINTILLAEDITVIEW`-style,
snapshot backup, macros, "save all". Also look at any paint/idle
callbacks enumerating `_buffers`.

**Fix idea:** either allocate the document eagerly (revert that
optimization, ~200 ms cost acceptable) or add a guard in every caller.

---

## R4. Buffer destructor with `_doc = 0`
**Status:** UNVERIFIED (high severity)

**Hypothesis:** Closing a lazy tab before it resolves → Buffer dtor
runs `SCI_RELEASEDOCUMENT(0)` which is UB or crash.

**Verification plan:** check Buffer::~Buffer or FileManager::closeBuffer
for the release call; manually simulate "Close all tabs" right after
startup.

**Fix idea:** `if (_doc) ... release ...` guard.

---

## R5. "Find in all open" misses lazy tabs
**Status:** UNVERIFIED

**Hypothesis:** Find-in-Files iterates open buffers and searches their
Scintilla docs. Lazy buffers have empty doc → miss content.

**Verification plan:** launch lazy session → Find → "Find All in All
Opened Documents" for a known-unique string in a non-active file →
observe whether result shows up.

**Fix idea:** when Find enumerates, materialise each lazy buffer on
demand (slow but correct), or read from the file path directly.

---

## R6. Plugins at NPPN_READY see incomplete buffer list
**Status:** UNVERIFIED

**Hypothesis:** NPPN_READY fires at end of init() — at which point only
the active buffer exists; the 341 pending ones are not yet created.
Plugins enumerating buffers at that point miss most of them.

**Verification plan:** inspect NPPM_GETNBOPENFILES / NPPM_GETOPENFILENAMES
handlers; list installed plugins that iterate on startup.

**Fix idea:** fire a late "all-ready" notification when
`_pendingSessionInserts` drains, or fire NPPN_READY after drain.

---

## R7. Dirty indicator delayed for backup-restored tabs
**Status:** UNVERIFIED (cosmetic)

**Hypothesis:** `addBuffer` in batch mode skips `bufferUpdated`. For
backup-restored untitled tabs (marked dirty at creation) the tab icon
stays as "saved" until my pump drains to them. Since I dropped the
dirty-marker update, it may never update for non-activated lazy tabs.

**Verification plan:** check tab icons after session load completes —
are untitled `*new N*` tabs showing the unsaved-dot?

**Fix idea:** send the correct icon in the initial TCITEM of addBuffer
based on buffer state, so no bufferUpdated roundtrip is needed.

---

## R8. Fold state / markers applied only on activation
**Status:** UNVERIFIED

**Hypothesis:** `_lazyPendingMarks` is applied in activateBuffer, not
in the background resolve path. If user never clicks a tab but content
gets loaded via some other path, marks never appear.

**Verification plan:** trace resolveLazyBuffer — does it apply marks?
If not, fix path.

**Fix idea:** apply marks in resolveLazyBuffer after content load.

---

## R9. Windows-dialog columns (Type / Modified) for lazy tabs
**Status:** PARTIALLY FIXED (Size)

**Hypothesis:** Type shows "normal" even when file is a UDL or has a
specific language detected from the extension (we skipped lang
detection in setFileNameForLazyInit). Modified time may show 1970
epoch because we skipped updateTimeStamp in ctor.

**Verification plan:** check the Windows-dialog Type column for a .sh /
.py lazy tab. Check Modified time.

**Fix idea:** compute language from extension in setFileNameForLazyInit
(cheap, just a map lookup); skip only the IO-bound bits.

---

## R10. External file changes not detected for lazy tabs
**Status:** UNVERIFIED

**Hypothesis:** `checkFilesystemChanges(false)` skips lazy buffers. If
the user edits a file externally while its tab is lazy-pending, NPP
won't prompt on activation — it will just load the latest content
silently (which is actually correct!) — but the session timestamp
mismatch could trigger a spurious prompt.

**Verification plan:** edit an untouched session file externally →
activate that tab in lazy session → does prompt fire?

**Fix idea:** resolveLazyBuffer already calls updateTimeStamp +
checkFileState; should be fine. Re-verify.

---

## R11. WM_SETREDRAW=FALSE stuck if exception thrown in pump
**Status:** UNVERIFIED (low likelihood but silent data loss of UI state)

**Hypothesis:** beginBatchInsert sets tab-bar WM_SETREDRAW to FALSE.
If an exception escapes the pump before endBatchInsert, the tab bar
never gets WM_SETREDRAW TRUE and stays frozen.

**Verification plan:** review every path that throws inside the pump.
loadSession uses non-throwing calls mostly; unlikely in practice.

**Fix idea:** RAII guard (`BatchInsertGuard` with begin/end in
ctor/dtor).

---

## R12. Bulk notify masks BufferChangeStatus — external-change detection?
**Status:** UNVERIFIED

**Hypothesis:** In resolveLazyBuffer I fire `BufferChangeMask &
~BufferChangeStatus` to avoid a spurious "reload externally modified"
prompt. But that prompt is how NPP tells the user a file on disk
changed after session-save. By suppressing it on first activation we
may miss legitimate external changes.

**Verification plan:** save session → edit file externally → launch NPP
→ activate that tab → should NPP prompt? With current code probably
not until the next tab switch / explicit refresh.

**Fix idea:** after lazy load completes, compare disk timestamp vs
stored timestamp from session (`_originalFileLastModifTimestamp`); if
differs, set status to DOC_MODIFIED manually to raise the prompt
through the existing path.
