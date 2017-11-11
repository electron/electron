//
//  NSColor+Hex.h
//  BitBar
//
//  Created by Mathias Leppich on 03/02/14.
//  Copyright (c) 2014 Bit Bar. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface NSColor (Hex)

+ (NSColor*) colorWithWebColorString:(NSString*)color;
+ (NSColor*) colorWithHexColorString:(NSString*)hex;

@end
