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
  static v8::Handle<v8::Value> Destroy(const v8::Arguments &args);

  // APIs for NativeWindow.
  static v8::Handle<v8::Value> Close(const v8::Arguments &args);
  static v8::Handle<v8::Value> Focus(const v8::Arguments &args);
  static v8::Handle<v8::Value> Show(const v8::Arguments &args);
  static v8::Handle<v8::Value> Hide(const v8::Arguments &args);
  static v8::Handle<v8::Value> Maximize(const v8::Arguments &args);
  static v8::Handle<v8::Value> Unmaximize(const v8::Arguments &args);
  static v8::Handle<v8::Value> Minimize(const v8::Arguments &args);
  static v8::Handle<v8::Value> Restore(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetFullscreen(const v8::Arguments &args);
  static v8::Handle<v8::Value> IsFullscreen(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetSize(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetSize(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetMinimumSize(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetMaximumSize(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetResizable(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetAlwaysOnTop(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetPosition(const v8::Arguments &args);
  static v8::Handle<v8::Value> GetPosition(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetTitle(const v8::Arguments &args);
  static v8::Handle<v8::Value> FlashFrame(const v8::Arguments &args);
  static v8::Handle<v8::Value> SetKiosk(const v8::Arguments &args);
  static v8::Handle<v8::Value> IsKiosk(const v8::Arguments &args);
  static v8::Handle<v8::Value> ShowDevTools(const v8::Arguments &args);
  static v8::Handle<v8::Value> CloseDevTools(const v8::Arguments &args);

  scoped_ptr<NativeWindow> window_;

  DISALLOW_COPY_AND_ASSIGN(Window);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_WINDOW_H_
