#ifndef BRIGHTRAY_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_
#define BRIGHTRAY_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_

@class NSBundle;

namespace base {
class FilePath;
}

namespace brightray {

// The "main" application bundle is the outermost bundle for this logical
// application. E.g., if you have MyApp.app and
// MyApp.app/Contents/Frameworks/MyApp Helper.app, the main application bundle
// is MyApp.app, no matter which executable is currently running.
NSBundle* MainApplicationBundle();
base::FilePath MainApplicationBundlePath();

}  // namespace brightray

#endif  // BRIGHTRAY_COMMON_MAC_MAIN_APPLICATION_BUNDLE_H_
