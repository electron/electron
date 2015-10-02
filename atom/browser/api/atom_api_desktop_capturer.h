// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_DESKTOP_CAPTURER_
#define ATOM_BROWSER_API_ATOM_API_DESKTOP_CAPTURER_

#include "base/memory/scoped_ptr.h"
#include "atom/browser/api/event_emitter.h"
#include "chrome/browser/media/desktop_media_list_observer.h"
#include "chrome/browser/media/native_desktop_media_list.h"
#include "native_mate/handle.h"

namespace atom {

namespace api {

class DesktopCapturer: public mate::EventEmitter,
                       public DesktopMediaListObserver {
 public:
  static mate::Handle<DesktopCapturer> Create(v8::Isolate* isolate,
    bool show_screens, bool show_windows);

  const DesktopMediaList::Source& GetSource(int index);

 protected:
  DesktopCapturer(bool show_screens, bool show_windows);
  ~DesktopCapturer();

  // DesktopMediaListObserver overrides.
  void OnSourceAdded(int index) override;
  void OnSourceRemoved(int index) override;
  void OnSourceMoved(int old_index, int new_index) override;
  void OnSourceNameChanged(int index) override;
  void OnSourceThumbnailChanged(int index) override;

 private:
  // mate::Wrappable:
  mate::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  scoped_ptr<DesktopMediaList> media_list_;

  DISALLOW_COPY_AND_ASSIGN(DesktopCapturer);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_DESKTOP_CAPTURER_
