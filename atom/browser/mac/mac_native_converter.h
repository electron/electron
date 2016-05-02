#import <Cocoa/Cocoa.h>

#include "base/values.h"
#include "base/strings/sys_string_conversions.h"

@interface MacNativeConverter : NSObject

- (base::ListValue*)arrayToListValue:(NSArray*)nsArray;
- (base::DictionaryValue*)dictionaryToDictionaryValue:(NSDictionary*)nsDictionary;

- (NSArray*)arrayFromListValue:(const base::ListValue&)list;
- (NSDictionary*)dictionaryFromDictionaryValue:(const base::DictionaryValue&)dict;

@end
