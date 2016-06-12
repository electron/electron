// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/ui/tray_icon_observer.h"
#include "base/memory/scoped_ptr.h"
#include "native_mate/handle.h"

namespace gfx {
class Image;
}

namespace mate {
class Arguments;
class Dictionary;
}

namespace atom {

class TrayIcon;

namespace api {

class Menu;
class NativeImage;

class Tray : public mate::TrackableObject<Tray>,
             public TrayIconObserver {
 public:
  static mate::WrappableBase* New(
      v8::Isolate* isolate, mate::Handle<NativeImage> image);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  Tray(v8::Isolate* isolate, mate::Handle<NativeImage> image);
  ~Tray() override;

  // TrayIconObserver:
  void OnClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnRightClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnBalloonShow() override;
  void OnBalloonClicked() override;
  void OnBalloonClosed() override;
  void OnDrop() override;
  void OnDropFiles(const std::vector<std::string>& files) override;
  void OnDragEntered() override;
  void OnDragExited() override;
  void OnDragEnded() override;

  void SetImage(v8::Isolate* isolate, mate::Handle<NativeImage> image);
  void SetPressedImage(v8::Isolate* isolate, mate::Handle<NativeImage> image);
  void SetToolTip(const std::string& tool_tip);
  void SetTitle(const std::string& title);
  void SetHighlightMode(bool highlight);
  void DisplayBalloon(mate::Arguments* args, const mate::Dictionary& options);
  void PopUpContextMenu(mate::Arguments* args);
  void SetContextMenu(v8::Isolate* isolate, mate::Handle<Menu> menu);

 private:
  v8::Local<v8::Object> ModifiersToObject(v8::Isolate* isolate, int modifiers);

  v8::Global<v8::Object> menu_;
  std::unique_ptr<TrayIcon> tray_icon_;

  DISALLOW_COPY_AND_ASSIGN(Tray);
};

}  // namespace api

}  // namespace atom
