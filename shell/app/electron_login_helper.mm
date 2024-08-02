// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

// launchApplication is deprecated; should be migrated to
// [NSWorkspace openApplicationAtURL:configuration:completionHandler:]
// UserNotifications.frameworks API
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

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

// -Wdeprecated-declarations
#pragma clang diagnostic pop
