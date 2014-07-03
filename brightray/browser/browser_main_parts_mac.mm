#import "browser_main_parts.h"

#import "base/logging.h"
#import "base/mac/bundle_locations.h"
#import <AppKit/AppKit.h>

namespace brightray {

namespace {

// Sets the file descriptor soft limit to |max_descriptors| or the OS hard limit, whichever is
// lower.
void SetFileDescriptorLimit(rlim_t max_descriptors) {
  rlimit limits;
  if (getrlimit(RLIMIT_NOFILE, &limits) != 0) {
    PLOG(INFO) << "Failed to get file descriptor limit";
    return;
  }

  auto new_limit = max_descriptors;
  if (limits.rlim_max > 0)
    new_limit = std::min(new_limit, limits.rlim_max);
  limits.rlim_cur = new_limit;
  if (setrlimit(RLIMIT_NOFILE, &limits) != 0)
    PLOG(INFO) << "Failed to set file descriptor limit";
}

}  // namespace

void BrowserMainParts::IncreaseFileDescriptorLimit() {
  // We use quite a few file descriptors for our IPC, and the default limit on the Mac is low (256),
  // so bump it up.
  // See http://src.chromium.org/viewvc/chrome/trunk/src/chrome/browser/chrome_browser_main_posix.cc?revision=244734#l295
  // and https://codereview.chromium.org/125151
  SetFileDescriptorLimit(1024);
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

}
