// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/atom_browser_main_parts.h"

#include "shell/browser/atom_paths.h"
#include "shell/browser/mac/atom_application_delegate.h"

namespace electron {

namespace {

base::scoped_nsobject<NSMenuItem> CreateMenuItem(NSString* title,
                                                 SEL action,
                                                 NSString* key_equivalent) {
  return base::scoped_nsobject<NSMenuItem>([[NSMenuItem alloc]
      initWithTitle:title
             action:action
      keyEquivalent:key_equivalent]);
}

// The App Menu refers to the dropdown titled "Electron".
base::scoped_nsobject<NSMenu> BuildAppMenu() {
  // The title is not used, as the title will always be the name of the App.
  base::scoped_nsobject<NSMenu> menu([[NSMenu alloc] initWithTitle:@""]);

  NSString* app_name = [[[NSBundle mainBundle] infoDictionary]
      objectForKey:(id)kCFBundleNameKey];

  base::scoped_nsobject<NSMenuItem> item =
      CreateMenuItem([NSString stringWithFormat:@"Quit %@", app_name],
                     @selector(terminate:), @"q");
  [menu addItem:item];

  return menu;
}

base::scoped_nsobject<NSMenu> BuildEmptyMainMenu() {
  base::scoped_nsobject<NSMenu> main_menu([[NSMenu alloc] initWithTitle:@""]);

  using Builder = base::scoped_nsobject<NSMenu> (*)();
  static const Builder kBuilderFuncs[] = {&BuildAppMenu};
  for (auto* builder : kBuilderFuncs) {
    NSMenuItem* item = [[[NSMenuItem alloc] initWithTitle:@""
                                                   action:NULL
                                            keyEquivalent:@""] autorelease];
    item.submenu = builder();
    [main_menu addItem:item];
  }
  return main_menu;
}

}  // namespace

void AtomBrowserMainParts::PreMainMessageLoopStart() {
  // Set our own application delegate.
  AtomApplicationDelegate* delegate = [[AtomApplicationDelegate alloc] init];
  [NSApp setDelegate:delegate];

  PreMainMessageLoopStartCommon();

  // Prevent Cocoa from turning command-line arguments into
  // |-application:openFiles:|, since we already handle them directly.
  [[NSUserDefaults standardUserDefaults]
      setObject:@"NO"
         forKey:@"NSTreatUnknownArgumentsAsOpen"];
}

void AtomBrowserMainParts::FreeAppDelegate() {
  [[NSApp delegate] release];
  [NSApp setDelegate:nil];
}

void AtomBrowserMainParts::InitializeEmptyApplicationMenu() {
  base::scoped_nsobject<NSMenu> main_menu = BuildEmptyMainMenu();
  [[NSApplication sharedApplication] setMainMenu:main_menu];
}

}  // namespace electron
