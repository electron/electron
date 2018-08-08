// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_UI_COCOA_ATOM_PREVIEW_ITEM_H_
#define ATOM_BROWSER_UI_COCOA_ATOM_PREVIEW_ITEM_H_

#import <Cocoa/Cocoa.h>
#import <Quartz/Quartz.h>

@interface AtomPreviewItem : NSObject<QLPreviewItem>
@property (nonatomic, retain) NSURL* previewItemURL;
@property (nonatomic, retain) NSString* previewItemTitle;
- (id)initWithURL:(NSURL*)url title:(NSString*)title;
@end

#endif  // ATOM_BROWSER_UI_COCOA_ATOM_PREVIEW_ITEM_H_
