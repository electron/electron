#import "browser_main_parts.h"

#import "base/mac/bundle_locations.h"
#import <AppKit/AppKit.h>

namespace brightray {

// Replicates NSApplicationMain, but doesn't start a run loop.
void BrowserMainParts::PreMainMessageLoopStart() {
  auto infoDictionary = base::mac::OuterBundle().infoDictionary;

  auto principalClass = NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
  auto application = [principalClass sharedApplication];

  NSString *mainNibName = [infoDictionary objectForKey:@"NSMainNibFile"];
  auto mainNib = [[NSNib alloc] initWithNibNamed:mainNibName bundle:base::mac::FrameworkBundle()];
  [mainNib instantiateNibWithOwner:application topLevelObjects:nil];
  [mainNib release];
}

}
