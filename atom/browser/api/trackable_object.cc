// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/trackable_object.h"

#include "base/supports_user_data.h"

namespace mate {

namespace {

const char* kTrackedObjectKey = "TrackedObjectKey";

class IDUserData : public base::SupportsUserData::Data {
 public:
  explicit IDUserData(int32_t id) : id_(id) {}

  operator int32_t() const { return id_; }

 private:
  int32_t id_;

  DISALLOW_COPY_AND_ASSIGN(IDUserData);
};

}  // namespace

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
TrackableObject* TrackableObject::FromWrappedClass(
    v8::Isolate* isolate, base::SupportsUserData* wrapped) {
  auto id = static_cast<IDUserData*>(wrapped->GetUserData(kTrackedObjectKey));
  if (!id)
    return nullptr;
  return FromWeakMapID(isolate, *id);
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

void TrackableObject::AttachAsUserData(base::SupportsUserData* wrapped) {
  wrapped->SetUserData(kTrackedObjectKey, new IDUserData(weak_map_id_));
}

}  // namespace mate
