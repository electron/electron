#import "VirtualDisplayBridge.h"
#import "../build_swift/virtual_display-Swift.h"

@implementation VirtualDisplayBridge

+ (NSInteger)addDisplay {
    return [VirtualDisplay addDisplay];
}

+ (BOOL)removeDisplay:(NSInteger)displayId {
    return [VirtualDisplay removeDisplayWithId:displayId];
}

@end