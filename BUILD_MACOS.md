# Building Notepad++ for macOS

## Prerequisites

### Required Tools
- **macOS 11.0 (Big Sur) or later**
- **Xcode 13.0 or later** (for Apple Silicon support)
- **CMake 3.20 or later**

Install CMake via Homebrew:
```bash
brew install cmake
```

### Supported Architectures
- **ARM64** (Apple Silicon: M1, M2, M3, M4)
- Intel x86_64 support can be added by modifying `CMAKE_OSX_ARCHITECTURES`

## Build Instructions

### 1. Clone the Repository
```bash
git clone https://github.com/notepad-plus-plus/notepad-plus-plus.git notepad-plus-plus_macos
cd notepad-plus-plus_macos
```

### 2. Configure with CMake
```bash
# Create build directory
mkdir build
cd build

# Configure for Apple Silicon (ARM64)
cmake .. -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64

# Alternative: Configure for Intel
# cmake .. -G Xcode -DCMAKE_OSX_ARCHITECTURES=x86_64

# Alternative: Universal binary (both architectures)
# cmake .. -G Xcode -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
```

### 3. Build the Project

**Option A: Using Xcode**
```bash
# Open the generated Xcode project
open NotepadPlusPlus.xcodeproj

# Build from Xcode GUI (Cmd+B)
```

**Option B: Using Command Line**
```bash
# Build Release configuration
cmake --build . --config Release

# Build Debug configuration
cmake --build . --config Debug
```

### 4. Run the Application
```bash
# After building, the app bundle will be in:
# build/bin/notepadpp.app (Unix Makefiles/Ninja generators)
# build/bin/{Debug|Release}/notepadpp.app (Xcode generator)

# Run the app
open bin/notepadpp.app
```

## Current Status

⚠️ **IMPORTANT: This is a work in progress!**

### ✅ Completed
- [x] Phase 1: CMake build system for macOS
- [x] Phase 2: Platform abstraction layer (baseline)
- [x] Phase 3: macOS entry point and native app shell
- [x] Phase 4A: Basic Scintilla integration (editor view replacement)

### 🚧 In Progress (Next)
- [ ] Phase 4B: Syntax highlighting and lexer wiring
  - Language detection by extension
  - Theme/color configuration
  - Scintilla notification delegate and modified tracking

### ❌ Not Yet Implemented
- [ ] Phase 4C: Advanced editor features (folding, status bar, preferences)
- [ ] Multi-document tab model
- [ ] Find/Replace advanced UI parity
- [ ] Plugin system (dylib loading)

## Build Verification (Phase 4A)

Latest local verification completed with `make -C build`:
- `scintilla`, `lexilla`, and `notepadpp` targets build successfully
- App bundle produced at `build/bin/notepadpp.app`
- Binary architecture:
  - `Mach-O 64-bit executable arm64`

## Known Issues

1. **Manual GUI validation pending**: command-line build is green, but interactive checks still need to be run from the app UI.
2. **Deprecated API warnings**: some C++ conversion and AppKit APIs emit warnings but do not block build.
3. **BoostRegex disabled for macOS path**: currently optional and not linked by the app target.

## Next Steps

1. Start `Phase 4B` (syntax highlighting pipeline).
2. Execute the full GUI checklist and capture outcomes in a test report.
3. Keep docs in sync (`MACOS_PORT_STATUS.md`, phase completion files).

## Development Workflow

### Testing Build Configuration
```bash
# Clean build
rm -rf build
mkdir build && cd build

# Configure and build
cmake .. -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build . --config Debug

# Check for errors
echo $?  # Should be 0 if successful
```

### Checking Binary Architecture
```bash
# Verify the binary is built for correct architecture
file bin/notepadpp.app/Contents/MacOS/notepadpp

# Expected output for ARM64:
# Mach-O 64-bit executable arm64
```

## Troubleshooting

### CMake Configuration Fails
- Ensure Xcode Command Line Tools are installed: `xcode-select --install`
- Verify CMake version: `cmake --version` (should be 3.20+)

### Link Errors From Scintilla Symbols
- Ensure `scintilla/CMakeLists.txt` includes `ChangeHistory.cxx`, `UndoHistory.cxx`, and `Geometry.cxx`
- Ensure `QuartzCore` is linked on macOS

## Contributing

This is a massive porting effort. Contributions are welcome!

Priority areas:
1. Platform abstraction layer implementation
2. Cocoa UI components
3. File system operations
4. Testing on different macOS versions and Apple Silicon variants

## License

Notepad++ is licensed under GPL v3. See LICENSE file for details.
