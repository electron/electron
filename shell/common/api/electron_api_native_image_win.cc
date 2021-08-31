// Copyright (c) 2020 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/api/electron_api_native_image.h"

#include <windows.h>

#include <thumbcache.h>
#include <wrl/client.h>

#include "base/win/scoped_com_initializer.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/skia_util.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

namespace api {

// static
v8::Local<v8::Promise> NativeImage::CreateThumbnailFromPath(
    v8::Isolate* isolate,
    const base::FilePath& path,
    const gfx::Size& size) {
  base::win::ScopedCOMInitializer scoped_com_initializer;

  gin_helper::Promise<gfx::Image> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  HRESULT hr;

  if (size.IsEmpty()) {
    promise.RejectWithErrorMessage("size must not be empty");
    return handle;
  }

  // create an IShellItem
  Microsoft::WRL::ComPtr<IShellItem> pItem;
  std::wstring image_path = path.value();
  hr = SHCreateItemFromParsingName(image_path.c_str(), nullptr,
                                   IID_PPV_ARGS(&pItem));

  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to create IShellItem from the given path");
    return handle;
  }

  // Init thumbnail cache
  Microsoft::WRL::ComPtr<IThumbnailCache> pThumbnailCache;
  hr = CoCreateInstance(CLSID_LocalThumbnailCache, nullptr, CLSCTX_INPROC,
                        IID_PPV_ARGS(&pThumbnailCache));
  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to acquire local thumbnail cache reference");
    return handle;
  }

  // Populate the IShellBitmap
  Microsoft::WRL::ComPtr<ISharedBitmap> pThumbnail;
  WTS_CACHEFLAGS flags;
  WTS_THUMBNAILID thumbId;
  hr = pThumbnailCache->GetThumbnail(pItem.Get(), size.width(),
                                     WTS_FLAGS::WTS_NONE, &pThumbnail, &flags,
                                     &thumbId);

  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to get thumbnail from local thumbnail cache reference");
    return handle;
  }

  // Init HBITMAP
  HBITMAP hBitmap = NULL;
  hr = pThumbnail->GetSharedBitmap(&hBitmap);
  if (FAILED(hr)) {
    promise.RejectWithErrorMessage("failed to extract bitmap from thumbnail");
    return handle;
  }

  // convert HBITMAP to gfx::Image
  BITMAP bitmap;
  if (!GetObject(hBitmap, sizeof(bitmap), &bitmap)) {
    promise.RejectWithErrorMessage("could not convert HBITMAP to BITMAP");
    return handle;
  }

  ICONINFO icon_info;
  icon_info.fIcon = TRUE;
  icon_info.hbmMask = hBitmap;
  icon_info.hbmColor = hBitmap;

  base::win::ScopedHICON icon(CreateIconIndirect(&icon_info));
  SkBitmap skbitmap = IconUtil::CreateSkBitmapFromHICON(icon.get());
  gfx::ImageSkia image_skia =
      gfx::ImageSkia::CreateFromBitmap(skbitmap, 1.0 /*scale factor*/);
  gfx::Image gfx_image = gfx::Image(image_skia);
  promise.Resolve(gfx_image);
  return handle;
}

}  // namespace api

}  // namespace electron
