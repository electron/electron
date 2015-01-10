// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/logging.h"
#import "base/mac/scoped_nsobject.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "content/public/browser/color_chooser.h"
#include "content/public/browser/web_contents.h"
#include "skia/ext/skia_utils_mac.h"

class ColorChooserMac;

// A Listener class to act as a event target for NSColorPanel and send
// the results to the C++ class, ColorChooserMac.
@interface ColorPanelCocoa : NSObject<NSWindowDelegate> {
 @private
  // We don't call DidChooseColor if the change wasn't caused by the user
  // interacting with the panel.
  BOOL nonUserChange_;
  ColorChooserMac* chooser_;  // weak, owns this
}

- (id)initWithChooser:(ColorChooserMac*)chooser;

// Called from NSColorPanel.
- (void)didChooseColor:(NSColorPanel*)panel;

// Sets color to the NSColorPanel as a non user change.
- (void)setColor:(NSColor*)color;

@end

class ColorChooserMac : public content::ColorChooser {
 public:
  static ColorChooserMac* Open(content::WebContents* web_contents,
                               SkColor initial_color);

  ColorChooserMac(content::WebContents* tab, SkColor initial_color);
  virtual ~ColorChooserMac();

  // Called from ColorPanelCocoa.
  void DidChooseColorInColorPanel(SkColor color);
  void DidCloseColorPabel();

  virtual void End() override;
  virtual void SetSelectedColor(SkColor color) override;

 private:
  static ColorChooserMac* current_color_chooser_;

  // The web contents invoking the color chooser.  No ownership because it will
  // outlive this class.
  content::WebContents* web_contents_;
  base::scoped_nsobject<ColorPanelCocoa> panel_;
};

ColorChooserMac* ColorChooserMac::current_color_chooser_ = NULL;

// static
ColorChooserMac* ColorChooserMac::Open(content::WebContents* web_contents,
                                       SkColor initial_color) {
  if (current_color_chooser_)
    current_color_chooser_->End();
  DCHECK(!current_color_chooser_);
  current_color_chooser_ =
      new ColorChooserMac(web_contents, initial_color);
  return current_color_chooser_;
}

ColorChooserMac::ColorChooserMac(content::WebContents* web_contents,
                                 SkColor initial_color)
    : web_contents_(web_contents) {
  panel_.reset([[ColorPanelCocoa alloc] initWithChooser:this]);
  [panel_ setColor:gfx::SkColorToDeviceNSColor(initial_color)];
  [[NSColorPanel sharedColorPanel] makeKeyAndOrderFront:nil];
}

ColorChooserMac::~ColorChooserMac() {
  // Always call End() before destroying.
  DCHECK(!panel_);
}

void ColorChooserMac::DidChooseColorInColorPanel(SkColor color) {
  if (web_contents_)
    web_contents_->DidChooseColorInColorChooser(color);
}

void ColorChooserMac::DidCloseColorPabel() {
  End();
}

void ColorChooserMac::End() {
  panel_.reset();
  DCHECK(current_color_chooser_ == this);
  current_color_chooser_ = NULL;
  if (web_contents_)
      web_contents_->DidEndColorChooser();
}

void ColorChooserMac::SetSelectedColor(SkColor color) {
  [panel_ setColor:gfx::SkColorToDeviceNSColor(color)];
}

@implementation ColorPanelCocoa

- (id)initWithChooser:(ColorChooserMac*)chooser {
  if ((self = [super init])) {
    chooser_ = chooser;
    NSColorPanel* panel = [NSColorPanel sharedColorPanel];
    [panel setShowsAlpha:NO];
    [panel setDelegate:self];
    [panel setTarget:self];
    [panel setAction:@selector(didChooseColor:)];
  }
  return self;
}

- (void)dealloc {
  NSColorPanel* panel = [NSColorPanel sharedColorPanel];
  if ([panel delegate] == self) {
    [panel setDelegate:nil];
    [panel setTarget:nil];
    [panel setAction:nil];
  }

  [super dealloc];
}

- (void)windowWillClose:(NSNotification*)notification {
  nonUserChange_ = NO;
  chooser_->DidCloseColorPabel();
}

- (void)didChooseColor:(NSColorPanel*)panel {
  if (nonUserChange_) {
    nonUserChange_ = NO;
    return;
  }
  chooser_->DidChooseColorInColorPanel(gfx::NSDeviceColorToSkColor(
      [[panel color] colorUsingColorSpaceName:NSDeviceRGBColorSpace]));
  nonUserChange_ = NO;
}

- (void)setColor:(NSColor*)color {
  nonUserChange_ = YES;
  [[NSColorPanel sharedColorPanel] setColor:color];
}

namespace chrome {

content::ColorChooser* ShowColorChooser(content::WebContents* web_contents,
                                        SkColor initial_color) {
  return ColorChooserMac::Open(web_contents, initial_color);
}

}  // namepace chrome

@end
