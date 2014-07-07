#include "browser/win/devtools_window.h"

#include "browser/inspectable_web_contents_impl.h"
#include "browser/win/inspectable_web_contents_view_win.h"

#include "content/public/browser/web_contents_view.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#include "ui/views/widget/widget_delegate.h"

namespace brightray {

namespace {

class WidgetDelegateView : public views::WidgetDelegateView {
 public:
  WidgetDelegateView() {
    SetLayoutManager(new views::FillLayout);
  }

  virtual void DeleteDelegate() OVERRIDE { delete this; }
  virtual views::View* GetContentsView() OVERRIDE { return this; }
  virtual bool CanResize() const OVERRIDE { return true; }
  virtual bool CanMaximize() const OVERRIDE { return true; }
  virtual base::string16 GetWindowTitle() const OVERRIDE { return L"Developer Tools"; }
  virtual gfx::Size GetPreferredSize() OVERRIDE { return gfx::Size(800, 600); }
  virtual gfx::Size GetMinimumSize() OVERRIDE { return gfx::Size(100, 100); }
};

}  // namespace

DevToolsWindow* DevToolsWindow::Create(
    InspectableWebContentsViewWin* controller) {
  return new DevToolsWindow(controller);
}

DevToolsWindow::DevToolsWindow(InspectableWebContentsViewWin* controller)
    : controller_(controller),
      widget_(new views::Widget) {
  auto delegate_view = new WidgetDelegateView;
  views::Widget::InitParams params;
  params.ownership = views::Widget::InitParams::WIDGET_OWNS_NATIVE_WIDGET;
  params.top_level = true;
  params.native_widget = new views::DesktopNativeWidgetAura(widget_.get());
  params.delegate = delegate_view;
  widget_->Init(params);
  delegate_view->AddChildView(controller->GetView());
  delegate_view->Layout();
}

DevToolsWindow::~DevToolsWindow() {
}

void DevToolsWindow::Show() {
  widget_->Show();
}

void DevToolsWindow::Close() {
  widget_->Hide();
}

void DevToolsWindow::Destroy() {
  delete this;
}

}  // namespace brightray
