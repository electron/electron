//
//  alert_xpc_service.h
//  AlertXPCService
//
//  Created by Yurii Lozytskyi on 03.08.2021.
//

#import <Foundation/Foundation.h>
#import "alert_xpc_protocol.h"

// This object implements the protocol which we have defined. It provides the
// actual behavior for the service. It is 'exported' by the service to make it
// available to the process hosting the service over an NSXPCConnection.
@interface AlertXPCService : NSObject <NSAlertDeliveryXPCProtocol>
@end
