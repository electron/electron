#include "brightray/browser/inspectable_web_contents.h"

#include "brightray/browser/inspectable_web_contents_impl.h"

namespace brightray {

InspectableWebContents* InspectableWebContents::Create(
    content::WebContents* web_contents,
    bool is_guest) {
  return new InspectableWebContentsImpl(web_contents, is_guest);
}

}  // namespace brightray
