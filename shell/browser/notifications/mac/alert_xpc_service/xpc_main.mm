//
//  xpc_main.m
//  AlertXPCService
//
//  Created by Yurii Lozytskyi on 03.08.2021.
//

#import <Foundation/Foundation.h>
#import "alert_xpc_service.h"

@interface ServiceDelegate
    : NSObject <NSXPCListenerDelegate, NSUserNotificationCenterDelegate>
@end

@implementation ServiceDelegate {
  NSXPCConnection* activeConnection_;
}

- (instancetype)init {
  self = [super init];
  if (!self)
    return nil;

  [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:self];
  return self;
}

- (void)dealloc {
  [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:nil];
  [super dealloc];
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
        didDeliverNotification:(NSUserNotification*)notification {
  [[activeConnection_ remoteObjectProxy] didDeliverNotification:notification];
}

- (void)userNotificationCenter:(NSUserNotificationCenter*)center
       didActivateNotification:(NSUserNotification*)notification {
  [[activeConnection_ remoteObjectProxy] didActivateNotification:notification];
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter*)center
     shouldPresentNotification:(NSUserNotification*)notification {
  // Display notifications even if the app is active.
  // SAP-20223 [N] TECH [MACOS] To support renotify on macOS
  NSDictionary* userInfo = notification.userInfo;
  NSString* shouldBePresented =
      userInfo && [userInfo valueForKey:@"_shouldBePresented"]
          ? userInfo[@"_shouldBePresented"]
          : @"YES";
  return [shouldBePresented isEqualToString:@"YES"] ? YES : NO;
}

- (BOOL)listener:(NSXPCListener*)listener
    shouldAcceptNewConnection:(NSXPCConnection*)newConnection {
  // This method is where the NSXPCListener configures, accepts, and resumes a
  // new incoming NSXPCConnection.

  // Configure the connection.
  // First, set the interface that the exported object implements.
  newConnection.exportedInterface = [NSXPCInterface
      interfaceWithProtocol:@protocol(NSAlertDeliveryXPCProtocol)];
  // Next, set remote interface which will handle replyes into main app module
  newConnection.remoteObjectInterface = [NSXPCInterface
      interfaceWithProtocol:@protocol(NSAlertResponceXPCProtocol)];

  // Next, set the object that the connection exports. All messages sent on the
  // connection to this service will be sent to the exported object to handle.
  // The connection retains the exported object.
  newConnection.exportedObject = [AlertXPCService new];

  // Resuming the connection allows the system to deliver more incoming
  // messages.
  [newConnection resume];

  activeConnection_ = newConnection;
  // Returning YES from this method tells the system that you have accepted this
  // connection. If you want to reject the connection for some reason, call
  // -invalidate on the connection and return NO.
  return YES;
}

@end

int main(int argc, const char* argv[]) {
  // Create the delegate for the service.
  ServiceDelegate* delegate = [ServiceDelegate new];

  // Set up the one NSXPCListener for this service. It will handle all incoming
  // connections.
  NSXPCListener* listener = [NSXPCListener serviceListener];
  listener.delegate = delegate;

  // Resuming the serviceListener starts this service. This method does not
  // return.
  [listener resume];
  return 0;
}
