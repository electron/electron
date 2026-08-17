#import "VirtualDisplayBridge.h"

#include <unistd.h>

static NSMutableDictionary<NSNumber*, CGVirtualDisplay*>* sDisplays;
static NSInteger sCounter = 0;

@implementation VirtualDisplayBridge

+ (void)initialize {
  if (self == [VirtualDisplayBridge class]) {
    sDisplays = [NSMutableDictionary new];
  }
}

+ (NSInteger)create:(int)width height:(int)height x:(int)x y:(int)y {
  int step = 1;
  int minX = 720, minY = 720, maxX = 8192, maxY = 8192;
  int minMultiplier = MAX((int)ceil((double)minX / (width * step)),
                          (int)ceil((double)minY / (height * step)));
  int maxMultiplier = MIN((int)floor((double)maxX / (width * step)),
                          (int)floor((double)maxY / (height * step)));

  CGVirtualDisplayDescriptor* descriptor = [[CGVirtualDisplayDescriptor alloc] init];
  if (!descriptor)
    return 0;

  descriptor.queue =
      dispatch_get_global_queue(QOS_CLASS_USER_INTERACTIVE, 0);
  descriptor.name =
      [NSString stringWithFormat:@"Dummy %dx%d", width, height];
  descriptor.whitePoint = CGPointMake(0.950, 1.000);
  descriptor.redPrimary = CGPointMake(0.454, 0.242);
  descriptor.greenPrimary = CGPointMake(0.353, 0.674);
  descriptor.bluePrimary = CGPointMake(0.157, 0.084);
  descriptor.maxPixelsWide = (unsigned int)(width * step * maxMultiplier);
  descriptor.maxPixelsHigh = (unsigned int)(height * step * maxMultiplier);

  double diagonal = (24.0 * 25.4) /
      sqrt((double)(width * width) + (double)(height * height));
  descriptor.sizeInMillimeters =
      CGSizeMake((double)width * diagonal, (double)height * diagonal);

  descriptor.serialNum = arc4random();
  descriptor.productID =
      (unsigned int)(MIN(width - 1, 255) * 256 + MIN(height - 1, 255));
  descriptor.vendorID = 0xF0F0;

  CGVirtualDisplay* display =
      [[CGVirtualDisplay alloc] initWithDescriptor:descriptor];
  if (!display)
    return 0;

  NSMutableArray* modes = [NSMutableArray new];
  for (int mult = minMultiplier; mult <= maxMultiplier; mult++) {
    unsigned int w = (unsigned int)(width * mult * step);
    unsigned int h = (unsigned int)(height * mult * step);
    CGVirtualDisplayMode* mode =
        [[CGVirtualDisplayMode alloc] initWithWidth:w height:h refreshRate:60.0];
    if (mode)
      [modes addObject:mode];
  }

  CGVirtualDisplaySettings* settings = [[CGVirtualDisplaySettings alloc] init];
  if (!settings)
    return 0;

  settings.modes = modes;
  settings.hiDPI = 0;

  if (![display applySettings:settings])
    return 0;

  CGDirectDisplayID displayID = [display displayID];
  if (![self waitForDisplayRegistration:displayID])
    return 0;

  [self positionDisplay:displayID x:x y:y];

  sCounter++;
  sDisplays[@(sCounter)] = display;
  return sCounter;
}

+ (BOOL)destroy:(NSInteger)displayId {
  NSNumber* key = @(displayId);
  if (!sDisplays[key])
    return NO;
  [sDisplays removeObjectForKey:key];
  return YES;
}

+ (BOOL)forceCleanup {
  [sDisplays removeAllObjects];
  sCounter = 0;

  CGDisplayConfigRef config = NULL;
  if (CGBeginDisplayConfiguration(&config) == kCGErrorSuccess) {
    CGCompleteDisplayConfiguration(config, kCGConfigurePermanently);
  }

  usleep(2000000);

  config = NULL;
  if (CGBeginDisplayConfiguration(&config) == kCGErrorSuccess) {
    CGCompleteDisplayConfiguration(config, kCGConfigureForSession);
  }

  return YES;
}

#pragma mark - Private

+ (BOOL)waitForDisplayRegistration:(CGDirectDisplayID)targetID {
  CGDirectDisplayID displays[32];
  uint32_t count = 0;

  for (int i = 0; i < 20; i++) {
    if (CGGetActiveDisplayList(32, displays, &count) == kCGErrorSuccess) {
      for (uint32_t j = 0; j < count; j++) {
        if (displays[j] == targetID)
          return YES;
      }
    }
    usleep(100000);
  }
  return NO;
}

+ (void)positionDisplay:(CGDirectDisplayID)displayID x:(int)x y:(int)y {
  CGDisplayConfigRef config = NULL;
  CGError err = CGBeginDisplayConfiguration(&config);
  if (err != kCGErrorSuccess)
    return;

  err = CGConfigureDisplayOrigin(config, displayID, (int32_t)x, (int32_t)y);
  if (err != kCGErrorSuccess) {
    CGCancelDisplayConfiguration(config);
    return;
  }

  CGCompleteDisplayConfiguration(config, kCGConfigurePermanently);
}

@end
