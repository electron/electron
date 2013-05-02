// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "browser/atom_application_mac.h"

#include "base/auto_reset.h"
#include "base/logging.h"
#include "browser/window_list.h"

@implementation AtomApplication

- (BOOL)isHandlingSendEvent {
  return handlingSendEvent_;
}

- (void)sendEvent:(NSEvent*)event {
  base::AutoReset<BOOL> scoper(&handlingSendEvent_, YES);
  [super sendEvent:event];
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent {
  handlingSendEvent_ = handlingSendEvent;
}

+ (AtomApplication*)sharedApplication {
  return (AtomApplication*)[super sharedApplication];
}

- (IBAction)closeAllWindows:(id)sender {
  atom::WindowList* window_list = atom::WindowList::GetInstance();
  if (window_list->size() == 0)
    [self terminate:self];

  window_list->CloseAllWindows();
}

@end
