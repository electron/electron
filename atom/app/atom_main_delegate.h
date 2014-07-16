// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_APP_ATOM_MAIN_DELEGATE_H_
#define ATOM_APP_ATOM_MAIN_DELEGATE_H_

#include "brightray/common/main_delegate.h"
#include "brightray/common/content_client.h"

namespace atom {

class AtomMainDelegate : public brightray::MainDelegate {
 public:
  AtomMainDelegate();
  ~AtomMainDelegate();

 protected:
  // brightray::MainDelegate:
  virtual void AddDataPackFromPath(
      ui::ResourceBundle* bundle, const base::FilePath& pak_dir) OVERRIDE;

  // content::ContentMainDelegate:
  virtual bool BasicStartupComplete(int* exit_code) OVERRIDE;
  virtual void PreSandboxStartup() OVERRIDE;

#if defined(OS_MACOSX)
  virtual void OverrideChildProcessPath() OVERRIDE;
  virtual void OverrideFrameworkBundlePath() OVERRIDE;
#endif

 private:
  virtual content::ContentBrowserClient* CreateContentBrowserClient() OVERRIDE;
  virtual content::ContentRendererClient*
      CreateContentRendererClient() OVERRIDE;

  brightray::ContentClient content_client_;
  scoped_ptr<content::ContentBrowserClient> browser_client_;
  scoped_ptr<content::ContentRendererClient> renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(AtomMainDelegate);
};

}  // namespace atom

#endif  // ATOM_APP_ATOM_MAIN_DELEGATE_H_
