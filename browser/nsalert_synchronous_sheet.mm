// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be

#import "browser/nsalert_synchronous_sheet.h"

// Private methods -- use prefixes to avoid collisions with Apple's methods
@interface NSAlert (SynchronousSheetPrivate)
-(IBAction) BE_stopSynchronousSheet:(id)sender;   // hide sheet & stop modal
-(void) BE_beginSheetModalForWindow:(NSWindow *)aWindow;
@end

@implementation NSAlert (SynchronousSheet)

-(NSInteger) runModalSheetForWindow:(NSWindow *)aWindow {
  // Set ourselves as the target for button clicks
  for (NSButton *button in [self buttons]) {
    [button setTarget:self];
    [button setAction:@selector(BE_stopSynchronousSheet:)];
  }

  // Bring up the sheet and wait until stopSynchronousSheet is triggered by a button click
  [self performSelectorOnMainThread:@selector(BE_beginSheetModalForWindow:) withObject:aWindow waitUntilDone:YES];
  NSInteger modalCode = [NSApp runModalForWindow:[self window]];

  // This is called only after stopSynchronousSheet is called (that is,
  // one of the buttons is clicked)
  [NSApp performSelectorOnMainThread:@selector(endSheet:) withObject:[self window] waitUntilDone:YES];

  // Remove the sheet from the screen
  [[self window] performSelectorOnMainThread:@selector(orderOut:) withObject:self waitUntilDone:YES];

  return modalCode;
}

-(NSInteger) runModalSheet {
  return [self runModalSheetForWindow:[NSApp mainWindow]];
}


#pragma mark Private methods

-(IBAction) BE_stopSynchronousSheet:(id)sender {
  [NSApp stopModalWithCode:[sender  tag]];
}

-(void) BE_beginSheetModalForWindow:(NSWindow *)aWindow {
  [self beginSheetModalForWindow:aWindow modalDelegate:nil didEndSelector:nil contextInfo:nil];
}

@end
