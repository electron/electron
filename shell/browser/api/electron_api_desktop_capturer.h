// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/timer/timer.h"
#include "chrome/browser/media/webrtc/desktop_media_list.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "shell/common/gin_helper/self_keep_alive.h"

namespace electron::api {

class DesktopCapturer final : public gin::Wrappable<DesktopCapturer> {
 public:
  struct Source {
    DesktopMediaList::Source media_list_source;
    // Will be an empty string if not available.
    std::string display_id;

    // Whether or not this source should provide an icon.
    bool fetch_icon = false;
  };

  static DesktopCapturer* Create(v8::Isolate* isolate);

  static bool IsDisplayMediaSystemPickerAvailable();

  void StartHandling(bool capture_window,
                     bool capture_screen,
                     const gfx::Size& thumbnail_size,
                     bool fetch_window_icons);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  void Trace(cppgc::Visitor* visitor) const override;
  const gin::WrapperInfo* wrapper_info() const override;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetHumanReadableName() const override;

  // disable copy
  DesktopCapturer(const DesktopCapturer&) = delete;
  DesktopCapturer& operator=(const DesktopCapturer&) = delete;

  // Make public for cppgc::MakeGarbageCollected.
  DesktopCapturer();
  ~DesktopCapturer() override;

 private:
  class ListObserver;

  cppgc::Persistent<gin::WeakCell<DesktopCapturer>> WeakCallbackTarget();

  void FinalizeList(std::unique_ptr<ListObserver>& observer,
                    std::unique_ptr<DesktopMediaList>& list);
  void OnListReady(DesktopMediaList::Type type);
  void OnReadyTimeout();
  void CollectSourcesFrom(DesktopMediaList* list);
  void HandleFailure();
  void HandleSuccess();

  std::unique_ptr<ListObserver> window_observer_;
  std::unique_ptr<ListObserver> screen_observer_;
  std::unique_ptr<DesktopMediaList> window_capturer_;
  std::unique_ptr<DesktopMediaList> screen_capturer_;
  std::vector<DesktopCapturer::Source> captured_sources_;
  int pending_lists_ = 0;
  bool finished_ = false;
  bool fetch_window_icons_ = false;
  base::OneShotTimer deadline_;
#if BUILDFLAG(IS_WIN)
  bool using_directx_capturer_ = false;
#endif  // BUILDFLAG(IS_WIN)

  gin_helper::SelfKeepAlive<DesktopCapturer> keep_alive_{this};
  gin::WeakCellFactory<DesktopCapturer> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_
