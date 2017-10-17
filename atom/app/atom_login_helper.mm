#import <Cocoa/Cocoa.h>

int main(int argc, char* argv[]) {
  NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
  NSArray* pathComponents = [[[NSBundle mainBundle] bundlePath] pathComponents];
  pathComponents = [pathComponents subarrayWithRange:NSMakeRange(0, [pathComponents count] - 4)];
  NSString* path = [NSString pathWithComponents:pathComponents];
  [[NSWorkspace sharedWorkspace] launchApplication:path];
  [pool drain];
  return 0;
}
