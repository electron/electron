// Copyright (c) 2025 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_ELECTRON_PRECONNECT_MANAGER_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_ELECTRON_PRECONNECT_MANAGER_DELEGATE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "content/public/browser/preconnect_manager.h"

class GURL;

namespace electron {

class ElectronPreconnectManagerDelegate
    : public content::PreconnectManager::Delegate {
 public:
  ElectronPreconnectManagerDelegate();
  ~ElectronPreconnectManagerDelegate() override;

  // disable copy
  ElectronPreconnectManagerDelegate(const ElectronPreconnectManagerDelegate&) =
      delete;
  ElectronPreconnectManagerDelegate& operator=(
      const ElectronPreconnectManagerDelegate&) = delete;

  // content::PreconnectManager::Delegate
  void PreconnectInitiated(const GURL& url,
                           const GURL& preconnect_url) override {}
  void PreconnectFinished(
      std::unique_ptr<content::PreconnectStats> stats) override {}
  bool IsPreconnectEnabled() override;

  base::WeakPtr<ElectronPreconnectManagerDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<ElectronPreconnectManagerDelegate> weak_factory_{this};
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_ELECTRON_PRECONNECT_MANAGER_DELEGATE_H_
