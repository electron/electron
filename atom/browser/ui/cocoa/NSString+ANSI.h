//
//  NSString+ANSI.h
//  BitBar
//
//  Created by Kent Karlsson on 3/11/16.
//  Copyright © 2016 Bit Bar. All rights reserved.
//

#ifndef ATOM_BROWSER_UI_COCOA_NSSTRING_ANSI_H_
#define ATOM_BROWSER_UI_COCOA_NSSTRING_ANSI_H_

#import <Foundation/Foundation.h>

@interface NSString(ANSI)

- (BOOL)containsANSICodes;
- (NSMutableAttributedString*)attributedStringParsingANSICodes;

@end

#endif  // ATOM_BROWSER_UI_COCOA_NSSTRING_ANSI_H_
