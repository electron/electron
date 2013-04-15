// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/api/atom_api_event_emitter.h"

namespace atom {

namespace api {

EventEmitter::EventEmitter(v8::Handle<v8::Object> wrapper) {
  Wrap(wrapper);
}

EventEmitter::~EventEmitter() {
}

}  // namespace api

}  // namespace atom
