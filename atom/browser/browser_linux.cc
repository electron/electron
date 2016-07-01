// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <stdlib.h>

#include "atom/browser/native_window.h"
#include "atom/browser/window_list.h"
#include "atom/common/atom_version.h"
#include "brightray/common/application_info.h"
#include "chrome/browser/ui/libgtk2ui/unity_service.h"

namespace atom {

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

void Browser::AddRecentDocument(const base::FilePath& path) {
}

void Browser::ClearRecentDocuments() {
}

void Browser::SetAppUserModelID(const base::string16& name) {
}

bool Browser::RemoveAsDefaultProtocolClient(const std::string& protocol) {
  return false;
}

bool Browser::SetAsDefaultProtocolClient(const std::string& protocol) {
  return false;
}

bool Browser::IsDefaultProtocolClient(const std::string& protocol) {
  return false;
}

void Browser::SetBadgeCount(int count) {
  current_badge_count_ = count;
  unity::SetDownloadCount(count);
}

std::string Browser::GetExecutableFileVersion() const {
  return brightray::GetApplicationVersion();
}

std::string Browser::GetExecutableFileProductName() const {
  return brightray::GetApplicationName();
}

bool Browser::IsUnityRunning() {
  return unity::IsRunning();
}

}  // namespace atom
