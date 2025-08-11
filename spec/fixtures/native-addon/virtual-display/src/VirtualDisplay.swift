import Foundation
import Cocoa
import os.log

@objc public class VirtualDisplay: NSObject {
    @objc public static func create(width: Int, height: Int, x: Int, y: Int) -> Int {
        let refreshRates: [Double] = [60.0] // Always 60Hz default
        let description = "\(width)x\(height) Display"
        let definition = DummyDefinition(width, height, 1, refreshRates, description, false)
        let displayId = DummyManager.createDummy(definition) ?? 0
        positionDisplay(displayId: displayId, x: x, y: y)

       return displayId
    }
    
    @objc public static func destroy(id: Int) -> Bool {
        DummyManager.discardDummyByNumber(id)
        return true
    }

    private static func positionDisplay(displayId: Int, x: Int, y: Int) {
        guard let definedDummy = DummyManager.definedDummies[displayId],
            definedDummy.dummy.isConnected else {
            os_log("VirtualDisplay: Cannot position display %{public}@: display not found or not connected", type: .error, "\(displayId)")
            return
        }
        
        let cgDisplayId = definedDummy.dummy.displayIdentifier

        var config: CGDisplayConfigRef? = nil
        let beginResult = CGBeginDisplayConfiguration(&config)
        
        if beginResult != .success {
            os_log("VirtualDisplay: Cannot position display, failed to begin display configuration via CGBeginDisplayConfiguration: error %{public}@", type: .error, "\(beginResult.rawValue)")
            return
        }

        let configResult = CGConfigureDisplayOrigin(config, cgDisplayId, Int32(x), Int32(y))
        
        if configResult != .success {
            os_log("VirtualDisplay: Cannot position display, failed to configure display origin via CGConfigureDisplayOrigin: error %{public}@", type: .error, "\(configResult.rawValue)")
            CGCancelDisplayConfiguration(config)
            return
        }

        let completeResult = CGCompleteDisplayConfiguration(config, .permanently)
        
        if completeResult == .success {
            os_log("VirtualDisplay: Successfully positioned display %{public}@ at (%{public}@, %{public}@)", type: .info, "\(displayId)", "\(x)", "\(y)")
        } else {
            os_log("VirtualDisplay: Cannot position display, failed to complete display configuration via CGCompleteDisplayConfiguration: error %{public}@", type: .error, "\(completeResult.rawValue)")
        }
    }
}