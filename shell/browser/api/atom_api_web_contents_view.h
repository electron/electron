// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_WEB_CONTENTS_VIEW_H_
#define SHELL_BROWSER_API_ATOM_API_WEB_CONTENTS_VIEW_H_

#include "content/public/browser/web_contents_observer.h"
#include "shell/browser/api/atom_api_view.h"

namespace electron {

class InspectableWebContents;

namespace api {

class WebContents;

class WebContentsView : public View, public content::WebContentsObserver {
 public:
  static gin_helper::WrappableBase* New(gin_helper::Arguments* args,
                                        gin::Handle<WebContents> web_contents);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  WebContentsView(v8::Isolate* isolate,
                  gin::Handle<WebContents> web_contents,
                  InspectableWebContents* iwc);
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

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_WEB_CONTENTS_VIEW_H_
