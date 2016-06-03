// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/render_process_preferences.h"

#include "atom/common/api/api_messages.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"

namespace atom {

RenderProcessPreferences::RenderProcessPreferences(const Predicate& predicate)
    : predicate_(predicate),
      next_id_(0),
      cache_needs_update_(true) {
  registrar_.Add(this,
                 content::NOTIFICATION_RENDERER_PROCESS_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

RenderProcessPreferences::~RenderProcessPreferences() {
}

int RenderProcessPreferences::AddEntry(const base::DictionaryValue& entry) {
  int id = ++next_id_;
  entries_[id] = entry.CreateDeepCopy();
  cache_needs_update_ = true;
  return id;
}

void RenderProcessPreferences::RemoveEntry(int id) {
  cache_needs_update_ = true;
  entries_.erase(id);
}

void RenderProcessPreferences::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(type, content::NOTIFICATION_RENDERER_PROCESS_CREATED);
  content::RenderProcessHost* process =
      content::Source<content::RenderProcessHost>(source).ptr();

  if (!predicate_.Run(process))
    return;

  UpdateCache();
  process->Send(new AtomMsg_UpdatePreferences(cached_entries_));
}

void RenderProcessPreferences::UpdateCache() {
  if (!cache_needs_update_)
    return;

  cached_entries_.Clear();
  for (const auto& iter : entries_)
    cached_entries_.Append(iter.second->CreateDeepCopy());
  cache_needs_update_ = false;
}

}  // namespace atom
