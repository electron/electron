// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#pragma once

#include <string>
#include <vector>

namespace gfx {
class Rect;
}

namespace atom {

class TrayIconObserver {
 public:
  virtual void OnClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnDoubleClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnBalloonShow() {}
  virtual void OnBalloonClicked() {}
  virtual void OnBalloonClosed() {}
  virtual void OnRightClicked(const gfx::Rect& bounds, int modifiers) {}
  virtual void OnDrop() {}
  virtual void OnDropFiles(const std::vector<std::string>& files) {}
  virtual void OnDropText(const std::string& text) {}
  virtual void OnDragEntered() {}
  virtual void OnDragExited() {}
  virtual void OnDragEnded() {}

 protected:
  virtual ~TrayIconObserver() {}
};

}  // namespace atom
