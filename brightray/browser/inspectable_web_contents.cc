#include "browser/inspectable_web_contents.h"

#include "browser/inspectable_web_contents_impl.h"

namespace brightray {

InspectableWebContents* InspectableWebContents::Create(const content::WebContents::CreateParams& create_params) {
  return new InspectableWebContentsImpl(content::WebContents::Create(create_params));
}

InspectableWebContents* InspectableWebContents::Create(content::WebContents* web_contents) {
  return new InspectableWebContentsImpl(web_contents);
}

}
