// Created by Kent Karlsson on 3/11/16.
// Copyright (c) 2016 Bit Bar. All rights reserved.
// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "content/public/common/color_parser.h"
#include "shell/browser/ui/cocoa/NSString+ANSI.h"
#include "skia/ext/skia_utils_mac.h"

@implementation NSMutableDictionary (ANSI)

- (NSMutableDictionary*)modifyAttributesForANSICodes:(NSString*)codes {
  BOOL bold = NO;
  NSFont* font = self[NSFontAttributeName];

  NSArray* codeArray = [codes componentsSeparatedByString:@";"];

  for (NSString* codeString in codeArray) {
    int code = codeString.intValue;
    SkColor color;
    switch (code) {
      case 0:
        [self removeAllObjects];
        // remove italic and bold from font here
        if (font)
          self[NSFontAttributeName] = font;
        break;

      case 1:
      case 22:
        bold = (code == 1);
        break;

        // case 3: italic
        // case 23: italic off
        // case 4: underlined
        // case 24: underlined off

      case 30:
        content::ParseHexColorString(bold ? "#7f7f7f" : "#000000", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 31:
        content::ParseHexColorString(bold ? "#cd0000" : "#ff0000", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 32:
        content::ParseHexColorString(bold ? "#00cd00" : "#00ff00", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 33:
        content::ParseHexColorString(bold ? "#cdcd00" : "#ffff00", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 34:
        content::ParseHexColorString(bold ? "#0000ee" : "#5c5cff", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 35:
        content::ParseHexColorString(bold ? "#cd00cd" : "#ff00ff", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 36:
        content::ParseHexColorString(bold ? "#00cdcd" : "#00ffff", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 37:
        content::ParseHexColorString(bold ? "#e5e5e5" : "#ffffff", &color);
        self[NSForegroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;

      case 39:
        [self removeObjectForKey:NSForegroundColorAttributeName];
        break;

      case 40:
        content::ParseHexColorString("#7f7f7f", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 41:
        content::ParseHexColorString("#cd0000", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 42:
        content::ParseHexColorString("#00cd00", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 43:
        content::ParseHexColorString("#cdcd00", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 44:
        content::ParseHexColorString("#0000ee", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 45:
        content::ParseHexColorString("cd00cd", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 46:
        content::ParseHexColorString("#00cdcd", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;
      case 47:
        content::ParseHexColorString("#e5e5e5", &color);
        self[NSBackgroundColorAttributeName] =
            skia::SkColorToCalibratedNSColor(color);
        break;

      case 49:
        [self removeObjectForKey:NSBackgroundColorAttributeName];
        break;

      default:
        break;
    }
  }

  return self;
}

@end

@implementation NSString (ANSI)

- (BOOL)containsANSICodes {
  return [self rangeOfString:@"\033["].location != NSNotFound;
}

- (NSMutableAttributedString*)attributedStringParsingANSICodes {
  NSMutableAttributedString* result = [[NSMutableAttributedString alloc] init];

  NSMutableDictionary* attributes([[NSMutableDictionary alloc] init]);
  NSArray* parts = [self componentsSeparatedByString:@"\033["];
  [result appendAttributedString:[[NSAttributedString alloc]
                                     initWithString:parts.firstObject
                                         attributes:nil]];

  for (NSString* part in
       [parts subarrayWithRange:NSMakeRange(1, parts.count - 1)]) {
    if (part.length == 0)
      continue;

    NSArray* sequence = [part componentsSeparatedByString:@"m"];
    NSString* text = sequence.lastObject;

    if (sequence.count < 2) {
      [result appendAttributedString:[[NSAttributedString alloc]
                                         initWithString:text
                                             attributes:attributes]];
    } else if (sequence.count >= 2) {
      text = [[sequence subarrayWithRange:NSMakeRange(1, sequence.count - 1)]
          componentsJoinedByString:@"m"];
      [attributes modifyAttributesForANSICodes:sequence[0]];
      [result appendAttributedString:[[NSAttributedString alloc]
                                         initWithString:text
                                             attributes:attributes]];
    }
  }

  return result;
}

@end
