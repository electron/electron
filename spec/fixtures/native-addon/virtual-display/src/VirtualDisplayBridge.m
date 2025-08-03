#import "VirtualDisplayBridge.h"
#import "../build_swift/virtual_display-Swift.h"

@implementation VirtualDisplayBridge

+ (NSInteger)addDisplay:(int)width height:(int)height {
    return [VirtualDisplay addDisplayWithWidth:width height:height];
}

+ (BOOL)removeDisplay:(NSInteger)displayId {
    return [VirtualDisplay removeDisplayWithId:(int)displayId];
}

@end