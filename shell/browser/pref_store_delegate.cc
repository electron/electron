// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/pref_store_delegate.h"

#include <utility>

#include "components/prefs/persistent_pref_store.h"
#include "components/prefs/pref_store.h"
#include "components/prefs/value_map_pref_store.h"
#include "shell/browser/electron_browser_context.h"

namespace electron {

PrefStoreDelegate::PrefStoreDelegate(
    base::WeakPtr<ElectronBrowserContext> browser_context)
    : browser_context_(std::move(browser_context)) {}

PrefStoreDelegate::~PrefStoreDelegate() {
  if (browser_context_)
    browser_context_->set_in_memory_pref_store(nullptr);
}

void PrefStoreDelegate::UpdateCommandLinePrefStore(
    PrefStore* command_line_prefs) {
  if (browser_context_)
    browser_context_->set_in_memory_pref_store(
        static_cast<ValueMapPrefStore*>(command_line_prefs));
}

}  // namespace electron
