// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_EVENT_EMITTER_H_
#define ATOM_BROWSER_API_EVENT_EMITTER_H_

#include "native_mate/wrappable.h"

namespace base {
class ListValue;
}

namespace mate {

// Provide helperers to emit event in JavaScript.
class EventEmitter : public Wrappable {
 protected:
  EventEmitter();

  // this.emit(name);
  bool Emit(const base::StringPiece& name);

  // this.emit(name, args...);
  bool Emit(const base::StringPiece& name, const base::ListValue& args);

 private:
  DISALLOW_COPY_AND_ASSIGN(EventEmitter);
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_EVENT_EMITTER_H_
