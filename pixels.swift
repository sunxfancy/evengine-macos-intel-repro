import AppKit
import Foundation
let image = NSBitmapImageRep(data: try Data(contentsOf: URL(fileURLWithPath: CommandLine.arguments[1])))!
func pixel(_ x: Double, _ y: Double) -> NSColor {
    return image.colorAt(x: Int(x * Double(image.pixelsWide) / 640.0), y: Int(y * Double(image.pixelsHigh) / 400.0))!.usingColorSpace(.deviceRGB)!
}
let map = pixel(54, 54)
let solid = pixel(400, 80)
let result: [String: Any] = [
    "width": image.pixelsWide, "height": image.pixelsHigh,
    "map_rgb": [map.redComponent, map.greenComponent, map.blueComponent],
    "map_difference": abs(map.redComponent - 0.08) + abs(map.greenComponent - 0.09) + abs(map.blueComponent - 0.12),
    "solid_red": solid.redComponent
]
print(String(data: try JSONSerialization.data(withJSONObject: result, options: [.sortedKeys]), encoding: .utf8)!)
