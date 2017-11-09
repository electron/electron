//
//  NSString+ANSI.h
//  BitBar
//
//  Created by Kent Karlsson on 3/11/16.
//  Copyright © 2016 Bit Bar. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (ANSI)

- (BOOL)containsANSICodes;
- (NSMutableAttributedString*)attributedStringParsingANSICodes;

@end
