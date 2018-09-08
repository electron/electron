// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/chrome_key_systems_provider.h"

#include "base/time/default_tick_clock.h"
#include "chrome/renderer/media/chrome_key_systems.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

ChromeKeySystemsProvider::ChromeKeySystemsProvider()
    : has_updated_(false),
      is_update_needed_(true),
      tick_clock_(base::DefaultTickClock::GetInstance()) {}

ChromeKeySystemsProvider::~ChromeKeySystemsProvider() {}

void ChromeKeySystemsProvider::AddSupportedKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems) {
  DCHECK(key_systems);
  DCHECK(thread_checker_.CalledOnValidThread());

  if (!test_provider_.is_null()) {
    test_provider_.Run(key_systems);
  } else {
    AddChromeKeySystems(key_systems);
  }

  has_updated_ = true;
  last_update_time_ticks_ = tick_clock_->NowTicks();

// Check whether all potentially supported key systems are supported. If so,
// no need to update again.
#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_IS_COMPONENT)
  for (const auto& properties : *key_systems) {
    if (properties->GetKeySystemName() == kWidevineKeySystem) {
      is_update_needed_ = false;
    }
  }
#else
  is_update_needed_ = false;
#endif
}

bool ChromeKeySystemsProvider::IsKeySystemsUpdateNeeded() {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Always needs update if we have never updated, regardless the
  // |last_update_time_ticks_|'s initial value.
  if (!has_updated_) {
    DCHECK(is_update_needed_);
    return true;
  }

  if (!is_update_needed_)
    return false;

  // The update could be expensive. For example, it could involve a sync IPC to
  // the browser process. Use a minimum update interval to avoid unnecessarily
  // frequent update.
  static const int kMinUpdateIntervalInMilliseconds = 1000;
  if ((tick_clock_->NowTicks() - last_update_time_ticks_).InMilliseconds() <
      kMinUpdateIntervalInMilliseconds) {
    return false;
  }

  return true;
}

void ChromeKeySystemsProvider::SetTickClockForTesting(
    base::TickClock* tick_clock) {
  tick_clock_ = tick_clock;
}

void ChromeKeySystemsProvider::SetProviderDelegateForTesting(
    const KeySystemsProviderDelegate& test_provider) {
  test_provider_ = test_provider;
}
