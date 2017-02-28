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
#include "native_mate/constructor.h"
#include "native_mate/persistent_dictionary.h"

@interface AtomTouchBar : NSObject {
 @protected
  std::map<std::string, mate::PersistentDictionary> item_id_map;
  std::map<std::string, NSTouchBarItem*> item_map;
  id<NSTouchBarDelegate> delegate_;
  atom::NativeWindow* window_;
}

- (id)initWithDelegate:(id<NSTouchBarDelegate>)delegate
                window:(atom::NativeWindow*)window;

- (NSTouchBar*)makeTouchBarFromItemOptions:(const std::vector<mate::PersistentDictionary>&)item_options;
- (NSTouchBar*)touchBarFromItemIdentifiers:(NSMutableArray*)items;
- (NSMutableArray*)identifierArrayFromDicts:(const std::vector<mate::PersistentDictionary>&)dicts;
- (void)refreshTouchBarItem:(mate::Arguments*)args;
- (void)clear;

- (NSString*)idFromIdentifier:(NSString*)identifier withPrefix:(NSString*)prefix;
- (bool)hasItemWithID:(const std::string&)id;
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
- (void)updateButton:(NSCustomTouchBarItem*)item withOptions:(const mate::PersistentDictionary&)options;
- (void)updateLabel:(NSCustomTouchBarItem*)item withOptions:(const mate::PersistentDictionary&)options;
- (void)updateColorPicker:(NSColorPickerTouchBarItem*)item withOptions:(const mate::PersistentDictionary&)options;
- (void)updateSlider:(NSSliderTouchBarItem*)item withOptions:(const mate::PersistentDictionary&)options;
- (void)updatePopover:(NSPopoverTouchBarItem*)item withOptions:(const mate::PersistentDictionary&)options;

@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_TOUCH_BAR_H_
