// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_TOUCH_BAR_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_TOUCH_BAR_H_

#import <Cocoa/Cocoa.h>

#include <map>
#include <string>
#include <vector>

#include "base/mac/scoped_nsobject.h"
#include "shell/browser/native_window.h"
#include "shell/common/gin_helper/persistent_dictionary.h"

@interface ElectronTouchBar : NSObject <NSScrubberDelegate,
                                        NSScrubberDataSource,
                                        NSScrubberFlowLayoutDelegate> {
 @protected
  std::vector<gin_helper::PersistentDictionary> ordered_settings_;
  std::map<std::string, gin_helper::PersistentDictionary> settings_;
  id<NSTouchBarDelegate> delegate_;
  electron::NativeWindow* window_;
}

- (id)initWithDelegate:(id<NSTouchBarDelegate>)delegate
                window:(electron::NativeWindow*)window
              settings:(std::vector<gin_helper::PersistentDictionary>)settings;

- (NSTouchBar*)makeTouchBar;
- (NSTouchBar*)touchBarFromItemIdentifiers:(NSMutableArray*)items;
- (NSMutableArray*)identifiersFromSettings:
    (const std::vector<gin_helper::PersistentDictionary>&)settings;
- (void)refreshTouchBarItem:(NSTouchBar*)touchBar
                         id:(const std::string&)item_id;
- (void)addNonDefaultTouchBarItems:
    (const std::vector<gin_helper::PersistentDictionary>&)items;
- (void)setEscapeTouchBarItem:(gin_helper::PersistentDictionary)item
                  forTouchBar:(NSTouchBar*)touchBar;

- (NSString*)idFromIdentifier:(NSString*)identifier
                   withPrefix:(NSString*)prefix;
- (NSTouchBarItemIdentifier)identifierFromID:(const std::string&)item_id
                                        type:(const std::string&)typere;
- (bool)hasItemWithID:(const std::string&)item_id;
- (NSColor*)colorFromHexColorString:(const std::string&)colorString;

// Selector actions
- (void)buttonAction:(id)sender;
- (void)colorPickerAction:(id)sender;
- (void)sliderAction:(id)sender;

// Helpers to create touch bar items
- (NSTouchBarItem*)makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier;
- (NSTouchBarItem*)makeButtonForID:(NSString*)id
                    withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeLabelForID:(NSString*)id
                   withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeColorPickerForID:(NSString*)id
                         withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeSliderForID:(NSString*)id
                    withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makePopoverForID:(NSString*)id
                     withIdentifier:(NSString*)identifier;
- (NSTouchBarItem*)makeGroupForID:(NSString*)id
                   withIdentifier:(NSString*)identifier;

// Helpers to update touch bar items
- (void)updateButton:(NSCustomTouchBarItem*)item
        withSettings:(const gin_helper::PersistentDictionary&)settings;
- (void)updateLabel:(NSCustomTouchBarItem*)item
       withSettings:(const gin_helper::PersistentDictionary&)settings;
- (void)updateColorPicker:(NSColorPickerTouchBarItem*)item
             withSettings:(const gin_helper::PersistentDictionary&)settings;
- (void)updateSlider:(NSSliderTouchBarItem*)item
        withSettings:(const gin_helper::PersistentDictionary&)settings;
- (void)updatePopover:(NSPopoverTouchBarItem*)item
         withSettings:(const gin_helper::PersistentDictionary&)settings;

@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_TOUCH_BAR_H_
