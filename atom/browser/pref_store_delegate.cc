// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/pref_store_delegate.h"

#include <utility>

#include "atom/browser/atom_browser_context.h"
#include "components/prefs/value_map_pref_store.h"

namespace atom {

PrefStoreDelegate::PrefStoreDelegate(
    base::WeakPtr<AtomBrowserContext> browser_context)
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

}  // namespace atom
