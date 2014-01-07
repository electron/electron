// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_COMMON_API_ATOM_API_SCREEN_H_
#define ATOM_COMMON_API_ATOM_API_SCREEN_H_

#include "common/api/atom_api_event_emitter.h"

namespace gfx {
class Screen;
}

namespace atom {

namespace api {

class Screen : public EventEmitter {
 public:
  virtual ~Screen();

  static void Initialize(v8::Handle<v8::Object> target);

 protected:
  explicit Screen(v8::Handle<v8::Object> wrapper);

 private:
  static void New(const v8::FunctionCallbackInfo<v8::Value>& args);

  static void GetCursorScreenPoint(
      const v8::FunctionCallbackInfo<v8::Value>& args);

  gfx::Screen* screen_;

  DISALLOW_COPY_AND_ASSIGN(Screen);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_COMMON_API_ATOM_API_SCREEN_H_
