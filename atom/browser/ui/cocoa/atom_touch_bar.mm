// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/ui/cocoa/atom_touch_bar.h"

#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "base/strings/sys_string_conversions.h"
#include "skia/ext/skia_utils_mac.h"
#include "ui/gfx/image/image.h"

@implementation AtomTouchBar

static NSTouchBarItemIdentifier ButtonIdentifier = @"com.electron.touchbar.button.";
static NSTouchBarItemIdentifier ColorPickerIdentifier = @"com.electron.touchbar.colorpicker.";
static NSTouchBarItemIdentifier GroupIdentifier = @"com.electron.touchbar.group.";
static NSTouchBarItemIdentifier LabelIdentifier = @"com.electron.touchbar.label.";
static NSTouchBarItemIdentifier PopOverIdentifier = @"com.electron.touchbar.popover.";
static NSTouchBarItemIdentifier SliderIdentifier = @"com.electron.touchbar.slider.";

- (id)initWithDelegate:(id<NSTouchBarDelegate>)delegate
                window:(atom::NativeWindow*)window
              settings:(const std::vector<mate::PersistentDictionary>&)settings {
  if ((self = [super init])) {
    delegate_ = delegate;
    window_ = window;
    ordered_settings_ = settings;
  }
  return self;
}

- (NSTouchBar*)makeTouchBar {
  NSMutableArray* identifiers = [self identifiersFromSettings:ordered_settings_];
  return [self touchBarFromItemIdentifiers:identifiers];
}

- (NSTouchBar*)touchBarFromItemIdentifiers:(NSMutableArray*)items {
  base::scoped_nsobject<NSTouchBar> bar(
      [[NSClassFromString(@"NSTouchBar") alloc] init]);
  [bar setDelegate:delegate_];
  [bar setDefaultItemIdentifiers:items];
  return bar.autorelease();
}

- (NSMutableArray*)identifiersFromSettings:(const std::vector<mate::PersistentDictionary>&)dicts {
  NSMutableArray* identifiers = [[NSMutableArray alloc] init];

  for (const auto& item : dicts) {
    std::string type;
    std::string item_id;
    if (item.Get("type", &type) && item.Get("id", &item_id)) {
      settings_[item_id] = item;
      if (type == "button") {
        [identifiers addObject:[NSString stringWithFormat:@"%@%@", ButtonIdentifier, base::SysUTF8ToNSString(item_id)]];
      } else if (type == "label") {
        [identifiers addObject:[NSString stringWithFormat:@"%@%@", LabelIdentifier, base::SysUTF8ToNSString(item_id)]];
      } else if (type == "colorpicker") {
        [identifiers addObject:[NSString stringWithFormat:@"%@%@", ColorPickerIdentifier, base::SysUTF8ToNSString(item_id)]];
      } else if (type == "slider") {
        [identifiers addObject:[NSString stringWithFormat:@"%@%@", SliderIdentifier, base::SysUTF8ToNSString(item_id)]];
      } else if (type == "popover") {
        [identifiers addObject:[NSString stringWithFormat:@"%@%@", PopOverIdentifier, base::SysUTF8ToNSString(item_id)]];
      } else if (type == "group") {
        [identifiers addObject:[NSString stringWithFormat:@"%@%@", GroupIdentifier, base::SysUTF8ToNSString(item_id)]];
      } else if (type == "spacer") {
        std::string size;
        item.Get("size", &size);
        if (size == "large") {
          [identifiers addObject:NSTouchBarItemIdentifierFixedSpaceLarge];
        } else if (size == "flexible") {
          [identifiers addObject:NSTouchBarItemIdentifierFlexibleSpace];
        } else {
          [identifiers addObject:NSTouchBarItemIdentifierFixedSpaceSmall];
        }
      }
    }
  }
  [identifiers addObject:NSTouchBarItemIdentifierOtherItemsProxy];

  return identifiers;
}

- (NSTouchBarItem*)makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
  base::scoped_nsobject<NSTouchBarItem> item;
  NSString* item_id = nil;

  if ([identifier hasPrefix:ButtonIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:ButtonIdentifier];
    item.reset([self makeButtonForID:item_id withIdentifier:identifier]);
  } else if ([identifier hasPrefix:LabelIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:LabelIdentifier];
    item.reset([self makeLabelForID:item_id withIdentifier:identifier]);
  } else if ([identifier hasPrefix:ColorPickerIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:ColorPickerIdentifier];
    item.reset([self makeColorPickerForID:item_id withIdentifier:identifier]);
  } else if ([identifier hasPrefix:SliderIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:SliderIdentifier];
    item.reset([self makeSliderForID:item_id withIdentifier:identifier]);
  } else if ([identifier hasPrefix:PopOverIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:PopOverIdentifier];
    item.reset([self makePopoverForID:item_id withIdentifier:identifier]);
  } else if ([identifier hasPrefix:GroupIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:GroupIdentifier];
    item.reset([self makeGroupForID:item_id withIdentifier:identifier]);
  }

  if (item_id)
    items_[[item_id UTF8String]] = item;

  return item.autorelease();
}


- (void)refreshTouchBarItem:(const std::string&)item_id {
  if (items_.find(item_id) == items_.end()) return;
  if (![self hasItemWithID:item_id]) return;

  mate::PersistentDictionary settings = settings_[item_id];
  std::string item_type;
  settings.Get("type", &item_type);

  if (item_type == "button") {
    [self updateButton:(NSCustomTouchBarItem*)items_[item_id]
          withSettings:settings];
  } else if (item_type == "label") {
    [self updateLabel:(NSCustomTouchBarItem*)items_[item_id]
         withSettings:settings];
  } else if (item_type == "colorpicker") {
    [self updateColorPicker:(NSColorPickerTouchBarItem*)items_[item_id]
               withSettings:settings];
  } else if (item_type == "slider") {
    [self updateSlider:(NSSliderTouchBarItem*)items_[item_id]
          withSettings:settings];
  } else if (item_type == "popover") {
    [self updatePopover:(NSPopoverTouchBarItem*)items_[item_id]
           withSettings:settings];
  }
}

- (void)buttonAction:(id)sender {
  NSString* item_id = [NSString stringWithFormat:@"%ld", ((NSButton*)sender).tag];
  window_->NotifyTouchBarItemInteraction([item_id UTF8String],
                                         base::DictionaryValue());
}

- (void)colorPickerAction:(id)sender {
  NSString* identifier = ((NSColorPickerTouchBarItem*)sender).identifier;
  NSString* item_id = [self idFromIdentifier:identifier
                                  withPrefix:ColorPickerIdentifier];
  NSColor* color = ((NSColorPickerTouchBarItem*)sender).color;
  std::string hex_color = atom::ToRGBHex(skia::NSDeviceColorToSkColor(color));
  base::DictionaryValue details;
  details.SetString("color", hex_color);
  window_->NotifyTouchBarItemInteraction([item_id UTF8String], details);
}

- (void)sliderAction:(id)sender {
  NSString* identifier = ((NSSliderTouchBarItem*)sender).identifier;
  NSString* item_id = [self idFromIdentifier:identifier
                                  withPrefix:SliderIdentifier];
  base::DictionaryValue details;
  details.SetInteger("value",
                     [((NSSliderTouchBarItem*)sender).slider intValue]);
  window_->NotifyTouchBarItemInteraction([item_id UTF8String], details);
}

- (NSString*)idFromIdentifier:(NSString*)identifier withPrefix:(NSString*)prefix {
  return [identifier substringFromIndex:[prefix length]];
}

- (bool)hasItemWithID:(const std::string&)item_id {
  return settings_.find(item_id) != settings_.end();
}

- (NSColor*)colorFromHexColorString:(const std::string&)colorString {
  SkColor color = atom::ParseHexColor(colorString);
  return skia::SkColorToCalibratedNSColor(color);
}

- (NSTouchBarItem*)makeButtonForID:(NSString*)id
                    withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  NSCustomTouchBarItem* item = [[NSClassFromString(@"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier];
  NSButton* button = [NSButton buttonWithTitle:@""
                                        target:self
                                        action:@selector(buttonAction:)];
  button.tag = [id floatValue];
  item.view = button;
  [self updateButton:item withSettings:settings];
  return item;
}

- (void)updateButton:(NSCustomTouchBarItem*)item
        withSettings:(const mate::PersistentDictionary&)settings {
  NSButton* button = (NSButton*)item.view;

  std::string backgroundColor;
  if (settings.Get("backgroundColor", &backgroundColor)) {
    button.bezelColor = [self colorFromHexColorString:backgroundColor];
  }

  std::string label;
  if (settings.Get("label", &label)) {
    button.title = base::SysUTF8ToNSString(label);
  }

  gfx::Image image;
  if (settings.Get("image", &image)) {
    button.image = image.AsNSImage();
  }
}

- (NSTouchBarItem*)makeLabelForID:(NSString*)id
                   withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary item = settings_[s_id];
  NSCustomTouchBarItem* customItem = [[NSClassFromString(@"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier];
  customItem.view = [NSTextField labelWithString:@""];
  [self updateLabel:customItem withSettings:item];

  return customItem;
}

- (void)updateLabel:(NSCustomTouchBarItem*)item
       withSettings:(const mate::PersistentDictionary&)settings {
  std::string label;
  settings.Get("label", &label);
  NSTextField* text_field = (NSTextField*)item.view;
  text_field.stringValue = base::SysUTF8ToNSString(label);
}

- (NSTouchBarItem*)makeColorPickerForID:(NSString*)id
                         withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  NSColorPickerTouchBarItem* item = [[NSClassFromString(@"NSColorPickerTouchBarItem") alloc] initWithIdentifier:identifier];
  item.target = self;
  item.action = @selector(colorPickerAction:);
  [self updateColorPicker:item withSettings:settings];
  return item;
}

- (void)updateColorPicker:(NSColorPickerTouchBarItem*)item
             withSettings:(const mate::PersistentDictionary&)settings {
  std::vector<std::string> colors;
  if (settings.Get("availableColors", &colors) && colors.size() > 0) {
    NSColorList* color_list  = [[[NSColorList alloc] initWithName:@""] autorelease];
    for (size_t i = 0; i < colors.size(); ++i) {
      [color_list insertColor:[self colorFromHexColorString:colors[i]]
                          key:base::SysUTF8ToNSString(colors[i])
                      atIndex:i];
    }
     item.colorList = color_list;
  }

  std::string selectedColor;
  if (settings.Get("selectedColor", &selectedColor)) {
    item.color = [self colorFromHexColorString:selectedColor];
  }
}

- (NSTouchBarItem*)makeSliderForID:(NSString*)id
                    withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  NSSliderTouchBarItem* item = [[NSClassFromString(@"NSSliderTouchBarItem") alloc] initWithIdentifier:identifier];
  item.target = self;
  item.action = @selector(sliderAction:);
  [self updateSlider:item withSettings:settings];
  return item;
}

- (void)updateSlider:(NSSliderTouchBarItem*)item
        withSettings:(const mate::PersistentDictionary&)settings {
  std::string label;
  settings.Get("label", &label);
  item.label = base::SysUTF8ToNSString(label);

  int maxValue = 100;
  int minValue = 0;
  int value = 50;
  settings.Get("minValue", &minValue);
  settings.Get("maxValue", &maxValue);
  settings.Get("value", &value);

  item.slider.minValue = minValue;
  item.slider.maxValue = maxValue;
  item.slider.doubleValue = value;
}

- (NSTouchBarItem*)makePopoverForID:(NSString*)id
                     withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  NSPopoverTouchBarItem* item = [[NSClassFromString(@"NSPopoverTouchBarItem") alloc] initWithIdentifier:identifier];
  [self updatePopover:item withSettings:settings];
  return item;
}

- (void)updatePopover:(NSPopoverTouchBarItem*)item
         withSettings:(const mate::PersistentDictionary&)settings {
  std::string label;
  gfx::Image image;
  if (settings.Get("label", &label)) {
    item.collapsedRepresentationLabel = base::SysUTF8ToNSString(label);
  } else if (settings.Get("image", &image)) {
    item.collapsedRepresentationImage = image.AsNSImage();
  }

  bool showCloseButton;
  if (settings.Get("showCloseButton", &showCloseButton)) {
    item.showsCloseButton = showCloseButton;
  }

  mate::PersistentDictionary child;
  std::vector<mate::PersistentDictionary> items;
  if (settings.Get("child", &child) && child.Get("ordereredItems", &items)) {
    item.popoverTouchBar = [self touchBarFromItemIdentifiers:[self identifiersFromSettings:items]];
  }
}

- (NSTouchBarItem*)makeGroupForID:(NSString*)id
                   withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;
  mate::PersistentDictionary settings = settings_[s_id];

  mate::PersistentDictionary child;
  if (!settings.Get("child", &child)) return nil;
  std::vector<mate::PersistentDictionary> items;
  if (!child.Get("ordereredItems", &items)) return nil;

  NSMutableArray* generatedItems = [[NSMutableArray alloc] init];
  NSMutableArray* identList = [self identifiersFromSettings:items];
  for (NSUInteger i = 0; i < [identList count]; i++) {
    if ([identList objectAtIndex:i] != NSTouchBarItemIdentifierOtherItemsProxy) {
      NSTouchBarItem* generatedItem = [self makeItemForIdentifier:[identList objectAtIndex:i]];
      if (generatedItem) {
        [generatedItems addObject:generatedItem];
      }
    }
  }
  return [NSClassFromString(@"NSGroupTouchBarItem") groupItemWithIdentifier:identifier
                                                                      items:generatedItems];
}

@end
