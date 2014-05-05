// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
#define ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_

#include "base/memory/scoped_ptr.h"
#include "brightray/browser/browser_context.h"

namespace atom {

class AtomResourceContext;
class AtomURLRequestContextGetter;

class AtomBrowserContext : public brightray::BrowserContext {
 public:
  AtomBrowserContext();
  virtual ~AtomBrowserContext();

  // Returns the browser context singleton.
  static AtomBrowserContext* Get();

  // Creates or returns the request context.
  AtomURLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap*);

  AtomURLRequestContextGetter* url_request_context_getter() const {
    DCHECK(url_request_getter_);
    return url_request_getter_.get();
  }

 protected:
  // content::BrowserContext implementations:
  virtual content::ResourceContext* GetResourceContext() OVERRIDE;

 private:
  scoped_ptr<AtomResourceContext> resource_context_;
  scoped_refptr<AtomURLRequestContextGetter> url_request_getter_;

  DISALLOW_COPY_AND_ASSIGN(AtomBrowserContext);
};

}  // namespace atom

#endif  // ATOM_BROWSER_ATOM_BROWSER_CONTEXT_H_
