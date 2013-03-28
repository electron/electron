#import "browser/notification_presenter_mac.h"

#import "base/strings/sys_string_conversions.h"
#import "content/public/browser/render_view_host.h"
#import "content/public/common/show_desktop_notification_params.h"

#import <Foundation/Foundation.h>

@interface BRYUserNotificationCenterDelegate : NSObject <NSUserNotificationCenterDelegate>
@end

namespace brightray {

NotificationPresenter* NotificationPresenter::Create() {
  return new NotificationPresenterMac;
}

NotificationPresenterMac::NotificationPresenterMac()
    : delegate_([[BRYUserNotificationCenterDelegate alloc] init]) {
  NSUserNotificationCenter.defaultUserNotificationCenter.delegate = delegate_;
}

NotificationPresenterMac::~NotificationPresenterMac() {
}

void NotificationPresenterMac::ShowNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id) {
  auto notification = [[NSUserNotification alloc] init];
  notification.title = base::SysUTF16ToNSString(params.title);
  notification.informativeText = base::SysUTF16ToNSString(params.body);

  [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:notification];
  [notification release];

  auto host = content::RenderViewHost::FromID(render_process_id, render_view_id);
  if (!host)
    return;

  host->DesktopNotificationPostDisplay(params.notification_id);
}

}

@implementation BRYUserNotificationCenterDelegate

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification {
  // Display notifications even if the app is active.
  return YES;
}

@end
