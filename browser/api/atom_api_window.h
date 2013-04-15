// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_WINDOW_H_
#define ATOM_BROWSER_API_ATOM_API_WINDOW_H_

#include "browser/api/atom_api_event_emitter.h"

namespace base {
class DictionaryValue;
}

namespace atom {

class NativeWindow;

namespace api {

class Window : public EventEmitter {
 public:
  virtual ~Window();

  static void Initialize(v8::Handle<v8::Object> target);

  NativeWindow* window() { return window_.get(); }

 protected:
  explicit Window(v8::Handle<v8::Object> wrapper,
                  base::DictionaryValue* options);

 private:
  static v8::Handle<v8::Value> New(const v8::Arguments &args);

  scoped_ptr<NativeWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(Window);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WINDOW_H_
