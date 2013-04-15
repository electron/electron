// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/atom_browser_context.h"

namespace atom {

// static
AtomBrowserContext* AtomBrowserContext::self_;

AtomBrowserContext::AtomBrowserContext() {
  DCHECK(!self_);

  self_ = this;
}

AtomBrowserContext::~AtomBrowserContext() {
}

// static
AtomBrowserContext* AtomBrowserContext::Get() {
  DCHECK(self_);

  return self_;
}

}  // namespace atom
