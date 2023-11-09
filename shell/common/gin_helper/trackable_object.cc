// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/trackable_object.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/supports_user_data.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/common/process_util.h"

namespace gin_helper {

namespace {

const char kTrackedObjectKey[] = "TrackedObjectKey";

class IDUserData : public base::SupportsUserData::Data {
 public:
  explicit IDUserData(int32_t id) : id_(id) {}

  explicit operator int32_t() const { return id_; }

 private:
  int32_t id_;
};

}  // namespace

TrackableObjectBase::TrackableObjectBase() {
  // TODO(zcbenz): Make TrackedObject work in renderer process.
  DCHECK(electron::IsBrowserProcess())
      << "This class only works for browser process";
}

TrackableObjectBase::~TrackableObjectBase() = default;

base::OnceClosure TrackableObjectBase::GetDestroyClosure() {
  return base::BindOnce(&TrackableObjectBase::Destroy,
                        weak_factory_.GetWeakPtr());
}

void TrackableObjectBase::Destroy() {
  delete this;
}

void TrackableObjectBase::AttachAsUserData(base::SupportsUserData* wrapped) {
  wrapped->SetUserData(kTrackedObjectKey,
                       std::make_unique<IDUserData>(weak_map_id_));
}

// static
int32_t TrackableObjectBase::GetIDFromWrappedClass(
    base::SupportsUserData* wrapped) {
  if (wrapped) {
    auto* id =
        static_cast<IDUserData*>(wrapped->GetUserData(kTrackedObjectKey));
    if (id)
      return int32_t(*id);
  }
  return 0;
}

}  // namespace gin_helper
