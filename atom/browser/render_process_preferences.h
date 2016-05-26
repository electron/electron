// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_RENDER_PROCESS_PREFERENCES_H_
#define ATOM_BROWSER_RENDER_PROCESS_PREFERENCES_H_

#include <memory>
#include <unordered_map>

#include "base/callback.h"
#include "base/values.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace content {
class RenderProcessHost;
}

namespace atom {

// Sets user preferences for render processes.
class RenderProcessPreferences : public content::NotificationObserver {
 public:
  using Predicate = base::Callback<bool(content::RenderProcessHost*)>;

  // The |predicate| is used to determine whether to set preferences for a
  // render process.
  explicit RenderProcessPreferences(const Predicate& predicate);
  virtual ~RenderProcessPreferences();

  int AddEntry(const base::DictionaryValue& entry);
  void RemoveEntry(int id);

 private:
  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void UpdateCache();

  // Manages our notification registrations.
  content::NotificationRegistrar registrar_;

  Predicate predicate_;

  int next_id_;
  std::unordered_map<int, std::unique_ptr<base::DictionaryValue>> entries_;

  // We need to convert the |entries_| to ListValue for multiple times, this
  // caches is only updated when we are sending messages.
  bool cache_needs_update_;
  base::ListValue cached_entries_;

  DISALLOW_COPY_AND_ASSIGN(RenderProcessPreferences);
};

}  // namespace atom

#endif  // ATOM_BROWSER_RENDER_PROCESS_PREFERENCES_H_
