// Copyright 2024 Microsoft GmbH.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/ui/views/caption_button_placeholder_container.h"

#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/views/view.h"

CaptionButtonPlaceholderContainer::CaptionButtonPlaceholderContainer() {
  SetPaintToLayer();
}

CaptionButtonPlaceholderContainer::~CaptionButtonPlaceholderContainer() =
    default;

BEGIN_METADATA(CaptionButtonPlaceholderContainer)
END_METADATA
