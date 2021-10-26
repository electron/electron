// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/zoom_level_delegate.h"

#include <functional>
#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "components/prefs/json_pref_store.h"
#include "components/prefs/pref_filter.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service_factory.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_thread.h"
#include "third_party/blink/public/common/page/page_zoom.h"

namespace electron {

namespace {

// Double that indicates the default zoom level.
const char kPartitionDefaultZoomLevel[] = "partition.default_zoom_level";

// Dictionary that maps hostnames to zoom levels.  Hosts not in this pref will
// be displayed at the default zoom level.
const char kPartitionPerHostZoomLevels[] = "partition.per_host_zoom_levels";

std::string GetHash(const base::FilePath& partition_path) {
  size_t int_key = std::hash<base::FilePath>()(partition_path);
  return base::NumberToString(int_key);
}

}  // namespace

// static
void ZoomLevelDelegate::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kPartitionDefaultZoomLevel);
  registry->RegisterDictionaryPref(kPartitionPerHostZoomLevels);
}

ZoomLevelDelegate::ZoomLevelDelegate(PrefService* pref_service,
                                     const base::FilePath& partition_path)
    : pref_service_(pref_service) {
  DCHECK(pref_service_);
  partition_key_ = GetHash(partition_path);
}

ZoomLevelDelegate::~ZoomLevelDelegate() = default;

void ZoomLevelDelegate::SetDefaultZoomLevelPref(double level) {
  if (blink::PageZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel()))
    return;

  DictionaryPrefUpdate update(pref_service_, kPartitionDefaultZoomLevel);
  update->SetDouble(partition_key_, level);
  host_zoom_map_->SetDefaultZoomLevel(level);
}

double ZoomLevelDelegate::GetDefaultZoomLevelPref() const {
  double default_zoom_level = 0.0;

  const base::DictionaryValue* default_zoom_level_dictionary =
      pref_service_->GetDictionary(kPartitionDefaultZoomLevel);
  // If no default has been previously set, the default returned is the
  // value used to initialize default_zoom_level in this function.
  absl::optional<double> maybe_default_zoom_level =
      default_zoom_level_dictionary->FindDoubleKey(partition_key_);
  if (maybe_default_zoom_level.has_value())
    default_zoom_level = maybe_default_zoom_level.value();

  return default_zoom_level;
}

void ZoomLevelDelegate::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  if (change.mode != content::HostZoomMap::ZOOM_CHANGED_FOR_HOST)
    return;

  double level = change.zoom_level;
  DictionaryPrefUpdate update(pref_service_, kPartitionPerHostZoomLevels);
  base::DictionaryValue* host_zoom_dictionaries = update.Get();
  DCHECK(host_zoom_dictionaries);

  bool modification_is_removal =
      blink::PageZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel());

  base::DictionaryValue* host_zoom_dictionary = nullptr;
  if (!host_zoom_dictionaries->GetDictionary(partition_key_,
                                             &host_zoom_dictionary)) {
    host_zoom_dictionary = host_zoom_dictionaries->SetDictionary(
        partition_key_, std::make_unique<base::DictionaryValue>());
  }

  if (modification_is_removal)
    host_zoom_dictionary->RemoveKey(change.host);
  else
    host_zoom_dictionary->SetKey(change.host, base::Value(level));
}

void ZoomLevelDelegate::ExtractPerHostZoomLevels(
    const base::DictionaryValue* host_zoom_dictionary) {
  std::vector<std::string> keys_to_remove;
  std::unique_ptr<base::DictionaryValue> host_zoom_dictionary_copy =
      host_zoom_dictionary->DeepCopyWithoutEmptyChildren();
  for (base::DictionaryValue::Iterator i(*host_zoom_dictionary_copy);
       !i.IsAtEnd(); i.Advance()) {
    const std::string& host(i.key());
    const absl::optional<double> zoom_level = i.value().GetIfDouble();

    // Filter out A) the empty host, B) zoom levels equal to the default; and
    // remember them, so that we can later erase them from Prefs.
    // Values of type B could further have been stored before the default zoom
    // level was set to its current value. In either case, SetZoomLevelForHost
    // will ignore type B values, thus, to have consistency with HostZoomMap's
    // internal state, these values must also be removed from Prefs.
    if (host.empty() || !zoom_level ||
        blink::PageZoomValuesEqual(*zoom_level,
                                   host_zoom_map_->GetDefaultZoomLevel())) {
      keys_to_remove.push_back(host);
      continue;
    }

    host_zoom_map_->SetZoomLevelForHost(host, *zoom_level);
  }

  // Sanitize prefs to remove entries that match the default zoom level and/or
  // have an empty host.
  {
    DictionaryPrefUpdate update(pref_service_, kPartitionPerHostZoomLevels);
    base::DictionaryValue* host_zoom_dictionaries = update.Get();
    base::DictionaryValue* sanitized_host_zoom_dictionary = nullptr;
    host_zoom_dictionaries->GetDictionary(partition_key_,
                                          &sanitized_host_zoom_dictionary);
    for (const std::string& s : keys_to_remove)
      sanitized_host_zoom_dictionary->RemoveKey(s);
  }
}

void ZoomLevelDelegate::InitHostZoomMap(content::HostZoomMap* host_zoom_map) {
  // This init function must be called only once.
  DCHECK(!host_zoom_map_);
  DCHECK(host_zoom_map);
  host_zoom_map_ = host_zoom_map;

  // Initialize the default zoom level.
  host_zoom_map_->SetDefaultZoomLevel(GetDefaultZoomLevelPref());

  // Initialize the HostZoomMap with per-host zoom levels from the persisted
  // zoom-level preference values.
  const base::DictionaryValue* host_zoom_dictionaries =
      pref_service_->GetDictionary(kPartitionPerHostZoomLevels);
  const base::DictionaryValue* host_zoom_dictionary = nullptr;
  if (host_zoom_dictionaries->GetDictionary(partition_key_,
                                            &host_zoom_dictionary)) {
    // Since we're calling this before setting up zoom_subscription_ below we
    // don't need to worry that host_zoom_dictionary is indirectly affected
    // by calls to HostZoomMap::SetZoomLevelForHost().
    ExtractPerHostZoomLevels(host_zoom_dictionary);
  }
  zoom_subscription_ =
      host_zoom_map_->AddZoomLevelChangedCallback(base::BindRepeating(
          &ZoomLevelDelegate::OnZoomLevelChanged, base::Unretained(this)));
}

}  // namespace electron
