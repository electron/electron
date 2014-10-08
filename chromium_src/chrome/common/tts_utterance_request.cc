// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/tts_utterance_request.h"

TtsUtteranceRequest::TtsUtteranceRequest()
    : id(0),
      volume(1.0),
      rate(1.0),
      pitch(1.0) {
}

TtsUtteranceRequest::~TtsUtteranceRequest() {
}

TtsVoice::TtsVoice()
    : local_service(true),
      is_default(false) {
}

TtsVoice::~TtsVoice() {
}

TtsUtteranceResponse::TtsUtteranceResponse()
    : id(0) {
}

TtsUtteranceResponse::~TtsUtteranceResponse() {
}