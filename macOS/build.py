#!/usr/bin/env python3
"""Build the native NoteQu application using only Xcode Command Line Tools.

Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""
import concurrent.futures
import os
import json
from languages import catalog
from codecs_catalog import encoding_tables
from pathlib import Path
import plistlib
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
VERSION = "1.7.4"
BUILD = "1"

def main():
    if sys.platform != "darwin":
        raise SystemExit("Build this target on macOS with Xcode Command Line Tools.")
    output = ROOT / "dist" / "macOS"
    if output.exists():
        raise SystemExit("dist/macOS already exists. Use a clean checkout or a new output directory.")
    bundle = output / "NoteQu.app" / "Contents"
    resources = bundle / "Resources"
    binary = bundle / "MacOS" / "NoteQu"
    binary.parent.mkdir(parents=True)
    resources.mkdir(parents=True)
    objects = output / "objects"
    objects.mkdir()
    sdk = subprocess.check_output(["xcrun", "--show-sdk-path"], text=True).strip()
    flags = ["-std=c++20", "-O2", "-DNDEBUG", "-DLEXILLA_NO_EXPORT", "-arch", "arm64",
             "-mmacosx-version-min=14.0", "-isysroot", sdk, "-Wno-deprecated-declarations"]
    for include in ["scintilla/include", "scintilla/src", "scintilla/cocoa",
                    "lexilla/include", "lexilla/lexlib", "boostregex"]:
        flags += ["-I", str(ROOT / include)]
    sources = sorted((ROOT / "macOS").glob("*.mm"))
    # Report all native front-end errors together before compiling shared engines.
    failures = []
    for source in sources:
        result = subprocess.run(["xcrun", "clang++", *flags, "-fobjc-arc", "-fsyntax-only", str(source)])
        if result.returncode:
            failures.append(source.name)
    if failures:
        raise SystemExit("Native syntax checks failed: " + ", ".join(failures))
    sources += sorted((ROOT / "scintilla/src").glob("*.cxx"))
    sources += sorted((ROOT / "scintilla/cocoa").glob("*.mm"))
    sources += sorted((ROOT / "lexilla/lexlib").glob("*.cxx"))
    sources += sorted((ROOT / "lexilla/lexers").glob("Lex*.cxx"))
    sources += [ROOT / "lexilla/src/Lexilla.cxx"]
    def compile_source(source):
        target = objects / ("_".join(source.relative_to(ROOT).parts) + ".o")
        command = ["xcrun", "clang++", *flags]
        if source.suffix == ".mm":
            command += ["-fobjc-arc"]
        subprocess.run([*command, "-c", str(source), "-o", str(target)], check=True)
        return target
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
        compiled = list(pool.map(compile_source, sources))
    subprocess.run(["xcrun", "clang++", *flags, *map(str, compiled), "-framework", "Cocoa",
                    "-framework", "QuartzCore", "-framework", "CoreText", "-o", str(binary)], check=True)
    info = {
        "CFBundleName": "NoteQu", "CFBundleDisplayName": "NoteQu",
        "CFBundleIdentifier": "com.berkkivircik.NoteQu", "CFBundleExecutable": "NoteQu",
        "CFBundlePackageType": "APPL", "CFBundleShortVersionString": VERSION,
        "CFBundleVersion": BUILD, "LSMinimumSystemVersion": "14.0",
        "CFBundleIconFile": "NoteQu.icns",
        "NSHighResolutionCapable": True, "NSPrincipalClass": "NSApplication",
        "NSHumanReadableCopyright": "Copyright © 2026 Berk Kivircik and contributors. GPLv3.",
        "CFBundleDocumentTypes": [{"CFBundleTypeName": "Text document", "CFBundleTypeRole": "Editor",
                                   "LSHandlerRank": "Alternate", "LSItemContentTypes": ["public.text", "public.source-code"]}],
    }
    with (bundle / "Info.plist").open("wb") as file:
        plistlib.dump(info, file)
    iconset = output / "NoteQu.iconset"
    subprocess.run(["xcrun", "swift", str(ROOT / "macOS/make_icon.swift"), str(iconset)], check=True)
    subprocess.run(["iconutil", "-c", "icns", str(iconset), "-o", str(resources / "NoteQu.icns")], check=True)
    for name in ["LICENSE", "THIRD_PARTY_NOTICES.md", "MODIFICATIONS.md", "UPDATING.md", "CODE_SIGNING.md"]:
        shutil.copy2(ROOT / name, resources / name)
    shutil.copytree(ROOT / "LICENSES", resources / "LICENSES")
    shutil.copytree(ROOT / "PowerEditor/installer/functionList", resources / "functionList")
    shutil.copytree(ROOT / "PowerEditor/installer/APIs", resources / "APIs")
    shutil.copy2(ROOT / "scintilla/License.txt", resources / "Scintilla-LICENSE.txt")
    shutil.copy2(ROOT / "lexilla/License.txt", resources / "Lexilla-LICENSE.txt")
    shutil.copy2(ROOT / "macOS/README.md", resources / "README.md")
    (resources / "encodings.json").write_text(json.dumps(encoding_tables()), encoding="utf-8")
    (resources / "languages.json").write_text(json.dumps(catalog(ROOT), ensure_ascii=False), encoding="utf-8")
    shutil.copytree(ROOT / "PowerEditor/Packaging/NoteQu/assets/fonts", resources / "Fonts")
    for source in (ROOT / "scintilla/cocoa/res").glob("*.png"):
        shutil.copy2(source, resources / source.name)
    # Xcode normally combines these PNG pairs into the TIFFs Cocoa loads.
    for stem in ["mac_cursor_busy", "mac_cursor_flipped", "info_bar_bg"]:
        subprocess.run(["tiffutil", "-cathidpicheck", str(resources / f"{stem}.png"),
                        str(resources / f"{stem}@2x.png"), "-out", str(resources / f"{stem}.tiff")], check=True)
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    (resources / "SOURCE.txt").write_text(
        f"NoteQu {VERSION} native macOS preview {BUILD}\nSource revision: {revision}\n"
        f"https://github.com/berkkivircik/NoteQu/tree/{revision}\n"
        "Build: python3 macOS/build.py\nLicense: GNU GPL version 3\n", encoding="utf-8")
    subprocess.run(["go", "build", "-trimpath", "-ldflags=-s -w", "-o", str(bundle / "MacOS/NoteQuUpdater"), "."], cwd=ROOT / "updater", check=True)
    subprocess.run(["codesign", "--force", "--sign", "-", str(bundle / "MacOS/NoteQuUpdater")], check=True)
    subprocess.run(["codesign", "--force", "--sign", "-", str(bundle.parent)], check=True)
    subprocess.run(["codesign", "--verify", "--strict", str(bundle.parent)], check=True)
    subprocess.run(["file", str(binary)], check=True)
    print(bundle.parent, flush=True)

if __name__ == "__main__":
    main()
