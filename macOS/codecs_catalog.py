"""Build explicit single-byte encoding maps for the shared Windows code pages.
Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
"""
def encoding_tables():
    pages = [437, 720, 737, 775, 850, 852, 855, 857, 858, 860, 861, 862, 863, 865, 866, 869,
             874, 1250, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258,
             28591, 28592, 28593, 28594, 28595, 28596, 28597, 28598, 28599, 28600, 28603, 28604, 28605,
             20866, 21866, 10007]
    result = []
    for page in pages:
        codec = f"iso8859_{page - 28590}" if 28591 <= page <= 28605 else {20866: "koi8_r", 21866: "koi8_u", 10007: "mac_cyrillic"}.get(page, f"cp{page}")
        mapping = []
        for byte in range(256):
            try:
                mapping.append(ord(bytes([byte]).decode(codec, errors="strict")))
            except UnicodeDecodeError:
                mapping.append(-1)
        result.append({"encoding": 0x100000000 + page, "name": f"{codec.upper().replace('_', '-')} (CP{page})", "mapping": mapping})
    return result
