#import "common/application_name.h"

#import "base/mac/bundle_locations.h"
#import "base/mac/foundation_util.h"

namespace brightray {

std::string GetApplicationName() {
  return [[base::mac::OuterBundle().infoDictionary objectForKey:base::mac::CFToNSCast(kCFBundleNameKey)] UTF8String];
}

}
