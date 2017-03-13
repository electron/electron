// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_ATOM_SCRUBBER_DATA_SOURCE_H_
#define ATOM_BROWSER_UI_COCOA_ATOM_SCRUBBER_DATA_SOURCE_H_

#import <Cocoa/Cocoa.h>

#include <string>
#include <vector>

#include "atom/browser/ui/cocoa/touch_bar_forward_declarations.h"
#include "native_mate/persistent_dictionary.h"

@interface AtomScrubberDataSource : NSObject<NSScrubberDataSource> {
  @protected
  std::vector<mate::PersistentDictionary> items_;
}

- (id)initWithItems:(std::vector<mate::PersistentDictionary>)items;
- (void)setItems:(std::vector<mate::PersistentDictionary>)items;

@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_SCRUBBER_DATA_SOURCE_H_
