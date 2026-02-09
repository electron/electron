// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/gin_helper/trackable_object.h"

#include "base/functional/bind.h"
#include "shell/common/process_util.h"

namespace gin_helper {

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

}  // namespace gin_helper
