// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/fake_location_provider.h"

#include "services/device/public/mojom/geoposition.mojom-shared.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace electron {

FakeLocationProvider::FakeLocationProvider() {
  result_ = device::mojom::GeopositionResult::NewError(
      device::mojom::GeopositionError::New(
          device::mojom::GeopositionErrorCode::kPositionUnavailable,
          "Position unavailable.", ""));
}

FakeLocationProvider::~FakeLocationProvider() = default;

void FakeLocationProvider::FillDiagnostics(
    device::mojom::GeolocationDiagnostics& diagnostics) {
  diagnostics.provider_state = state_;
}

void FakeLocationProvider::SetUpdateCallback(
    const LocationProviderUpdateCallback& callback) {
  callback_ = callback;
}

void FakeLocationProvider::StartProvider(bool high_accuracy) {
  state_ =
      high_accuracy
          ? device::mojom::GeolocationDiagnostics::ProviderState::kHighAccuracy
          : device::mojom::GeolocationDiagnostics::ProviderState::kLowAccuracy;
}

void FakeLocationProvider::StopProvider() {
  state_ = device::mojom::GeolocationDiagnostics::ProviderState::kStopped;
}

const device::mojom::GeopositionResult* FakeLocationProvider::GetPosition() {
  return result_.get();
}

void FakeLocationProvider::OnPermissionGranted() {
  if (!callback_.is_null()) {
    callback_.Run(this, result_.Clone());
  }
}

}  // namespace electron
