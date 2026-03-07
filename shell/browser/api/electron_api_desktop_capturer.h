// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "chrome/browser/media/webrtc/desktop_media_list_observer.h"
#include "chrome/browser/media/webrtc/native_desktop_media_list.h"
#include "shell/common/gin_helper/pinnable.h"
#include "shell/common/gin_helper/wrappable.h"

namespace gin_helper {
template <typename T>
class Handle;
}  // namespace gin_helper

namespace electron::api {

class DesktopCapturer final
    : public gin_helper::DeprecatedWrappable<DesktopCapturer>,
      public gin_helper::Pinnable<DesktopCapturer> {
 public:
  struct Source {
    DesktopMediaList::Source media_list_source;
    // Will be an empty string if not available.
    std::string display_id;

    // Whether or not this source should provide an icon.
    bool fetch_icon = false;
  };

  static gin_helper::Handle<DesktopCapturer> Create(v8::Isolate* isolate);

  static bool IsDisplayMediaSystemPickerAvailable();

  void StartHandling(bool capture_window,
                     bool capture_screen,
                     const gfx::Size& thumbnail_size,
                     bool fetch_window_icons);

  // gin_helper::Wrappable
  static gin::DeprecatedWrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

  // disable copy
  DesktopCapturer(const DesktopCapturer&) = delete;
  DesktopCapturer& operator=(const DesktopCapturer&) = delete;

 protected:
  explicit DesktopCapturer(v8::Isolate* isolate);
  ~DesktopCapturer() override;

 private:
  // Observes a DesktopMediaList and fires the ready callback when sources
  // (and optionally thumbnails) are available. Replaces the old one-shot
  // Update() approach with event-based StartUpdating().
  class SourceListObserver : public DesktopMediaListObserver {
   public:
    SourceListObserver(base::OnceClosure ready_callback,
                       base::OnceClosure failure_callback,
                       DesktopMediaList* list,
                       bool need_thumbnails);
    ~SourceListObserver() override;

   protected:
    // DesktopMediaListObserver:
    void OnSourceAdded(int index) override;
    void OnSourceRemoved(int index) override {}
    void OnSourceMoved(int old_index, int new_index) override {}
    void OnSourceNameChanged(int index) override {}
    void OnSourceThumbnailChanged(int index) override;
    void OnSourcePreviewChanged(size_t index) override {}
    void OnDelegatedSourceListSelection() override {}
    void OnDelegatedSourceListDismissed() override;

   private:
    void CheckReady();

    base::OnceClosure ready_callback_;
    base::OnceClosure failure_callback_;
    raw_ptr<DesktopMediaList> list_;
    bool need_thumbnails_;
    bool finished_ = false;
  };

  void UpdateSourcesList(DesktopMediaList* list);
  void HandleFailure();
  void HandleSuccess();
  void HandleSafetyTimeout();

  std::unique_ptr<SourceListObserver> window_observer_;
  std::unique_ptr<SourceListObserver> screen_observer_;
  std::unique_ptr<DesktopMediaList> window_capturer_;
  std::unique_ptr<DesktopMediaList> screen_capturer_;
  std::vector<DesktopCapturer::Source> captured_sources_;
  bool capture_window_ = false;
  bool capture_screen_ = false;
  bool fetch_window_icons_ = false;
  base::OneShotTimer safety_timer_;
#if BUILDFLAG(IS_WIN)
  bool using_directx_capturer_ = false;
#endif  // BUILDFLAG(IS_WIN)

  base::WeakPtrFactory<DesktopCapturer> weak_ptr_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_DESKTOP_CAPTURER_H_
