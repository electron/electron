// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included message file, hence no include guard.

#include <vector>

#include "chrome/common/tts_utterance_request.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_param_traits.h"

#define IPC_MESSAGE_START TtsMsgStart

IPC_STRUCT_TRAITS_BEGIN(TtsUtteranceRequest)
IPC_STRUCT_TRAITS_MEMBER(id)
IPC_STRUCT_TRAITS_MEMBER(text)
IPC_STRUCT_TRAITS_MEMBER(lang)
IPC_STRUCT_TRAITS_MEMBER(voice)
IPC_STRUCT_TRAITS_MEMBER(volume)
IPC_STRUCT_TRAITS_MEMBER(rate)
IPC_STRUCT_TRAITS_MEMBER(pitch)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(TtsVoice)
IPC_STRUCT_TRAITS_MEMBER(voice_uri)
IPC_STRUCT_TRAITS_MEMBER(name)
IPC_STRUCT_TRAITS_MEMBER(lang)
IPC_STRUCT_TRAITS_MEMBER(local_service)
IPC_STRUCT_TRAITS_MEMBER(is_default)
IPC_STRUCT_TRAITS_END()

// Renderer -> Browser messages.

IPC_MESSAGE_CONTROL0(TtsHostMsg_InitializeVoiceList)
IPC_MESSAGE_CONTROL1(TtsHostMsg_Speak,
                     TtsUtteranceRequest)
IPC_MESSAGE_CONTROL0(TtsHostMsg_Pause)
IPC_MESSAGE_CONTROL0(TtsHostMsg_Resume)
IPC_MESSAGE_CONTROL0(TtsHostMsg_Cancel)

// Browser -> Renderer messages.

IPC_MESSAGE_CONTROL1(TtsMsg_SetVoiceList,
                     std::vector<TtsVoice>)
IPC_MESSAGE_CONTROL1(TtsMsg_DidStartSpeaking,
                     int /* utterance id */)
IPC_MESSAGE_CONTROL1(TtsMsg_DidFinishSpeaking,
                     int /* utterance id */)
IPC_MESSAGE_CONTROL1(TtsMsg_DidPauseSpeaking,
                     int /* utterance id */)
IPC_MESSAGE_CONTROL1(TtsMsg_DidResumeSpeaking,
                     int /* utterance id */)
IPC_MESSAGE_CONTROL2(TtsMsg_WordBoundary,
                     int /* utterance id */,
                     int /* char index */)
IPC_MESSAGE_CONTROL2(TtsMsg_SentenceBoundary,
                     int /* utterance id */,
                     int /* char index */)
IPC_MESSAGE_CONTROL2(TtsMsg_MarkerEvent,
                     int /* utterance id */,
                     int /* char index */)
IPC_MESSAGE_CONTROL1(TtsMsg_WasInterrupted,
                     int /* utterance id */)
IPC_MESSAGE_CONTROL1(TtsMsg_WasCancelled,
                     int /* utterance id */)
IPC_MESSAGE_CONTROL2(TtsMsg_SpeakingErrorOccurred,
                     int /* utterance id */,
                     std::string /* error message */)
