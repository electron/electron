#import "common/application_info.h"

#import "common/mac/foundation_util.h"
#import "common/mac/main_application_bundle.h"

#import "base/strings/sys_string_conversions.h"

namespace brightray {

namespace {

std::string ApplicationInfoDictionaryValue(NSString* key) {
  return base::SysNSStringToUTF8([MainApplicationBundle().infoDictionary objectForKey:key]);
}

std::string ApplicationInfoDictionaryValue(CFStringRef key) {
  return ApplicationInfoDictionaryValue(base::mac::CFToNSCast(key));
}

}

std::string GetApplicationName() {
  return ApplicationInfoDictionaryValue(kCFBundleNameKey);
}

std::string GetApplicationVersion() {
  return ApplicationInfoDictionaryValue(@"CFBundleShortVersionString");
}

}
