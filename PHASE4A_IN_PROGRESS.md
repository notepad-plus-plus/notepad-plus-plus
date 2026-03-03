# Phase 4A: Basic Scintilla Integration - Archived

> Superseded by `PHASE4A_COMPLETE.md`.

## Summary

Phase 4A focuses on replacing NSTextView with ScintillaView to enable advanced text editing capabilities. The code changes are complete, but the build system requires CMake to be installed to compile and test.

## Completed Changes

### 1. Build System Updates ✅

**File:** `PowerEditor/CMakeLists.txt`

- **Enabled Scintilla and Lexilla libraries** (lines 88-90)
  - Uncommented `scintilla` library link
  - Uncommented `lexilla` library link
  - These provide syntax highlighting and advanced editing features

- **Added Scintilla include paths** (line 66)
  - Added `${CMAKE_SOURCE_DIR}/scintilla/cocoa` to include directories
  - This allows access to ScintillaView.h and related Cocoa headers

### 2. Header File Updates ✅

**File:** `PowerEditor/src/cocoa/NotepadPlusWindowController.h`

- **Replaced NSTextView with ScintillaView** (line 24)
  ```objc
  @property (strong, nonatomic) ScintillaView* textView;  // Was: NSTextView*
  ```

- **Added forward declaration** (line 20)
  ```objc
  @class ScintillaView;
  ```

### 3. Implementation Updates ✅

**File:** `PowerEditor/src/cocoa/NotepadPlusWindowController.mm`

- **Added Scintilla headers** (lines 18-19)
  ```objc
  #import "ScintillaView.h"
  #import "Scintilla.h"
  ```

- **Replaced setupTextView method** (lines 70-115)
  - Creates ScintillaView instead of NSTextView
  - Configures basic Scintilla properties:
    - ✅ Monospaced font (Menlo, 12pt)
    - ✅ Line numbers in margin
    - ✅ UTF-8 encoding
    - ✅ Undo/redo support
    - ✅ Tab width (4 spaces)
    - ✅ Caret line highlighting
    - ✅ Selection colors

## Key Features Implemented

### ScintillaView Configuration

```objc
// Font setup
[self.textView setStringProperty:SCI_STYLESETFONT parameter:STYLE_DEFAULT value:@"Menlo"];
[self.textView setGeneralProperty:SCI_STYLESETSIZE parameter:STYLE_DEFAULT value:12];

// Line numbers (40px wide margin)
[self.textView setGeneralProperty:SCI_SETMARGINTYPEN parameter:0 value:SC_MARGIN_NUMBER];
[self.textView setGeneralProperty:SCI_SETMARGINWIDTHN parameter:0 value:40];

// UTF-8 encoding
[self.textView setGeneralProperty:SCI_SETCODEPAGE parameter:SC_CP_UTF8 value:0];

// Undo collection
[self.textView setGeneralProperty:SCI_SETUNDOCOLLECTION parameter:1 value:0];

// Tab width
[self.textView setGeneralProperty:SCI_SETTABWIDTH parameter:4 value:0];

// Caret line highlighting
[self.textView setGeneralProperty:SCI_SETCARETLINEVISIBLE parameter:1 value:0];

// Selection colors
[self.textView setColorProperty:SCI_SETSELBACK parameter:1 value:[NSColor selectedTextBackgroundColor]];
```

### API Compatibility

ScintillaView maintains NSTextView compatibility for basic operations:

| Operation | NSTextView | ScintillaView | Status |
|-----------|-----------|---------------|--------|
| Get text | `textView.string` | `[textView string]` | ✅ Compatible |
| Set text | `textView.string = @"..."` | `[textView setString:@"..."]` | ✅ Compatible |
| Get selection | `textView.selectedRange` | `[textView selectedRange]` | ✅ Compatible |
| Editable | `textView.editable` | `[textView isEditable]` | ✅ Compatible |

## Architecture Changes

### Before (Phase 3):
```
NSWindow
  └─ NSScrollView
      └─ NSTextView
```

### After (Phase 4A):
```
NSWindow
  └─ ScintillaView (manages its own scroll view)
      ├─ NSScrollView (internal)
      ├─ SCIContentView (editing surface)
      └─ SCIMarginView (line numbers, markers)
```

## What's Different

### Advantages of ScintillaView:

1. **Syntax Highlighting** - Ready for 127 language lexers
2. **Line Numbers** - Built-in margin system
3. **Code Folding** - Collapsible code blocks
4. **Advanced Editing** - Column mode, multiple selections
5. **Performance** - Optimized for large files
6. **Extensibility** - Full Scintilla API (2000+ messages)

### Current Limitations:

- ❌ **No syntax highlighting yet** (Phase 4B will add lexers)
- ❌ **No change notifications** (will implement delegate in Phase 4B)
- ⚠️ **Modified tracking** - Currently only tracked on save operations

## Build Requirements

### Prerequisites

To build and test Phase 4A, you need:

1. **CMake** (version 3.20 or later)
   ```bash
   brew install cmake
   ```

2. **Xcode** (with Command Line Tools)
   ```bash
   xcode-select --install
   ```

### Build Commands

```bash
# Option 1: Use the build script
chmod +x build_macos.sh
./build_macos.sh Debug arm64

# Option 2: Manual CMake
mkdir -p build && cd build
cmake .. -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build . --config Debug

# Run the app
open bin/Debug/notepadpp.app
```

## Testing Checklist

Once built, test the following:

### Basic Functionality
- [ ] App launches without crashing
- [ ] Main window appears with ScintillaView
- [ ] Line numbers visible in left margin
- [ ] Can type text
- [ ] Cmd+N creates new document
- [ ] Cmd+O opens file picker
- [ ] Can open and display text file
- [ ] Cmd+S saves file
- [ ] Cmd+Shift+S shows Save As dialog
- [ ] Cmd+Z/Cmd+Shift+Z undo/redo works
- [ ] Cmd+X/C/V cut/copy/paste works

### ScintillaView-Specific
- [ ] Line numbers update as you type
- [ ] Current line is highlighted
- [ ] Monospaced font (Menlo) is used
- [ ] Tab key inserts 4 spaces
- [ ] Selection has correct color
- [ ] Window title shows filename
- [ ] "Edited" appears when modified
- [ ] Close button prompts to save if modified

## Known Issues

### IDE Lint Errors (Expected)

The IDE shows lint errors because it hasn't reloaded the CMake configuration:
- `'ScintillaView.h' file not found`
- `'Scintilla.h' file not found`
- `Use of undeclared identifier 'SCI_*'`

**These will be resolved** when CMake configures the project and the IDE reloads.

### Missing Features (Deferred to Phase 4B)

- No syntax highlighting (need to load lexers)
- No language detection
- No color schemes
- No Scintilla notification delegate
- Modified tracking only works on save

## File Changes Summary

| File | Lines Changed | Type | Status |
|------|--------------|------|--------|
| `PowerEditor/CMakeLists.txt` | 5 | Build config | ✅ Complete |
| `PowerEditor/src/cocoa/NotepadPlusWindowController.h` | 3 | Header | ✅ Complete |
| `PowerEditor/src/cocoa/NotepadPlusWindowController.mm` | ~50 | Implementation | ✅ Complete |

**Total:** ~58 lines changed across 3 files

## Next Steps

### Immediate (Complete Phase 4A):

1. **Install CMake**
   ```bash
   brew install cmake
   ```

2. **Build the project**
   ```bash
   ./build_macos.sh Debug arm64
   ```

3. **Test basic functionality**
   - Launch app
   - Test text editing
   - Verify line numbers
   - Test file operations

4. **Create PHASE4A_COMPLETE.md** if tests pass

### Phase 4B: Syntax Highlighting (Next)

1. Implement lexer loading system
2. Add language detection (by file extension)
3. Configure color scheme
4. Test with various file types (C++, Python, JavaScript, etc.)
5. Implement Scintilla notification delegate
6. Add proper change tracking

### Phase 4C: Advanced Features (Later)

1. Code folding
2. Find/Replace dialog
3. Preferences window
4. Status bar
5. Multiple selections
6. Column editing mode

## Technical Notes

### ScintillaView Initialization

ScintillaView is a drop-in replacement for NSTextView with additional capabilities:

```objc
// Create
ScintillaView* view = [[ScintillaView alloc] initWithFrame:frame];

// Configure via Scintilla messages
[view setGeneralProperty:SCI_PROPERTY parameter:param value:value];
[view setStringProperty:SCI_PROPERTY parameter:param value:@"string"];
[view setColorProperty:SCI_PROPERTY parameter:param value:color];

// Or use NSTextView-compatible API
[view setString:@"Hello, World!"];
NSString* text = [view string];
```

### Scintilla Message System

Scintilla uses a message-based API similar to Windows:

- **SCI_*** constants define message IDs (e.g., `SCI_SETTEXT`)
- **SC_*** constants define values (e.g., `SC_CP_UTF8`)
- **STYLE_*** constants define style numbers (e.g., `STYLE_DEFAULT`)

All defined in `scintilla/include/Scintilla.h` (1522 lines, 2000+ messages)

### Memory Management

- ScintillaView uses ARC (Automatic Reference Counting)
- No manual retain/release needed
- Internal scroll view is managed by ScintillaView
- Proper cleanup in `dealloc` and `windowWillClose:`

## References

### Documentation
- **Scintilla API**: `scintilla/include/Scintilla.h`
- **ScintillaView API**: `scintilla/cocoa/ScintillaView.h`
- **Working Example**: `scintilla/cocoa/ScintillaTest/AppController.mm`

### Key Files
- **Scintilla Core**: `scintilla/src/` (37 files)
- **Cocoa Backend**: `scintilla/cocoa/ScintillaCocoa.mm` (97KB)
- **Platform Layer**: `scintilla/cocoa/PlatCocoa.mm` (74KB)
- **Lexers**: `lexilla/lexers/` (127 language lexers)

---

**Phase 4A Status:** ✅ Code Complete, ⏳ Awaiting Build/Test  
**Blocked By:** CMake installation  
**Next Action:** Install CMake and build project  
**Estimated Time to Complete:** 5-10 minutes (after CMake install)
