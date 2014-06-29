// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_LIBGTK2UI_G_OBJECT_DESTRUCTOR_FILO_H_
#define CHROME_BROWSER_UI_LIBGTK2UI_G_OBJECT_DESTRUCTOR_FILO_H_

#include <glib.h>
#include <list>
#include <map>

#include "base/basictypes.h"

template <typename T> struct DefaultSingletonTraits;

typedef struct _GObject GObject;

namespace libgtk2ui {

// This class hooks calls to g_object_weak_ref()/unref() and executes them in
// FILO order. This is important if there are several hooks to the single object
// (set up at different levels of class hierarchy) and the lowest hook (set up
// first) is deleting self - it must be called last (among hooks for the given
// object). Unfortunately Glib does not provide this guarantee.
//
// Use it as follows:
//
// static void OnDestroyedThunk(gpointer data, GObject *where_the_object_was) {
//   reinterpret_cast<MyClass*>(data)->OnDestroyed(where_the_object_was);
// }
// void MyClass::OnDestroyed(GObject *where_the_object_was) {
//   destroyed_ = true;
//   delete this;
// }
// MyClass::Init() {
//   ...
//   ui::GObjectDestructorFILO::GetInstance()->Connect(
//       G_OBJECT(my_widget), &OnDestroyedThunk, this);
// }
// MyClass::~MyClass() {
//   if (!destroyed_) {
//     ui::GObjectDestructorFILO::GetInstance()->Disconnect(
//         G_OBJECT(my_widget), &OnDestroyedThunk, this);
//   }
// }
//
// TODO(glotov): Probably worth adding ScopedGObjectDtor<T>.
//
// This class is a singleton. Not thread safe. Must be called within UI thread.
class GObjectDestructorFILO {
 public:
  typedef void (*DestructorHook)(void* context, GObject* where_the_object_was);

  static GObjectDestructorFILO* GetInstance();
  void Connect(GObject* object, DestructorHook callback, void* context);
  void Disconnect(GObject* object, DestructorHook callback, void* context);

 private:
  struct Hook {
    Hook(GObject* o, DestructorHook cb, void* ctx)
        : object(o), callback(cb), context(ctx) {
    }
    bool equal(GObject* o, DestructorHook cb, void* ctx) const {
      return object == o && callback == cb && context == ctx;
    }
    GObject* object;
    DestructorHook callback;
    void* context;
  };
  typedef std::list<Hook> HandlerList;
  typedef std::map<GObject*, HandlerList> HandlerMap;

  GObjectDestructorFILO();
  ~GObjectDestructorFILO();
  friend struct DefaultSingletonTraits<GObjectDestructorFILO>;

  void WeakNotify(GObject* where_the_object_was);
  static void WeakNotifyThunk(gpointer data, GObject* where_the_object_was) {
    reinterpret_cast<GObjectDestructorFILO*>(data)->WeakNotify(
        where_the_object_was);
  }

  HandlerMap handler_map_;

  DISALLOW_COPY_AND_ASSIGN(GObjectDestructorFILO);
};

}  // namespace libgtk2ui

#endif  // CHROME_BROWSER_UI_LIBGTK2UI_G_OBJECT_DESTRUCTOR_FILO_H_
