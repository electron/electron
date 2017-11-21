#import "brightray/browser/browser_main_parts.h"

#import "base/logging.h"
#import "base/mac/bundle_locations.h"
#import <AppKit/AppKit.h>

namespace brightray {

void BrowserMainParts::OverrideAppLogsPath() {
  base::FilePath path;
  NSString* bundleName = [[[NSBundle mainBundle] infoDictionary]
    objectForKey:@"CFBundleName"];
  NSString* logsPath = [NSString stringWithFormat:@"Library/Logs/%@",bundleName];
  NSString* libraryPath = [NSHomeDirectory() stringByAppendingPathComponent:logsPath];

  PathService::Override(DIR_APP_LOGS, base::FilePath([libraryPath UTF8String]));
}

// Replicates NSApplicationMain, but doesn't start a run loop.
void BrowserMainParts::InitializeMainNib() {
  auto infoDictionary = base::mac::OuterBundle().infoDictionary;

  auto principalClass = NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
  auto application = [principalClass sharedApplication];

  NSString *mainNibName = [infoDictionary objectForKey:@"NSMainNibFile"];
  auto mainNib = [[NSNib alloc] initWithNibNamed:mainNibName bundle:base::mac::FrameworkBundle()];
  [mainNib instantiateWithOwner:application topLevelObjects:nil];
  [mainNib release];
}

}  // namespace brightray
