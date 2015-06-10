// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_TRAY_H_
#define ATOM_BROWSER_API_ATOM_API_TRAY_H_

#include <string>

#include "atom/browser/api/event_emitter.h"
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

class Tray : public mate::EventEmitter,
             public TrayIconObserver {
 public:
  static mate::Wrappable* New(v8::Isolate* isolate, const gfx::Image& image);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

 protected:
  explicit Tray(const gfx::Image& image);
  virtual ~Tray();

  // TrayIconObserver:
  void OnClicked(const gfx::Rect&) override;
  void OnDoubleClicked() override;
  void OnBalloonShow() override;
  void OnBalloonClicked() override;
  void OnBalloonClosed() override;

  void Destroy();
  void SetImage(mate::Arguments* args, const gfx::Image& image);
  void SetPressedImage(mate::Arguments* args, const gfx::Image& image);
  void SetToolTip(mate::Arguments* args, const std::string& tool_tip);
  void SetTitle(mate::Arguments* args, const std::string& title);
  void SetHighlightMode(mate::Arguments* args, bool highlight);
  void DisplayBalloon(mate::Arguments* args, const mate::Dictionary& options);
  void SetContextMenu(mate::Arguments* args, Menu* menu);

 private:
  bool CheckTrayLife(mate::Arguments* args);

  scoped_ptr<TrayIcon> tray_icon_;

  DISALLOW_COPY_AND_ASSIGN(Tray);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_TRAY_H_
