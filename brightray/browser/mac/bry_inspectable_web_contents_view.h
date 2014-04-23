#import <AppKit/AppKit.h>

@class BRYInspectableWebContentsViewPrivate;

@interface BRYInspectableWebContentsView
    : NSView<NSWindowDelegate, NSSplitViewDelegate> {
@private
  BRYInspectableWebContentsViewPrivate *_private;
}

- (void)removeFromNotificationCenter;
- (IBAction)showDevTools:(id)sender;

@end
