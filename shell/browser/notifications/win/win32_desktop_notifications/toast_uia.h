// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_TOAST_UIA_H_
#define ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_TOAST_UIA_H_

#include "shell/browser/notifications/win/win32_desktop_notifications/toast.h"

#include <combaseapi.h>

#include <UIAutomationCore.h>

namespace electron {

class DesktopNotificationController::Toast::UIAutomationInterface
    : public IRawElementProviderSimple,
      public IWindowProvider,
      public IInvokeProvider,
      public ITextProvider {
 public:
  explicit UIAutomationInterface(Toast* toast);

  void DetachToast() { hwnd_ = NULL; }

  bool IsDetached() const { return !hwnd_; }

 private:
  virtual ~UIAutomationInterface() = default;

  // IUnknown
 public:
  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;
  STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv) override;

  // IRawElementProviderSimple
 public:
  STDMETHODIMP get_ProviderOptions(ProviderOptions* retval) override;
  STDMETHODIMP GetPatternProvider(PATTERNID pattern_id,
                                  IUnknown** retval) override;
  STDMETHODIMP GetPropertyValue(PROPERTYID property_id,
                                VARIANT* retval) override;
  STDMETHODIMP get_HostRawElementProvider(
      IRawElementProviderSimple** retval) override;

  // IWindowProvider
 public:
  STDMETHODIMP SetVisualState(WindowVisualState state) override;
  STDMETHODIMP Close() override;
  STDMETHODIMP WaitForInputIdle(int milliseconds, BOOL* retval) override;
  STDMETHODIMP get_CanMaximize(BOOL* retval) override;
  STDMETHODIMP get_CanMinimize(BOOL* retval) override;
  STDMETHODIMP get_IsModal(BOOL* retval) override;
  STDMETHODIMP get_WindowVisualState(WindowVisualState* retval) override;
  STDMETHODIMP get_WindowInteractionState(
      WindowInteractionState* retval) override;
  STDMETHODIMP get_IsTopmost(BOOL* retval) override;

  // IInvokeProvider
 public:
  STDMETHODIMP Invoke() override;

  // ITextProvider
 public:
  STDMETHODIMP GetSelection(SAFEARRAY** retval) override;
  STDMETHODIMP GetVisibleRanges(SAFEARRAY** retval) override;
  STDMETHODIMP RangeFromChild(IRawElementProviderSimple* child_element,
                              ITextRangeProvider** retval) override;
  STDMETHODIMP RangeFromPoint(UiaPoint point,
                              ITextRangeProvider** retval) override;
  STDMETHODIMP get_DocumentRange(ITextRangeProvider** retval) override;
  STDMETHODIMP get_SupportedTextSelection(
      SupportedTextSelection* retval) override;

 private:
  volatile LONG cref_ = 0;
  HWND hwnd_;
  std::u16string text_;
};

}  // namespace electron

#endif  // ELECTRON_SHELL_BROWSER_NOTIFICATIONS_WIN_WIN32_DESKTOP_NOTIFICATIONS_TOAST_UIA_H_
