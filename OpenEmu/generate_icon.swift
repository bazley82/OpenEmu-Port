import Cocoa

func generateIconSet() {
    let scriptURL = URL(fileURLWithPath: CommandLine.arguments[0])
    let rootURL = scriptURL.deletingLastPathComponent().deletingLastPathComponent()
    let assetURL = rootURL.appendingPathComponent("OpenEmu/Graphics.xcassets/OpenEmu.appiconset")
    let iconsetURL = rootURL.appendingPathComponent("OpenEmu/OpenEmu.iconset")
    
    print("Base Directory: \(rootURL.path)")
    print("Target Asset Path: \(assetURL.path)")

    try? FileManager.default.createDirectory(at: iconsetURL, withIntermediateDirectories: true)

    // The sizes we need to generate to cover all variants
    let targetSizes: [CGFloat] = [16, 32, 64, 128, 256, 512, 1024]
    
    for size in targetSizes {
        autoreleasepool {
            let image = NSImage(size: NSSize(width: size, height: size))
            image.lockFocus()
            let context = NSGraphicsContext.current!.cgContext
            drawN64Controller(in: context, size: size)
            image.unlockFocus()
            
            let tiffData = image.tiffRepresentation!
            let bitmap = NSBitmapImageRep(data: tiffData)!
            let pngData = bitmap.representation(using: .png, properties: [:])!
            
            // 1. iconset for iconutil
            let nameMap: [CGFloat: String] = [
                16: "icon_16x16.png",
                32: "icon_16x16@2x.png",
                64: "icon_32x32@2x.png",
                128: "icon_128x128.png",
                256: "icon_128x128@2x.png",
                512: "icon_512x512.png",
                1024: "icon_512x512@2x.png"
            ]
            if let iconsetName = nameMap[size] {
                try! pngData.write(to: iconsetURL.appendingPathComponent(iconsetName))
                if size == 32 { try? pngData.write(to: iconsetURL.appendingPathComponent("icon_32x32.png")) }
                if size == 256 { try? pngData.write(to: iconsetURL.appendingPathComponent("icon_256x256.png")) }
            }
            
            // 2. Asset Catalog Injection
            let assetBase = "icon-\(Int(size))"
            try? pngData.write(to: assetURL.appendingPathComponent("\(assetBase)-srgb.png"))
            try? pngData.write(to: assetURL.appendingPathComponent("\(assetBase)-p3.png"))
        }
    }
    
    print("Iconset and Asset Catalog updated successfully.")
}

func drawN64Controller(in context: CGContext, size: CGFloat) {
    let scale = size / 1024.0
    
    // Background Circle
    let bgPath = CGPath(ellipseIn: CGRect(x: 10*scale, y: 10*scale, width: 1004*scale, height: 1004*scale), transform: nil)
    context.addPath(bgPath)
    context.clip()
    
    // Deeper Retro Gradient
    let bgColors = [
        NSColor(red: 0.1, green: 0.05, blue: 0.2, alpha: 1.0).cgColor,
        NSColor(red: 0.2, green: 0.1, blue: 0.5, alpha: 1.0).cgColor
    ]
    let bgGradient = CGGradient(colorsSpace: CGColorSpaceCreateDeviceRGB(), colors: bgColors as CFArray, locations: [0.0, 1.0])!
    context.drawLinearGradient(bgGradient, start: CGPoint(x: 0, y: size), end: CGPoint(x: size, y: 0), options: [])

    // N64 Controller Body (Classic Grey)
    context.saveGState()
    context.setShadow(offset: CGSize(width: 0, height: -10*scale), blur: 40*scale, color: NSColor.black.withAlphaComponent(0.7).cgColor)
    
    let path = CGMutablePath()
    path.addRoundedRect(in: CGRect(x: 180*scale, y: 450*scale, width: 664*scale, height: 350*scale), cornerWidth: 120*scale, cornerHeight: 120*scale)
    path.addRoundedRect(in: CGRect(x: 200*scale, y: 120*scale, width: 160*scale, height: 500*scale), cornerWidth: 80*scale, cornerHeight: 80*scale)
    path.addRoundedRect(in: CGRect(x: 432*scale, y: 80*scale, width: 160*scale, height: 550*scale), cornerWidth: 80*scale, cornerHeight: 80*scale)
    path.addRoundedRect(in: CGRect(x: 664*scale, y: 120*scale, width: 160*scale, height: 500*scale), cornerWidth: 80*scale, cornerHeight: 80*scale)
    
    context.addPath(path)
    context.setFillColor(NSColor(white: 0.8, alpha: 1.0).cgColor)
    context.fillPath()
    context.restoreGState()

    // Joystick
    drawEllipse(in: context, rect: CGRect(x: 452*scale, y: 320*scale, width: 120*scale, height: 120*scale), color: .darkGray)
    drawEllipse(in: context, rect: CGRect(x: 472*scale, y: 340*scale, width: 80*scale, height: 80*scale), color: .lightGray)

    // A/B Buttons (Vibrant)
    drawEllipse(in: context, rect: CGRect(x: 680*scale, y: 520*scale, width: 90*scale, height: 90*scale), color: .systemBlue)
    drawEllipse(in: context, rect: CGRect(x: 760*scale, y: 600*scale, width: 90*scale, height: 90*scale), color: .systemGreen)
    
    // C-Buttons (Yellow)
    let yell = NSColor.systemYellow
    drawEllipse(in: context, rect: CGRect(x: 735*scale, y: 740*scale, width: 50*scale, height: 50*scale), color: yell) // Up
    drawEllipse(in: context, rect: CGRect(x: 735*scale, y: 670*scale, width: 50*scale, height: 50*scale), color: yell) // Down
    drawEllipse(in: context, rect: CGRect(x: 700*scale, y: 705*scale, width: 50*scale, height: 50*scale), color: yell) // Left
    drawEllipse(in: context, rect: CGRect(x: 770*scale, y: 705*scale, width: 50*scale, height: 50*scale), color: yell) // Right

    // Start Button (Red)
    drawEllipse(in: context, rect: CGRect(x: 487*scale, y: 620*scale, width: 50*scale, height: 50*scale), color: .systemRed)

    // Premium Gloss Effect
    let glossPath = CGPath(ellipseIn: CGRect(x: 40*scale, y: 650*scale, width: 944*scale, height: 350*scale), transform: nil)
    context.addPath(glossPath)
    context.setFillColor(NSColor(white: 1.0, alpha: 0.3).cgColor)
    context.fillPath()
}

func drawEllipse(in context: CGContext, rect: CGRect, color: NSColor) {
    context.addEllipse(in: rect)
    context.setFillColor(color.cgColor)
    context.fillPath()
}

generateIconSet()
