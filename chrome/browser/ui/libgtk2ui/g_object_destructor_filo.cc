// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtk2ui/g_object_destructor_filo.h"

#include <glib-object.h>

#include "base/logging.h"
#include "base/memory/singleton.h"

namespace libgtk2ui {

GObjectDestructorFILO::GObjectDestructorFILO() {
}

GObjectDestructorFILO::~GObjectDestructorFILO() {
  // Probably CHECK(handler_map_.empty()) would look natural here. But
  // some tests (some views_unittests) violate this assertion.
}

// static
GObjectDestructorFILO* GObjectDestructorFILO::GetInstance() {
  return Singleton<GObjectDestructorFILO>::get();
}

void GObjectDestructorFILO::Connect(
    GObject* object, DestructorHook callback, void* context) {
  const Hook hook(object, callback, context);
  HandlerMap::iterator iter = handler_map_.find(object);
  if (iter == handler_map_.end()) {
    g_object_weak_ref(object, WeakNotifyThunk, this);
    handler_map_[object].push_front(hook);
  } else {
    iter->second.push_front(hook);
  }
}

void GObjectDestructorFILO::Disconnect(
    GObject* object, DestructorHook callback, void* context) {
  HandlerMap::iterator iter = handler_map_.find(object);
  if (iter == handler_map_.end()) {
    LOG(DFATAL) << "Unable to disconnect destructor hook for object " << object
                << ": hook not found (" << callback << ", " << context << ").";
    return;
  }
  HandlerList& dtors = iter->second;
  if (dtors.empty()) {
    LOG(DFATAL) << "Destructor list is empty for specified object " << object
                << " Maybe it is being executed?";
    return;
  }
  if (!dtors.front().equal(object, callback, context)) {
    // Reenable this warning once this bug is fixed:
    // http://code.google.com/p/chromium/issues/detail?id=85603
    DVLOG(1) << "Destructors should be unregistered the reverse order they "
             << "were registered. But for object " << object << " "
             << "deleted hook is "<< context << ", the last queued hook is "
             << dtors.front().context;
  }
  for (HandlerList::iterator i = dtors.begin(); i != dtors.end(); ++i) {
    if (i->equal(object, callback, context)) {
      dtors.erase(i);
      break;
    }
  }
  if (dtors.empty()) {
    g_object_weak_unref(object, WeakNotifyThunk, this);
    handler_map_.erase(iter);
  }
}

void GObjectDestructorFILO::WeakNotify(GObject* where_the_object_was) {
  HandlerMap::iterator iter = handler_map_.find(where_the_object_was);
  DCHECK(iter != handler_map_.end());
  DCHECK(!iter->second.empty());

  // Save destructor list for given object into local copy to avoid reentrancy
  // problem: if callee wants to modify the caller list.
  HandlerList dtors;
  iter->second.swap(dtors);
  handler_map_.erase(iter);

  // Execute hooks in local list in FILO order.
  for (HandlerList::iterator i = dtors.begin(); i != dtors.end(); ++i)
    i->callback(i->context, where_the_object_was);
}

}  // namespace libgtk2ui
