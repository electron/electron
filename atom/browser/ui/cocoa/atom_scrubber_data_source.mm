// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/ui/cocoa/atom_scrubber_data_source.h"

#include "atom/common/native_mate_converters/image_converter.h"
#include "base/strings/sys_string_conversions.h"
#include "ui/gfx/image/image.h"

@implementation AtomScrubberDataSource

static NSString* const TextItemIdentifier = @"scrubber.text.item";
static NSString* const ImageItemIdentifier = @"scrubber.image.item";

- (id)initWithItems:(std::vector<mate::PersistentDictionary>)items {
  if ((self = [super init])) {
    items_ = items;
  }
  return self;
}

- (NSInteger)numberOfItemsForScrubber:(NSScrubber *)theScrubber {
  return items_.size();
}

- (NSScrubberItemView *)scrubber:(NSScrubber *)scrubber viewForItemAtIndex:(NSInteger)index {
  mate::PersistentDictionary item = items_[index];

  NSScrubberItemView* itemView;
  std::string title;

  if (item.Get("label", &title)) {
    NSScrubberTextItemView* view = [scrubber makeItemWithIdentifier:TextItemIdentifier owner:self];
    view.title = base::SysUTF8ToNSString(title);

    itemView = view;
  } else {
    NSScrubberImageItemView* view = [scrubber makeItemWithIdentifier:ImageItemIdentifier owner:self];
    gfx::Image image;
    if (item.Get("image", &image)) {
      view.image = image.AsNSImage();
    }

    itemView = view;
  }

  return itemView;
}

- (void)setItems:(std::vector<mate::PersistentDictionary>)items {
  items_ = items;
}

@end
