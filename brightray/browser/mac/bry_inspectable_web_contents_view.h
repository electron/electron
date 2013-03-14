#import <AppKit/AppKit.h>

@class BRYInspectableWebContentsViewPrivate;

@interface BRYInspectableWebContentsView : NSView {
@private
  BRYInspectableWebContentsViewPrivate *_private;
}

- (IBAction)showDevTools:(id)sender;

@end
