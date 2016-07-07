// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/tray_icon.h"

namespace atom {

TrayIcon::TrayIcon() {
}

TrayIcon::~TrayIcon() {
}

void TrayIcon::SetPressedImage(ImageType image) {
}

void TrayIcon::SetTitle(const std::string& title) {
}

void TrayIcon::SetHighlightMode(bool highlight) {
}

void TrayIcon::DisplayBalloon(ImageType icon,
                              const base::string16& title,
                              const base::string16& contents) {
}

void TrayIcon::PopUpContextMenu(const gfx::Point& pos,
                                ui::MenuModel* menu_model) {
}

gfx::Rect TrayIcon::GetBounds() {
  return gfx::Rect();
}

void TrayIcon::NotifyClicked(const gfx::Rect& bounds, int modifiers) {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnClicked(bounds, modifiers));
}

void TrayIcon::NotifyDoubleClicked(const gfx::Rect& bounds, int modifiers) {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_,
                    OnDoubleClicked(bounds, modifiers));
}

void TrayIcon::NotifyBalloonShow() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnBalloonShow());
}

void TrayIcon::NotifyBalloonClicked() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnBalloonClicked());
}

void TrayIcon::NotifyBalloonClosed() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnBalloonClosed());
}

void TrayIcon::NotifyRightClicked(const gfx::Rect& bounds, int modifiers) {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_,
                    OnRightClicked(bounds, modifiers));
}

void TrayIcon::NotifyDrop() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnDrop());
}

void TrayIcon::NotifyDropFiles(const std::vector<std::string>& files) {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnDropFiles(files));
}

void TrayIcon::NotifyDragEntered() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnDragEntered());
}

void TrayIcon::NotifyDragExited() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnDragExited());
}

void TrayIcon::NotifyDragEnded() {
  FOR_EACH_OBSERVER(TrayIconObserver, observers_, OnDragEnded());
}

}  // namespace atom
