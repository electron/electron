// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/media/media_device_id_salt.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace atom {

namespace {

const char kMediaDeviceIdSalt[] = "electron.media.device_id_salt";

}  // namespace

MediaDeviceIDSalt::MediaDeviceIDSalt(PrefService* pref_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  media_device_id_salt_.Init(kMediaDeviceIdSalt, pref_service);
  if (media_device_id_salt_.GetValue().empty()) {
    media_device_id_salt_.SetValue(
        content::BrowserContext::CreateRandomMediaDeviceIDSalt());
  }
}

MediaDeviceIDSalt::~MediaDeviceIDSalt() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  media_device_id_salt_.Destroy();
}

std::string MediaDeviceIDSalt::GetSalt() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return media_device_id_salt_.GetValue();
}

// static
void MediaDeviceIDSalt::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kMediaDeviceIdSalt, std::string());
}

// static
void MediaDeviceIDSalt::Reset(PrefService* pref_service) {
  pref_service->SetString(
      kMediaDeviceIdSalt,
      content::BrowserContext::CreateRandomMediaDeviceIDSalt());
}

}  // namespace atom
