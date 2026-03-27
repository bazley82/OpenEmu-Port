import Foundation
import AppKit
import CoreImage

func removeWhiteBackground(at path: String, outPath: String) {
    guard let image = NSImage(contentsOfFile: path) else {
        print("Failed to load image at \(path)")
        return
    }
    
    guard let tiffData = image.tiffRepresentation,
          let ciImage = CIImage(data: tiffData) else {
        print("Failed to convert image to CIImage")
        return
    }
    
    // Create a mask for white-ish pixels
    // White is 1.0, 1.0, 1.0. We want to remove anything very close to white.
    let chromaKeyFilter = CIFilter(name: "CIColorCube")!
    let size = 64
    var cubeData = [Float](repeating: 0, count: size * size * size * 4)
    var offset = 0
    
    for z in 0..<size {
        for y in 0..<size {
            for x in 0..<size {
                let r = Float(x) / Float(size - 1)
                let g = Float(y) / Float(size - 1)
                let b = Float(z) / Float(size - 1)
                
                // If the color is very bright (near white), make it transparent
                let threshold: Float = 0.85
                let alpha: Float = (r > threshold && g > threshold && b > threshold) ? 0.0 : 1.0
                
                cubeData[offset] = r * alpha
                cubeData[offset + 1] = g * alpha
                cubeData[offset + 2] = b * alpha
                cubeData[offset + 3] = alpha
                offset += 4
            }
        }
    }
    
    let data = Data(bytes: cubeData, count: cubeData.count * MemoryLayout<Float>.size)
    chromaKeyFilter.setValue(size, forKey: "inputCubeDimension")
    chromaKeyFilter.setValue(data, forKey: "inputCubeData")
    chromaKeyFilter.setValue(ciImage, forKey: kCIInputImageKey)
    
    guard let outputImage = chromaKeyFilter.outputImage else {
        print("Failed to apply filter")
        return
    }
    
    let rep = NSBitmapImageRep(ciImage: outputImage)
    guard let pngData = rep.representation(using: .png, properties: [:]) else {
        print("Failed to create PNG data")
        return
    }
    
    do {
        try pngData.write(to: URL(fileURLWithPath: outPath))
        print("Successfully processed \(path) -> \(outPath)")
    } catch {
        print("Failed to write to \(outPath): \(error)")
    }
}

let args = ProcessInfo.processInfo.arguments
if args.count < 3 {
    print("Usage: swift process_icon.swift <input> <output>")
    exit(1)
}

removeWhiteBackground(at: args[1], outPath: args[2])
