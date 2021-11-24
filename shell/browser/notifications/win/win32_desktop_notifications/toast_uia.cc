// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/win/win32_desktop_notifications/toast_uia.h"

#include <UIAutomation.h>

#include "base/check_op.h"
#include "base/strings/string_util_win.h"
#include "shell/browser/notifications/win/win32_desktop_notifications/common.h"

#pragma comment(lib, "uiautomationcore.lib")

namespace electron {

DesktopNotificationController::Toast::UIAutomationInterface::
    UIAutomationInterface(Toast* toast)
    : hwnd_(toast->hwnd_) {
  text_ = toast->data_->caption;
  if (!toast->data_->body_text.empty()) {
    if (!text_.empty())
      text_.append(u", ");
    text_.append(toast->data_->body_text);
  }
}

ULONG DesktopNotificationController::Toast::UIAutomationInterface::AddRef() {
  return InterlockedIncrement(&cref_);
}

ULONG DesktopNotificationController::Toast::UIAutomationInterface::Release() {
  LONG ret = InterlockedDecrement(&cref_);
  if (ret == 0) {
    delete this;
    return 0;
  }
  DCHECK_GT(ret, 0);
  return ret;
}

STDMETHODIMP
DesktopNotificationController::Toast::UIAutomationInterface::QueryInterface(
    REFIID riid,
    LPVOID* ppv) {
  if (!ppv)
    return E_INVALIDARG;

  if (riid == IID_IUnknown) {
    *ppv =
        static_cast<IUnknown*>(static_cast<IRawElementProviderSimple*>(this));
  } else if (riid == __uuidof(IRawElementProviderSimple)) {
    *ppv = static_cast<IRawElementProviderSimple*>(this);
  } else if (riid == __uuidof(IWindowProvider)) {
    *ppv = static_cast<IWindowProvider*>(this);
  } else if (riid == __uuidof(IInvokeProvider)) {
    *ppv = static_cast<IInvokeProvider*>(this);
  } else if (riid == __uuidof(ITextProvider)) {
    *ppv = static_cast<ITextProvider*>(this);
  } else {
    *ppv = nullptr;
    return E_NOINTERFACE;
  }

  this->AddRef();
  return S_OK;
}

HRESULT DesktopNotificationController::Toast::UIAutomationInterface::
    get_ProviderOptions(ProviderOptions* retval) {
  *retval = ProviderOptions_ServerSideProvider;
  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::GetPatternProvider(
    PATTERNID pattern_id,
    IUnknown** retval) {
  switch (pattern_id) {
    case UIA_WindowPatternId:
      *retval = static_cast<IWindowProvider*>(this);
      break;
    case UIA_InvokePatternId:
      *retval = static_cast<IInvokeProvider*>(this);
      break;
    case UIA_TextPatternId:
      *retval = static_cast<ITextProvider*>(this);
      break;
    default:
      *retval = nullptr;
      return S_OK;
  }
  this->AddRef();
  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::GetPropertyValue(
    PROPERTYID property_id,
    VARIANT* retval) {
  // Note: In order to have the toast read by the NVDA screen reader, we
  // pretend that we're a Windows 8 native toast notification by reporting
  // these property values:
  //    ClassName: ToastContentHost
  //    ControlType: UIA_ToolTipControlTypeId

  retval->vt = VT_EMPTY;
  switch (property_id) {
    case UIA_NamePropertyId:
      retval->vt = VT_BSTR;
      retval->bstrVal = SysAllocString(base::as_wcstr(text_));
      break;

    case UIA_ClassNamePropertyId:
      retval->vt = VT_BSTR;
      retval->bstrVal = SysAllocString(L"ToastContentHost");
      break;

    case UIA_ControlTypePropertyId:
      retval->vt = VT_I4;
      retval->lVal = UIA_ToolTipControlTypeId;
      break;

    case UIA_LiveSettingPropertyId:
      retval->vt = VT_I4;
      retval->lVal = Assertive;
      break;

    case UIA_IsContentElementPropertyId:
    case UIA_IsControlElementPropertyId:
    case UIA_IsPeripheralPropertyId:
      retval->vt = VT_BOOL;
      retval->lVal = VARIANT_TRUE;
      break;

    case UIA_HasKeyboardFocusPropertyId:
    case UIA_IsKeyboardFocusablePropertyId:
    case UIA_IsOffscreenPropertyId:
      retval->vt = VT_BOOL;
      retval->lVal = VARIANT_FALSE;
      break;
  }
  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::
    get_HostRawElementProvider(IRawElementProviderSimple** retval) {
  if (!hwnd_)
    return E_FAIL;
  return UiaHostProviderFromHwnd(hwnd_, retval);
}

HRESULT DesktopNotificationController::Toast::UIAutomationInterface::Invoke() {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::SetVisualState(
    WindowVisualState state) {
  // setting the visual state is not supported
  return E_FAIL;
}

HRESULT DesktopNotificationController::Toast::UIAutomationInterface::Close() {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::WaitForInputIdle(
    int milliseconds,
    BOOL* retval) {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::get_CanMaximize(
    BOOL* retval) {
  *retval = FALSE;
  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::get_CanMinimize(
    BOOL* retval) {
  *retval = FALSE;
  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::get_IsModal(
    BOOL* retval) {
  *retval = FALSE;
  return S_OK;
}

HRESULT DesktopNotificationController::Toast::UIAutomationInterface::
    get_WindowVisualState(WindowVisualState* retval) {
  *retval = WindowVisualState_Normal;
  return S_OK;
}

HRESULT DesktopNotificationController::Toast::UIAutomationInterface::
    get_WindowInteractionState(WindowInteractionState* retval) {
  if (!hwnd_)
    *retval = WindowInteractionState_Closing;
  else
    *retval = WindowInteractionState_ReadyForUserInteraction;

  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::get_IsTopmost(
    BOOL* retval) {
  *retval = TRUE;
  return S_OK;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::GetSelection(
    SAFEARRAY** retval) {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::GetVisibleRanges(
    SAFEARRAY** retval) {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::RangeFromChild(
    IRawElementProviderSimple* child_element,
    ITextRangeProvider** retval) {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::RangeFromPoint(
    UiaPoint point,
    ITextRangeProvider** retval) {
  return E_NOTIMPL;
}

HRESULT
DesktopNotificationController::Toast::UIAutomationInterface::get_DocumentRange(
    ITextRangeProvider** retval) {
  return E_NOTIMPL;
}

HRESULT DesktopNotificationController::Toast::UIAutomationInterface::
    get_SupportedTextSelection(SupportedTextSelection* retval) {
  *retval = SupportedTextSelection_None;
  return S_OK;
}

}  // namespace electron
