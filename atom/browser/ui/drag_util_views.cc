// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/drag_util.h"

#include "ui/aura/window.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/drag_utils.h"
#include "ui/base/dragdrop/file_info.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/public/drag_drop_client.h"

namespace atom {

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view) {
  // Set up our OLE machinery
  ui::OSExchangeData data;

  drag_utils::CreateDragImageForFile(files[0], icon.AsImageSkia(), &data);

  std::vector<ui::FileInfo> file_infos;
  for (const base::FilePath& file : files) {
    file_infos.push_back(ui::FileInfo(file, base::FilePath()));
  }
  data.SetFilenames(file_infos);

  aura::Window* root_window = view->GetRootWindow();
  if (!root_window || !aura::client::GetDragDropClient(root_window))
    return;

  gfx::Point location = display::Screen::GetScreen()->GetCursorScreenPoint();
  // TODO(varunjain): Properly determine and send DRAG_EVENT_SOURCE below.
  aura::client::GetDragDropClient(root_window)->StartDragAndDrop(
      data,
      root_window,
      view,
      location,
      ui::DragDropTypes::DRAG_COPY | ui::DragDropTypes::DRAG_LINK,
      ui::DragDropTypes::DRAG_EVENT_SOURCE_MOUSE);
}

}  // namespace atom
