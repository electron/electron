// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/trackable_object.h"

#include "atom/browser/atom_browser_main_parts.h"
#include "base/bind.h"
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

TrackableObjectBase::TrackableObjectBase()
    : weak_map_id_(0), wrapped_(nullptr), weak_factory_(this) {
  cleanup_ = RegisterDestructionCallback(GetDestroyClosure());
}

TrackableObjectBase::~TrackableObjectBase() {
  cleanup_.Run();
}

base::Closure TrackableObjectBase::GetDestroyClosure() {
  return base::Bind(&TrackableObjectBase::Destroy, weak_factory_.GetWeakPtr());
}

void TrackableObjectBase::Destroy() {
  delete this;
}

void TrackableObjectBase::AttachAsUserData(base::SupportsUserData* wrapped) {
  if (weak_map_id_ != 0) {
    wrapped->SetUserData(kTrackedObjectKey, new IDUserData(weak_map_id_));
    wrapped_ = nullptr;
  } else {
    // If the TrackableObjectBase is not ready yet then delay SetUserData until
    // AfterInit is called.
    wrapped_ = wrapped;
  }
}

// static
int32_t TrackableObjectBase::GetIDFromWrappedClass(base::SupportsUserData* w) {
  auto id = static_cast<IDUserData*>(w->GetUserData(kTrackedObjectKey));
  if (id)
    return *id;
  else
    return 0;
}

// static
base::Closure TrackableObjectBase::RegisterDestructionCallback(
    const base::Closure& c) {
  return atom::AtomBrowserMainParts::Get()->RegisterDestructionCallback(c);
}

}  // namespace mate
