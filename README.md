<p align="center">
  <img src="docs/assets/covers/cover-neon-chameleon.png" alt="Notepad++ ReCodera-AI Community Edition" width="100%" style="border-radius: 12px; box-shadow: 0 12px 36px rgba(0,0,0,0.18); max-width: 960px;">
</p>

<div align="center">

# Notepad++ ReCodera-AI Community Edition
### Enterprise-Grade Bidirectional Text Engine & Advanced Typography

[![Release Version](https://img.shields.io/badge/Release-v8.9.8.2-10b981?style=for-the-badge&logo=notepad%2B%2B&logoColor=white&labelColor=0f172a)](https://github.com/ReCodera-AI/notepad-plus-plus/releases)
[![Build Status](https://img.shields.io/badge/Build-Passing%20(3823%20Tests)-10b981?style=for-the-badge&logo=githubactions&logoColor=white&labelColor=0f172a)](https://github.com/ReCodera-AI/notepad-plus-plus/actions)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-38bdf8?style=for-the-badge&logo=windows11&logoColor=white&labelColor=0f172a)](https://github.com/ReCodera-AI/notepad-plus-plus/releases)
[![BiDi Engine](https://img.shields.io/badge/Engine-Uniscribe%20%2B%20DirectWrite-6366f1?style=for-the-badge&logo=windows&logoColor=white&labelColor=0f172a)](docs/releases/v8.9.8.2.md)
[![License](https://img.shields.io/badge/License-GPL%20v3+-f59e0b?style=for-the-badge&labelColor=0f172a)](LICENSE)

<p align="center">
  <em>Careful, AI-assisted precision engineering delivering production-grade multilingual text handling to developers, technical writers, and authors worldwide.</em>
</p>

<p align="center">
  <a href="https://github.com/ReCodera-AI/notepad-plus-plus/releases"><strong>📦 Release Notes</strong></a> &nbsp;&nbsp;•&nbsp;&nbsp;
  <a href="https://recodera-ai.github.io/notepad-plus-plus/"><strong>🌐 Project Portal</strong></a> &nbsp;&nbsp;•&nbsp;&nbsp;
  <a href="docs/releases/v8.9.8.2.md"><strong>📝 Change Specifications</strong></a> &nbsp;&nbsp;•&nbsp;&nbsp;
  <a href="https://github.com/notepad-plus-plus/notepad-plus-plus"><strong>🔗 Upstream Notepad++</strong></a>
</p>

</div>

---

## About This Build

Notepad++ is the renowned open-source Windows text and source code editor created and maintained by Don Ho and upstream contributors at [notepad-plus-plus/notepad-plus-plus](https://github.com/notepad-plus-plus/notepad-plus-plus).

This repository is an independent **ReCodera-AI Community Edition**. It preserves upstream’s battle-tested foundation while introducing architectural enhancements to solve long-standing bidirectional (BiDi) layout, RTL mirroring, and multilingual typography challenges.

---

## Key Enhancements In Version 8.9.8.2

### 1. Natural English & Latin Reading Order in RTL Mode (Microsoft Word Parity)
- **The Challenge:** In mirrored RTL view (`WS_EX_LAYOUTRTL`), Scintilla’s lexical styling divides text into distinct segments. In GDI mode, sequentially drawing segments along the mirrored X-axis inverted the order of Latin words (e.g. `ChatGPT Work Repeatedly Stops Before Completing Tasks` appeared backwards).
- **The Solution:** Implemented `ScreenLineLayoutGDI` backed by Windows Uniscribe (`usp10.dll` via `ScriptItemize` and `ScriptLayout`). Latin runs and tokens are placed with exact visual coordinates, ensuring English flows in natural Left-to-Right sentence order anchored to the right margin—fully matching Microsoft Word behavior.

### 2. End-of-Line Dark Strips & Background Artifact Elimination
- **The Challenge:** Drawing bidirectional backgrounds previously calculated visual-to-logical gaps inaccurately at line terminators, producing black or dark background strips and seams.
- **The Solution:** Corrected the trailing gap calculation within the Scintilla rendering pipeline, guaranteeing smooth, continuous syntax-highlighted backgrounds across all document lines.

### 3. Mixed Arabic & English Rendering in LTR Documents
- **The Challenge:** Arabic text in standard LTR documents suffered from clipping, incorrect run positions, and broken selection highlighting.
- **The Solution:** Leveraged DirectWrite visual interval mapping for text, selection rectangles, whitespace rendering, and margin alignment.

### 4. Per-Document Direction Persistence
- Retains and restores the Right-to-Left (RTL) or Left-to-Right (LTR) display choice for every individual document across buffer switching, file reopening, and application session restores.

### 5. Adaptive Dual-Engine Architecture
- Maintains hardware-accelerated DirectWrite for LTR documents, seamlessly transitioning to a robust, mirrored GDI Uniscribe layout for RTL views without glyph corruption or font inversion.

---

## Verification & Quality Assurance

Every modification undergoes strict algorithmic, unit, and end-to-end integration testing:

- **Scintilla Unit Test Suite:** All **3,823 assertions** across 53 test cases passed with zero regressions.
- **Visual & Layout Harness:** Validated against Microsoft Word rendering standards across pure English, pure Arabic, and mixed-script Markdown/code files.
- **Production Compilation:** Clean Release x64 compilation via Microsoft Visual Studio MSBuild (`0 Errors, 0 Warnings`).

---

## Deployment & Quick Start

The verified Release x64 binary is available directly in the repository:

- **Root Directory:** [`notepad++.exe`](notepad++.exe)
- **Build Output Directory:** [`PowerEditor/bin64/notepad++.exe`](PowerEditor/bin64/notepad++.exe)

### PowerShell Replacement Script (Administrator)

To update your system installation in `C:\Program Files\Notepad++\`:

```powershell
# Run PowerShell as Administrator
$sourceExe = "D:\Test\03 │ ReCodera-AI\notepad-plus-plus\notepad++.exe"
$targetDir = "C:\Program Files\Notepad++"
$targetExe = Join-Path $targetDir "notepad++.exe"
$backupExe = Join-Path $targetDir "notepad++.exe.bak"

Stop-Process -Name "notepad++" -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 1
if (Test-Path $targetExe) { Copy-Item -Path $targetExe -Destination $backupExe -Force }
Copy-Item -Path $sourceExe -Destination $targetExe -Force
Start-Process -FilePath $targetExe
```

---

## Build From Source

To build from source on Windows:

1. Open `PowerEditor/visual.net/notepadPlus.sln` in Visual Studio 2022 / 2026.
2. Select **Release** configuration and **x64** platform.
3. Build the solution, or execute via MSBuild:
   ```cmd
   MSBuild PowerEditor\visual.net\notepadPlus.sln /m /p:configuration="Release" /p:platform="x64"
   ```

Refer to [BUILD.md](BUILD.md) and [CONTRIBUTING.md](CONTRIBUTING.md) for full development environment details.

---

## Source & Ownership Notice

- **ReCodera-AI Community Repository:** <https://github.com/ReCodera-AI/notepad-plus-plus>
- **Official Notepad++ Project:** <https://github.com/notepad-plus-plus/notepad-plus-plus>
- **License:** GNU General Public License v3 or later ([LICENSE](LICENSE))

Notepad++ is the intellectual property of Don Ho and its upstream contributors. This community build is an independent distribution and is not endorsed by the upstream maintainers.
