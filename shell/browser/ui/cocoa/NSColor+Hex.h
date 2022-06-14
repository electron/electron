// Created by Mathias Leppich on 03/02/14.
// Copyright (c) 2014 Bit Bar. All rights reserved.
// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_NSCOLOR_HEX_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_NSCOLOR_HEX_H_

#import <Cocoa/Cocoa.h>

@interface NSColor (Hex)
- (NSString*)hexadecimalValue;
- (NSString*)RGBAValue;
+ (NSColor*)colorWithHexColorString:(NSString*)hex;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_NSCOLOR_HEX_H_
