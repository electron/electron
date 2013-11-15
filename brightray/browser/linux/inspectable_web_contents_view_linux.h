#ifndef BRIGHTRAY_BROWSER_LINUX_INSPECTABLE_WEB_CONTENTS_VIEW_LINUX_H_
#define BRIGHTRAY_BROWSER_LINUX_INSPECTABLE_WEB_CONTENTS_VIEW_LINUX_H_

#include "browser/inspectable_web_contents_view.h"

#include "base/compiler_specific.h"

namespace brightray {

class InspectableWebContentsImpl;

class InspectableWebContentsViewLinux : public InspectableWebContentsView {
public:
  InspectableWebContentsViewLinux(InspectableWebContentsImpl*);
  ~InspectableWebContentsViewLinux();

  virtual gfx::NativeView GetNativeView() const OVERRIDE;
  virtual void ShowDevTools() OVERRIDE;
  virtual void CloseDevTools() OVERRIDE;
  virtual bool SetDockSide(const std::string& side) OVERRIDE;

  InspectableWebContentsImpl* inspectable_web_contents() { return inspectable_web_contents_; }

private:
  // Owns us.
  InspectableWebContentsImpl* inspectable_web_contents_;

  std::string dockside;
  GtkWidget *devtools_window;

	// Show the dev tools in their own window.  If they're already shown
	// somewhere else, remove them cleanly and take any GtkPaned out of the
	// window.
  void ShowDevToolsInWindow();

	// Show the dev tools in a vpaned (on the bottom) or hpaned (on the
	// right).  If they're already shown in a pane, move them and remove the
	// old pane.  If they're already shown in a window, hide (don't delete)
	// that window.
  void ShowDevToolsInPane(bool on_bottom);

	// Create a new window for dev tools.  This function doesn't actually
	// put the dev tools in the window or show the window.
  void MakeDevToolsWindow();

	// Get the GtkWindow* that contains this object.
  GtkWidget *GetBrowserWindow();

  DISALLOW_COPY_AND_ASSIGN(InspectableWebContentsViewLinux);
};

}

#endif
