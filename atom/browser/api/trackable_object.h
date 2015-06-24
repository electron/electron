// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_TRACKABLE_OBJECT_H_
#define ATOM_BROWSER_API_TRACKABLE_OBJECT_H_

#include "atom/browser/api/event_emitter.h"
#include "atom/common/id_weak_map.h"

namespace mate {

// All instances of TrackableObject will be kept in a weak map and can be got
// from its ID.
class TrackableObject : public mate::EventEmitter {
 public:
  // Find out the TrackableObject from its ID in weak map:
  static TrackableObject* FromWeakMapID(v8::Isolate* isolate, int32_t id);

  // Releases all weak references in weak map, called when app is terminating.
  static void ReleaseAllWeakReferences();

  // mate::Wrappable:
  void AfterInit(v8::Isolate* isolate) override;

  // The ID in weak map.
  int32_t weak_map_id() const { return weak_map_id_; }

 protected:
  TrackableObject();
  ~TrackableObject() override;

 private:
  static atom::IDWeakMap weak_map_;

  int32_t weak_map_id_;

  DISALLOW_COPY_AND_ASSIGN(TrackableObject);
};

}  // namespace mate

#endif  // ATOM_BROWSER_API_TRACKABLE_OBJECT_H_
