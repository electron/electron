#import "brightray/common/application_info.h"

#import "base/mac/foundation_util.h"
#import "base/strings/sys_string_conversions.h"
#import "brightray/common/mac/main_application_bundle.h"

namespace brightray {

namespace {

std::string ApplicationInfoDictionaryValue(NSString* key) {
  return base::SysNSStringToUTF8([MainApplicationBundle().infoDictionary objectForKey:key]);
}

std::string ApplicationInfoDictionaryValue(CFStringRef key) {
  return ApplicationInfoDictionaryValue(base::mac::CFToNSCast(key));
}

}  // namespace

std::string GetApplicationName() {
  return ApplicationInfoDictionaryValue(kCFBundleNameKey);
}

std::string GetApplicationVersion() {
  return ApplicationInfoDictionaryValue(@"CFBundleShortVersionString");
}

}  // namespace brightray
