#import "common/application_name.h"

#import "common/mac/main_application_bundle.h"

#import "base/mac/foundation_util.h"

namespace brightray {

std::string GetApplicationName() {
  return [[MainApplicationBundle().infoDictionary objectForKey:base::mac::CFToNSCast(kCFBundleNameKey)] UTF8String];
}

}
