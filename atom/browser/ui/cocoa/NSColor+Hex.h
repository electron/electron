// Created by Mathias Leppich on 03/02/14.
// Copyright (c) 2014 Bit Bar. All rights reserved.
// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>
#import <string>

@interface NSColor(Hex)

+ (NSColor*)colorWithWebColorString:(NSString*)color;
+ (NSColor*)colorWithHexColorString:(NSString*)hex;

@end

#endif  // ATOM_BROWSER_UI_COCOA_NSCOLOR_HEX_H_
