// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "atom/browser/ui/drag_util.h"
#include "base/files/file_path.h"
#include "base/strings/sys_string_conversions.h"

namespace atom {

namespace {

// Write information about the file being dragged to the pasteboard.
void AddFilesToPasteboard(NSPasteboard* pasteboard,
                          const std::vector<base::FilePath>& files) {
  NSMutableArray* fileList = [NSMutableArray array];
  for (const base::FilePath& file : files)
    [fileList addObject:base::SysUTF8ToNSString(file.value())];
  [pasteboard declareTypes:[NSArray arrayWithObject:NSFilenamesPboardType]
                     owner:nil];
  [pasteboard setPropertyList:fileList forType:NSFilenamesPboardType];
}

}  // namespace

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view) {
  NSPasteboard* pasteboard = [NSPasteboard pasteboardWithName:NSDragPboard];
  AddFilesToPasteboard(pasteboard, files);

  // Synthesize a drag event, since we don't have access to the actual event
  // that initiated a drag (possibly consumed by the Web UI, for example).
  NSPoint position = [[view window] mouseLocationOutsideOfEventStream];
  NSTimeInterval eventTime = [[NSApp currentEvent] timestamp];
  NSEvent* dragEvent = [NSEvent mouseEventWithType:NSLeftMouseDragged
                                          location:position
                                     modifierFlags:NSLeftMouseDraggedMask
                                         timestamp:eventTime
                                      windowNumber:[[view window] windowNumber]
                                           context:nil
                                       eventNumber:0
                                        clickCount:1
                                          pressure:1.0];

  // Run the drag operation.
  [[view window] dragImage:icon.ToNSImage()
                        at:position
                    offset:NSZeroSize
                     event:dragEvent
                pasteboard:pasteboard
                    source:view
                 slideBack:YES];
}

}  // namespace atom
