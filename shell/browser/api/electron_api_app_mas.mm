// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_app.h"

#include <string>

#import <Cocoa/Cocoa.h>

#include "base/strings/sys_string_conversions.h"

namespace electron::api {

// Callback passed to js which will stop accessing the given bookmark.
void OnStopAccessingSecurityScopedResource(NSURL* bookmarkUrl) {
  [bookmarkUrl stopAccessingSecurityScopedResource];
  [bookmarkUrl release];
}

// Get base64 encoded NSData, create a bookmark for it and start accessing it.
base::RepeatingCallback<void()> App::StartAccessingSecurityScopedResource(
    gin::Arguments* args) {
  std::string data;
  args->GetNext(&data);
  NSString* base64str = base::SysUTF8ToNSString(data);
  NSData* bookmarkData = [[NSData alloc] initWithBase64EncodedString:base64str
                                                             options:0];

  // Create bookmarkUrl from NSData.
  BOOL isStale = false;
  NSError* error = nil;
  NSURL* bookmarkUrl = [NSURL
      URLByResolvingBookmarkData:bookmarkData
                         options:NSURLBookmarkResolutionWithSecurityScope |
                                 NSURLBookmarkResolutionWithoutMounting
                   relativeToURL:nil
             bookmarkDataIsStale:&isStale
                           error:&error];

  if (error != nil) {
    NSString* err =
        [NSString stringWithFormat:@"NSError: %@ %@", error, [error userInfo]];
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError(base::SysNSStringToUTF8(err));
  }

  if (isStale) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("bookmarkDataIsStale - try recreating the bookmark");
  }

  if (error == nil && isStale == false) {
    [bookmarkUrl startAccessingSecurityScopedResource];
  }

  // Stop the NSURL from being GC'd.
  [bookmarkUrl retain];

  // Return a js callback which will close the bookmark.
  return base::BindRepeating(&OnStopAccessingSecurityScopedResource,
                             bookmarkUrl);
}

}  // namespace electron::api
