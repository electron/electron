import Foundation
import Cocoa
import os.log

class DummyManager {
    struct DefinedDummy {
        var dummy: Dummy
    }

    static var definedDummies: [Int: DefinedDummy] = [:]
    static var dummyCounter: Int = 0

    static func createDummy(_ dummyDefinition: DummyDefinition, isPortrait _: Bool = false, serialNum: UInt32 = 0, doConnect: Bool = true) -> Int? {
        let dummy = Dummy(dummyDefinition: dummyDefinition, serialNum: serialNum, doConnect: doConnect)

        if !dummy.isConnected {
            print("[DummyManager.createDummy:\(#line)] Failed to create virtual display - not connected")
            return nil
        }
        self.dummyCounter += 1
        self.definedDummies[self.dummyCounter] = DefinedDummy(dummy: dummy)
        return self.dummyCounter
    }

    static func discardDummyByNumber(_ number: Int) {
        if let definedDummy = self.definedDummies[number] {
            if definedDummy.dummy.isConnected {
                definedDummy.dummy.disconnect()
            }
        }
        self.definedDummies[number] = nil
    }

    static func forceCleanup() {
        for (_, definedDummy) in self.definedDummies {
            if definedDummy.dummy.isConnected {
                definedDummy.dummy.virtualDisplay = nil
                definedDummy.dummy.displayIdentifier = 0
                definedDummy.dummy.isConnected = false
            }
        }
        
        self.definedDummies.removeAll()
        self.dummyCounter = 0
    
        var config: CGDisplayConfigRef? = nil
        if CGBeginDisplayConfiguration(&config) == .success {
            CGCompleteDisplayConfiguration(config, .permanently)
        }
        
        usleep(2000000)
        
        if CGBeginDisplayConfiguration(&config) == .success {
            CGCompleteDisplayConfiguration(config, .forSession)
        }
    }
}

struct DummyDefinition {
    let aspectWidth, aspectHeight, multiplierStep, minMultiplier, maxMultiplier: Int
    let refreshRates: [Double]
    let description: String
    let addSeparatorAfter: Bool

    init(_ aspectWidth: Int, _ aspectHeight: Int, _ step: Int, _ refreshRates: [Double], _ description: String, _ addSeparatorAfter: Bool = false) {
        let minX: Int = 720
        let minY: Int = 720
        let maxX: Int = 8192
        let maxY: Int = 8192
        let minMultiplier = max(Int(ceil(Float(minX) / (Float(aspectWidth) * Float(step)))), Int(ceil(Float(minY) / (Float(aspectHeight) * Float(step)))))
        let maxMultiplier = min(Int(floor(Float(maxX) / (Float(aspectWidth) * Float(step)))), Int(floor(Float(maxY) / (Float(aspectHeight) * Float(step)))))
        
        self.aspectWidth = aspectWidth
        self.aspectHeight = aspectHeight
        self.minMultiplier = minMultiplier
        self.maxMultiplier = maxMultiplier
        self.multiplierStep = step
        self.refreshRates = refreshRates
        self.description = description
        self.addSeparatorAfter = addSeparatorAfter
    }
}

class Dummy: Equatable {
    var virtualDisplay: CGVirtualDisplay?
    var dummyDefinition: DummyDefinition
    let serialNum: UInt32
    var isConnected: Bool = false
    var displayIdentifier: CGDirectDisplayID = 0

    static func == (lhs: Dummy, rhs: Dummy) -> Bool {
        lhs.serialNum == rhs.serialNum
    }

    init(dummyDefinition: DummyDefinition, serialNum: UInt32 = 0, doConnect: Bool = true) {
        var storedSerialNum: UInt32 = serialNum
        if storedSerialNum == 0 {
            storedSerialNum = UInt32.random(in: 0 ... UInt32.max)
        }
        self.dummyDefinition = dummyDefinition
        self.serialNum = storedSerialNum
        if doConnect {
            _ = self.connect()
        }
    }

    func getName() -> String {
        "Dummy \(self.dummyDefinition.description.components(separatedBy: " ").first ?? self.dummyDefinition.description)"
    }

    func connect() -> Bool {
        if self.virtualDisplay != nil || self.isConnected {
            self.disconnect()
        }
        let name: String = self.getName()
        if let virtualDisplay = Dummy.createVirtualDisplay(self.dummyDefinition, name: name, serialNum: self.serialNum) {
            self.virtualDisplay = virtualDisplay
            self.displayIdentifier = virtualDisplay.displayID
            self.isConnected = true
            print("[Dummy.connect:\(#line)] Successfully connected virtual display: \(name)")
            return true
        } else {
            print("[Dummy.connect:\(#line)] Failed to connect virtual display: \(name)")
            return false
        }
    }

    func disconnect() {
        self.virtualDisplay = nil
        self.isConnected = false
        print("[Dummy.disconnect:\(#line)] Disconnected virtual display: \(self.getName())")
    }

   private static func waitForDisplayRegistration(_ displayId: CGDirectDisplayID) -> Bool {
        for _ in 0..<20 {
            var count: UInt32 = 0, displays = [CGDirectDisplayID](repeating: 0, count: 32)
            if CGGetActiveDisplayList(32, &displays, &count) == .success && displays[0..<Int(count)].contains(displayId) {
                return true
            }
            usleep(100000)
        }
        print("[Dummy.waitForDisplayRegistration:\(#line)] Failed to register virtual display: \(displayId)")
        return false
    }

    static func createVirtualDisplay(_ definition: DummyDefinition, name: String, serialNum: UInt32, hiDPI: Bool = false) -> CGVirtualDisplay? {
        if let descriptor = CGVirtualDisplayDescriptor() {
            descriptor.queue = DispatchQueue.global(qos: .userInteractive)
            descriptor.name = name
            descriptor.whitePoint = CGPoint(x: 0.950, y: 1.000)
            descriptor.redPrimary = CGPoint(x: 0.454, y: 0.242)
            descriptor.greenPrimary = CGPoint(x: 0.353, y: 0.674)
            descriptor.bluePrimary = CGPoint(x: 0.157, y: 0.084)
            descriptor.maxPixelsWide = UInt32(definition.aspectWidth * definition.multiplierStep * definition.maxMultiplier)
            descriptor.maxPixelsHigh = UInt32(definition.aspectHeight * definition.multiplierStep * definition.maxMultiplier)
            let diagonalSizeRatio: Double = (24 * 25.4) / sqrt(Double(definition.aspectWidth * definition.aspectWidth + definition.aspectHeight * definition.aspectHeight))
            descriptor.sizeInMillimeters = CGSize(width: Double(definition.aspectWidth) * diagonalSizeRatio, height: Double(definition.aspectHeight) * diagonalSizeRatio)
            descriptor.serialNum = serialNum
            descriptor.productID = UInt32(min(definition.aspectWidth - 1, 255) * 256 + min(definition.aspectHeight - 1, 255))
            descriptor.vendorID = UInt32(0xF0F0)
            if let display = CGVirtualDisplay(descriptor: descriptor) {
                var modes = [CGVirtualDisplayMode?](repeating: nil, count: definition.maxMultiplier - definition.minMultiplier + 1)
                for multiplier in definition.minMultiplier ... definition.maxMultiplier {
                    for refreshRate in definition.refreshRates {
                        let width = UInt32(definition.aspectWidth * multiplier * definition.multiplierStep)
                        let height = UInt32(definition.aspectHeight * multiplier * definition.multiplierStep)
                        modes[multiplier - definition.minMultiplier] = CGVirtualDisplayMode(width: width, height: height, refreshRate: refreshRate)!
                    }
                }
                if let settings = CGVirtualDisplaySettings() {
                    settings.hiDPI = hiDPI ? 1 : 0
                    settings.modes = modes as [Any]
                    if display.applySettings(settings) {
                        return waitForDisplayRegistration(display.displayID) ? display : nil
                    }
                }
            }
        }
        return nil
    }
}