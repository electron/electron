#import "browser/browser_client.h"

#import "base/strings/sys_string_conversions.h"
#import "content/public/common/show_desktop_notification_params.h"

#import <Foundation/Foundation.h>

namespace brightray {

void BrowserClient::ShowDesktopNotification(
    const content::ShowDesktopNotificationHostMsgParams& params,
    int render_process_id,
    int render_view_id,
    bool worker) {
  auto notification = [[NSUserNotification alloc] init];
  notification.title = base::SysUTF16ToNSString(params.title);
  notification.informativeText = base::SysUTF16ToNSString(params.body);

  [NSUserNotificationCenter.defaultUserNotificationCenter deliverNotification:notification];
  [notification release];
}

}
