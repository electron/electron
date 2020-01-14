// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_NET_H_
#define SHELL_BROWSER_API_ATOM_API_NET_H_

#include "shell/common/gin_helper/wrappable.h"

namespace electron {

namespace api {

class Net : public gin_helper::Wrappable<Net> {
 public:
  static v8::Local<v8::Value> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  v8::Local<v8::Value> URLLoader(v8::Isolate* isolate);

 protected:
  explicit Net(v8::Isolate* isolate);
  ~Net() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(Net);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_NET_H_
