// Copyright (C) 2026 Berk Kivircik. SPDX-License-Identifier: GPL-3.0-only
// Draw the native NoteX application icon using AppKit, without external assets.
import AppKit

let folder = URL(fileURLWithPath: CommandLine.arguments[1], isDirectory: true)
try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true)
for size in [16, 32, 128, 256, 512] {
    for scale in [1, 2] {
        let pixels = size * scale
        let bitmap = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: pixels, pixelsHigh: pixels,
            bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
            colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0)!
        NSGraphicsContext.saveGraphicsState()
        NSGraphicsContext.current = NSGraphicsContext(bitmapImageRep: bitmap)
        let transform = NSAffineTransform()
        transform.scale(by: CGFloat(pixels) / 1024)
        transform.concat()
        NSColor(calibratedRed: 0.10, green: 0.15, blue: 0.20, alpha: 1).setFill()
        NSBezierPath(roundedRect: NSRect(x: 48, y: 48, width: 928, height: 928), xRadius: 210, yRadius: 210).fill()
        NSColor(calibratedRed: 0.42, green: 0.83, blue: 0.94, alpha: 1).setStroke()
        let letter = NSBezierPath()
        letter.lineWidth = 104
        letter.lineCapStyle = .round
        letter.lineJoinStyle = .round
        letter.move(to: NSPoint(x: 300, y: 290))
        letter.line(to: NSPoint(x: 300, y: 734))
        letter.line(to: NSPoint(x: 724, y: 290))
        letter.line(to: NSPoint(x: 724, y: 734))
        letter.stroke()
        NSColor(calibratedRed: 1, green: 0.72, blue: 0.38, alpha: 1).setFill()
        NSBezierPath(roundedRect: NSRect(x: 280, y: 172, width: 464, height: 40), xRadius: 20, yRadius: 20).fill()
        NSGraphicsContext.restoreGraphicsState()
        let name = "icon_\(size)x\(size)" + (scale == 2 ? "@2x" : "") + ".png"
        try bitmap.representation(using: .png, properties: [:])!.write(to: folder.appendingPathComponent(name))
    }
}
