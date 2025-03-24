// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import <Security/Security.h>

NSArray<NSString*>* GetAppGroups() {
  SecCodeRef code = NULL;
  if (SecCodeCopySelf(kSecCSDefaultFlags, &code) != errSecSuccess)
    return nil;
  CFDictionaryRef signingInfo = NULL;
  OSStatus status = SecCodeCopySigningInformation(
      code, kSecCSSigningInformation, &signingInfo);
  CFRelease(code);
  if (status != errSecSuccess || !signingInfo)
    return nil;
  NSDictionary* info = CFBridgingRelease(signingInfo);
  return info[(__bridge NSString*)kSecCodeInfoEntitlementsDict]
             [@"com.apple.security.application-groups"];
}

int main(int argc, char* argv[]) {
  @autoreleasepool {
    NSArray<NSString*>* appGroups = GetAppGroups();
    if (appGroups && appGroups.count != 0) {
      NSString* groupsId = [appGroups firstObject];
      NSUserDefaults* sharedDefaults =
          [[NSUserDefaults alloc] initWithSuiteName:groupsId];
      [sharedDefaults setBool:YES forKey:@"wasOpendAtLogin"];
    }

    NSArray* pathComponents =
        [[[NSBundle mainBundle] bundlePath] pathComponents];
    pathComponents = [pathComponents
        subarrayWithRange:NSMakeRange(0, [pathComponents count] - 4)];
    NSString* path = [NSString pathWithComponents:pathComponents];
    NSURL* url = [NSURL fileURLWithPath:path];

    [[NSWorkspace sharedWorkspace]
        openApplicationAtURL:url
               configuration:NSWorkspaceOpenConfiguration.configuration
           completionHandler:^(NSRunningApplication* app, NSError* error) {
             if (error) {
               NSLog(@"Failed to launch application: %@", error);
             } else {
               NSLog(@"Application launched successfully: %@", app);
             }
           }];
    return 0;
  }
}
