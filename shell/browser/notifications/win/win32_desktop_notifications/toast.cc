// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "shell/browser/notifications/win/win32_desktop_notifications/toast.h"

#include <combaseapi.h>

#include <UIAutomation.h>
#include <uxtheme.h>
#include <windowsx.h>
#include <algorithm>
#include <cmath>
#include <memory>

#include "base/logging.h"
#include "base/strings/string_util_win.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/common.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/toast_uia.h"

#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "uxtheme.lib")

using std::min;
using std::shared_ptr;

namespace electron {

static COLORREF GetAccentColor() {
  bool success = false;
  if (IsAppThemed()) {
    HKEY hkey;
    if (RegOpenKeyEx(HKEY_CURRENT_USER,
                     TEXT("SOFTWARE\\Microsoft\\Windows\\DWM"), 0,
                     KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS) {
      COLORREF color;
      DWORD type, size;
      if (RegQueryValueEx(hkey, TEXT("AccentColor"), nullptr, &type,
                          reinterpret_cast<BYTE*>(&color),
                          &(size = sizeof(color))) == ERROR_SUCCESS &&
          type == REG_DWORD) {
        // convert from RGBA
        color = RGB(GetRValue(color), GetGValue(color), GetBValue(color));
        success = true;
      } else if (RegQueryValueEx(hkey, TEXT("ColorizationColor"), nullptr,
                                 &type, reinterpret_cast<BYTE*>(&color),
                                 &(size = sizeof(color))) == ERROR_SUCCESS &&
                 type == REG_DWORD) {
        // convert from BGRA
        color = RGB(GetBValue(color), GetGValue(color), GetRValue(color));
        success = true;
      }

      RegCloseKey(hkey);

      if (success)
        return color;
    }
  }

  return GetSysColor(COLOR_ACTIVECAPTION);
}

// Stretches a bitmap to the specified size, preserves alpha channel
static HBITMAP StretchBitmap(HBITMAP bitmap, unsigned width, unsigned height) {
  // We use StretchBlt for the scaling, but that discards the alpha channel.
  // So we first create a separate grayscale bitmap from the alpha channel,
  // scale that separately, and copy it back to the scaled color bitmap.

  BITMAP bm;
  if (!GetObject(bitmap, sizeof(bm), &bm))
    return NULL;

  if (width == 0 || height == 0)
    return NULL;

  HBITMAP result_bitmap = NULL;

  HDC hdc_screen = GetDC(NULL);

  HBITMAP alpha_src_bitmap;
  {
    BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
    bmi.biWidth = bm.bmWidth;
    bmi.biHeight = bm.bmHeight;
    bmi.biPlanes = bm.bmPlanes;
    bmi.biBitCount = bm.bmBitsPixel;
    bmi.biCompression = BI_RGB;

    void* alpha_src_bits;
    alpha_src_bitmap =
        CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&bmi),
                         DIB_RGB_COLORS, &alpha_src_bits, NULL, 0);

    if (alpha_src_bitmap) {
      if (GetDIBits(hdc_screen, bitmap, 0, 0, 0,
                    reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS) &&
          bmi.biSizeImage > 0 && (bmi.biSizeImage % 4) == 0) {
        auto* buf = reinterpret_cast<BYTE*>(
            _aligned_malloc(bmi.biSizeImage, sizeof(DWORD)));

        if (buf) {
          GetDIBits(hdc_screen, bitmap, 0, bm.bmHeight, buf,
                    reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS);

          const DWORD* src = reinterpret_cast<DWORD*>(buf);
          const DWORD* end = reinterpret_cast<DWORD*>(buf + bmi.biSizeImage);

          BYTE* dest = reinterpret_cast<BYTE*>(alpha_src_bits);

          for (; src != end; ++src, ++dest) {
            BYTE a = *src >> 24;
            *dest++ = a;
            *dest++ = a;
            *dest++ = a;
          }

          _aligned_free(buf);
        }
      }
    }
  }

  if (alpha_src_bitmap) {
    BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
    bmi.biWidth = width;
    bmi.biHeight = height;
    bmi.biPlanes = 1;
    bmi.biBitCount = 32;
    bmi.biCompression = BI_RGB;

    void* color_bits;
    auto* color_bitmap =
        CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&bmi),
                         DIB_RGB_COLORS, &color_bits, NULL, 0);

    void* alpha_bits;
    auto* alpha_bitmap =
        CreateDIBSection(NULL, reinterpret_cast<BITMAPINFO*>(&bmi),
                         DIB_RGB_COLORS, &alpha_bits, NULL, 0);

    HDC hdc = CreateCompatibleDC(NULL);
    HDC hdc_src = CreateCompatibleDC(NULL);

    if (color_bitmap && alpha_bitmap && hdc && hdc_src) {
      SetStretchBltMode(hdc, HALFTONE);

      // resize color channels
      SelectObject(hdc, color_bitmap);
      SelectObject(hdc_src, bitmap);
      StretchBlt(hdc, 0, 0, width, height, hdc_src, 0, 0, bm.bmWidth,
                 bm.bmHeight, SRCCOPY);

      // resize alpha channel
      SelectObject(hdc, alpha_bitmap);
      SelectObject(hdc_src, alpha_src_bitmap);
      StretchBlt(hdc, 0, 0, width, height, hdc_src, 0, 0, bm.bmWidth,
                 bm.bmHeight, SRCCOPY);

      // flush before touching the bits
      GdiFlush();

      // apply the alpha channel
      auto* dest = reinterpret_cast<BYTE*>(color_bits);
      auto* src = reinterpret_cast<const BYTE*>(alpha_bits);
      auto* end = src + (width * height * 4);
      while (src != end) {
        dest[3] = src[0];
        dest += 4;
        src += 4;
      }

      // create the resulting bitmap
      result_bitmap =
          CreateDIBitmap(hdc_screen, &bmi, CBM_INIT, color_bits,
                         reinterpret_cast<BITMAPINFO*>(&bmi), DIB_RGB_COLORS);
    }

    if (hdc_src)
      DeleteDC(hdc_src);
    if (hdc)
      DeleteDC(hdc);

    if (alpha_bitmap)
      DeleteObject(alpha_bitmap);
    if (color_bitmap)
      DeleteObject(color_bitmap);

    DeleteObject(alpha_src_bitmap);
  }

  ReleaseDC(NULL, hdc_screen);

  return result_bitmap;
}

const TCHAR DesktopNotificationController::Toast::class_name_[] =
    TEXT("DesktopNotificationToast");

DesktopNotificationController::Toast::Toast(HWND hwnd,
                                            shared_ptr<NotificationData>* data)
    : hwnd_(hwnd), data_(*data) {
  HDC hdc_screen = GetDC(NULL);
  hdc_ = CreateCompatibleDC(hdc_screen);
  ReleaseDC(NULL, hdc_screen);
}

DesktopNotificationController::Toast::~Toast() {
  if (uia_) {
    auto* UiaDisconnectProvider =
        reinterpret_cast<decltype(&::UiaDisconnectProvider)>(GetProcAddress(
            GetModuleHandle(L"uiautomationcore.dll"), "UiaDisconnectProvider"));
    // first detach from the toast, then call UiaDisconnectProvider;
    // UiaDisconnectProvider may call WM_GETOBJECT and we don't want
    // it to return the object that we're disconnecting
    uia_->DetachToast();

    if (UiaDisconnectProvider)
      UiaDisconnectProvider(uia_);

    uia_->Release();
    uia_ = nullptr;
  }

  DeleteDC(hdc_);
  if (bitmap_)
    DeleteBitmap(bitmap_);
  if (scaled_image_)
    DeleteBitmap(scaled_image_);
}

void DesktopNotificationController::Toast::Register(HINSTANCE hinstance) {
  WNDCLASSEX wc = {sizeof(wc)};
  wc.lpfnWndProc = &Toast::WndProc;
  wc.lpszClassName = class_name_;
  wc.cbWndExtra = sizeof(Toast*);
  wc.hInstance = hinstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);

  RegisterClassEx(&wc);
}

LRESULT DesktopNotificationController::Toast::WndProc(HWND hwnd,
                                                      UINT message,
                                                      WPARAM wparam,
                                                      LPARAM lparam) {
  switch (message) {
    case WM_CREATE: {
      auto*& cs = reinterpret_cast<const CREATESTRUCT*&>(lparam);
      auto* data =
          static_cast<shared_ptr<NotificationData>*>(cs->lpCreateParams);
      auto* inst = new Toast(hwnd, data);
      SetWindowLongPtr(hwnd, 0, (LONG_PTR)inst);
    } break;

    case WM_NCDESTROY:
      delete Get(hwnd);
      SetWindowLongPtr(hwnd, 0, 0);
      return 0;

    case WM_DESTROY:
      if (Get(hwnd)->uia_) {
        // free UI Automation resources associated with this window
        UiaReturnRawElementProvider(hwnd, 0, 0, nullptr);
      }
      break;

    case WM_MOUSEACTIVATE:
      return MA_NOACTIVATE;

    case WM_TIMER: {
      if (wparam == TimerID_AutoDismiss) {
        auto* inst = Get(hwnd);

        Notification notification(inst->data_);
        inst->data_->controller->OnNotificationDismissed(notification);

        inst->AutoDismiss();
      }
    }
      return 0;

    case WM_LBUTTONDOWN: {
      auto* inst = Get(hwnd);

      inst->Dismiss();

      Notification notification(inst->data_);
      if (inst->is_close_hot_)
        inst->data_->controller->OnNotificationDismissed(notification);
      else
        inst->data_->controller->OnNotificationClicked(notification);
    }
      return 0;

    case WM_MOUSEMOVE: {
      auto* inst = Get(hwnd);
      if (!inst->is_highlighted_) {
        inst->is_highlighted_ = true;

        TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd};
        TrackMouseEvent(&tme);
      }

      POINT cursor = {GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam)};
      inst->is_close_hot_ =
          (PtInRect(&inst->close_button_rect_, cursor) != FALSE);

      if (!inst->is_non_interactive_)
        inst->CancelDismiss();

      inst->UpdateContents();
    }
      return 0;

    case WM_MOUSELEAVE: {
      auto* inst = Get(hwnd);
      inst->is_highlighted_ = false;
      inst->is_close_hot_ = false;
      inst->UpdateContents();

      if (!inst->ease_out_active_ && inst->ease_in_pos_ == 1.0f)
        inst->ScheduleDismissal();

      // Make sure stack collapse happens if needed
      inst->data_->controller->StartAnimation();
    }
      return 0;

    case WM_WINDOWPOSCHANGED: {
      auto*& wp = reinterpret_cast<WINDOWPOS*&>(lparam);
      if (wp->flags & SWP_HIDEWINDOW) {
        if (!IsWindowVisible(hwnd))
          Get(hwnd)->is_highlighted_ = false;
      }
    } break;

    case WM_GETOBJECT:
      if (lparam == UiaRootObjectId) {
        auto* inst = Get(hwnd);
        if (!inst->uia_) {
          inst->uia_ = new UIAutomationInterface(inst);
          inst->uia_->AddRef();
        }
        // don't return the interface if it's being disconnected
        if (!inst->uia_->IsDetached()) {
          return UiaReturnRawElementProvider(hwnd, wparam, lparam, inst->uia_);
        }
      }
      break;
  }

  return DefWindowProc(hwnd, message, wparam, lparam);
}

HWND DesktopNotificationController::Toast::Create(
    HINSTANCE hinstance,
    shared_ptr<NotificationData> data) {
  return CreateWindowEx(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST,
                        class_name_, nullptr, WS_POPUP, 0, 0, 0, 0, NULL, NULL,
                        hinstance, &data);
}

void DesktopNotificationController::Toast::Draw() {
  const COLORREF accent = GetAccentColor();

  COLORREF back_color;
  {
    // base background color is 2/3 of accent
    // highlighted adds a bit of intensity to every channel

    int h = is_highlighted_ ? (0xff / 20) : 0;

    back_color = RGB(min(0xff, (GetRValue(accent) * 2 / 3) + h),
                     min(0xff, (GetGValue(accent) * 2 / 3) + h),
                     min(0xff, (GetBValue(accent) * 2 / 3) + h));
  }

  const float back_luma = (GetRValue(back_color) * 0.299f / 255) +
                          (GetGValue(back_color) * 0.587f / 255) +
                          (GetBValue(back_color) * 0.114f / 255);

  const struct {
    float r, g, b;
  } back_f = {
      GetRValue(back_color) / 255.0f,
      GetGValue(back_color) / 255.0f,
      GetBValue(back_color) / 255.0f,
  };

  COLORREF fore_color, dimmed_color;
  {
    // based on the lightness of background, we draw foreground in light
    // or dark shades of gray blended onto the background with slight
    // transparency to avoid sharp contrast

    constexpr float alpha = 0.9f;
    constexpr float intensity_light[] = {(1.0f * alpha), (0.8f * alpha)};
    constexpr float intensity_dark[] = {(0.1f * alpha), (0.3f * alpha)};

    // select foreground intensity values (light or dark)
    auto& i = (back_luma < 0.6f) ? intensity_light : intensity_dark;

    float r, g, b;

    r = i[0] + back_f.r * (1 - alpha);
    g = i[0] + back_f.g * (1 - alpha);
    b = i[0] + back_f.b * (1 - alpha);
    fore_color = RGB(r * 0xff, g * 0xff, b * 0xff);

    r = i[1] + back_f.r * (1 - alpha);
    g = i[1] + back_f.g * (1 - alpha);
    b = i[1] + back_f.b * (1 - alpha);
    dimmed_color = RGB(r * 0xff, g * 0xff, b * 0xff);
  }

  // Draw background
  {
    auto* brush = CreateSolidBrush(back_color);

    RECT rc = {0, 0, toast_size_.cx, toast_size_.cy};
    FillRect(hdc_, &rc, brush);

    DeleteBrush(brush);
  }

  SetBkMode(hdc_, TRANSPARENT);

  const auto close = L'\x2715';
  auto* caption_font = data_->controller->GetCaptionFont();
  auto* body_font = data_->controller->GetBodyFont();

  TEXTMETRIC tm_cap;
  SelectFont(hdc_, caption_font);
  GetTextMetrics(hdc_, &tm_cap);

  auto text_offset_x = margin_.cx;

  BITMAP image_info = {};
  if (scaled_image_) {
    GetObject(scaled_image_, sizeof(image_info), &image_info);

    text_offset_x += margin_.cx + image_info.bmWidth;
  }

  // calculate close button rect
  POINT close_pos;
  {
    SIZE extent = {};
    GetTextExtentPoint32W(hdc_, &close, 1, &extent);

    close_button_rect_.right = toast_size_.cx;
    close_button_rect_.top = 0;

    close_pos.x = close_button_rect_.right - margin_.cy - extent.cx;
    close_pos.y = close_button_rect_.top + margin_.cy;

    close_button_rect_.left = close_pos.x - margin_.cy;
    close_button_rect_.bottom = close_pos.y + extent.cy + margin_.cy;
  }

  // image
  if (scaled_image_) {
    HDC hdc_image = CreateCompatibleDC(NULL);
    SelectBitmap(hdc_image, scaled_image_);
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    AlphaBlend(hdc_, margin_.cx, margin_.cy, image_info.bmWidth,
               image_info.bmHeight, hdc_image, 0, 0, image_info.bmWidth,
               image_info.bmHeight, blend);
    DeleteDC(hdc_image);
  }

  // caption
  {
    RECT rc = {text_offset_x, margin_.cy, close_button_rect_.left,
               toast_size_.cy};

    SelectFont(hdc_, caption_font);
    SetTextColor(hdc_, fore_color);
    DrawText(hdc_, base::as_wcstr(data_->caption),
             (UINT)data_->caption.length(), &rc,
             DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
  }

  // body text
  if (!data_->body_text.empty()) {
    RECT rc = {text_offset_x, 2 * margin_.cy + tm_cap.tmAscent,
               toast_size_.cx - margin_.cx, toast_size_.cy - margin_.cy};

    SelectFont(hdc_, body_font);
    SetTextColor(hdc_, dimmed_color);
    DrawText(hdc_, base::as_wcstr(data_->body_text),
             (UINT)data_->body_text.length(), &rc,
             DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_END_ELLIPSIS |
                 DT_EDITCONTROL);
  }

  // close button
  {
    SelectFont(hdc_, caption_font);
    SetTextColor(hdc_, is_close_hot_ ? fore_color : dimmed_color);
    ExtTextOut(hdc_, close_pos.x, close_pos.y, 0, nullptr, &close, 1, nullptr);
  }

  is_content_updated_ = true;
}

void DesktopNotificationController::Toast::Invalidate() {
  is_content_updated_ = false;
}

bool DesktopNotificationController::Toast::IsRedrawNeeded() const {
  return !is_content_updated_;
}

void DesktopNotificationController::Toast::UpdateBufferSize() {
  if (hdc_) {
    SIZE new_size;
    {
      TEXTMETRIC tm_cap = {};
      HFONT font = data_->controller->GetCaptionFont();
      if (font) {
        SelectFont(hdc_, font);
        if (!GetTextMetrics(hdc_, &tm_cap))
          return;
      }

      TEXTMETRIC tm_body = {};
      font = data_->controller->GetBodyFont();
      if (font) {
        SelectFont(hdc_, font);
        if (!GetTextMetrics(hdc_, &tm_body))
          return;
      }

      this->margin_ = {tm_cap.tmAveCharWidth * 2, tm_cap.tmAscent / 2};

      new_size.cx = margin_.cx + (32 * tm_cap.tmAveCharWidth) + margin_.cx;
      new_size.cy = margin_.cy + (tm_cap.tmHeight) + margin_.cy;

      if (!data_->body_text.empty())
        new_size.cy += margin_.cy + (3 * tm_body.tmHeight);

      if (data_->image) {
        BITMAP bm;
        if (GetObject(data_->image, sizeof(bm), &bm)) {
          // cap the image size
          const int max_dim_size = 80;

          auto width = bm.bmWidth;
          auto height = bm.bmHeight;
          if (width < height) {
            if (height > max_dim_size) {
              width = width * max_dim_size / height;
              height = max_dim_size;
            }
          } else {
            if (width > max_dim_size) {
              height = height * max_dim_size / width;
              width = max_dim_size;
            }
          }

          ScreenMetrics scr;
          SIZE image_draw_size = {scr.X(width), scr.Y(height)};

          new_size.cx += image_draw_size.cx + margin_.cx;

          auto height_with_image =
              margin_.cy + (image_draw_size.cy) + margin_.cy;

          if (new_size.cy < height_with_image)
            new_size.cy = height_with_image;

          UpdateScaledImage(image_draw_size);
        }
      }
    }

    if (new_size.cx != this->toast_size_.cx ||
        new_size.cy != this->toast_size_.cy) {
      HDC hdc_screen = GetDC(NULL);
      auto* new_bitmap =
          CreateCompatibleBitmap(hdc_screen, new_size.cx, new_size.cy);
      ReleaseDC(NULL, hdc_screen);

      if (new_bitmap) {
        if (SelectBitmap(hdc_, new_bitmap)) {
          RECT dirty1 = {}, dirty2 = {};
          if (toast_size_.cx < new_size.cx) {
            dirty1 = {toast_size_.cx, 0, new_size.cx, toast_size_.cy};
          }
          if (toast_size_.cy < new_size.cy) {
            dirty2 = {0, toast_size_.cy, new_size.cx, new_size.cy};
          }

          if (this->bitmap_)
            DeleteBitmap(this->bitmap_);
          this->bitmap_ = new_bitmap;
          this->toast_size_ = new_size;

          Invalidate();

          // Resize also the DWM buffer to prevent flicker during
          // window resizing. Make sure any existing data is not
          // overwritten by marking the dirty region.
          {
            POINT origin = {0, 0};

            UPDATELAYEREDWINDOWINFO ulw;
            ulw.cbSize = sizeof(ulw);
            ulw.hdcDst = NULL;
            ulw.pptDst = nullptr;
            ulw.psize = &toast_size_;
            ulw.hdcSrc = hdc_;
            ulw.pptSrc = &origin;
            ulw.crKey = 0;
            ulw.pblend = nullptr;
            ulw.dwFlags = 0;
            ulw.prcDirty = &dirty1;
            auto b1 = UpdateLayeredWindowIndirect(hwnd_, &ulw);
            ulw.prcDirty = &dirty2;
            auto b2 = UpdateLayeredWindowIndirect(hwnd_, &ulw);
            DCHECK(b1 && b2);
          }

          return;
        }

        DeleteBitmap(new_bitmap);
      }
    }
  }
}

void DesktopNotificationController::Toast::UpdateScaledImage(const SIZE& size) {
  BITMAP bm;
  if (!GetObject(scaled_image_, sizeof(bm), &bm) || bm.bmWidth != size.cx ||
      bm.bmHeight != size.cy) {
    if (scaled_image_)
      DeleteBitmap(scaled_image_);
    scaled_image_ = StretchBitmap(data_->image, size.cx, size.cy);
  }
}

void DesktopNotificationController::Toast::UpdateContents() {
  Draw();

  if (IsWindowVisible(hwnd_)) {
    RECT rc;
    GetWindowRect(hwnd_, &rc);
    POINT origin = {0, 0};
    SIZE size = {rc.right - rc.left, rc.bottom - rc.top};
    UpdateLayeredWindow(hwnd_, NULL, nullptr, &size, hdc_, &origin, 0, nullptr,
                        0);
  }
}

void DesktopNotificationController::Toast::Dismiss() {
  if (!is_non_interactive_) {
    // Set a flag to prevent further interaction. We don't disable the HWND
    // because we still want to receive mouse move messages in order to keep
    // the toast under the cursor and not collapse it while dismissing.
    is_non_interactive_ = true;

    AutoDismiss();
  }
}

void DesktopNotificationController::Toast::AutoDismiss() {
  KillTimer(hwnd_, TimerID_AutoDismiss);
  StartEaseOut();
}

void DesktopNotificationController::Toast::CancelDismiss() {
  KillTimer(hwnd_, TimerID_AutoDismiss);
  ease_out_active_ = false;
  ease_out_pos_ = 0;
}

void DesktopNotificationController::Toast::ScheduleDismissal() {
  ULONG duration;
  if (!SystemParametersInfo(SPI_GETMESSAGEDURATION, 0, &duration, 0)) {
    duration = 5;
  }
  SetTimer(hwnd_, TimerID_AutoDismiss, duration * 1000, nullptr);
}

void DesktopNotificationController::Toast::ResetContents() {
  if (scaled_image_) {
    DeleteBitmap(scaled_image_);
    scaled_image_ = NULL;
  }

  Invalidate();
}

void DesktopNotificationController::Toast::PopUp(int y) {
  vertical_pos_target_ = vertical_pos_ = y;
  StartEaseIn();
}

void DesktopNotificationController::Toast::SetVerticalPosition(int y) {
  // Don't restart animation if current target is the same
  if (y == vertical_pos_target_)
    return;

  // Make sure the new animation's origin is at the current position
  vertical_pos_ += static_cast<int>((vertical_pos_target_ - vertical_pos_) *
                                    stack_collapse_pos_);

  // Set new target position and start the animation
  vertical_pos_target_ = y;
  stack_collapse_start_ = GetTickCount();
  data_->controller->StartAnimation();
}

HDWP DesktopNotificationController::Toast::Animate(HDWP hdwp,
                                                   const POINT& origin) {
  UpdateBufferSize();

  if (IsRedrawNeeded())
    Draw();

  POINT src_origin = {0, 0};

  UPDATELAYEREDWINDOWINFO ulw;
  ulw.cbSize = sizeof(ulw);
  ulw.hdcDst = NULL;
  ulw.pptDst = nullptr;
  ulw.psize = nullptr;
  ulw.hdcSrc = hdc_;
  ulw.pptSrc = &src_origin;
  ulw.crKey = 0;
  ulw.pblend = nullptr;
  ulw.dwFlags = 0;
  ulw.prcDirty = nullptr;

  POINT pt = {0, 0};
  SIZE size = {0, 0};
  BLENDFUNCTION blend;
  UINT dwpFlags =
      SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_NOREDRAW | SWP_NOCOPYBITS;

  auto ease_in_pos = AnimateEaseIn();
  auto ease_out_pos = AnimateEaseOut();
  auto stack_collapse_pos = AnimateStackCollapse();

  auto y_offset = (vertical_pos_target_ - vertical_pos_) * stack_collapse_pos;

  size.cx = static_cast<int>(toast_size_.cx * ease_in_pos);
  size.cy = toast_size_.cy;

  pt.x = origin.x - size.cx;
  pt.y = static_cast<int>(origin.y - vertical_pos_ - y_offset - size.cy);

  ulw.pptDst = &pt;
  ulw.psize = &size;

  if (ease_in_active_ && ease_in_pos == 1.0f) {
    ease_in_active_ = false;
    ScheduleDismissal();
  }

  this->ease_in_pos_ = ease_in_pos;
  this->stack_collapse_pos_ = stack_collapse_pos;

  if (ease_out_pos != this->ease_out_pos_) {
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = (BYTE)(255 * (1.0f - ease_out_pos));
    blend.AlphaFormat = 0;

    ulw.pblend = &blend;
    ulw.dwFlags = ULW_ALPHA;

    this->ease_out_pos_ = ease_out_pos;

    if (ease_out_pos == 1.0f) {
      ease_out_active_ = false;

      dwpFlags &= ~SWP_SHOWWINDOW;
      dwpFlags |= SWP_HIDEWINDOW;
    }
  }

  if (stack_collapse_pos == 1.0f) {
    vertical_pos_ = vertical_pos_target_;
  }

  // `UpdateLayeredWindowIndirect` updates position, size, and transparency.
  // `DeferWindowPos` updates z-order, and also position and size in case
  // ULWI fails, which can happen when one of the dimensions is zero (e.g.
  // at the beginning of ease-in).

  UpdateLayeredWindowIndirect(hwnd_, &ulw);
  hdwp = DeferWindowPos(hdwp, hwnd_, HWND_TOPMOST, pt.x, pt.y, size.cx, size.cy,
                        dwpFlags);
  return hdwp;
}

void DesktopNotificationController::Toast::StartEaseIn() {
  DCHECK(!ease_in_active_);
  ease_in_start_ = GetTickCount();
  ease_in_active_ = true;
  data_->controller->StartAnimation();
}

void DesktopNotificationController::Toast::StartEaseOut() {
  DCHECK(!ease_out_active_);
  ease_out_start_ = GetTickCount();
  ease_out_active_ = true;
  data_->controller->StartAnimation();
}

bool DesktopNotificationController::Toast::IsStackCollapseActive() const {
  return (vertical_pos_ != vertical_pos_target_);
}

float DesktopNotificationController::Toast::AnimateEaseIn() {
  if (!ease_in_active_)
    return ease_in_pos_;

  constexpr DWORD duration = 500;
  auto elapsed = GetTickCount() - ease_in_start_;
  float time = std::min(duration, elapsed) / static_cast<float>(duration);

  // decelerating exponential ease
  const float a = -8.0f;
  auto pos = (std::exp(a * time) - 1.0f) / (std::exp(a) - 1.0f);

  return pos;
}

float DesktopNotificationController::Toast::AnimateEaseOut() {
  if (!ease_out_active_)
    return ease_out_pos_;

  constexpr DWORD duration = 120;
  auto elapsed = GetTickCount() - ease_out_start_;
  float time = std::min(duration, elapsed) / static_cast<float>(duration);

  // accelerating circle ease
  auto pos = 1.0f - std::sqrt(1 - time * time);

  return pos;
}

float DesktopNotificationController::Toast::AnimateStackCollapse() {
  if (!IsStackCollapseActive())
    return stack_collapse_pos_;

  constexpr DWORD duration = 500;
  auto elapsed = GetTickCount() - stack_collapse_start_;
  float time = std::min(duration, elapsed) / static_cast<float>(duration);

  // decelerating exponential ease
  const float a = -8.0f;
  auto pos = (std::exp(a * time) - 1.0f) / (std::exp(a) - 1.0f);

  return pos;
}

}  // namespace electron
