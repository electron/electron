// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_PREF_STORE_DELEGATE_H_
#define SHELL_BROWSER_PREF_STORE_DELEGATE_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_value_store.h"

namespace electron {

class ElectronBrowserContext;

// Retrieves handle to the in memory pref store that gets
// initialized with the pref service.
class PrefStoreDelegate : public PrefValueStore::Delegate {
 public:
  explicit PrefStoreDelegate(
      base::WeakPtr<ElectronBrowserContext> browser_context);
  ~PrefStoreDelegate() override;

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

  DISALLOW_COPY_AND_ASSIGN(PrefStoreDelegate);
};

}  // namespace electron

#endif  // SHELL_BROWSER_PREF_STORE_DELEGATE_H_
