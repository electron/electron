// Created by Mathias Leppich on 03/02/14.
// Copyright (c) 2014 Bit Bar. All rights reserved.
// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/NSColor+Hex.h"

@implementation NSColor (Hex)

- (NSString*)RGBAValue {
  double redFloatValue, greenFloatValue, blueFloatValue, alphaFloatValue;

  NSColor* convertedColor =
      [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];

  if (convertedColor) {
    [convertedColor getRed:&redFloatValue
                     green:&greenFloatValue
                      blue:&blueFloatValue
                     alpha:&alphaFloatValue];

    int redIntValue = redFloatValue * 255.99999f;
    int greenIntValue = greenFloatValue * 255.99999f;
    int blueIntValue = blueFloatValue * 255.99999f;
    int alphaIntValue = alphaFloatValue * 255.99999f;

    return
        [NSString stringWithFormat:@"%02x%02x%02x%02x", redIntValue,
                                   greenIntValue, blueIntValue, alphaIntValue];
  }

  return nil;
}

- (NSString*)hexadecimalValue {
  double redFloatValue, greenFloatValue, blueFloatValue;

  NSColor* convertedColor =
      [self colorUsingColorSpaceName:NSCalibratedRGBColorSpace];

  if (convertedColor) {
    [convertedColor getRed:&redFloatValue
                     green:&greenFloatValue
                      blue:&blueFloatValue
                     alpha:NULL];

    int redIntValue = redFloatValue * 255.99999f;
    int greenIntValue = greenFloatValue * 255.99999f;
    int blueIntValue = blueFloatValue * 255.99999f;

    return [NSString stringWithFormat:@"#%02x%02x%02x", redIntValue,
                                      greenIntValue, blueIntValue];
  }

  return nil;
}

+ (NSColor*)colorWithHexColorString:(NSString*)inColorString {
  unsigned colorCode = 0;

  if (inColorString) {
    NSScanner* scanner = [NSScanner scannerWithString:inColorString];
    (void)[scanner scanHexInt:&colorCode];  // ignore error
  }

  unsigned char redByte = (unsigned char)(colorCode >> 16);
  unsigned char greenByte = (unsigned char)(colorCode >> 8);
  unsigned char blueByte = (unsigned char)(colorCode);  // masks off high bits

  return [NSColor colorWithCalibratedRed:(CGFloat)redByte / 0xff
                                   green:(CGFloat)greenByte / 0xff
                                    blue:(CGFloat)blueByte / 0xff
                                   alpha:1.0];
}

@end
