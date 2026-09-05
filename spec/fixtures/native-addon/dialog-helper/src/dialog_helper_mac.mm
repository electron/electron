#include "dialog_helper.h"

#import <Cocoa/Cocoa.h>

namespace {

// Extract the NSWindow* from the native handle buffer.
// The buffer contains an NSView* (the content view of the window).
NSWindow* GetNSWindowFromHandle(char* handle, size_t size) {
  if (size != sizeof(void*))
    return nil;
  // Read the raw pointer from the buffer, then bridge to ARC.
  void* raw = *reinterpret_cast<void**>(handle);
  NSView* view = (__bridge NSView*)raw;
  if (!view || ![view isKindOfClass:[NSView class]])
    return nil;
  return [view window];
}

}  // namespace

namespace dialog_helper {

DialogInfo GetDialogInfo(char* handle, size_t size) {
  DialogInfo info;
  info.type = "none";

  NSWindow* window = GetNSWindowFromHandle(handle, size);
  if (!window)
    return info;

  NSWindow* sheet = [window attachedSheet];
  if (!sheet)
    return info;

  // NSOpenPanel is a subclass of NSSavePanel, so check NSOpenPanel first.
  if ([sheet isKindOfClass:[NSOpenPanel class]]) {
    info.type = "open-dialog";
    NSOpenPanel* panel = (NSOpenPanel*)sheet;
    info.message = [[panel title] UTF8String] ?: "";
    info.prompt = [[panel prompt] UTF8String] ?: "";
    info.panel_message = [[panel message] UTF8String] ?: "";
    if ([panel directoryURL])
      info.directory = [[[panel directoryURL] path] UTF8String] ?: "";
    info.can_choose_files = [panel canChooseFiles];
    info.can_choose_directories = [panel canChooseDirectories];
    info.allows_multiple_selection = [panel allowsMultipleSelection];
    info.shows_hidden_files = [panel showsHiddenFiles];
    info.resolves_aliases = [panel resolvesAliases];
    info.treats_packages_as_directories = [panel treatsFilePackagesAsDirectories];
    info.can_create_directories = [panel canCreateDirectories];
    return info;
  }

  if ([sheet isKindOfClass:[NSSavePanel class]]) {
    info.type = "save-dialog";
    NSSavePanel* panel = (NSSavePanel*)sheet;
    info.message = [[panel title] UTF8String] ?: "";
    info.prompt = [[panel prompt] UTF8String] ?: "";
    info.panel_message = [[panel message] UTF8String] ?: "";
    if ([panel directoryURL])
      info.directory = [[[panel directoryURL] path] UTF8String] ?: "";
    info.name_field_label = [[panel nameFieldLabel] UTF8String] ?: "";
    info.name_field_value = [[panel nameFieldStringValue] UTF8String] ?: "";
    info.shows_tag_field = [panel showsTagField];
    info.shows_hidden_files = [panel showsHiddenFiles];
    info.treats_packages_as_directories =
        [panel treatsFilePackagesAsDirectories];
    info.can_create_directories = [panel canCreateDirectories];
    return info;
  }

  // For NSAlert, the sheet window is not an NSSavePanel.
  // Check if it contains typical NSAlert button structure.
  // NSAlert's window contains buttons as subviews in its content view.
  NSView* contentView = [sheet contentView];
  NSMutableArray<NSButton*>* buttons = [NSMutableArray array];

  // Recursively find all NSButton instances in the view hierarchy.
  NSMutableArray<NSView*>* stack =
      [NSMutableArray arrayWithObject:contentView];
  while ([stack count] > 0) {
    NSView* current = [stack lastObject];
    [stack removeLastObject];
    if ([current isKindOfClass:[NSButton class]]) {
      NSButton* btn = (NSButton*)current;
      // Filter to push-type buttons (not checkboxes, radio buttons, etc.)
      if ([btn bezelStyle] == NSBezelStyleRounded ||
          [btn bezelStyle] == NSBezelStyleRegularSquare) {
        [buttons addObject:btn];
      }
    }
    for (NSView* subview in [current subviews]) {
      [stack addObject:subview];
    }
  }

  if ([buttons count] > 0) {
    info.type = "message-box";

    // Sort buttons by tag to maintain the order they were added.
    [buttons sortUsingComparator:^NSComparisonResult(NSButton* a, NSButton* b) {
      if ([a tag] < [b tag])
        return NSOrderedAscending;
      if ([a tag] > [b tag])
        return NSOrderedDescending;
      return NSOrderedSame;
    }];

    std::string btn_json = "[";
    for (NSUInteger i = 0; i < [buttons count]; i++) {
      if (i > 0)
        btn_json += ",";
      btn_json += "\"";
      NSString* title = [[buttons objectAtIndex:i] title];
      btn_json += [title UTF8String] ?: "";
      btn_json += "\"";
    }
    btn_json += "]";
    info.buttons = btn_json;

    // NSAlert's content view contains static NSTextFields for message and
    // detail text. The first non-editable text field with content is the
    // message; the second is the detail (informative text).
    int text_field_index = 0;
    // Walk all subviews (non-recursive — NSAlert places labels directly).
    for (NSView* subview in [contentView subviews]) {
      if ([subview isKindOfClass:[NSTextField class]]) {
        NSTextField* field = (NSTextField*)subview;
        if (![field isEditable] && [[field stringValue] length] > 0) {
          if (text_field_index == 0) {
            info.message = [[field stringValue] UTF8String];
          } else if (text_field_index == 1) {
            info.detail = [[field stringValue] UTF8String];
          }
          text_field_index++;
        }
      }
    }

    // Check for the suppression (checkbox) button.
    // NSAlert's suppression button is a non-bordered NSButton, unlike
    // push buttons which are bordered. This reliably identifies it
    // across macOS versions where the accessibility role may differ.
    NSMutableArray<NSView*>* cbStack =
        [NSMutableArray arrayWithObject:contentView];
    while ([cbStack count] > 0) {
      NSView* current = [cbStack lastObject];
      [cbStack removeLastObject];
      if ([current isKindOfClass:[NSButton class]]) {
        NSButton* btn = (NSButton*)current;
        if (![btn isBordered]) {
          NSString* title = [btn title];
          if (title && [title length] > 0) {
            info.checkbox_label = [title UTF8String];
            info.checkbox_checked =
                ([btn state] == NSControlStateValueOn);
          }
        }
      }
      for (NSView* sub in [current subviews]) {
        [cbStack addObject:sub];
      }
    }
  }

  return info;
}

bool ClickMessageBoxButton(char* handle, size_t size, int button_index) {
  NSWindow* window = GetNSWindowFromHandle(handle, size);
  if (!window)
    return false;

  NSWindow* sheet = [window attachedSheet];
  if (!sheet)
    return false;

  // Find buttons in the sheet, sorted by tag.
  NSView* contentView = [sheet contentView];
  NSMutableArray<NSButton*>* buttons = [NSMutableArray array];

  NSMutableArray<NSView*>* stack =
      [NSMutableArray arrayWithObject:contentView];
  while ([stack count] > 0) {
    NSView* current = [stack lastObject];
    [stack removeLastObject];
    if ([current isKindOfClass:[NSButton class]]) {
      NSButton* btn = (NSButton*)current;
      if ([btn bezelStyle] == NSBezelStyleRounded ||
          [btn bezelStyle] == NSBezelStyleRegularSquare) {
        [buttons addObject:btn];
      }
    }
    for (NSView* subview in [current subviews]) {
      [stack addObject:subview];
    }
  }

  [buttons sortUsingComparator:^NSComparisonResult(NSButton* a, NSButton* b) {
    if ([a tag] < [b tag])
      return NSOrderedAscending;
    if ([a tag] > [b tag])
      return NSOrderedDescending;
    return NSOrderedSame;
  }];

  if (button_index < 0 || button_index >= (int)[buttons count])
    return false;

  NSButton* target = [buttons objectAtIndex:button_index];
  [target performClick:nil];
  return true;
}

bool ClickCheckbox(char* handle, size_t size) {
  NSWindow* window = GetNSWindowFromHandle(handle, size);
  if (!window)
    return false;

  NSWindow* sheet = [window attachedSheet];
  if (!sheet)
    return false;

  // Find the suppression/checkbox button — it is a non-bordered NSButton,
  // unlike the push buttons which are bordered.
  NSView* contentView = [sheet contentView];
  NSMutableArray<NSView*>* stack =
      [NSMutableArray arrayWithObject:contentView];
  while ([stack count] > 0) {
    NSView* current = [stack lastObject];
    [stack removeLastObject];
    if ([current isKindOfClass:[NSButton class]]) {
      NSButton* btn = (NSButton*)current;
      if (![btn isBordered] && [[btn title] length] > 0) {
        [btn performClick:nil];
        return true;
      }
    }
    for (NSView* subview in [current subviews]) {
      [stack addObject:subview];
    }
  }

  return false;
}

bool CancelFileDialog(char* handle, size_t size) {
  NSWindow* window = GetNSWindowFromHandle(handle, size);
  if (!window)
    return false;

  NSWindow* sheet = [window attachedSheet];
  if (!sheet)
    return false;

  // sheet is the NSSavePanel/NSOpenPanel window itself when presented as a
  // sheet. We need to find the actual panel object. On macOS, when an
  // NSSavePanel is run as a sheet, [window attachedSheet] returns the panel's
  // window. The panel can be retrieved because NSSavePanel IS the window.
  if ([sheet isKindOfClass:[NSSavePanel class]]) {
    NSSavePanel* panel = (NSSavePanel*)sheet;
    [panel cancel:nil];
    return true;
  }

  // If it's not a recognized panel type, try ending the sheet directly.
  [NSApp endSheet:sheet returnCode:NSModalResponseCancel];
  return true;
}

bool AcceptFileDialog(char* handle, size_t size, const std::string& filename) {
  NSWindow* window = GetNSWindowFromHandle(handle, size);
  if (!window)
    return false;

  NSWindow* sheet = [window attachedSheet];
  if (!sheet)
    return false;

  if (![sheet isKindOfClass:[NSSavePanel class]])
    return false;

  NSSavePanel* panel = (NSSavePanel*)sheet;

  // Set the filename if provided (for save dialogs).
  if (!filename.empty()) {
    NSString* name = [NSString stringWithUTF8String:filename.c_str()];
    [panel setNameFieldStringValue:name];
    // Resign first responder to commit the name field edit. Without this,
    // the panel may still use the previous value (e.g. "Untitled") when
    // the accept button is clicked immediately after.
    [sheet makeFirstResponder:nil];
  }

  NSView* contentView = [sheet contentView];

  // Search for the default button (key equivalent "\r") in the view hierarchy.
  NSMutableArray<NSView*>* stack =
      [NSMutableArray arrayWithObject:contentView];
  while ([stack count] > 0) {
    NSView* current = [stack lastObject];
    [stack removeLastObject];
    if ([current isKindOfClass:[NSButton class]]) {
      NSButton* btn = (NSButton*)current;
      if ([[btn keyEquivalent] isEqualToString:@"\r"]) {
        [btn performClick:nil];
        return true;
      }
    }
    for (NSView* subview in [current subviews]) {
      [stack addObject:subview];
    }
  }

  [NSApp endSheet:sheet returnCode:NSModalResponseOK];
  return true;
}

}  // namespace dialog_helper
