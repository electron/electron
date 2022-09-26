// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BROWSER_VIEW_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BROWSER_VIEW_H_

#include <memory>
#include <string>
#include <vector>

#include "content/public/browser/web_contents_observer.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "shell/browser/extended_web_contents_observer.h"
#include "shell/browser/native_browser_view.h"
#include "shell/browser/native_window.h"
#include "shell/common/api/api.mojom.h"
#include "shell/common/gin_helper/constructible.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/pinnable.h"

namespace gfx {
class Rect;
}

namespace gin_helper {
class Dictionary;
}

namespace electron::api {

class WebContents;
class BaseWindow;

class BrowserView : public gin::Wrappable<BrowserView>,
                    public gin_helper::Constructible<BrowserView>,
                    public gin_helper::Pinnable<BrowserView>,
                    public content::WebContentsObserver,
                    public ExtendedWebContentsObserver {
 public:
  // gin_helper::Constructible
  static gin::Handle<BrowserView> New(gin_helper::ErrorThrower thrower,
                                      gin::Arguments* args);
  static v8::Local<v8::ObjectTemplate> FillObjectTemplate(
      v8::Isolate*,
      v8::Local<v8::ObjectTemplate>);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;

  WebContents* web_contents() const { return api_web_contents_; }
  NativeBrowserView* view() const { return view_.get(); }

  BaseWindow* owner_window() const { return owner_window_.get(); }

  void SetOwnerWindow(BaseWindow* window);

  int32_t ID() const { return id_; }

  // disable copy
  BrowserView(const BrowserView&) = delete;
  BrowserView& operator=(const BrowserView&) = delete;

 protected:
  BrowserView(gin::Arguments* args, const gin_helper::Dictionary& options);
  ~BrowserView() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  // ExtendedWebContentsObserver:
  void OnDraggableRegionsUpdated(
      const std::vector<mojom::DraggableRegionPtr>& regions) override;

 private:
  void SetAutoResize(AutoResizeFlags flags);
  void SetBounds(const gfx::Rect& bounds);
  gfx::Rect GetBounds();
  void SetBackgroundColor(const std::string& color_name);
  v8::Local<v8::Value> GetWebContents(v8::Isolate*);

  v8::Global<v8::Value> web_contents_;
  class WebContents* api_web_contents_ = nullptr;

  std::unique_ptr<NativeBrowserView> view_;
  base::WeakPtr<BaseWindow> owner_window_;

  int32_t id_;
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_BROWSER_VIEW_H_
