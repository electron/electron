// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/atom_preview_item.h"

@implementation AtomPreviewItem

@synthesize previewItemURL;
@synthesize previewItemTitle;

- (id)initWithURL:(NSURL*)url title:(NSString*)title {
  self = [super init];
  if (self) {
    self.previewItemURL = url;
    self.previewItemTitle = title;
  }
  return self;
}

@end
