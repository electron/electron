// Copyright (c) 2026 Anthropic, PBC.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

// The executable of an app shim: a small app, in a bundle of its own, whose
// windows are drawn by a host Electron app. It links nothing from Electron, so
// one build of it works with any later host: it loads the host's framework and
// calls ElectronAppShimMain.
//
// The shim bundle's Info.plist names the host with one of:
//   ElectronAppShimHostPath              the host .app's absolute path
//   ElectronAppShimHostBundleIdentifier  the host's bundle identifier
// See docs/api/app-shim-host.md.

#import <Cocoa/Cocoa.h>
#include <dlfcn.h>
#include <stdlib.h>

namespace {

typedef int (*AppShimMain)(int argc, char* argv[]);

NSString* StringForKey(NSString* key) {
  id value = [NSBundle.mainBundle objectForInfoDictionaryKey:key];
  return [value isKindOfClass:[NSString class]] && [value length] ? value : nil;
}

[[noreturn]] void Fail(NSString* message) {
  NSLog(@"app shim: %@", message);
  NSAlert* alert = [[NSAlert alloc] init];
  alert.messageText = @"This app can’t be opened.";
  alert.informativeText = message;
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
  [alert runModal];
  exit(1);
}

NSString* HostBundlePath() {
  // The path wins while the host is still there; a host that has been moved
  // is found again by its identifier.
  NSString* path = StringForKey(@"ElectronAppShimHostPath");
  if (path && [NSFileManager.defaultManager fileExistsAtPath:path])
    return path;
  if (NSString* bundle_id =
          StringForKey(@"ElectronAppShimHostBundleIdentifier")) {
    if (NSURL* url = [NSWorkspace.sharedWorkspace
            URLForApplicationWithBundleIdentifier:bundle_id]) {
      return url.path;
    }
  }
  return path;
}

}  // namespace

int main(int argc, char* argv[]) {
  AppShimMain app_shim_main = nullptr;
  @autoreleasepool {
    NSString* host = HostBundlePath();
    if (!host)
      Fail(@"The app it belongs to could not be found.");

    NSString* framework_name =
        StringForKey(@"ElectronAppShimFrameworkName") ?: @"Electron Framework";
    NSString* framework =
        [NSString stringWithFormat:@"%@/Contents/Frameworks/%@.framework/%@",
                                   host, framework_name, framework_name];

    void* library = dlopen(framework.fileSystemRepresentation, RTLD_NOW);
    if (!library) {
      Fail([NSString stringWithFormat:@"%@ could not be loaded: %s",
                                      framework_name, dlerror()]);
    }
    app_shim_main =
        reinterpret_cast<AppShimMain>(dlsym(library, "ElectronAppShimMain"));
    if (!app_shim_main) {
      Fail([NSString stringWithFormat:@"%@ is too old to run this app.",
                                      host.lastPathComponent]);
    }
  }
  return app_shim_main(argc, argv);
}
