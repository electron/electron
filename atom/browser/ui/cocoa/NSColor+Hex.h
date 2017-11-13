//
//  NSColor+Hex.h
//  BitBar
//
//  Created by Mathias Leppich on 03/02/14.
//  Copyright (c) 2014 Bit Bar. All rights reserved.
//
#ifndef ATOM_BROWSER_UI_COCOA_NSCOLOR_HEX_H_
#define ATOM_BROWSER_UI_COCOA_NSCOLOR_HEX_H_

#import <Cocoa/Cocoa.h>
#import <string>

@interface NSColor(Hex)

+ (NSColor*) colorWithWebColorString:(NSString*)color;
+ (NSColor*) colorWithHexColorString:(NSString*)hex;

@end

#endif  // ATOM_BROWSER_UI_COCOA_NSCOLOR_HEX_H_
