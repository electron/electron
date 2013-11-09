#include "browser/inspectable_web_contents.h"

#include "browser/inspectable_web_contents_impl.h"

#include "content/public/browser/web_contents_view.h"

namespace brightray {

InspectableWebContents* InspectableWebContents::Create(const content::WebContents::CreateParams& create_params) {
  auto contents = content::WebContents::Create(create_params);
#if defined(OS_MACOSX)
  // Work around http://crbug.com/279472.
  contents->GetView()->SetAllowOverlappingViews(true);
#endif

  return Create(contents);
}

InspectableWebContents* InspectableWebContents::Create(content::WebContents* web_contents) {
  return new InspectableWebContentsImpl(web_contents);
}

}
