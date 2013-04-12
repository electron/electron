// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_
#define ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_

#include "brightray/browser/browser_main_parts.h"

namespace atom {

class AtomBrowserMainParts : public brightray::BrowserMainParts {
public:
  AtomBrowserMainParts();
  ~AtomBrowserMainParts();

protected:
  virtual void PreMainMessageLoopRun() OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserMainParts);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_MAIN_PARTS_
