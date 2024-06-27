// Copyright 2024 Microsoft GmbH.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_VIEWS_CAPTION_BUTTON_PLACEHOLDER_CONTAINER_H_
#define ELECTRON_SHELL_BROWSER_UI_VIEWS_CAPTION_BUTTON_PLACEHOLDER_CONTAINER_H_

#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/view.h"

// A placeholder container for control buttons with window controls
// overlay display override. Does not interact with the buttons. It is just
// used to indicate that this is non-client-area.
class CaptionButtonPlaceholderContainer : public views::View {
  METADATA_HEADER(CaptionButtonPlaceholderContainer, views::View)

 public:
  CaptionButtonPlaceholderContainer();
  CaptionButtonPlaceholderContainer(const CaptionButtonPlaceholderContainer&) =
      delete;
  CaptionButtonPlaceholderContainer& operator=(
      const CaptionButtonPlaceholderContainer&) = delete;
  ~CaptionButtonPlaceholderContainer() override;
};

#endif  // ELECTRON_SHELL_BROWSER_UI_VIEWS_CAPTION_BUTTON_PLACEHOLDER_CONTAINER_H_
