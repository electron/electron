#import "VirtualDisplayBridge.h"
#import "../build_swift/virtual_display-Swift.h"

@implementation VirtualDisplayBridge

+ (NSInteger)create:(int)width height:(int)height x:(int)x y:(int)y {
    return [VirtualDisplay createWithWidth:width height:height x:x y:y];
}

+ (BOOL)destroy:(NSInteger)displayId {
    return [VirtualDisplay destroyWithId:(int)displayId];
}

+ (BOOL)forceCleanup {
    return [VirtualDisplay forceCleanup];
}

@end