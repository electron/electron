// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_MEDIA_CHROME_KEY_SYSTEMS_H_
#define CHROME_RENDERER_MEDIA_CHROME_KEY_SYSTEMS_H_

#include <vector>

#include "media/base/key_system_info.h"

void AddChromeKeySystems(std::vector<media::KeySystemInfo>* key_systems_info);

#endif  // CHROME_RENDERER_MEDIA_CHROME_KEY_SYSTEMS_H_
