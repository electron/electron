// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_PREF_STORE_DELEGATE_H_
#define ELECTRON_SHELL_BROWSER_PREF_STORE_DELEGATE_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "components/prefs/pref_value_store.h"

class PersistentPrefStore;
class PrefNotifier;
class PrefRegistry;
class PrefStore;

namespace electron {

class ElectronBrowserContext;

// Retrieves handle to the in memory pref store that gets
// initialized with the pref service.
class PrefStoreDelegate : public PrefValueStore::Delegate {
 public:
  explicit PrefStoreDelegate(
      base::WeakPtr<ElectronBrowserContext> browser_context);
  ~PrefStoreDelegate() override;

  // disable copy
  PrefStoreDelegate(const PrefStoreDelegate&) = delete;
  PrefStoreDelegate& operator=(const PrefStoreDelegate&) = delete;

  void Init(PrefStore* managed_prefs,
            PrefStore* supervised_user_prefs,
            PrefStore* extension_prefs,
            PrefStore* command_line_prefs,
            PrefStore* user_prefs,
            PrefStore* recommended_prefs,
            PrefStore* default_prefs,
            PrefNotifier* pref_notifier) override {}

  void InitIncognitoUserPrefs(
      scoped_refptr<PersistentPrefStore> incognito_user_prefs_overlay,
      scoped_refptr<PersistentPrefStore> incognito_user_prefs_underlay,
      const std::vector<const char*>& overlay_pref_names) override {}

  void InitPrefRegistry(PrefRegistry* pref_registry) override {}

  void UpdateCommandLinePrefStore(PrefStore* command_line_prefs) override;

 private:
  base::WeakPtr<ElectronBrowserContext> browser_context_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_PREF_STORE_DELEGATE_H_
