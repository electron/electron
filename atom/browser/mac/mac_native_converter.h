#import <Cocoa/Cocoa.h>

#include "base/values.h"
#include "base/strings/sys_string_conversions.h"

@interface MacNativeConverter : NSObject

- (base::ListValue*)arrayToV8:(NSArray*)nsArray;
- (base::DictionaryValue*)dictionaryToV8:(NSDictionary*)nsDictionary;

@end
