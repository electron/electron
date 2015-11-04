// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_TRAY_H_
#define ATOM_BROWSER_API_ATOM_API_TRAY_H_

#include <string>
#include <vector>

#include "atom/browser/api/trackable_object.h"
#include "atom/browser/ui/tray_icon_observer.h"
#include "base/memory/scoped_ptr.h"

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

class Tray : public mate::TrackableObject<Tray>,
             public TrayIconObserver {
 public:
  static mate::Wrappable* New(v8::Isolate* isolate, const gfx::Image& image);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  explicit Tray(const gfx::Image& image);
  ~Tray() override;

  // TrayIconObserver:
  void OnClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnRightClicked(const gfx::Rect& bounds, int modifiers) override;
  void OnBalloonShow() override;
  void OnBalloonClicked() override;
  void OnBalloonClosed() override;
  void OnDropFiles(const std::vector<std::string>& files) override;

  // mate::Wrappable:
  bool IsDestroyed() const override;

  // mate::TrackableObject:
  void Destroy() override;

  void SetImage(mate::Arguments* args, const gfx::Image& image);
  void SetPressedImage(mate::Arguments* args, const gfx::Image& image);
  void SetToolTip(mate::Arguments* args, const std::string& tool_tip);
  void SetTitle(mate::Arguments* args, const std::string& title);
  void SetHighlightMode(mate::Arguments* args, bool highlight);
  void DisplayBalloon(mate::Arguments* args, const mate::Dictionary& options);
  void PopUpContextMenu(mate::Arguments* args);
  void SetContextMenu(mate::Arguments* args, Menu* menu);

 private:
  v8::Local<v8::Object> ModifiersToObject(v8::Isolate* isolate, int modifiers);

  scoped_ptr<TrayIcon> tray_icon_;

  DISALLOW_COPY_AND_ASSIGN(Tray);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_TRAY_H_
