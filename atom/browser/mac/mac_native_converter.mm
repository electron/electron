#import "atom/browser/mac/mac_native_converter.h"

@implementation MacNativeConverter

- (base::ListValue*)arrayToV8:(NSArray*)nsArray {
  scoped_ptr<base::ListValue> list(new base::ListValue);

  for (id value in nsArray) {
    if ([value isKindOfClass:[NSArray class]]) {
      list->Append([self arrayToV8:value]);
    } else if ([value isKindOfClass:[NSDictionary class]]) {
      list->Append([self dictionaryToV8:value]);
    } else if ([value isKindOfClass:[NSString class]]) {
      list->AppendString(base::SysNSStringToUTF8(value));
    } else if ([value isKindOfClass:[NSNumber class]]) {
      list->AppendInteger(((NSNumber* )value).intValue);
    }
  }

  return list.get();
}

- (base::DictionaryValue*)dictionaryToV8:(NSDictionary*)nsDictionary {
  scoped_ptr<base::DictionaryValue> dict(new base::DictionaryValue);

  NSEnumerator *it = [nsDictionary keyEnumerator];
  while (NSString *key = [it nextObject]) {
    id value = [nsDictionary objectForKey:key];

    std::string key_str(base::SysNSStringToUTF8(key));

    if ([value isKindOfClass:[NSArray class]]) {
      dict->Set(key_str, [self arrayToV8:value]);
    } else if ([value isKindOfClass:[NSDictionary class]]) {
      dict->Set(key_str, [self dictionaryToV8:value]);
    } else if ([value isKindOfClass:[NSString class]]) {
      dict->SetString(key_str, base::SysNSStringToUTF8(value));
    } else if ([value isKindOfClass:[NSNumber class]]) {
      dict->SetInteger(key_str, ((NSNumber* )value).intValue);
    }
  }

  return dict.get();
}

@end
