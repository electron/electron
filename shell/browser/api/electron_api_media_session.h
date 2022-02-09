// Copyright (c) 2022 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MEDIA_SESSION_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MEDIA_SESSION_H_

#include <vector>

#include "base/callback.h"
#include "base/values.h"
#include "gin/arguments.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/media_session/public/mojom/media_session.mojom.h"
#include "shell/browser/event_emitter_mixin.h"

namespace content {
class MediaSession;
}  // namespace content

namespace electron {

namespace api {

class MediaSession : public gin::Wrappable<MediaSession>,
                     public gin_helper::EventEmitterMixin<MediaSession>,
                     public media_session::mojom::MediaSessionObserver {
 public:
  static gin::Handle<MediaSession> Create(v8::Isolate* isolate,
                                          content::MediaSession* media_session);

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  MediaSession(const MediaSession&) = delete;
  MediaSession& operator=(const MediaSession&) = delete;

 protected:
  MediaSession(v8::Isolate* isolate, content::MediaSession* media_session);
  ~MediaSession() override;

 private:
  // media_session::mojom::MediaSessionObserver implementation.
  void MediaSessionInfoChanged(
      media_session::mojom::MediaSessionInfoPtr info) override;
  void MediaSessionMetadataChanged(
      const absl::optional<media_session::MediaMetadata>& metadata) override;
  void MediaSessionActionsChanged(
      const std::vector<media_session::mojom::MediaSessionAction>& action)
      override;
  void MediaSessionImagesChanged(
      const base::flat_map<media_session::mojom::MediaSessionImageType,
                           std::vector<media_session::MediaImage>>& images)
      override;
  void MediaSessionPositionChanged(
      const absl::optional<media_session::MediaPosition>& position) override;

  void Play();
  void Pause();
  void Stop();

  content::MediaSession* const media_session_;

  // Binding through which notifications are received from the MediaSession.
  mojo::Receiver<media_session::mojom::MediaSessionObserver> observer_receiver_;
};

}  // namespace api

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_MEDIA_SESSION_H_
