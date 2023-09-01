// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/drag_util.h"

#import <Cocoa/Cocoa.h>

#include <vector>

#include "base/apple/foundation_util.h"
#include "base/files/file_path.h"

// Contents largely copied from
// chrome/browser/download/drag_download_item_mac.mm.

@interface DragDownloadItemSource : NSObject <NSDraggingSource>
@end

@implementation DragDownloadItemSource

- (NSDragOperation)draggingSession:(NSDraggingSession*)session
    sourceOperationMaskForDraggingContext:(NSDraggingContext)context {
  return context == NSDraggingContextOutsideApplication ? NSDragOperationCopy
                                                        : NSDragOperationEvery;
}

@end

namespace {

id<NSDraggingSource> GetDraggingSource() {
  static id<NSDraggingSource> source = [[DragDownloadItemSource alloc] init];
  return source;
}

}  // namespace

namespace electron {

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view) {
  DCHECK(view);
  auto* native_view = view.GetNativeNSView();
  NSPoint current_position =
      native_view.window.mouseLocationOutsideOfEventStream;
  current_position =
      [native_view backingAlignedRect:NSMakeRect(current_position.x,
                                                 current_position.y, 0, 0)
                              options:NSAlignAllEdgesOutward]
          .origin;

  NSMutableArray* file_items = [NSMutableArray array];
  for (auto const& file : files) {
    NSURL* file_url = base::apple::FilePathToNSURL(file);
    NSDraggingItem* file_item =
        [[NSDraggingItem alloc] initWithPasteboardWriter:file_url];
    NSImage* file_image = icon.ToNSImage();
    NSSize image_size = file_image.size;
    NSRect image_rect = NSMakeRect(current_position.x - image_size.width / 2,
                                   current_position.y - image_size.height / 2,
                                   image_size.width, image_size.height);
    [file_item setDraggingFrame:image_rect contents:file_image];
    [file_items addObject:file_item];
  }

  // Synthesize a drag event, since we don't have access to the actual event
  // that initiated a drag (possibly consumed by the Web UI, for example).
  NSEvent* dragEvent =
      [NSEvent mouseEventWithType:NSEventTypeLeftMouseDragged
                         location:current_position
                    modifierFlags:0
                        timestamp:NSApp.currentEvent.timestamp
                     windowNumber:native_view.window.windowNumber
                          context:nil
                      eventNumber:0
                       clickCount:1
                         pressure:1.0];

  // Run the drag operation.
  [native_view beginDraggingSessionWithItems:file_items
                                       event:dragEvent
                                      source:GetDraggingSource()];
}

}  // namespace electron
