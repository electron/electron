// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_TTS_UTTERANCE_REQUEST_H_
#define CHROME_COMMON_TTS_UTTERANCE_REQUEST_H_

#include <vector>

#include "base/basictypes.h"
#include "base/strings/string16.h"

struct TtsUtteranceRequest {
  TtsUtteranceRequest();
  ~TtsUtteranceRequest();

  int id;
  std::string text;
  std::string lang;
  std::string voice;
  float volume;
  float rate;
  float pitch;
};

struct TtsVoice {
  TtsVoice();
  ~TtsVoice();

  std::string voice_uri;
  std::string name;
  std::string lang;
  bool local_service;
  bool is_default;
};

struct TtsUtteranceResponse {
  TtsUtteranceResponse();
  ~TtsUtteranceResponse();

  int id;
};

#endif  // CHROME_COMMON_TTS_UTTERANCE_REQUEST_H_