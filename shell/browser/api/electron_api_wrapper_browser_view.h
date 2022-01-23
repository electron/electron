#ifndef SHELL_BROWSER_API_ELECTRON_API_WRAPPER_BROWSER_VIEW_H_
#define SHELL_BROWSER_API_ELECTRON_API_WRAPPER_BROWSER_VIEW_H_

#include <memory>

#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/electron_api_base_view.h"
#include "shell/browser/ui/native_wrapper_browser_view.h"

namespace gin_helper {
class Dictionary;
}

namespace electron {

namespace api {

class BrowserView;

class WrapperBrowserView : public BaseView,
                           public content::WebContentsObserver {
 public:
  static gin_helper::WrappableBase* New(gin_helper::ErrorThrower thrower,
                                        gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // disable copy
  WrapperBrowserView(const WrapperBrowserView&) = delete;
  WrapperBrowserView& operator=(const WrapperBrowserView&) = delete;

 protected:
  WrapperBrowserView(gin::Arguments* args,
                     const gin_helper::Dictionary& options,
                     NativeWrapperBrowserView* view);
  ~WrapperBrowserView() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

  v8::Local<v8::Value> GetBrowserView(v8::Isolate*);

 private:
  v8::Global<v8::Value> browser_view_;
  class BrowserView* api_browser_view_ = nullptr;

  NativeWrapperBrowserView* view_;
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_WRAPPER_BROWSER_VIEW_H_
