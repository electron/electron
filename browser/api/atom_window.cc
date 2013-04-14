// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_window.h"

namespace atom {

static void Initialize(v8::Handle<v8::Object> target) {
  target->Set(v8::String::New("hello"), v8::String::New("world"));
}

}  // namespace atom

NODE_MODULE(atom_window, atom::Initialize)
