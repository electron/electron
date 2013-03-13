// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE-CHROMIUM file.

#ifndef __brightray__main_parts__
#define __brightray__main_parts__

#include "base/compiler_specific.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_main_parts.h"

namespace brightray {

class BrowserContext;

class BrowserMainParts : public content::BrowserMainParts {
public:
  BrowserMainParts();
  ~BrowserMainParts();

  BrowserContext* browser_context() { return browser_context_.get(); }

protected:
#if defined(OS_MACOSX)
  virtual void PreMainMessageLoopStart() OVERRIDE;
#endif

  virtual void PreMainMessageLoopRun() OVERRIDE;

private:
  scoped_ptr<BrowserContext> browser_context_;

  DISALLOW_COPY_AND_ASSIGN(BrowserMainParts);
};

}

#endif /* defined(__brightray__main_parts__) */
