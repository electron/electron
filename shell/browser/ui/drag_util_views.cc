// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/drag_util.h"

#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/window.h"
#include "ui/base/clipboard/file_info.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom-shared.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/views/button_drag_utils.h"
#include "ui/views/widget/widget.h"
#include "url/gurl.h"

namespace electron {

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view) {
  // Set up our OLE machinery
  auto data = std::make_unique<ui::OSExchangeData>();

  button_drag_utils::SetDragImage(GURL(), files[0].LossyDisplayName(),
                                  icon.AsImageSkia(), nullptr, data.get());

  std::vector<ui::FileInfo> file_infos;
  file_infos.reserve(files.size());
  for (const base::FilePath& file : files) {
    file_infos.emplace_back(file, base::FilePath());
  }
  data->SetFilenames(file_infos);

  aura::Window* root_window = view->GetRootWindow();
  if (!root_window || !aura::client::GetDragDropClient(root_window))
    return;

  gfx::Point location = display::Screen::GetScreen()->GetCursorScreenPoint();
  // TODO(varunjain): Properly determine and send DragEventSource below.
  aura::client::GetDragDropClient(root_window)
      ->StartDragAndDrop(
          std::move(data), root_window, view, location,
          ui::DragDropTypes::DRAG_COPY | ui::DragDropTypes::DRAG_LINK,
          ui::mojom::DragEventSource::kMouse);
}

}  // namespace electron
