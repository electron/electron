#include "browser/default_web_contents_delegate.h"

#include "browser/media/media_stream_devices_controller.h"

namespace brightray {

DefaultWebContentsDelegate::DefaultWebContentsDelegate() {
}

DefaultWebContentsDelegate::~DefaultWebContentsDelegate() {
}

void DefaultWebContentsDelegate::RequestMediaAccessPermission(
    content::WebContents*,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  MediaStreamDevicesController controller(request, callback);
  controller.TakeAction();
}

}  // namespace brightray
