#include "inspectable_web_contents_view_linux.h"

#include "browser/browser_client.h"
#include "browser/inspectable_web_contents_impl.h"

#include "content/public/browser/web_contents_view.h"

namespace brightray {

InspectableWebContentsView* CreateInspectableContentsView(InspectableWebContentsImpl* inspectable_web_contents) {
  return new InspectableWebContentsViewLinux(inspectable_web_contents);
}

InspectableWebContentsViewLinux::InspectableWebContentsViewLinux(InspectableWebContentsImpl* inspectable_web_contents)
    : inspectable_web_contents_(inspectable_web_contents) {
	// TODO
	fprintf(stderr, "InspectableWebContentsViewLinux::InspectableWebContentsViewLinux\n");
}

InspectableWebContentsViewLinux::~InspectableWebContentsViewLinux() {
	// TODO
	fprintf(stderr, "InspectableWebContentsViewLinux::~InspectableWebContentsViewLinux\n");
}

gfx::NativeView InspectableWebContentsViewLinux::GetNativeView() const {
	// TODO
	fprintf(stderr, "InspectableWebContentsViewLinux::GetNativeView\n");
	return NULL;
}

void InspectableWebContentsViewLinux::ShowDevTools() {
	// TODO
	fprintf(stderr, "InspectableWebContentsViewLinux::ShowDevTools\n");
}

void InspectableWebContentsViewLinux::CloseDevTools() {
	// TODO
	fprintf(stderr, "InspectableWebContentsViewLinux::CloseDevTools\n");
}

bool InspectableWebContentsViewLinux::SetDockSide(const std::string& side) {
	// TODO
	fprintf(stderr, "InspectableWebContentsViewLinux::SetDockSide\n");
  return false;
}

}
