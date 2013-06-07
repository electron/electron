// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

@interface NSAlert (SynchronousSheet)
-(NSInteger) runModalSheetForWindow:(NSWindow*)aWindow;
-(NSInteger) runModalSheet;
@end
