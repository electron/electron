// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_FAKE_LOCATION_PROVIDER_H_
#define ELECTRON_SHELL_BROWSER_FAKE_LOCATION_PROVIDER_H_

#include "services/device/public/cpp/geolocation/location_provider.h"
#include "services/device/public/mojom/geoposition.mojom.h"

namespace electron {

class FakeLocationProvider : public device::LocationProvider {
 public:
  FakeLocationProvider();
  ~FakeLocationProvider() override;

  // disable copy
  FakeLocationProvider(const FakeLocationProvider&) = delete;
  FakeLocationProvider& operator=(const FakeLocationProvider&) = delete;

  // LocationProvider Implementation:
  void SetUpdateCallback(
      const LocationProviderUpdateCallback& callback) override;
  void StartProvider(bool high_accuracy) override;
  void StopProvider() override;
  const device::mojom::Geoposition& GetPosition() override;
  void OnPermissionGranted() override;

 private:
  device::mojom::Geoposition position_;
  LocationProviderUpdateCallback callback_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_FAKE_LOCATION_PROVIDER_H_
