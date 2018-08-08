// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_VIEW_H_
#define ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_VIEW_H_

#include "atom/browser/api/atom_api_view.h"
#include "content/public/browser/web_contents_observer.h"
#include "native_mate/handle.h"

namespace brightray {
class InspectableWebContents;
}

namespace atom {

namespace api {

class WebContents;

class WebContentsView : public View, public content::WebContentsObserver {
 public:
  static mate::WrappableBase* New(mate::Arguments* args,
                                  mate::Handle<WebContents> web_contents);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  WebContentsView(v8::Isolate* isolate,
                  mate::Handle<WebContents> web_contents,
                  brightray::InspectableWebContents* iwc);
  ~WebContentsView() override;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;

 private:
  // Keep a reference to v8 wrapper.
  v8::Global<v8::Value> web_contents_;
  api::WebContents* api_web_contents_;

  DISALLOW_COPY_AND_ASSIGN(WebContentsView);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WEB_CONTENTS_VIEW_H_
