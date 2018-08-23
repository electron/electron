// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/fake_location_provider.h"

#include "base/callback.h"
#include "base/time/time.h"

namespace atom {

FakeLocationProvider::FakeLocationProvider() {
  position_.latitude = 10;
  position_.longitude = -10;
  position_.accuracy = 1;
  position_.error_code =
      device::mojom::Geoposition::ErrorCode::POSITION_UNAVAILABLE;
}

FakeLocationProvider::~FakeLocationProvider() = default;

void FakeLocationProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {
  callback_ = callback;
}

void FakeLocationProvider::StartProvider(bool high_accuracy) {}

void FakeLocationProvider::StopProvider() {}

const device::mojom::Geoposition& FakeLocationProvider::GetPosition() {
  return position_;
}

void FakeLocationProvider::OnPermissionGranted() {
  if (!callback_.is_null()) {
    // Check device::ValidateGeoPosition for range of values.
    position_.error_code = device::mojom::Geoposition::ErrorCode::NONE;
    position_.timestamp = base::Time::Now();
    callback_.Run(this, position_);
  }
}

}  // namespace atom
