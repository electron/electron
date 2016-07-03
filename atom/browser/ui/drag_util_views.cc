// Copyright (c) 2016 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/drag_util.h"

#if defined(OS_WIN)
#include <objidl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <stddef.h>
#endif

#include "ui/aura/window.h"
#include "ui/base/dragdrop/drag_drop_types.h"
#include "ui/base/dragdrop/drag_utils.h"
#include "ui/base/dragdrop/file_info.h"
#include "ui/base/dragdrop/os_exchange_data.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/screen.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/public/drag_drop_client.h"

#if defined(OS_WIN)
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_hdc.h"
#include "ui/base/dragdrop/os_exchange_data_provider_win.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/gdi_util.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/skbitmap_operations.h"
#endif

namespace atom {

namespace {

#if defined(OS_WIN)
void SetDragImageOnDataObject(HBITMAP hbitmap,
                              const gfx::Size& size_in_pixels,
                              const gfx::Vector2d& cursor_offset,
                              IDataObject* data_object) {
  base::win::ScopedComPtr<IDragSourceHelper> helper;
  HRESULT rv = CoCreateInstance(CLSID_DragDropHelper, 0, CLSCTX_INPROC_SERVER,
                                IID_IDragSourceHelper, helper.ReceiveVoid());
  if (SUCCEEDED(rv)) {
    SHDRAGIMAGE sdi;
    sdi.sizeDragImage = size_in_pixels.ToSIZE();
    sdi.crColorKey = 0xFFFFFFFF;
    sdi.hbmpDragImage = hbitmap;
    sdi.ptOffset = gfx::PointAtOffsetFromOrigin(cursor_offset).ToPOINT();
    helper->InitializeFromBitmap(&sdi, data_object);
  }
}

// Blit the contents of the canvas to a new HBITMAP. It is the caller's
// responsibility to release the |bits| buffer.
HBITMAP CreateHBITMAPFromSkBitmap(const SkBitmap& sk_bitmap) {
  base::win::ScopedGetDC screen_dc(NULL);
  BITMAPINFOHEADER header;
  gfx::CreateBitmapHeader(sk_bitmap.width(), sk_bitmap.height(), &header);
  void* bits;
  HBITMAP bitmap =
      CreateDIBSection(screen_dc, reinterpret_cast<BITMAPINFO*>(&header),
                       DIB_RGB_COLORS, &bits, NULL, 0);
  if (!bitmap || !bits)
    return NULL;
  DCHECK_EQ(sk_bitmap.rowBytes(), static_cast<size_t>(sk_bitmap.width() * 4));
  SkAutoLockPixels lock(sk_bitmap);
  memcpy(
      bits, sk_bitmap.getPixels(), sk_bitmap.height() * sk_bitmap.rowBytes());
  return bitmap;
}

void SetDragImageOnDataObject(const gfx::ImageSkia& image_skia,
                              const gfx::Vector2d& cursor_offset,
                              ui::OSExchangeData* data_object) {
  DCHECK(data_object && !image_skia.size().IsEmpty());
  // InitializeFromBitmap() doesn't expect an alpha channel and is confused
  // by premultiplied colors, so unpremultiply the bitmap.
  // SetDragImageOnDataObject(HBITMAP) takes ownership of the bitmap.
  HBITMAP bitmap = CreateHBITMAPFromSkBitmap(
      SkBitmapOperations::UnPreMultiply(*image_skia.bitmap()));
  if (bitmap) {
    // Attach 'bitmap' to the data_object.
    SetDragImageOnDataObject(
        bitmap,
        gfx::Size(image_skia.bitmap()->width(), image_skia.bitmap()->height()),
        cursor_offset,
        ui::OSExchangeDataProviderWin::GetIDataObject(*data_object));
  }
}
#endif

}  // namespace

void DragFileItems(const std::vector<base::FilePath>& files,
                   const gfx::Image& icon,
                   gfx::NativeView view) {
  // Set up our OLE machinery
  ui::OSExchangeData data;

#if defined(OS_WIN)
  SetDragImageOnDataObject(icon.AsImageSkia(), gfx::Vector2d(), &data);
#endif
  data.provider().SetDragImage(icon.AsImageSkia(), gfx::Vector2d());

  std::vector<ui::FileInfo> file_infos;
  for (const base::FilePath& file : files) {
    file_infos.push_back(ui::FileInfo(file, base::FilePath()));
  }
  data.SetFilenames(file_infos);

  aura::Window* root_window = view->GetRootWindow();
  if (!root_window || !aura::client::GetDragDropClient(root_window))
    return;

  gfx::Point location = gfx::Screen::GetScreen()->GetCursorScreenPoint();
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
