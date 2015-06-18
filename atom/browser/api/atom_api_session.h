// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_SESSION_H_
#define ATOM_BROWSER_API_ATOM_API_SESSION_H_

#include "native_mate/handle.h"
#include "native_mate/wrappable.h"

namespace content {
class WebContents;
}

namespace atom {

namespace api {

class Session: public mate::Wrappable {
 public:
  static mate::Handle<Session> Create(v8::Isolate* isolate,
                                      content::WebContents* web_contents);

 protected:
  explicit Session(content::WebContents* web_contents);
  ~Session();

  // mate::Wrappable implementations:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

 private:
  v8::Local<v8::Value> Cookies(v8::Isolate* isolate);

  v8::Global<v8::Value> cookies_;

  // The webContents which owns the Sesssion.
  content::WebContents* web_contents_;

  DISALLOW_COPY_AND_ASSIGN(Session);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_SESSION_H_
