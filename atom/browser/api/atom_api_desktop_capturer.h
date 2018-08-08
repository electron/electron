// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_DESKTOP_CAPTURER_H_
#define ATOM_BROWSER_API_ATOM_API_DESKTOP_CAPTURER_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
#include "chrome/browser/media/desktop_media_list_observer.h"
#include "chrome/browser/media/native_desktop_media_list.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class DesktopCapturer : public mate::EventEmitter<DesktopCapturer>,
                        public DesktopMediaListObserver {
 public:
  struct Source {
    DesktopMediaList::Source media_list_source;
    // Will be an empty string if not available.
    std::string display_id;
  };

  static mate::Handle<DesktopCapturer> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void StartHandling(bool capture_window,
                     bool capture_screen,
                     const gfx::Size& thumbnail_size);

  std::unique_ptr<DesktopMediaList> media_list_;
#if defined(OS_WIN)
  bool using_directx_capturer_ = false;
#endif  // defined(OS_WIN)

 protected:
  explicit DesktopCapturer(v8::Isolate* isolate);
  ~DesktopCapturer() override;

  // DesktopMediaListObserver overrides.
  void OnSourceAdded(int index) override;
  void OnSourceRemoved(int index) override;
  void OnSourceMoved(int old_index, int new_index) override;
  void OnSourceNameChanged(int index) override;
  void OnSourceThumbnailChanged(int index) override;
  bool OnRefreshFinished() override;

 private:
  scoped_refptr<base::SequencedTaskRunner> capture_thread_;

  DISALLOW_COPY_AND_ASSIGN(DesktopCapturer);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_DESKTOP_CAPTURER_H_
