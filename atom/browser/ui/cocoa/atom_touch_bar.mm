// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import "atom/browser/ui/cocoa/atom_touch_bar.h"

#include "atom/common/color_util.h"
#include "atom/common/native_mate_converters/image_converter.h"
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
                window:(atom::NativeWindow*)window {
  if ((self = [super init])) {
    delegate_ = delegate;
    window_ = window;
  }
  return self;
}

- (void)clear {
  item_id_map.clear();
}

- (NSTouchBar*)makeTouchBarFromItemOptions:(const std::vector<mate::PersistentDictionary>&)item_options {
  NSMutableArray* identifiers = [self identifierArrayFromDicts:item_options];
  return [self touchBarFromMutatableArray:identifiers];
}

- (NSTouchBar*)touchBarFromMutatableArray:(NSMutableArray*)items {
  NSTouchBar* bar = [[NSClassFromString(@"NSTouchBar") alloc] init];
  bar.delegate = delegate_;
  bar.defaultItemIdentifiers = items;
  return bar;
}

- (NSMutableArray*)identifierArrayFromDicts:(const std::vector<mate::PersistentDictionary>&)dicts {
  NSMutableArray* idents = [[NSMutableArray alloc] init];

  for (const auto& item : dicts) {
    std::string type;
    std::string item_id;
    if (item.Get("type", &type) && item.Get("id", &item_id)) {
      item_id_map.insert(make_pair(item_id, item));
      if (type == "button") {
        [idents addObject:[NSString stringWithFormat:@"%@%@", ButtonIdentifier, [NSString stringWithUTF8String:item_id.c_str()]]];
      } else if (type == "label") {
        [idents addObject:[NSString stringWithFormat:@"%@%@", LabelIdentifier, [NSString stringWithUTF8String:item_id.c_str()]]];
      } else if (type == "colorpicker") {
        [idents addObject:[NSString stringWithFormat:@"%@%@", ColorPickerIdentifier, [NSString stringWithUTF8String:item_id.c_str()]]];
      } else if (type == "slider") {
        [idents addObject:[NSString stringWithFormat:@"%@%@", SliderIdentifier, [NSString stringWithUTF8String:item_id.c_str()]]];
      } else if (type == "popover") {
        [idents addObject:[NSString stringWithFormat:@"%@%@", PopOverIdentifier, [NSString stringWithUTF8String:item_id.c_str()]]];
      } else if (type == "group") {
        [idents addObject:[NSString stringWithFormat:@"%@%@", GroupIdentifier, [NSString stringWithUTF8String:item_id.c_str()]]];
      }
    }
  }
  [idents addObject:NSTouchBarItemIdentifierOtherItemsProxy];

  return idents;
}

- (NSTouchBarItem*)makeItemForIdentifier:(NSTouchBarItemIdentifier)identifier {
  NSTouchBarItem* item = nil;
  NSString* id = nil;
  if ([identifier hasPrefix:ButtonIdentifier]) {
    id = [self idFromIdentifier:identifier withPrefix:ButtonIdentifier];
    item = [self makeButtonForID:id withIdentifier:identifier];
  } else if ([identifier hasPrefix:LabelIdentifier]) {
    id = [self idFromIdentifier:identifier withPrefix:LabelIdentifier];
    item = [self makeLabelForID:id withIdentifier:identifier];
  } else if ([identifier hasPrefix:ColorPickerIdentifier]) {
    id = [self idFromIdentifier:identifier withPrefix:ColorPickerIdentifier];
    item = [self makeColorPickerForID:id withIdentifier:identifier];
  } else if ([identifier hasPrefix:SliderIdentifier]) {
    id = [self idFromIdentifier:identifier withPrefix:SliderIdentifier];
    item = [self makeSliderForID:id withIdentifier:identifier];
  } else if ([identifier hasPrefix:PopOverIdentifier]) {
    id = [self idFromIdentifier:identifier withPrefix:PopOverIdentifier];
    item = [self makePopoverForID:id withIdentifier:identifier];
  } else if ([identifier hasPrefix:GroupIdentifier]) {
    id = [self idFromIdentifier:identifier withPrefix:GroupIdentifier];
    item = [self makeGroupForID:id withIdentifier:identifier];
  }

  item_map.insert(make_pair(std::string([id UTF8String]), item));

  return item;
}


- (void)refreshTouchBarItem:(mate::Arguments*)args {
  std::string item_id;
  std::string type;
  mate::PersistentDictionary options;
  if (!args->GetNext(&options)) return;
  if (!options.Get("type", &type)) return;
  if (!options.Get("id", &item_id)) return;
  if (item_map.find(item_id) == item_map.end()) return;

  if (type == "button") {
    [self updateButton:(NSCustomTouchBarItem*)item_map[item_id]
           withOptions:options];
  } else if (type == "label") {
    [self updateLabel:(NSCustomTouchBarItem*)item_map[item_id]
          withOptions:options];
  } else if (type == "colorpicker") {
    [self updateColorPicker:(NSColorPickerTouchBarItem*)item_map[item_id]
                withOptions:options];
  } else if (type == "slider") {
    [self updateSlider:(NSSliderTouchBarItem*)item_map[item_id]
           withOptions:options];
  } else if (type == "popover") {
    [self updatePopover:(NSPopoverTouchBarItem*)item_map[item_id]
            withOptions:options];
  } else if (type == "group") {
    args->ThrowError("You can not update the config of a group. Update the individual items or replace the group");
  }
}

- (void)buttonAction:(id)sender {
  NSString* item_id = [NSString stringWithFormat:@"%@.%d", ButtonIdentifier, (int)((NSButton*)sender).tag];
  window_->NotifyTouchBarItemInteraction("button", { std::string([item_id UTF8String]) });
}

- (void)colorPickerAction:(id)sender {
  NSString* item_id = ((NSColorPickerTouchBarItem*)sender).identifier;
  NSColor* color = ((NSColorPickerTouchBarItem*)sender).color;
  std::string hex_color = atom::ToRGBHex(skia::NSDeviceColorToSkColor(color));
  window_->NotifyTouchBarItemInteraction("color_picker", { std::string([item_id UTF8String]), hex_color });
}

- (void)sliderAction:(id)sender {
  NSString* item_id = ((NSSliderTouchBarItem*)sender).identifier;
  window_->NotifyTouchBarItemInteraction("slider", { std::string([item_id UTF8String]), std::to_string([((NSSliderTouchBarItem*)sender).slider intValue]) });
}

- (NSString*)idFromIdentifier:(NSString*)identifier withPrefix:(NSString*)prefix {
  return [identifier substringFromIndex:[prefix length]];
}

- (bool)hasTBDict:(const std::string&)id {
  return item_id_map.find(id) != item_id_map.end();
}

- (NSColor*)colorFromHexColorString:(const std::string&)colorString {
  SkColor color = atom::ParseHexColor(colorString);
  return skia::SkColorToCalibratedNSColor(color);
}

- (NSTouchBarItem*)makeButtonForID:(NSString*)id
                             withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasTBDict:s_id]) return nil;

  mate::PersistentDictionary options = item_id_map[s_id];
  NSCustomTouchBarItem* item = [[NSClassFromString(@"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier];
  NSButton* button = [NSButton buttonWithTitle:@""
                                        target:self
                                        action:@selector(buttonAction:)];
  button.tag = [id floatValue];
  item.view = button;
  [self updateButton:item withOptions:options];
  return item;
}

- (void)updateButton:(NSCustomTouchBarItem*)item
         withOptions:(const mate::PersistentDictionary&)options {
  NSButton* button = (NSButton*)item.view;

  std::string customizationLabel;
  if (options.Get("customizationLabel", &customizationLabel)) {
    item.customizationLabel = [NSString stringWithUTF8String:customizationLabel.data()];
  }

  std::string backgroundColor;
  if (options.Get("backgroundColor", &backgroundColor)) {
    button.bezelColor = [self colorFromHexColorString:backgroundColor];
  }

  std::string label;
  if (options.Get("label", &label)) {
    button.title = [NSString stringWithUTF8String:label.data()];
  }

  std::string labelColor;
  if (!label.empty() && options.Get("labelColor", &labelColor)) {
    NSMutableAttributedString* attrTitle = [[[NSMutableAttributedString alloc] initWithString:[NSString stringWithUTF8String:label.data()]] autorelease];
    NSRange range = NSMakeRange(0, [attrTitle length]);
    [attrTitle addAttribute:NSForegroundColorAttributeName
                      value:[self colorFromHexColorString:labelColor]
                      range:range];
    [attrTitle fixAttributesInRange:range];
    button.attributedTitle = attrTitle;
  }

  gfx::Image image;
  if (options.Get("image", &image)) {
    button.image = image.AsNSImage();
  }
}

- (NSTouchBarItem*)makeLabelForID:(NSString*)id
                            withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasTBDict:s_id]) return nil;

  mate::PersistentDictionary item = item_id_map[s_id];
  NSCustomTouchBarItem* customItem = [[NSClassFromString(@"NSCustomTouchBarItem") alloc] initWithIdentifier:identifier];
  customItem.view = [NSTextField labelWithString:@""];
  [self updateLabel:customItem withOptions:item];

  return customItem;
}

- (void)updateLabel:(NSCustomTouchBarItem*)item
        withOptions:(const mate::PersistentDictionary&)options {
  std::string label;
  options.Get("label", &label);
  NSTextField* text_field = (NSTextField*)item.view;
  text_field.stringValue = [NSString stringWithUTF8String:label.data()];

  std::string customizationLabel;
  if (options.Get("customizationLabel", &customizationLabel)) {
    item.customizationLabel = [NSString stringWithUTF8String:customizationLabel.data()];
  }
}

- (NSTouchBarItem*)makeColorPickerForID:(NSString*)id
                                  withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasTBDict:s_id]) return nil;

  mate::PersistentDictionary options = item_id_map[s_id];
  NSColorPickerTouchBarItem* item = [[NSClassFromString(@"NSColorPickerTouchBarItem") alloc] initWithIdentifier:identifier];
  item.target = self;
  item.action = @selector(colorPickerAction:);
  [self updateColorPicker:item withOptions:options];
  return item;
}

- (void)updateColorPicker:(NSColorPickerTouchBarItem*)item
              withOptions:(const mate::PersistentDictionary)options {
  std::string customizationLabel;
  if (options.Get("customizationLabel", &customizationLabel)) {
    item.customizationLabel = [NSString stringWithUTF8String:customizationLabel.data()];
  }
}

- (NSTouchBarItem*)makeSliderForID:(NSString*)id
                             withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasTBDict:s_id]) return nil;

  mate::PersistentDictionary options = item_id_map[s_id];
  NSSliderTouchBarItem* item = [[NSClassFromString(@"NSSliderTouchBarItem") alloc] initWithIdentifier:identifier];
  item.target = self;
  item.action = @selector(sliderAction:);
  [self updateSlider:item withOptions:options];
  return item;
}

- (void)updateSlider:(NSSliderTouchBarItem*)item
         withOptions:(const mate::PersistentDictionary&)options {
  std::string customizationLabel;
  if (options.Get("customizationLabel", &customizationLabel)) {
    item.customizationLabel = [NSString stringWithUTF8String:customizationLabel.data()];
  }

  std::string label;
  options.Get("label", &label);
  item.label = [NSString stringWithUTF8String:label.data()];

  int maxValue = 100;
  int minValue = 0;
  int initialValue = 50;
  options.Get("minValue", &minValue);
  options.Get("maxValue", &maxValue);
  options.Get("initialValue", &initialValue);

  item.slider.minValue = minValue;
  item.slider.maxValue = maxValue;
  item.slider.doubleValue = initialValue;
}

- (NSTouchBarItem*)makePopoverForID:(NSString*)id
                              withIdentifier:(NSString*)identifier {
  std::string s_id = std::string([id UTF8String]);
  if (![self hasTBDict:s_id]) return nil;

  mate::PersistentDictionary options = item_id_map[s_id];
  NSPopoverTouchBarItem* item = [[NSClassFromString(@"NSPopoverTouchBarItem") alloc] initWithIdentifier:identifier];
  [self updatePopover:item withOptions:options];
  return item;
}

- (void)updatePopover:(NSPopoverTouchBarItem*)item
          withOptions:(const mate::PersistentDictionary&)options {
  std::string customizationLabel;
  if (options.Get("customizationLabel", &customizationLabel)) {
    item.customizationLabel = [NSString stringWithUTF8String:customizationLabel.data()];
  }

  std::string label;
  gfx::Image image;
  if (options.Get("label", &label)) {
    item.collapsedRepresentationLabel = [NSString stringWithUTF8String:label.data()];
  } else if (options.Get("image", &image)) {
    item.collapsedRepresentationImage = image.AsNSImage();
  }

  bool showCloseButton;
  if (options.Get("showCloseButton", &showCloseButton)) {
    item.showsCloseButton = showCloseButton;
  }

  std::vector<mate::PersistentDictionary> touchBar;
  if (options.Get("touchBar", &touchBar)) {
    item.popoverTouchBar = [self touchBarFromMutatableArray:[self identifierArrayFromDicts:touchBar]];
  }
}

- (NSTouchBarItem*)makeGroupForID:(NSString*)id
                   withIdentifier:(NSString*)identifier {
  std::string s_id([id UTF8String]);
  if (![self hasTBDict:s_id]) return nil;
  mate::PersistentDictionary options = item_id_map[s_id];

  std::vector<mate::PersistentDictionary> items;
  if (!options.Get("items", &items)) return nil;

  NSMutableArray* generatedItems = [[NSMutableArray alloc] init];
  NSMutableArray* identList = [self identifierArrayFromDicts:items];
  for (NSUInteger i = 0; i < [identList count]; i++) {
    if ([identList objectAtIndex:i] != NSTouchBarItemIdentifierOtherItemsProxy) {
      NSTouchBarItem* generatedItem = [self makeItemForIdentifier:[identList objectAtIndex:i]];
      if (generatedItem) {
        [generatedItems addObject:generatedItem];
      }
    }
  }

  NSGroupTouchBarItem* item = [NSClassFromString(@"NSGroupTouchBarItem") groupItemWithIdentifier:identifier items:generatedItems];
  std::string customizationLabel;
  if (options.Get("customizationLabel", &customizationLabel)) {
    item.customizationLabel = [NSString stringWithUTF8String:customizationLabel.data()];
  }
  return item;
}

@end
