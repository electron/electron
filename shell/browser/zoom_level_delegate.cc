// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/zoom_level_delegate.h"

#include <functional>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/strings/string_number_conversions.h"
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

  ScopedDictPrefUpdate update(pref_service_, kPartitionDefaultZoomLevel);
  update->Set(partition_key_, level);
  host_zoom_map_->SetDefaultZoomLevel(level);
}

double ZoomLevelDelegate::GetDefaultZoomLevelPref() const {
  const base::Value::Dict& default_zoom_level_dictionary =
      pref_service_->GetDict(kPartitionDefaultZoomLevel);
  // If no default has been previously set, the default returned is the
  // value used to initialize default_zoom_level in this function.
  return default_zoom_level_dictionary.FindDouble(partition_key_).value_or(0.0);
}

void ZoomLevelDelegate::OnZoomLevelChanged(
    const content::HostZoomMap::ZoomLevelChange& change) {
  if (change.mode != content::HostZoomMap::ZOOM_CHANGED_FOR_HOST)
    return;

  double level = change.zoom_level;
  ScopedDictPrefUpdate update(pref_service_, kPartitionPerHostZoomLevels);
  base::Value::Dict& host_zoom_dictionaries = update.Get();

  bool modification_is_removal =
      blink::PageZoomValuesEqual(level, host_zoom_map_->GetDefaultZoomLevel());

  base::Value::Dict* host_zoom_dictionary =
      host_zoom_dictionaries.FindDict(partition_key_);
  if (!host_zoom_dictionary) {
    base::Value::Dict dict;
    host_zoom_dictionaries.Set(partition_key_, std::move(dict));
    host_zoom_dictionary = host_zoom_dictionaries.FindDict(partition_key_);
  }

  if (modification_is_removal)
    host_zoom_dictionary->Remove(change.host);
  else
    host_zoom_dictionary->Set(change.host, base::Value(level));
}

void ZoomLevelDelegate::ExtractPerHostZoomLevels(
    const base::Value::Dict& host_zoom_dictionary) {
  std::vector<std::string> keys_to_remove;
  base::Value::Dict host_zoom_dictionary_copy = host_zoom_dictionary.Clone();
  for (auto [host, value] : host_zoom_dictionary_copy) {
    const absl::optional<double> zoom_level = value.GetIfDouble();

    // Filter out A) the empty host, B) zoom levels equal to the default; and
    // remember them, so that we can later erase them from Prefs.
    // Values of type B could further have been stored before the default zoom
    // level was set to its current value. In either case, SetZoomLevelForHost
    // will ignore type B values, thus, to have consistency with HostZoomMap's
    // internal state, these values must also be removed from Prefs.
    if (host.empty() || !zoom_level.has_value() ||
        blink::PageZoomValuesEqual(zoom_level.value(),
                                   host_zoom_map_->GetDefaultZoomLevel())) {
      keys_to_remove.push_back(host);
      continue;
    }

    host_zoom_map_->SetZoomLevelForHost(host, zoom_level.value());
  }

  // Sanitize prefs to remove entries that match the default zoom level and/or
  // have an empty host.
  {
    ScopedDictPrefUpdate update(pref_service_, kPartitionPerHostZoomLevels);
    base::Value::Dict* sanitized_host_zoom_dictionary =
        update->FindDict(partition_key_);
    if (sanitized_host_zoom_dictionary) {
      for (const std::string& key : keys_to_remove)
        sanitized_host_zoom_dictionary->Remove(key);
    }
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
  const base::Value::Dict& host_zoom_dictionaries =
      pref_service_->GetDict(kPartitionPerHostZoomLevels);
  const base::Value::Dict* host_zoom_dictionary =
      host_zoom_dictionaries.FindDict(partition_key_);
  if (host_zoom_dictionary) {
    // Since we're calling this before setting up zoom_subscription_ below we
    // don't need to worry that host_zoom_dictionary is indirectly affected
    // by calls to HostZoomMap::SetZoomLevelForHost().
    ExtractPerHostZoomLevels(*host_zoom_dictionary);
  }
  zoom_subscription_ =
      host_zoom_map_->AddZoomLevelChangedCallback(base::BindRepeating(
          &ZoomLevelDelegate::OnZoomLevelChanged, base::Unretained(this)));
}

}  // namespace electron
