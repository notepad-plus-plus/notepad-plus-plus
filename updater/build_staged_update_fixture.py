# Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""Build an isolated macOS E2E helper with a fixture transport for unpublished assets."""

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

UPDATER = Path(__file__).resolve().parent
TRANSPORT = r"""
type stagedReleaseTransport struct {
    real http.RoundTripper
}

func (t stagedReleaseTransport) RoundTrip(r *http.Request) (*http.Response, error) {
    const api = "https://api.github.com/repos/" + repository + "/releases?per_page=100&page=1"
    const name = "NoteQu-update-darwin-arm64.json"
    const version = __VERSION__
    const directory = __DIRECTORY__
    const pinned = downloadBase + "v" + version + "/"
    const archive = "NoteQu-" + version + "-darwin-arm64-update.zip"

    response := &http.Response{Request: r, Header: make(http.Header), Body: http.NoBody}
    switch r.URL.String() {
    case api:
        response.StatusCode = http.StatusForbidden
        response.Header.Set("X-RateLimit-Remaining", "0")
    case releaseBase + "latest/download/" + name:
        response.StatusCode = http.StatusFound
        response.Header.Set("Location", pinned + name)
    case pinned + name, pinned + name + ".sig", pinned + archive:
        data, err := os.Open(filepath.Join(directory, path.Base(r.URL.Path)))
        if err != nil {
            return nil, err
        }
        info, err := data.Stat()
        if err != nil {
            data.Close()
            return nil, err
        }
        response.StatusCode = http.StatusOK
        response.Body = data
        response.ContentLength = info.Size()
    default:
        // Authenticate the original 1.7.3 bundle against the actual public inventory.
        return t.real.RoundTrip(r)
    }
    return response, nil
}

func client() *http.Client {
    c := productionClient()
    c.Transport = stagedReleaseTransport{real: c.Transport}
    return c
}

func productionClient() *http.Client {
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", required=True)
    parser.add_argument("--assets", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not re.fullmatch(r"(0|[1-9][0-9]*)(\.(0|[1-9][0-9]*)){2}", args.version):
        parser.error("expected a numeric release version")
    assets = args.assets.resolve(strict=True)
    names = ("NoteQu-update-darwin-arm64.json", "NoteQu-update-darwin-arm64.json.sig",
             f"NoteQu-{args.version}-darwin-arm64-update.zip")
    if any(not (assets / name).is_file() for name in names):
        parser.error("the staged macOS inventory, signature, and archive are required")

    original = UPDATER / "core.go"
    source = original.read_text(encoding="utf-8")
    anchor = "func client() *http.Client {"
    if source.count(anchor) != 1:
        raise RuntimeError("Could not locate the updater HTTP client.")
    transport = TRANSPORT.replace("__VERSION__", json.dumps(args.version)).replace("__DIRECTORY__", json.dumps(str(assets)))
    with tempfile.TemporaryDirectory(prefix="notequ-staged-update-build-") as directory:
        work = Path(directory)
        replacement = work / "core.go"
        replacement.write_text(source.replace(anchor, transport), encoding="utf-8")
        overlay = work / "overlay.json"
        overlay.write_text(json.dumps({"Replace": {str(original): str(replacement)}}), encoding="utf-8")
        output = args.output.resolve()
        subprocess.run(["go", "build", "-trimpath", "-overlay", str(overlay), "-o", str(output), "."],
                       cwd=UPDATER, check=True)
        subprocess.run(["codesign", "--force", "--sign", "-", str(output)], check=True)


if __name__ == "__main__":
    main()

