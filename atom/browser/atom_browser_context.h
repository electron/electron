// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include "brightray/browser/browser_context.h"

namespace atom {

class AtomURLRequestJobFactory;

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  AtomBrowserContext();
  virtual ~AtomBrowserContext();

  // Returns the browser context singleton.
  static AtomBrowserContext* Get();

  AtomURLRequestJobFactory* job_factory() const { return job_factory_; }

 protected:
  // brightray::BrowserContext:
  virtual scoped_ptr<net::URLRequestJobFactory> CreateURLRequestJobFactory(
      content::ProtocolHandlerMap* handlers,
      content::ProtocolHandlerScopedVector* interceptors) OVERRIDE;

 private:
  AtomURLRequestJobFactory* job_factory_;  // Weak reference.

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
