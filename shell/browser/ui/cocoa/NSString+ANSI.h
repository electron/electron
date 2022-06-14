// Created by Kent Karlsson on 3/11/16.
// Copyright (c) 2016 Bit Bar. All rights reserved.
// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_NSSTRING_ANSI_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_NSSTRING_ANSI_H_

#import <Foundation/Foundation.h>

@interface NSString (ANSI)
- (BOOL)containsANSICodes;
- (NSMutableAttributedString*)attributedStringParsingANSICodes;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_NSSTRING_ANSI_H_
