// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/browser.h"

#include <stdlib.h>

#include "browser/native_window.h"
#include "browser/window_list.h"
#include "common/atom_version.h"

namespace atom {

void Browser::Terminate() {
  is_quiting_ = true;
  exit(0);
}

void Browser::Focus() {
  // Focus on the first visible window.
  WindowList* list = WindowList::GetInstance();
  for (WindowList::iterator iter = list->begin(); iter != list->end(); ++iter) {
    NativeWindow* window = *iter;
    if (window->IsVisible()) {
      window->Focus(true);
      break;
    }
  }
}

std::string Browser::GetExecutableFileVersion() const {
  return ATOM_VERSION_STRING;
}

std::string Browser::GetExecutableFileProductName() const {
  return "Atom-Shell";
}

void Browser::CancelQuit() {
  // No way to cancel quit on Linux.
}

}  // namespace atom
