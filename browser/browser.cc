// Copyright (c) 2013 GitHub, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "browser/browser.h"

#include "browser/atom_browser_main_parts.h"
#include "browser/window_list.h"

namespace atom {

Browser::Browser()
    : is_quiting_(false) {
  WindowList::AddObserver(this);
}

Browser::~Browser() {
  WindowList::RemoveObserver(this);
}

// static
Browser* Browser::Get() {
  return AtomBrowserMainParts::Get()->browser();
}

void Browser::Quit() {
  atom::WindowList* window_list = atom::WindowList::GetInstance();
  if (window_list->size() == 0)
    Terminate();

  is_quiting_ = true;
  window_list->CloseAllWindows();
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  // Once a beforeunload handler has prevented the closing, we think the quit
  // is cancelled too.
  is_quiting_ = false;
}

void Browser::OnWindowAllClosed() {
  if (is_quiting_)
    Terminate();
}

}  // namespace atom
