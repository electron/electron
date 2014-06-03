// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/status_icons/status_tray_state_changer_win.h"

namespace {

////////////////////////////////////////////////////////////////////////////////
// Status Tray API

// The folowing describes the interface to the undocumented Windows Exporer APIs
// for manipulating with the status tray area.  This code should be used with
// care as it can change with versions (even minor versions) of Windows.

// ITrayNotify is an interface describing the API for manipulating the state of
// the Windows notification area, as well as for registering for change
// notifications.
class __declspec(uuid("FB852B2C-6BAD-4605-9551-F15F87830935")) ITrayNotify
    : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE
      RegisterCallback(INotificationCB* callback) = 0;
  virtual HRESULT STDMETHODCALLTYPE
      SetPreference(const NOTIFYITEM* notify_item) = 0;
  virtual HRESULT STDMETHODCALLTYPE EnableAutoTray(BOOL enabled) = 0;
};

// ITrayNotifyWin8 is the interface that replaces ITrayNotify for newer versions
// of Windows.
class __declspec(uuid("D133CE13-3537-48BA-93A7-AFCD5D2053B4")) ITrayNotifyWin8
    : public IUnknown {
 public:
  virtual HRESULT STDMETHODCALLTYPE
      RegisterCallback(INotificationCB* callback, unsigned long*) = 0;
  virtual HRESULT STDMETHODCALLTYPE UnregisterCallback(unsigned long*) = 0;
  virtual HRESULT STDMETHODCALLTYPE SetPreference(NOTIFYITEM const*) = 0;
  virtual HRESULT STDMETHODCALLTYPE EnableAutoTray(BOOL) = 0;
  virtual HRESULT STDMETHODCALLTYPE DoAction(BOOL) = 0;
};

const CLSID CLSID_TrayNotify = {
    0x25DEAD04,
    0x1EAC,
    0x4911,
    {0x9E, 0x3A, 0xAD, 0x0A, 0x4A, 0xB5, 0x60, 0xFD}};

}  // namespace

StatusTrayStateChangerWin::StatusTrayStateChangerWin(UINT icon_id, HWND window)
    : interface_version_(INTERFACE_VERSION_UNKNOWN),
      icon_id_(icon_id),
      window_(window) {
  wchar_t module_name[MAX_PATH];
  ::GetModuleFileName(NULL, module_name, MAX_PATH);

  file_name_ = module_name;
}

void StatusTrayStateChangerWin::EnsureTrayIconVisible() {
  DCHECK(CalledOnValidThread());

  if (!CreateTrayNotify()) {
    VLOG(1) << "Unable to create COM object for ITrayNotify.";
    return;
  }

  scoped_ptr<NOTIFYITEM> notify_item = RegisterCallback();

  // If the user has already hidden us explicitly, try to honor their choice by
  // not changing anything.
  if (notify_item->preference == PREFERENCE_SHOW_NEVER)
    return;

  // If we are already on the taskbar, return since nothing needs to be done.
  if (notify_item->preference == PREFERENCE_SHOW_ALWAYS)
    return;

  notify_item->preference = PREFERENCE_SHOW_ALWAYS;

  SendNotifyItemUpdate(notify_item.Pass());
}

STDMETHODIMP_(ULONG) StatusTrayStateChangerWin::AddRef() {
  DCHECK(CalledOnValidThread());
  return base::win::IUnknownImpl::AddRef();
}

STDMETHODIMP_(ULONG) StatusTrayStateChangerWin::Release() {
  DCHECK(CalledOnValidThread());
  return base::win::IUnknownImpl::Release();
}

STDMETHODIMP StatusTrayStateChangerWin::QueryInterface(REFIID riid,
                                                       PVOID* ptr_void) {
  DCHECK(CalledOnValidThread());
  if (riid == __uuidof(INotificationCB)) {
    *ptr_void = static_cast<INotificationCB*>(this);
    AddRef();
    return S_OK;
  }

  return base::win::IUnknownImpl::QueryInterface(riid, ptr_void);
}

STDMETHODIMP StatusTrayStateChangerWin::Notify(ULONG event,
                                               NOTIFYITEM* notify_item) {
  DCHECK(CalledOnValidThread());
  DCHECK(notify_item);
  if (notify_item->hwnd != window_ || notify_item->id != icon_id_ ||
      base::string16(notify_item->exe_name) != file_name_) {
    return S_OK;
  }

  notify_item_.reset(new NOTIFYITEM(*notify_item));
  return S_OK;
}

StatusTrayStateChangerWin::~StatusTrayStateChangerWin() {
  DCHECK(CalledOnValidThread());
}

bool StatusTrayStateChangerWin::CreateTrayNotify() {
  DCHECK(CalledOnValidThread());

  tray_notify_.Release();  // Release so this method can be called more than
                           // once.

  HRESULT hr = tray_notify_.CreateInstance(CLSID_TrayNotify);
  if (FAILED(hr))
    return false;

  base::win::ScopedComPtr<ITrayNotifyWin8> tray_notify_win8;
  hr = tray_notify_win8.QueryFrom(tray_notify_);
  if (SUCCEEDED(hr)) {
    interface_version_ = INTERFACE_VERSION_WIN8;
    return true;
  }

  base::win::ScopedComPtr<ITrayNotify> tray_notify_legacy;
  hr = tray_notify_legacy.QueryFrom(tray_notify_);
  if (SUCCEEDED(hr)) {
    interface_version_ = INTERFACE_VERSION_LEGACY;
    return true;
  }

  return false;
}

scoped_ptr<NOTIFYITEM> StatusTrayStateChangerWin::RegisterCallback() {
  // |notify_item_| is used to store the result of the callback from
  // Explorer.exe, which happens synchronously during
  // RegisterCallbackWin8 or RegisterCallbackLegacy.
  DCHECK(notify_item_.get() == NULL);

  // TODO(dewittj): Add UMA logging here to report if either of our strategies
  // has a tendency to fail on particular versions of Windows.
  switch (interface_version_) {
    case INTERFACE_VERSION_WIN8:
      if (!RegisterCallbackWin8())
        VLOG(1) << "Unable to successfully run RegisterCallbackWin8.";
      break;
    case INTERFACE_VERSION_LEGACY:
      if (!RegisterCallbackLegacy())
        VLOG(1) << "Unable to successfully run RegisterCallbackLegacy.";
      break;
    default:
      NOTREACHED();
  }

  // Adding an intermediate scoped pointer here so that |notify_item_| is reset
  // to NULL.
  scoped_ptr<NOTIFYITEM> rv(notify_item_.release());
  return rv.Pass();
}

bool StatusTrayStateChangerWin::RegisterCallbackWin8() {
  base::win::ScopedComPtr<ITrayNotifyWin8> tray_notify_win8;
  HRESULT hr = tray_notify_win8.QueryFrom(tray_notify_);
  if (FAILED(hr))
    return false;

  // The following two lines cause Windows Explorer to call us back with all the
  // existing tray icons and their preference.  It would also presumably notify
  // us if changes were made in realtime while we registered as a callback, but
  // we just want to modify our own entry so we immediately unregister.
  unsigned long callback_id = 0;
  hr = tray_notify_win8->RegisterCallback(this, &callback_id);
  tray_notify_win8->UnregisterCallback(&callback_id);
  if (FAILED(hr)) {
    return false;
  }

  return true;
}

bool StatusTrayStateChangerWin::RegisterCallbackLegacy() {
  base::win::ScopedComPtr<ITrayNotify> tray_notify;
  HRESULT hr = tray_notify.QueryFrom(tray_notify_);
  if (FAILED(hr)) {
    return false;
  }

  // The following two lines cause Windows Explorer to call us back with all the
  // existing tray icons and their preference.  It would also presumably notify
  // us if changes were made in realtime while we registered as a callback.  In
  // this version of the API, there can be only one registered callback so it is
  // better to unregister as soon as possible.
  // TODO(dewittj): Try to notice if the notification area icon customization
  // window is open and postpone this call until the user closes it;
  // registering the callback while the window is open can cause stale data to
  // be displayed to the user.
  hr = tray_notify->RegisterCallback(this);
  tray_notify->RegisterCallback(NULL);
  if (FAILED(hr)) {
    return false;
  }

  return true;
}

void StatusTrayStateChangerWin::SendNotifyItemUpdate(
    scoped_ptr<NOTIFYITEM> notify_item) {
  if (interface_version_ == INTERFACE_VERSION_LEGACY) {
    base::win::ScopedComPtr<ITrayNotify> tray_notify;
    HRESULT hr = tray_notify.QueryFrom(tray_notify_);
    if (SUCCEEDED(hr))
      tray_notify->SetPreference(notify_item.get());
  } else if (interface_version_ == INTERFACE_VERSION_WIN8) {
    base::win::ScopedComPtr<ITrayNotifyWin8> tray_notify;
    HRESULT hr = tray_notify.QueryFrom(tray_notify_);
    if (SUCCEEDED(hr))
      tray_notify->SetPreference(notify_item.get());
  }
}

