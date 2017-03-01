// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_ATOM_TOUCH_BAR_H_
#define ATOM_BROWSER_UI_COCOA_ATOM_TOUCH_BAR_H_

#import <Cocoa/Cocoa.h>

#include <map>
#include <string>
#include <vector>

#include "atom/browser/native_window.h"
#include "atom/browser/ui/cocoa/touch_bar_forward_declarations.h"
#include "base/mac/scoped_nsobject.h"
#include "native_mate/constructor.h"
#include "native_mate/persistent_dictionary.h"

@interface AtomTouchBar : NSObject {
 @protected
  std::vector<mate::PersistentDictionary> ordered_settings_;
  std::map<std::string, mate::PersistentDictionary> settings_;
  std::map<std::string, base::scoped_nsobject<NSTouchBarItem>> items_;
  id<NSTouchBarDelegate> delegate_;
  atom::NativeWindow* window_;
}

- (id)initWithDelegate:(id<NSTouchBarDelegate>)delegate window:(atom::NativeWindow*)window settings:(const std::vector<mate::PersistentDictionary>&)settings;

- (NSTouchBar*)makeTouchBar;
- (NSTouchBar*)touchBarFromItemIdentifiers:(NSMutableArray*)items;
- (NSMutableArray*)identifiersFromSettings:(const std::vector<mate::PersistentDictionary>&)settings;
- (void)refreshTouchBarItem:(const std::string&)item_id;

- (NSString*)idFromIdentifier:(NSString*)identifier withPrefix:(NSString*)prefix;
- (bool)hasItemWithID:(const std::string&)item_id;
- (NSColor*)colorFromHexColorString:(const std::string&)colorString;

// Selector actions
- (void)buttonAction:(id)sender;
- (void)colorPickerAction:(id)sender;
- (void)sliderAction:(id)sender;

// Helpers to create touch bar items
- (NSTouchBarItem*)makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier;
- (NSTouchBarItem*)makeButtonForID:(NSString*)id withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeLabelForID:(NSString*)id withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeColorPickerForID:(NSString*)id withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeSliderForID:(NSString*)id withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makePopoverForID:(NSString*)id withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeGroupForID:(NSString*)id withIdentifier:(NSString*)identifier;

// Helpers to update touch bar items
- (void)updateButton:(NSCustomTouchBarItem*)item withSettings:(const mate::PersistentDictionary&)settings;
- (void)updateLabel:(NSCustomTouchBarItem*)item withSettings:(const mate::PersistentDictionary&)settings;
- (void)updateColorPicker:(NSColorPickerTouchBarItem*)item withSettings:(const mate::PersistentDictionary&)settings;
- (void)updateSlider:(NSSliderTouchBarItem*)item withSettings:(const mate::PersistentDictionary&)settings;
- (void)updatePopover:(NSPopoverTouchBarItem*)item withSettings:(const mate::PersistentDictionary&)settings;

@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_TOUCH_BAR_H_
