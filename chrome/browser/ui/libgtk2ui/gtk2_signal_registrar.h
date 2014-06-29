// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTK2UI_GTK2_SIGNAL_REGISTRAR_H_
#define CHROME_BROWSER_UI_LIBGTK2UI_GTK2_SIGNAL_REGISTRAR_H_

#include <glib.h>
#include <map>
#include <vector>

#include "base/basictypes.h"

typedef void (*GCallback) (void);
typedef struct _GObject GObject;
typedef struct _GtkWidget GtkWidget;

namespace libgtk2ui {

// A class that ensures that callbacks don't run on stale owner objects. Similar
// in spirit to NotificationRegistrar. Use as follows:
//
//   class ChromeObject {
//    public:
//     ChromeObject() {
//       ...
//
//       signals_.Connect(widget, "event", CallbackThunk, this);
//     }
//
//     ...
//
//    private:
//     Gtk2SignalRegistrar signals_;
//   };
//
// When |signals_| goes down, it will disconnect the handlers connected via
// Connect.
class Gtk2SignalRegistrar {
 public:
  Gtk2SignalRegistrar();
  ~Gtk2SignalRegistrar();

  // Connect before the default handler. Returns the handler id.
  glong Connect(gpointer instance, const gchar* detailed_signal,
                GCallback signal_handler, gpointer data);
  // Connect after the default handler. Returns the handler id.
  glong ConnectAfter(gpointer instance, const gchar* detailed_signal,
                     GCallback signal_handler, gpointer data);

  // Disconnects all signal handlers connected to |instance|.
  void DisconnectAll(gpointer instance);

 private:
  typedef std::vector<glong> HandlerList;
  typedef std::map<GObject*, HandlerList> HandlerMap;

  static void WeakNotifyThunk(gpointer data, GObject* where_the_object_was) {
    reinterpret_cast<Gtk2SignalRegistrar*>(data)->WeakNotify(
        where_the_object_was);
  }
  void WeakNotify(GObject* where_the_object_was);

  glong ConnectInternal(gpointer instance, const gchar* detailed_signal,
                        GCallback signal_handler, gpointer data, bool after);

  HandlerMap handler_lists_;

  DISALLOW_COPY_AND_ASSIGN(Gtk2SignalRegistrar);
};

}  // namespace libgtk2ui

#endif  // CHROME_BROWSER_UI_LIBGTK2UI_GTK2_SIGNAL_REGISTRAR_H_
