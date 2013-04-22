// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include "base/memory/scoped_ptr.h"
#include "brightray/browser/browser_context.h"

namespace atom {

namespace api {
class ObjectsRegistry;
}

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  AtomBrowserContext();
  virtual ~AtomBrowserContext();

  static AtomBrowserContext* Get();

  api::ObjectsRegistry* objects_registry() const {
    return objects_registry_.get();
  }

 private:
  scoped_ptr<api::ObjectsRegistry> objects_registry_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
