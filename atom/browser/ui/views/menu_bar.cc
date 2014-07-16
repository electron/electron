// Copyright (c) 2014 GitHub, Inc. All rights reserved.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/views/menu_bar.h"

#include "ui/gfx/canvas.h"

namespace atom {

namespace {

const char kViewClassName[] = "AtomMenuBar";

// Default color of the menu bar.
const SkColor kDefaultColor = SkColorSetARGB(255, 233, 233, 233);

}  // namespace

MenuBar::MenuBar() {
}

MenuBar::~MenuBar() {
}

void MenuBar::Paint(gfx::Canvas* canvas) {
  canvas->FillRect(bounds(), kDefaultColor);
}

const char* MenuBar::GetClassName() const {
  return kViewClassName;
}

}  // namespace atom
