#import "bry_application.h"

@interface BRYApplication ()

@property (nonatomic, assign, getter = isHandlingSendEvent) BOOL handlingSendEvent;

@end

@implementation BRYApplication

@synthesize handlingSendEvent = _handlingSendEvent;

- (void)sendEvent:(NSEvent *)theEvent
{
  base::mac::ScopedSendingEvent scopedSendingEvent;
  [super sendEvent:theEvent];
}

@end
