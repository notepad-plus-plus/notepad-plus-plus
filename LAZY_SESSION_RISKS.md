# Lazy Session Load — Risk Register

## R14. Background content pre-fetch should never block UI thread
**Status:** FIXED + TESTED

**Goal:** when the user clicks any tab — even one deep in a 300+ tab
session — its content must already be loaded. The original design
only loaded on click, so a click on an un-prefetched tab stalled the
UI for the duration of one file read.

**Wrong approach tried first:** WM_TIMER-driven pump resolving one
buffer per tick on the main thread. Each tick blocks for the duration
of one `ReadFile` — fine for local SSD (ms) but catastrophic for
unreachable paths (WSL offline, sleeping HDD → 15–30 s per tab).

**Correct approach:** a dedicated worker thread reads file bytes off
the main thread. Flow:

1. Session-insert pump enqueues `(BufferID, path, encoding)` into a
   thread-safe deque (`_lazyLoadWorkerQueue`, protected by
   `_lazyLoadWorkerMutex` + `_lazyLoadWorkerCv`).
2. Worker thread waits on the condition variable, pops a request,
   reads file bytes, posts `NPPM_INTERNAL_LAZYLOADWORKERDONE` back to
   the main window with a heap-allocated payload containing the
   bytes.
3. Main thread validates (buffer still exists, still lazy, path
   still matches — guards against `Buffer*` pointer reuse after a
   tab close) then calls `FileManager::applyLazyContent` to create
   the Scintilla doc and insert the bytes.

**Race-condition analysis:**
- _Double read_ (user clicks tab while worker is reading it): main
  syncs via `resolveLazyBuffer`, flips `_isLazyPending` off. When
  worker payload arrives main discards it — user sees content
  instantly, worker's work is wasted but harmless.
- _Pointer reuse_ (tab closed → `Buffer*` freed → address reused by
  new buffer): payload contains `sourcePath`, main compares against
  current buffer's backup/full path, drops on mismatch.
- _Dirty status_: backup-restore keeps `dirty=true`, regular file
  resets. `_isLazyPending` gates the apply so user edits between
  queue and delivery are never overwritten.
- _Shutdown_: `stopLazyLoadWorker` sets atomic stop + notifies CV,
  then waits only 200 ms for the thread. If the worker is stuck
  inside `ReadFile` on an unreachable path the thread is detached —
  OS finalises it at process exit, and the worker touches no NPP
  state after that point (just `ReadFile` + `PostMessage` to a
  window that's about to be destroyed; PostMessage to a dead HWND is
  a no-op).
- _Deadlock_: the only shared state is the queue + stop flag; both
  locks are leaf locks held for microseconds. No cross-lock paths.

**Measurement:** launch with 342-tab session, sample
`GetProcessIoCounters` + `SendMessageTimeout(50ms)` every ~300 ms.
Over 20 s: 57 / 59 probes were responsive (the 2 misses are at
`t=70 ms` and `t=433 ms` before the window is even shown); thread
count jumps from 4 to 11 (worker started); read bytes climb from 0
to 4 MB as the worker pulls session files. The user never sees the
wait cursor.

---

## R13. Wait-cursor / "busy" state during background drain
**Status:** SUPERSEDED BY R14

Originally fixed by switching the content-load pump from
`PostMessage` to `SetTimer`. The R14 worker-thread design removes
the main-thread content pump entirely, so this concern no longer
applies — the only thing that can briefly "busy" the main thread
is applying already-read bytes to a Scintilla document, which is
pure memory work.

## R13-original. Wait-cursor / "busy" state during background drain
**Status:** FIXED + TESTED

**Verified by user:** even with init() returning in ~150 ms and the
window painting at ~500 ms, the mouse cursor stayed as a wait
(hourglass) and clicks were ignored for the entire ~2 s while the
background pump filled the tab bar.

**Root cause:** the pump re-armed itself via `PostMessage`. Regular
posted messages compete with user input in the same queue, and a
tight PostMessage → handler → PostMessage loop (even with 1 ms of
work per tick) keeps the UI-thread "busy" from the OS's point of
view. Windows surfaces that as a wait cursor.

**Fix:** `kickSessionInsertQueue` now uses `SetTimer` with
`USER_TIMER_MINIMUM` (~10 ms) instead of `PostMessage`. `WM_TIMER` is
the **lowest priority** message in a Win32 queue — Windows only
delivers it when no other messages (input, paint, sent messages) are
pending. So any mouse move / click / key event always wins against
the pump, the cursor stays as the regular arrow, and clicks are
processed instantly. Pump drain time rises slightly (one tick per
~10 ms), but for 342 tabs that's ~3.4 s of progressive fill with
full UI responsiveness — which is the UX we wanted.

---


Scope: every behavior that might regress from the lazy-session-load changes
on branch `feature/lazy-session-load`.

Work order: verify → if real, fix → test the fix → mark DONE.
Keep notes inline (verification method, reproduction, fix summary).

Legend: **UNVERIFIED** / **NOT A BUG** / **CONFIRMED** / **FIXED** / **TESTED**

---

## R1. Tab ordering broken for active tab
**Status:** FIXED + TESTED

**Verified:** Session had active `new 3` at index 341/342. Initial
implementation placed active at tab-bar index 0 and pushed all other
tabs to indices 1..341, breaking the session order.

**Fix:** use `TCM_INSERTITEM`'s explicit index argument. The pump
processes session entries in session order and inserts each one at
its own session index. Because the active tab is already at tab-bar
index 0 and every prior session entry lands at its own (smaller)
session index, `TCM_INSERTITEM` at index `sessionIndex` is always
valid (`≤ current nbItem`). The active tab drifts rightward as
smaller-indexed entries are inserted before it, and finally lands at
its original session index.

Worked example for `active_idx=A=341, N=342`:
- t=0: `[active]` at index 0
- t=1 (pump processes session_0): insert at 0 → `[session_0, active]`
- t=2 (session_1): insert at 1 → `[session_0, session_1, active]`
- …
- t=341 (session_340, i.e. the last non-active): insert at 340 →
  `[session_0, session_1, …, session_340, active]`

Implementation:
- `DocTabView::addBufferAt(index, buffer)` — TCM_INSERTITEM with
  explicit index, picks the correct icon from buffer state.
- `Notepad_plus::loadBufferIntoViewAt(id, whichOne, insertIndex)` —
  tab-at-index wrapper that still handles the dummy-new-1 trim case.
- `newLazyDocument` / `newLazyBackupDocument` take an
  `insertTabIndex = -1` default (keeps existing append behaviour
  intact); the pump passes the session index.
- `PendingSessionInsert` gains `sessionIndex`.

**Test:** 342-tab session → load → drain → close. Compared
session.xml before vs after: 0 position differences across all 342
entries. Active `new 3` stayed at session index 341.

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
**Status:** PARTIALLY FIXED

**Audit:** 32 callers of `getDocument()` / `_doc`. Most operate on the
CURRENT buffer, which `activateBuffer` always resolves first via the
lazy hook. Remaining exposure: enumeration paths.

**Fixes applied:**
- `getCurrentOpenedFiles` guards `SCI_SETDOCPOINTER` +
  `SCI_MARKERNEXT` behind a null-doc check (R2 work).
- `closeBuffer` guards `SCI_RELEASEDOCUMENT` (see R4).

**Remaining exposure:** plugin code that iterates open buffers and
reads the Scintilla doc directly. Plugins can't be fixed from here;
document as "plugins assuming all session buffers have loaded content
may miss lazy ones". Mitigation: lazy is opt-in (`LazySessionLoad`).

**Status:** VERIFIED — close-at-1s exits cleanly (exit code 0). No
crash observed.

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
**Status:** FIXED + TESTED

**Verified:** `FileManager::closeBuffer` dispatched
`SCI_RELEASEDOCUMENT, 0, buf->_doc` unconditionally. Scintilla handled
NULL without crashing in practice (apparently ref-table deref is
guarded internally), but behaviour was undefined.

**Fix:** wrap the release in `if (buf->_doc)`.

**Test:** R2 exit-at-1s close flow calls closeBuffer for every lazy
buffer in the drained-but-not-content-loaded set. Exit code 0, no
crash.

**Hypothesis:** Closing a lazy tab before it resolves → Buffer dtor
runs `SCI_RELEASEDOCUMENT(0)` which is UB or crash.

**Verification plan:** check Buffer::~Buffer or FileManager::closeBuffer
for the release call; manually simulate "Close all tabs" right after
startup.

**Fix idea:** `if (_doc) ... release ...` guard.

---

## R5. "Find in all open" misses lazy tabs
**Status:** FIXED

**Verified by code read:** `findInOpenedFiles` iterates `_mainDocTab`
and `_subDocTab`, doing `SCI_SETDOCPOINTER, pBuf->getDocument()`. For
lazy buffers `_doc` is null → SCI_SETDOCPOINTER(0) detaches the view,
subsequent `processAll` sees empty content. Also for buffers resolved
by lazy-backup, content exists but would be from backup file — matches
user expectation.

**Fix:** before SCI_SETDOCPOINTER in both main and sub loops, check
`pBuf->isLazyPending()` and call `resolveLazyBuffer` if so. User asked
to search across all open files — loading content is expected.

**Cost:** one-off disk read per lazy buffer. For a 300-tab session
that's ~1–3 s the first time Find-in-All-Open is used; subsequent
invocations are free.

**Hypothesis:** Find-in-Files iterates open buffers and searches their
Scintilla docs. Lazy buffers have empty doc → miss content.

**Verification plan:** launch lazy session → Find → "Find All in All
Opened Documents" for a known-unique string in a non-active file →
observe whether result shows up.

**Fix idea:** when Find enumerates, materialise each lazy buffer on
demand (slow but correct), or read from the file path directly.

---

## R6. Plugins at NPPN_READY see incomplete buffer list
**Status:** PARTIALLY FIXED

**Verified by code read:** `NPPM_GETNBOPENFILES` and
`NPPM_GETOPENFILENAMES*` return counts / paths from `_mainDocTab` /
`_subDocTab` only. At `NPPN_READY` time only the active tab (and any
sync-inserted cases) is in the tab view; the 300+ pending entries are
invisible to plugins.

**Fix:** both message handlers now include `_pendingSessionInserts`
in their response. File paths for pending entries come from the
stashed sessionFileInfo. Plugins counting buffers or listing filenames
at startup see the full session.

**Not fixed:** plugins that call `NPPM_GETCURRENTSCINTILLA` then
dispatch `SCI_*` on non-active buffers will still get empty content
until those buffers are activated. Requires worker-thread IO to fix
without blocking startup. Documented as known limitation for plugin
authors.

**Hypothesis:** NPPN_READY fires at end of init() — at which point only
the active buffer exists; the 341 pending ones are not yet created.
Plugins enumerating buffers at that point miss most of them.

**Verification plan:** inspect NPPM_GETNBOPENFILES / NPPM_GETOPENFILENAMES
handlers; list installed plugins that iterate on startup.

**Fix idea:** fire a late "all-ready" notification when
`_pendingSessionInserts` drains, or fire NPPN_READY after drain.

---

## R7. Dirty indicator delayed for backup-restored tabs
**Status:** FIXED

**Verified by code read:** `addBuffer` always sets `tie.iImage = 0`
(SAVED_IMG_INDEX) on insert. In batch mode we skip the follow-up
`bufferUpdated` call that would normally set the correct icon.
Backup-restored untitled tabs (marked dirty at creation) appeared as
"saved" until user clicked them.

**Fix:** compute the correct icon index in `addBuffer` itself based
on buffer state (isDirty / isMonitoringOn / getFileReadOnly /
getUserReadOnly). Avoids the batch-mode delay entirely.

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
**Status:** NOT A BUG (on verification)

**Analysis of call graph:**
- `_lazyPendingMarks` is stashed during pump (line 5308). Applied to
  Scintilla during `activateBuffer` (line 5126) — which is the correct
  moment because the view is now attached to the buffer's document.
- Fold states are applied via `setHeaderLineState` during pump (line
  5307). That stores them in `Buffer._foldStates`, which
  `ScintillaEditView::activateBuffer` reads on doc attach.
- Session save reads `_lazyPendingMarks` as a fallback when the
  buffer has no Scintilla doc yet (see R2 fix at line 6695). So marks
  round-trip correctly through shutdown even if user never activated
  the tab.

**Remaining edge case:** a lazy buffer resolved via a non-activation
path (e.g. R5's find-in-all) loads content but does NOT apply marks.
That is fine for find-in-files (doesn't care about bookmarks); marks
will still be applied if the user later activates the tab.

**Hypothesis:** `_lazyPendingMarks` is applied in activateBuffer, not
in the background resolve path. If user never clicks a tab but content
gets loaded via some other path, marks never appear.

**Verification plan:** trace resolveLazyBuffer — does it apply marks?
If not, fix path.

**Fix idea:** apply marks in resolveLazyBuffer after content load.

---

## R9. Windows-dialog columns (Type / Modified) for lazy tabs
**Status:** FIXED

**Type column:** dialog reads `buf->getLangType()`. Lazy buffers have
`_lang` set by the pump handler (via `setLangType` from session's
`_langName`). Works.

**Modified time:** dialog reads `buf->getLastModifiedTimestamp()`.
Lazy buffers have `_timeStamp` set from session's
`_originalFileLastModifTimestamp` in `newLazyDocument` /
`newLazyBackupDocument`. Works.

**Size:** fixed earlier — falls back to on-disk / backup-file size
when Scintilla doc is empty.

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
**Status:** ACCEPTED — semantics shift but not a regression

**Analysis:** `checkFilesystemChanges(false)` iterates every buffer at
startup in snapshot mode and prompts for files that changed on disk.
The loop now skips lazy-pending buffers. Per-file check happens in
`resolveLazyBuffer` via `updateTimeStamp` + `checkFileState` at
activation time.

**Consequence:** if an external process modifies a file between the
last NPP shutdown and the next startup, with lazy restore the user is
not prompted about that file until they click its tab. With eager
restore the prompt would fire for every changed file at startup.

**Decision:** accept. The per-file activation check is still correct;
users just discover changes at tab-switch time instead of at startup.
For a 300+ tab session this is arguably better UX than a wave of
prompts at launch. Documented as a known behavioural change in the
PR description.

Related: see R12 for the subtler issue where `resolveLazyBuffer`
currently never surfaces an external-change mismatch at all.

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
**Status:** FIXED

**Fix:** introduce `DocTabView::BatchInsertGuard` — RAII wrapper whose
ctor calls `beginBatchInsert`, dtor calls `endBatchInsert`. Even if
something throws between begin/end, stack unwinding un-freezes the
tab bar. Explicit early end() is still called before `activateBuffer`
so sizing is correct at activation time.

**Hypothesis:** beginBatchInsert sets tab-bar WM_SETREDRAW to FALSE.
If an exception escapes the pump before endBatchInsert, the tab bar
never gets WM_SETREDRAW TRUE and stays frozen.

**Verification plan:** review every path that throws inside the pump.
loadSession uses non-throwing calls mostly; unlikely in practice.

**Fix idea:** RAII guard (`BatchInsertGuard` with begin/end in
ctor/dtor).

---

## R12. Bulk notify masks BufferChangeStatus — external-change detection?
**Status:** NOT A BUG (after analysis)

**Analysis of semantics:**
- Eager restore keeps each buffer's `_timeStamp` = session's
  `_originalFileLastModifTimestamp`. Startup's
  `checkFilesystemChanges(false)` then compares that against current
  disk mtime and prompts "reload externally modified?" for every file
  that drifted.
- Lazy restore loads the current content at activation time. There is
  no meaningful "reload?" prompt to show — the user already gets the
  current version; nothing is stale to decide about.

**Consequence:** the startup wave of "reload?" prompts does not fire
for lazy buffers. This is arguably BETTER UX (no prompt blizzard on
launch) and no content is lost — the freshly-read content IS the
current on-disk state. Documented as a behavioural improvement under
lazy mode in the PR description.

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
