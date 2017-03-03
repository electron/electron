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
static NSTouchBarItemIdentifier PopoverIdentifier = @"com.electron.touchbar.popover.";
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
  NSMutableArray* identifiers = [NSMutableArray array];

  for (const auto& item : dicts) {
    std::string type;
    std::string item_id;
    if (item.Get("type", &type) && item.Get("id", &item_id)) {
      NSTouchBarItemIdentifier identifier = nil;
      if (type == "spacer") {
        std::string size;
        item.Get("size", &size);
        if (size == "large") {
          identifier = NSTouchBarItemIdentifierFixedSpaceLarge;
        } else if (size == "flexible") {
          identifier = NSTouchBarItemIdentifierFlexibleSpace;
        } else {
          identifier = NSTouchBarItemIdentifierFixedSpaceSmall;
        }
      } else {
        identifier = [self identifierFromID:item_id type:type];
      }

      if (identifier) {
        settings_[item_id] = item;
        [identifiers addObject:identifier];
      }
    }
  }
  [identifiers addObject:NSTouchBarItemIdentifierOtherItemsProxy];

  return identifiers;
}

- (NSTouchBarItem*)makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
  NSString* item_id = nil;

  if ([identifier hasPrefix:ButtonIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:ButtonIdentifier];
    return [self makeButtonForID:item_id withIdentifier:identifier];
  } else if ([identifier hasPrefix:LabelIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:LabelIdentifier];
    return [self makeLabelForID:item_id withIdentifier:identifier];
  } else if ([identifier hasPrefix:ColorPickerIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:ColorPickerIdentifier];
    return [self makeColorPickerForID:item_id withIdentifier:identifier];
  } else if ([identifier hasPrefix:SliderIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:SliderIdentifier];
    return [self makeSliderForID:item_id withIdentifier:identifier];
  } else if ([identifier hasPrefix:PopoverIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:PopoverIdentifier];
    return [self makePopoverForID:item_id withIdentifier:identifier];
  } else if ([identifier hasPrefix:GroupIdentifier]) {
    item_id = [self idFromIdentifier:identifier withPrefix:GroupIdentifier];
    return [self makeGroupForID:item_id withIdentifier:identifier];
  }

  return nil;
}


- (void)refreshTouchBarItem:(NSTouchBar*)touchBar
                         id:(const std::string&)item_id {
  if (![self hasItemWithID:item_id]) return;

  mate::PersistentDictionary settings = settings_[item_id];
  std::string item_type;
  settings.Get("type", &item_type);

  NSTouchBarItemIdentifier identifier = [self identifierFromID:item_id
                                                          type:item_type];
  if (!identifier) return;

  NSTouchBarItem* item = [touchBar itemForIdentifier:identifier];
  if (!item) return;

  if (item_type == "button") {
    [self updateButton:(NSCustomTouchBarItem*)item withSettings:settings];
  } else if (item_type == "label") {
    [self updateLabel:(NSCustomTouchBarItem*)item withSettings:settings];
  } else if (item_type == "colorpicker") {
    [self updateColorPicker:(NSColorPickerTouchBarItem*)item
               withSettings:settings];
  } else if (item_type == "slider") {
    [self updateSlider:(NSSliderTouchBarItem*)item withSettings:settings];
  } else if (item_type == "popover") {
    [self updatePopover:(NSPopoverTouchBarItem*)item withSettings:settings];
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

- (NSString*)idFromIdentifier:(NSString*)identifier
                  withPrefix:(NSString*)prefix {
  return [identifier substringFromIndex:[prefix length]];
}

- (NSTouchBarItemIdentifier)identifierFromID:(const std::string&)item_id
                                        type:(const std::string&)type {
  NSTouchBarItemIdentifier base_identifier = nil;
  if (type == "button")
    base_identifier = ButtonIdentifier;
  else if (type == "label")
    base_identifier = LabelIdentifier;
  else if (type == "colorpicker")
    base_identifier = ColorPickerIdentifier;
  else if (type == "slider")
    base_identifier = SliderIdentifier;
  else if (type == "popover")
    base_identifier = PopoverIdentifier;
  else if (type == "group")
    base_identifier = GroupIdentifier;

  if (base_identifier)
    return [NSString stringWithFormat:@"%@%s", base_identifier, item_id.data()];
  else
    return nil;
}

- (bool)hasItemWithID:(const std::string&)item_id {
  return settings_.find(item_id) != settings_.end();
}

- (NSColor*)colorFromHexColorString:(const std::string&)colorString {
  SkColor color = atom::ParseHexColor(colorString);
  return skia::SkColorToDeviceNSColor(color);
}

- (NSTouchBarItem*)makeButtonForID:(NSString*)id
                    withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  base::scoped_nsobject<NSCustomTouchBarItem> item([[NSClassFromString(
      @"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier]);
  NSButton* button = [NSButton buttonWithTitle:@""
                                        target:self
                                        action:@selector(buttonAction:)];
  button.tag = [id floatValue];
  [item setView:button];
  [self updateButton:item withSettings:settings];
  return item.autorelease();
}

- (void)updateButton:(NSCustomTouchBarItem*)item
        withSettings:(const mate::PersistentDictionary&)settings {
  NSButton* button = (NSButton*)item.view;

  std::string backgroundColor;
  if (settings.Get("backgroundColor", &backgroundColor)) {
    button.bezelColor = [self colorFromHexColorString:backgroundColor];
  }

  std::string label;
  settings.Get("label", &label);
  button.title = base::SysUTF8ToNSString(label);

  gfx::Image image;
  if (settings.Get("icon", &image)) {
    button.image = image.AsNSImage();
  }
}

- (NSTouchBarItem*)makeLabelForID:(NSString*)id
                   withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  base::scoped_nsobject<NSCustomTouchBarItem> item([[NSClassFromString(
      @"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier]);
  [item setView:[NSTextField labelWithString:@""]];
  [self updateLabel:item withSettings:settings];
  return item.autorelease();
}

- (void)updateLabel:(NSCustomTouchBarItem*)item
       withSettings:(const mate::PersistentDictionary&)settings {
  NSTextField* text_field = (NSTextField*)item.view;

  std::string label;
  settings.Get("label", &label);
  text_field.stringValue = base::SysUTF8ToNSString(label);

  std::string textColor;
  if (settings.Get("textColor", &textColor) && !textColor.empty()) {
    text_field.textColor = [self colorFromHexColorString:textColor];
  } else {
    text_field.textColor = nil;
  }
}

- (NSTouchBarItem*)makeColorPickerForID:(NSString*)id
                         withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasItemWithID:s_id]) return nil;

  mate::PersistentDictionary settings = settings_[s_id];
  base::scoped_nsobject<NSColorPickerTouchBarItem> item([[NSClassFromString(
    @"NSColorPickerTouchBarItem") alloc] initWithIdentifier:identifier]);
  [item setTarget:self];
  [item setAction:@selector(colorPickerAction:)];
  [self updateColorPicker:item withSettings:settings];
  return item.autorelease();
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
  base::scoped_nsobject<NSSliderTouchBarItem> item([[NSClassFromString(
      @"NSSliderTouchBarItem") alloc] initWithIdentifier:identifier]);
  [item setTarget:self];
  [item setAction:@selector(sliderAction:)];
  [self updateSlider:item withSettings:settings];
  return item.autorelease();
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
  base::scoped_nsobject<NSPopoverTouchBarItem> item([[NSClassFromString(
      @"NSPopoverTouchBarItem") alloc] initWithIdentifier:identifier]);
  [self updatePopover:item withSettings:settings];
  return item.autorelease();
}

- (void)updatePopover:(NSPopoverTouchBarItem*)item
         withSettings:(const mate::PersistentDictionary&)settings {
  std::string label;
  settings.Get("label", &label);
  item.collapsedRepresentationLabel = base::SysUTF8ToNSString(label);

  gfx::Image image;
  if (settings.Get("icon", &image)) {
    item.collapsedRepresentationImage = image.AsNSImage();
  }

  bool showCloseButton = true;
  settings.Get("showCloseButton", &showCloseButton);
  item.showsCloseButton = showCloseButton;

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

  NSMutableArray* generatedItems = [NSMutableArray array];
  NSMutableArray* identifiers = [self identifiersFromSettings:items];
  for (NSUInteger i = 0; i < [identifiers count]; i++) {
    if ([identifiers objectAtIndex:i] != NSTouchBarItemIdentifierOtherItemsProxy) {
      NSTouchBarItem* generatedItem = [self makeItemForIdentifier:[identifiers objectAtIndex:i]];
      if (generatedItem) {
        [generatedItems addObject:generatedItem];
      }
    }
  }
  return [NSClassFromString(@"NSGroupTouchBarItem") groupItemWithIdentifier:identifier
                                                                      items:generatedItems];
}

@end
