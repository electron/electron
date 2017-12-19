// Created by Mathias Leppich on 03/02/14.
// Copyright (c) 2014 Bit Bar. All rights reserved.
// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/cocoa/NSColor+Hex.h"

@implementation NSColor (Hex)

+ (NSColor*)colorWithHexColorString:(NSString*)inColorString {
  unsigned colorCode = 0;
  unsigned char redByte, greenByte, blueByte;

  if (inColorString) {
    NSScanner* scanner = [NSScanner scannerWithString:inColorString];
    (void) [scanner scanHexInt:&colorCode]; // ignore error
  }
  redByte = (unsigned char)(colorCode >> 16);
  greenByte = (unsigned char)(colorCode >> 8);
  blueByte = (unsigned char)(colorCode); // masks off high bits

  return [NSColor colorWithCalibratedRed:(CGFloat)redByte / 0xff
                                   green:(CGFloat)greenByte / 0xff
                                    blue:(CGFloat)blueByte / 0xff
                                   alpha:1.0];
}

@end
