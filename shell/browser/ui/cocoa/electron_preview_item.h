// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_PREVIEW_ITEM_H_
#define ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_PREVIEW_ITEM_H_

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

@interface ElectronPreviewItem : NSObject <QLPreviewItem>
@property(nonatomic, retain) NSURL* previewItemURL;
@property(nonatomic, retain) NSString* previewItemTitle;
- (id)initWithURL:(NSURL*)url title:(NSString*)title;
@end

#endif  // ELECTRON_SHELL_BROWSER_UI_COCOA_ELECTRON_PREVIEW_ITEM_H_
