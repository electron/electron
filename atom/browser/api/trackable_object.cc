// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/trackable_object.h"

namespace mate {

atom::IDWeakMap TrackableObject::weak_map_;

// static
TrackableObject* TrackableObject::FromWeakMapID(v8::Isolate* isolate,
                                                int32_t id) {
  v8::MaybeLocal<v8::Object> object = weak_map_.Get(isolate, id);
  if (object.IsEmpty())
    return nullptr;

  TrackableObject* self = nullptr;
  mate::ConvertFromV8(isolate, object.ToLocalChecked(), &self);
  return self;
}

// static
void TrackableObject::ReleaseAllWeakReferences() {
  weak_map_.Clear();
}

TrackableObject::TrackableObject() : weak_map_id_(0) {
}

TrackableObject::~TrackableObject() {
  weak_map_.Remove(weak_map_id_);
}

void TrackableObject::AfterInit(v8::Isolate* isolate) {
  weak_map_id_ = weak_map_.Add(isolate, GetWrapper(isolate));
}

}  // namespace mate
