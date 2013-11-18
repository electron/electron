#import "base/mac/scoped_sending_event.h"

@interface BRYApplication : NSApplication<CrAppProtocol, CrAppControlProtocol> {
  BOOL _handlingSendEvent;
}

@end
