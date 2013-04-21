// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_
#define ATOM_BROWSER_API_ATOM_BROWSER_BINDINGS_

#include "common/api/atom_bindings.h"

namespace atom {

class AtomBrowserBindings : public AtomBindings {
 public:
  AtomBrowserBindings();
  virtual ~AtomBrowserBindings();

  // Called when the node.js main script has been loaded.
  virtual void AfterLoad();

  // The require('atom').browserMainParts object.
  v8::Handle<v8::Object> browser_main_parts() {
    return browser_main_parts_;
  }

 private:
  v8::Persistent<v8::Object> browser_main_parts_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserBindings);
};

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_BINDINGS_
