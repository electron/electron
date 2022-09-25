//
//  alert_xpc_service.mm
//  AlertXPCService
//
//  Created by Yurii Lozytskyi on 03.08.2021.
//

#import "alert_xpc_service.h"

@implementation AlertXPCService

// This implements the example protocol. Replace the body of this class with the
// implementation of this service's protocol.
- (void)upperCaseString:(NSString*)aString
              withReply:(void (^)(NSString*))reply {
  NSString* response = [aString uppercaseString];
  reply(response);
}

- (void)deliverNotification:(NSUserNotification*)notification {
  [[NSUserNotificationCenter defaultUserNotificationCenter]
      deliverNotification:notification];
}

- (void)removeDeliveredNotification:(NSUserNotification*)notification {
  [NSUserNotificationCenter.defaultUserNotificationCenter
      removeDeliveredNotification:notification];
}

@end
