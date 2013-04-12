// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_DELEGATE_
#define ATOM_APP_ATOM_MAIN_DELEGATE_

#include "brightray/common/main_delegate.h"

namespace atom {

class AtomMainDelegate : public brightray::MainDelegate {
public:
  AtomMainDelegate();
  ~AtomMainDelegate();

private:
  virtual content::ContentBrowserClient* CreateContentBrowserClient() OVERRIDE;
  virtual content::ContentRendererClient*
      CreateContentRendererClient() OVERRIDE;

  scoped_ptr<content::ContentBrowserClient> browser_client_;
  scoped_ptr<content::ContentRendererClient> renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomMainDelegate);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_DELEGATE_
