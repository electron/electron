// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_API_RUNTIME_SHELL_RUNTIME_API_DELEGATE_H_
#define EXTENSIONS_SHELL_BROWSER_API_RUNTIME_SHELL_RUNTIME_API_DELEGATE_H_

#include "base/macros.h"
#include "extensions/browser/api/runtime/runtime_api_delegate.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace extensions {

class ShellRuntimeAPIDelegate : public RuntimeAPIDelegate {
 public:
  explicit ShellRuntimeAPIDelegate(content::BrowserContext* browser_context);
  ~ShellRuntimeAPIDelegate() override;

  // RuntimeAPIDelegate implementation.
  void AddUpdateObserver(UpdateObserver* observer) override;
  void RemoveUpdateObserver(UpdateObserver* observer) override;
  void ReloadExtension(const std::string& extension_id) override;
  bool CheckForUpdates(const std::string& extension_id,
                       const UpdateCheckCallback& callback) override;
  void OpenURL(const GURL& uninstall_url) override;
  bool GetPlatformInfo(api::runtime::PlatformInfo* info) override;
  bool RestartDevice(std::string* error_message) override;

 private:
  content::BrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(ShellRuntimeAPIDelegate);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_API_RUNTIME_SHELL_RUNTIME_API_DELEGATE_H_
