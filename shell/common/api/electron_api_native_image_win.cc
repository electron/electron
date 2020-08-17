#include "shell/common/api/electron_api_native_image.h"

#include <windows.h>  // NOLINT(build/include_order)

#include <thumbcache.h>  // NOLINT(build/include_order)
#include <string>        // NOLINT(build/include_order)
#include <vector>        // NOLINT(build/include_order)

// #include "shell/common/gin_helper/dictionary.h"
#include "shell/common/skia_util.h"

namespace electron {

v8::Local<v8::Promise> NativeImage::CreateThumbnailFromPath(
    v8::Isolate* isolate,
    const base::FilePath& path,
    const gfx::Size& size) {
  gin_helper::Promise<gfx::Image> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  HRESULT hr;

  if (size.width() <= 0) {
    promise.RejectWithErrorMessage(
        "invalid width, please enter a positive number");
    return handle;
  }

  if (!path.IsAbsolute()) {
    promise.RejectWithErrorMessage("path must be absolute");
    return handle;
  }

  // create an IShellItem
  IShellItem* pItem = nullptr;
  std::wstring image_path = path.AsUTF16Unsafe();
  hr = SHCreateItemFromParsingName(image_path.c_str(), nullptr,
                                   IID_PPV_ARGS(&pItem));

  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to create IShellItem from the given path");
    return handle;
  }

  // Init thumbnail cache
  IThumbnailCache* pThumbnailCache = nullptr;
  hr = CoCreateInstance(CLSID_LocalThumbnailCache, nullptr, CLSCTX_INPROC,
                        IID_PPV_ARGS(&pThumbnailCache));
  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to acquire local thumbnail cache reference");
    pItem->Release();
    return handle;
  }

  // Populate the IShellBitmap
  ISharedBitmap* pThumbnail = nullptr;
  WTS_CACHEFLAGS flags;
  WTS_THUMBNAILID thumbId;
  hr = pThumbnailCache->GetThumbnail(pItem, size.width(), WTS_FLAGS::WTS_NONE,
                                     &pThumbnail, &flags, &thumbId);
  pItem->Release();

  if (FAILED(hr)) {
    promise.RejectWithErrorMessage(
        "failed to get thumbnail from local thumbnail cache reference");
    pThumbnailCache->Release();
    return handle;
  }

  // Init HBITMAP
  HBITMAP hBitmap = NULL;
  hr = pThumbnail->GetSharedBitmap(&hBitmap);
  if (FAILED(hr)) {
    promise.RejectWithErrorMessage("failed to extract bitmap from thumbnail");
    pThumbnailCache->Release();
    pThumbnail->Release();
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

  HICON icon(CreateIconIndirect(&icon_info));
  SkBitmap skbitmap = IconUtil::CreateSkBitmapFromHICON(icon);
  DestroyIcon(icon);
  gfx::ImageSkia image_skia;
  image_skia.AddRepresentation(
      gfx::ImageSkiaRep(skbitmap, 1.0 /*scale factor*/));
  gfx::Image gfx_image = gfx::Image(image_skia);
  promise.Resolve(gfx_image);
  return handle;
}

}  // namespace electron
