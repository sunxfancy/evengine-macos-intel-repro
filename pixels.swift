import AppKit
import Foundation
let image = NSBitmapImageRep(data: try Data(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1])))!
func pixel(_ x: Double, _ y: Double) -> NSColor {
    return image.colorAt(x: Int(x * Double(image.pixelsWide) / 640.0), y: Int(y * Double(image.pixelsHigh) / 400.0))!.usingColorSpace(.deviceRGB)!
}
let map = pixel(54, 54)
let solid = pixel(400, 80)
let background = pixel(600, 350)
var redCount = 0
var blueCount = 0
for y in stride(from: 0, to: image.pixelsHigh, by: 4) {
    for x in stride(from: 0, to: image.pixelsWide, by: 4) {
        let c = image.colorAt(x: x, y: y)!.usingColorSpace(.deviceRGB)!
        if c.redComponent > 0.8 && c.greenComponent < 0.2 && c.blueComponent < 0.2 { redCount += 1 }
        if c.blueComponent > 0.8 && c.greenComponent < 0.2 && c.redComponent < 0.2 { blueCount += 1 }
    }
}
let result: [String: Any] = [
    "width": image.pixelsWide, "height": image.pixelsHigh,
    "map_rgb": [map.redComponent, map.greenComponent, map.blueComponent],
    "background_rgb": [background.redComponent, background.greenComponent, background.blueComponent],
    "red_samples": redCount, "blue_samples": blueCount,
    "map_difference": abs(map.redComponent - 0.08) + abs(map.greenComponent - 0.09) + abs(map.blueComponent - 0.12),
    "solid_red": solid.redComponent
]
print(String(data: try JSONSerialization.data(withJSONObject: result, options: [.sortedKeys]), encoding: .utf8)!)
