// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

int main(int argc, char* argv[]) {
  @autoreleasepool {
    NSArray* pathComponents =
        [[[NSBundle mainBundle] bundlePath] pathComponents];
    pathComponents = [pathComponents
        subarrayWithRange:NSMakeRange(0, [pathComponents count] - 4)];
    NSString* path = [NSString pathWithComponents:pathComponents];
    [[NSWorkspace sharedWorkspace] launchApplication:path];
    return 0;
  }
}
