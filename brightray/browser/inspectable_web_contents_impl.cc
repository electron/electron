#include "browser/inspectable_web_contents_impl.h"

#include "browser/devtools_frontend.h"
#include "browser/inspectable_web_contents_view.h"

namespace brightray {

// Implemented separately on each platform.
InspectableWebContentsView* CreateInspectableContentsView(InspectableWebContentsImpl*);

InspectableWebContentsImpl::InspectableWebContentsImpl(const content::WebContents::CreateParams& create_params)
    : web_contents_(content::WebContents::Create(create_params)) {
  view_.reset(CreateInspectableContentsView(this));
}

InspectableWebContentsImpl::~InspectableWebContentsImpl() {
}

InspectableWebContentsView* InspectableWebContentsImpl::GetView() const {
  return view_.get();
}

content::WebContents* InspectableWebContentsImpl::GetWebContents() const {
  return web_contents_.get();
}

void InspectableWebContentsImpl::ShowDevTools() {
  if (!devtools_web_contents_)
    devtools_web_contents_.reset(DevToolsFrontend::Show(web_contents_.get()));
  view_->ShowDevTools();
}

}
