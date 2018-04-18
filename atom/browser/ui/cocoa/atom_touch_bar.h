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

@interface AtomTouchBar : NSObject <NSScrubberDelegate,
                                    NSScrubberDataSource,
                                    NSScrubberFlowLayoutDelegate> {
 @protected
  std::vector<mate::PersistentDictionary> ordered_settings_;
  std::map<std::string, mate::PersistentDictionary> settings_;
  id<NSTouchBarDelegate> delegate_;
  atom::NativeWindow* window_;
}

- (id)initWithDelegate:(id<NSTouchBarDelegate>)delegate
                window:(atom::NativeWindow*)window
              settings:(const std::vector<mate::PersistentDictionary>&)settings;

- (NSTouchBar*)makeTouchBar API_AVAILABLE(macosx(10.12.2));
- (NSTouchBar*)touchBarFromItemIdentifiers:(NSMutableArray*)items
    API_AVAILABLE(macosx(10.12.2));
- (NSMutableArray*)identifiersFromSettings:
    (const std::vector<mate::PersistentDictionary>&)settings;
- (void)refreshTouchBarItem:(NSTouchBar*)touchBar
                         id:(const std::string&)item_id
    API_AVAILABLE(macosx(10.12.2));
- (void)addNonDefaultTouchBarItems:
    (const std::vector<mate::PersistentDictionary>&)items;
- (void)setEscapeTouchBarItem:(const mate::PersistentDictionary&)item
                  forTouchBar:(NSTouchBar*)touchBar
    API_AVAILABLE(macosx(10.12.2));

- (NSString*)idFromIdentifier:(NSString*)identifier
                   withPrefix:(NSString*)prefix;
- (NSTouchBarItemIdentifier)identifierFromID:(const std::string&)item_id
                                        type:(const std::string&)typere;
- (bool)hasItemWithID:(const std::string&)item_id;
- (NSColor*)colorFromHexColorString:(const std::string&)colorString;

// Selector actions
- (void)buttonAction:(id)sender;
- (void)colorPickerAction:(id)sender API_AVAILABLE(macosx(10.12.2));
- (void)sliderAction:(id)sender API_AVAILABLE(macosx(10.12.2));

// Helpers to create touch bar items
- (NSTouchBarItem*)makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier
    API_AVAILABLE(macosx(10.12.2));
- (NSTouchBarItem*)makeButtonForID:(NSString*)id
                    withIdentifier:(NSString*)identifier
    API_AVAILABLE(macosx(10.12.2));
- (NSTouchBarItem*)makeLabelForID:(NSString*)id
                   withIdentifier:(NSString*)identifier
    API_AVAILABLE(macosx(10.12.2));
- (NSTouchBarItem*)makeColorPickerForID:(NSString*)id
                         withIdentifier:(NSString*)identifier
    API_AVAILABLE(macosx(10.12.2));
- (NSTouchBarItem*)makeSliderForID:(NSString*)id
                    withIdentifier:(NSString*)identifier
    API_AVAILABLE(macosx(10.12.2));
- (NSTouchBarItem*)makePopoverForID:(NSString*)id
                     withIdentifier:(NSString*)identifier
    API_AVAILABLE(macosx(10.12.2));
- (NSTouchBarItem*)makeGroupForID:(NSString*)id
                   withIdentifier:(NSString*)identifier
    API_AVAILABLE(macosx(10.12.2));

// Helpers to update touch bar items
- (void)updateButton:(NSCustomTouchBarItem*)item
        withSettings:(const mate::PersistentDictionary&)settings
    API_AVAILABLE(macosx(10.12.2));
- (void)updateLabel:(NSCustomTouchBarItem*)item
       withSettings:(const mate::PersistentDictionary&)settings
    API_AVAILABLE(macosx(10.12.2));
- (void)updateColorPicker:(NSColorPickerTouchBarItem*)item
             withSettings:(const mate::PersistentDictionary&)settings
    API_AVAILABLE(macosx(10.12.2));
- (void)updateSlider:(NSSliderTouchBarItem*)item
        withSettings:(const mate::PersistentDictionary&)settings
    API_AVAILABLE(macosx(10.12.2));
- (void)updatePopover:(NSPopoverTouchBarItem*)item
         withSettings:(const mate::PersistentDictionary&)settings
    API_AVAILABLE(macosx(10.12.2));

@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_TOUCH_BAR_H_
