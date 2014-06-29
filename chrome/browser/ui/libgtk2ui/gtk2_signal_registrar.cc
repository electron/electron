// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/libgtk2ui/gtk2_signal_registrar.h"

#include <glib-object.h>

#include "base/logging.h"
#include "chrome/browser/ui/libgtk2ui/g_object_destructor_filo.h"

namespace libgtk2ui {

Gtk2SignalRegistrar::Gtk2SignalRegistrar() {
}

Gtk2SignalRegistrar::~Gtk2SignalRegistrar() {
  for (HandlerMap::iterator list_iter = handler_lists_.begin();
       list_iter != handler_lists_.end(); ++list_iter) {
    GObject* object = list_iter->first;
    GObjectDestructorFILO::GetInstance()->Disconnect(
        object, WeakNotifyThunk, this);

    HandlerList& handlers = list_iter->second;
    for (HandlerList::iterator ids_iter = handlers.begin();
         ids_iter != handlers.end(); ++ids_iter) {
      g_signal_handler_disconnect(object, *ids_iter);
    }
  }
}

glong Gtk2SignalRegistrar::Connect(gpointer instance,
                                   const gchar* detailed_signal,
                                   GCallback signal_handler,
                                   gpointer data) {
  return ConnectInternal(instance, detailed_signal, signal_handler, data,
                         false);
}

glong Gtk2SignalRegistrar::ConnectAfter(gpointer instance,
                                        const gchar* detailed_signal,
                                        GCallback signal_handler,
                                        gpointer data) {
  return ConnectInternal(instance, detailed_signal, signal_handler, data, true);
}

glong Gtk2SignalRegistrar::ConnectInternal(gpointer instance,
                                          const gchar* detailed_signal,
                                          GCallback signal_handler,
                                          gpointer data,
                                          bool after) {
  GObject* object = G_OBJECT(instance);

  HandlerMap::iterator iter = handler_lists_.find(object);
  if (iter == handler_lists_.end()) {
    GObjectDestructorFILO::GetInstance()->Connect(
        object, WeakNotifyThunk, this);
    handler_lists_[object] = HandlerList();
    iter = handler_lists_.find(object);
  }

  glong handler_id = after ?
      g_signal_connect_after(instance, detailed_signal, signal_handler, data) :
      g_signal_connect(instance, detailed_signal, signal_handler, data);
  iter->second.push_back(handler_id);

  return handler_id;
}

void Gtk2SignalRegistrar::WeakNotify(GObject* where_the_object_was) {
  HandlerMap::iterator iter = handler_lists_.find(where_the_object_was);
  if (iter == handler_lists_.end()) {
    NOTREACHED();
    return;
  }
  // The signal handlers will be disconnected automatically. Just erase the
  // handler id list.
  handler_lists_.erase(iter);
}

void Gtk2SignalRegistrar::DisconnectAll(gpointer instance) {
  GObject* object = G_OBJECT(instance);
  HandlerMap::iterator iter = handler_lists_.find(object);
  if (iter == handler_lists_.end())
    return;

  GObjectDestructorFILO::GetInstance()->Disconnect(
      object, WeakNotifyThunk, this);
  HandlerList& handlers = iter->second;
  for (HandlerList::iterator ids_iter = handlers.begin();
       ids_iter != handlers.end(); ++ids_iter) {
    g_signal_handler_disconnect(object, *ids_iter);
  }

  handler_lists_.erase(iter);
}

}  // namespace libgtk2ui
