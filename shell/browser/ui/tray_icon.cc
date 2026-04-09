// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/tray_icon.h"

namespace electron {

TrayIcon::BalloonOptions::BalloonOptions() = default;

TrayIcon::TrayIcon() = default;

TrayIcon::~TrayIcon() = default;

gfx::Rect TrayIcon::GetBounds() {
  return {};
}

void TrayIcon::SetAutoSaveName(const std::string& name) {}

void TrayIcon::NotifyClicked(const gfx::Rect& bounds,
                             const gfx::Point& location,
                             int modifiers) {
  observers_.Notify(&TrayIconObserver::OnClicked, bounds, location, modifiers);
}

void TrayIcon::NotifyDoubleClicked(const gfx::Rect& bounds, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnDoubleClicked, bounds, modifiers);
}

void TrayIcon::NotifyMiddleClicked(const gfx::Rect& bounds, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnMiddleClicked, bounds, modifiers);
}

void TrayIcon::NotifyBalloonShow() {
  observers_.Notify(&TrayIconObserver::OnBalloonShow);
}

void TrayIcon::NotifyBalloonClicked() {
  observers_.Notify(&TrayIconObserver::OnBalloonClicked);
}

void TrayIcon::NotifyBalloonClosed() {
  observers_.Notify(&TrayIconObserver::OnBalloonClosed);
}

void TrayIcon::NotifyRightClicked(const gfx::Rect& bounds, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnRightClicked, bounds, modifiers);
}

void TrayIcon::NotifyDrop() {
  observers_.Notify(&TrayIconObserver::OnDrop);
}

void TrayIcon::NotifyDropFiles(const std::vector<std::string>& files) {
  observers_.Notify(&TrayIconObserver::OnDropFiles, files);
}

void TrayIcon::NotifyDropText(const std::string& text) {
  observers_.Notify(&TrayIconObserver::OnDropText, text);
}

void TrayIcon::NotifyMouseUp(const gfx::Point& location, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnMouseUp, location, modifiers);
}

void TrayIcon::NotifyMouseDown(const gfx::Point& location, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnMouseDown, location, modifiers);
}

void TrayIcon::NotifyMouseEntered(const gfx::Point& location, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnMouseEntered, location, modifiers);
}

void TrayIcon::NotifyMouseExited(const gfx::Point& location, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnMouseExited, location, modifiers);
}

void TrayIcon::NotifyMouseMoved(const gfx::Point& location, int modifiers) {
  observers_.Notify(&TrayIconObserver::OnMouseMoved, location, modifiers);
}

void TrayIcon::NotifyDragEntered() {
  observers_.Notify(&TrayIconObserver::OnDragEntered);
}

void TrayIcon::NotifyDragExited() {
  observers_.Notify(&TrayIconObserver::OnDragExited);
}

void TrayIcon::NotifyDragEnded() {
  observers_.Notify(&TrayIconObserver::OnDragEnded);
}

}  // namespace electron
