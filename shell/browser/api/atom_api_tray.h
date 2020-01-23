// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_TRAY_H_
#define SHELL_BROWSER_API_ATOM_API_TRAY_H_

#include <memory>
#include <string>
#include <vector>

#include "gin/handle.h"
#include "shell/browser/ui/tray_icon.h"
#include "shell/browser/ui/tray_icon_observer.h"
#include "shell/common/gin_helper/error_thrower.h"
#include "shell/common/gin_helper/trackable_object.h"

namespace gfx {
class Image;
}

namespace gin_helper {
class Dictionary;
}

namespace electron {

class TrayIcon;

namespace api {

class Menu;
class NativeImage;

class Tray : public gin_helper::TrackableObject<Tray>, public TrayIconObserver {
 public:
  static gin_helper::WrappableBase* New(gin_helper::ErrorThrower thrower,
                                        gin::Handle<NativeImage> image,
                                        gin_helper::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  Tray(gin::Handle<NativeImage> image, gin_helper::Arguments* args);
  ~Tray() override;

  // TrayIconObserver:
  void OnClicked(const gfx::Rect& bounds,
                 const gfx::Point& location,
                 int modifiers) override;
  void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnRightClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnBalloonShow() override;
  void OnBalloonClicked() override;
  void OnBalloonClosed() override;
  void OnDrop() override;
  void OnDropFiles(const std::vector<std::string>& files) override;
  void OnDropText(const std::string& text) override;
  void OnDragEntered() override;
  void OnDragExited() override;
  void OnDragEnded() override;
  void OnMouseUp(const gfx::Point& location, int modifiers) override;
  void OnMouseDown(const gfx::Point& location, int modifiers) override;
  void OnMouseEntered(const gfx::Point& location, int modifiers) override;
  void OnMouseExited(const gfx::Point& location, int modifiers) override;
  void OnMouseMoved(const gfx::Point& location, int modifiers) override;

  void SetImage(v8::Isolate* isolate, gin::Handle<NativeImage> image);
  void SetPressedImage(v8::Isolate* isolate, gin::Handle<NativeImage> image);
  void SetToolTip(const std::string& tool_tip);
  void SetTitle(const std::string& title);
  std::string GetTitle();
  void SetIgnoreDoubleClickEvents(bool ignore);
  bool GetIgnoreDoubleClickEvents();
  void DisplayBalloon(gin_helper::ErrorThrower thrower,
                      const gin_helper::Dictionary& options);
  void RemoveBalloon();
  void Focus();
  void PopUpContextMenu(gin_helper::Arguments* args);
  void CloseContextMenu();
  void SetContextMenu(gin_helper::ErrorThrower thrower,
                      v8::Local<v8::Value> arg);
  gfx::Rect GetBounds();

 private:
  v8::Global<v8::Value> menu_;
  std::unique_ptr<TrayIcon> tray_icon_;

  DISALLOW_COPY_AND_ASSIGN(Tray);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_TRAY_H_
