// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_media_session.h"

#include <string>
#include <vector>

#include "content/public/browser/media_session.h"
#include "gin/object_template_builder.h"
#include "gin/per_isolate_data.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/std_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"

namespace gin {

template <>
struct Converter<media_session::mojom::MediaSessionAction> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      media_session::mojom::MediaSessionAction val) {
    std::string action;
    switch (val) {
      case media_session::mojom::MediaSessionAction::kPlay:
        action = "play";
        break;
      case media_session::mojom::MediaSessionAction::kPause:
        action = "pause";
        break;
      case media_session::mojom::MediaSessionAction::kPreviousTrack:
        action = "previous-track";
        break;
      case media_session::mojom::MediaSessionAction::kNextTrack:
        action = "next-track";
        break;
      case media_session::mojom::MediaSessionAction::kSeekBackward:
        action = "seek-backward";
        break;
      case media_session::mojom::MediaSessionAction::kSeekForward:
        action = "seek-forward";
        break;
      case media_session::mojom::MediaSessionAction::kSkipAd:
        action = "skip-ad";
        break;
      case media_session::mojom::MediaSessionAction::kStop:
        action = "stop";
        break;
      case media_session::mojom::MediaSessionAction::kSeekTo:
        action = "seek-to";
        break;
      case media_session::mojom::MediaSessionAction::kScrubTo:
        action = "scrub-to";
        break;
      case media_session::mojom::MediaSessionAction::kEnterPictureInPicture:
        action = "enter-picture-in-picture";
        break;
      case media_session::mojom::MediaSessionAction::kExitPictureInPicture:
        action = "exit-picture-in-picture";
        break;
      case media_session::mojom::MediaSessionAction::kSwitchAudioDevice:
        action = "switch-audio-device";
        break;
      case media_session::mojom::MediaSessionAction::kToggleMicrophone:
        action = "toggle-microphone";
        break;
      case media_session::mojom::MediaSessionAction::kToggleCamera:
        action = "toggle-camera";
        break;
      case media_session::mojom::MediaSessionAction::kHangUp:
        action = "hang-up";
        break;
      case media_session::mojom::MediaSessionAction::kRaise:
        action = "raise";
        break;
      case media_session::mojom::MediaSessionAction::kSetMute:
        action = "set-mute";
        break;
        // When adding a new value, remember to update the docs!
    }
    return gin::ConvertToV8(isolate, action);
  }
};

template <>
struct Converter<media_session::mojom::MediaSessionInfoPtr> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const media_session::mojom::MediaSessionInfoPtr& val) {
    auto dict = gin::Dictionary::CreateEmpty(isolate);

    // TODO(samuelmaddock): Determine which state we want to expose in the API.
    // std::string state;
    // switch (val->state) {
    //   case media_session::mojom::MediaSessionInfo::SessionState::kActive:
    //   case media_session::mojom::MediaSessionInfo::SessionState::kDucking:
    //     state = "playing";
    //     break;
    //   case media_session::mojom::MediaSessionInfo::SessionState::kSuspended:
    //     state = "paused";
    //     break;
    //   case media_session::mojom::MediaSessionInfo::SessionState::kInactive:
    //     state = "idle";
    //     break;
    // }
    // dict.Set("state", state);
    // dict.Set("ducked", val->state ==
    // media_session::mojom::MediaSessionInfo::SessionState::kDucking);

    std::string playback_state;
    switch (val->playback_state) {
      case media_session::mojom::MediaPlaybackState::kPlaying:
        playback_state = "playing";
        break;
      case media_session::mojom::MediaPlaybackState::kPaused:
        playback_state = "paused";
        break;
    }
    dict.Set("playbackState", playback_state);

    return gin::ConvertToV8(isolate, dict);
  }
};

}  // namespace gin

namespace electron {

namespace api {

gin::WrapperInfo MediaSession::kWrapperInfo = {gin::kEmbedderNativeGin};

MediaSession::MediaSession(v8::Isolate* isolate,
                           content::MediaSession* media_session)
    : media_session_(media_session), observer_receiver_(this) {
  if (!observer_receiver_.is_bound()) {
    // |media_session| will notify us via our MediaSessionObserver interface
    // of the current state of the session (metadata, actions, etc) in response
    // to AddObserver().
    media_session_->AddObserver(observer_receiver_.BindNewPipeAndPassRemote());
  }
}

MediaSession::~MediaSession() = default;

void MediaSession::MediaSessionInfoChanged(
    media_session::mojom::MediaSessionInfoPtr info) {
  Emit("info-changed", info);
}

void MediaSession::MediaSessionMetadataChanged(
    const absl::optional<media_session::MediaMetadata>& metadata_mojo) {
  // TODO(samuelmaddock): emit event
}

void MediaSession::MediaSessionActionsChanged(
    const std::vector<media_session::mojom::MediaSessionAction>& actions) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details = gin_helper::Dictionary::CreateEmpty(isolate);
  details.Set("actions", actions);
  Emit("actions-changed", details);
}

void MediaSession::MediaSessionImagesChanged(
    const base::flat_map<media_session::mojom::MediaSessionImageType,
                         std::vector<media_session::MediaImage>>& images) {
  // TODO(samuelmaddock): emit event
}

void MediaSession::MediaSessionPositionChanged(
    const absl::optional<media_session::MediaPosition>& position) {
  // TODO(samuelmaddock): emit event
}

void MediaSession::Play() {
  media_session_->Resume(content::MediaSession::SuspendType::kUI);
}

void MediaSession::Pause() {
  media_session_->Suspend(content::MediaSession::SuspendType::kUI);
}

void MediaSession::Stop() {
  media_session_->Stop(content::MediaSession::SuspendType::kUI);
}

// static
gin::Handle<MediaSession> MediaSession::Create(
    v8::Isolate* isolate,
    content::MediaSession* media_session) {
  return gin::CreateHandle(isolate, new MediaSession(isolate, media_session));
}

gin::ObjectTemplateBuilder MediaSession::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<MediaSession>::GetObjectTemplateBuilder(
             isolate)
      .SetMethod("play", &MediaSession::Play)
      .SetMethod("pause", &MediaSession::Pause)
      .SetMethod("stop", &MediaSession::Stop);
}

const char* MediaSession::GetTypeName() {
  return "MediaSession";
}

}  // namespace api

}  // namespace electron
