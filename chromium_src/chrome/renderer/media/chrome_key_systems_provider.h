// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_CHROME_KEY_SYSTEMS_PROVIDER_H_
#define CHROME_RENDERER_MEDIA_CHROME_KEY_SYSTEMS_PROVIDER_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/threading/thread_checker.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "media/base/key_system_properties.h"

typedef std::vector<std::unique_ptr<media::KeySystemProperties>>
    KeySystemPropertiesVector;
typedef base::Callback<void(KeySystemPropertiesVector*)>
    KeySystemsProviderDelegate;

class ChromeKeySystemsProvider {
 public:
  ChromeKeySystemsProvider();
  ~ChromeKeySystemsProvider();

  // Adds properties for supported key systems.
  void AddSupportedKeySystems(KeySystemPropertiesVector* key_systems);

  // Returns whether client key systems properties should be updated.
  // TODO(chcunningham): Refactor this to a proper change "observer" API that is
  // less fragile (don't assume AddSupportedKeySystems has just one caller).
  bool IsKeySystemsUpdateNeeded();

  void SetTickClockForTesting(base::TickClock* tick_clock);

  void SetProviderDelegateForTesting(
      const KeySystemsProviderDelegate& test_provider);

 private:
  // Whether AddSupportedKeySystems() has ever been called.
  bool has_updated_;

  // Whether a future update is needed. For example, when some potentially
  // supported key systems are NOT supported yet. This could happen when the
  // required component for a key system is not yet available.
  bool is_update_needed_;

  // Throttle how often we signal an update is needed to avoid unnecessary high
  // frequency of expensive IPC calls.
  base::TimeTicks last_update_time_ticks_;
  base::TickClock* tick_clock_;

  // Ensure all methods are called from the same (Main) thread.
  base::ThreadChecker thread_checker_;

  // For unit tests to inject their own key systems. Will bypass adding default
  // Chrome key systems when set.
  KeySystemsProviderDelegate test_provider_;

  DISALLOW_COPY_AND_ASSIGN(ChromeKeySystemsProvider);
};

#endif  // CHROME_RENDERER_MEDIA_CHROME_KEY_SYSTEMS_PROVIDER_H_
