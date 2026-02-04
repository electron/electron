// Copyright (c) 2025 Microsoft, GmbH
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/notifications/win/windows_toast_activator.h"

#include <propkey.h>
#include <propvarutil.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <wrl/module.h>
#ifdef StrCat
// Undefine Windows shlwapi.h StrCat macro to avoid conflict with
// base/strings/strcat.h which deliberately defines a StrCat macro sentinel.
#undef StrCat
#endif

#include <string>
#include <vector>

#include <algorithm>
#include <atomic>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions_win.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/registry.h"
#include "base/win/scoped_propvariant.h"
#include "base/win/win_util.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "shell/browser/api/electron_api_notification.h"  // nogncheck - for delegate events
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/notifications/notification.h"
#include "shell/browser/notifications/notification_delegate.h"
#include "shell/browser/notifications/notification_presenter.h"
#include "shell/browser/notifications/win/notification_presenter_win.h"
#include "shell/common/application_info.h"

namespace electron {

namespace {

class NotificationActivatorFactory final : public IClassFactory {
 public:
  NotificationActivatorFactory() : ref_count_(1) {}
  ~NotificationActivatorFactory() = default;
  IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
    if (!ppv)
      return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IClassFactory) {
      *ppv = static_cast<IClassFactory*>(this);
    } else {
      *ppv = nullptr;
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }
  IFACEMETHODIMP_(ULONG) AddRef() override { return ++ref_count_; }
  IFACEMETHODIMP_(ULONG) Release() override {
    ULONG res = --ref_count_;
    if (res == 0)
      delete this;
    return res;
  }

  IFACEMETHODIMP CreateInstance(IUnknown* outer,
                                REFIID riid,
                                void** ppv) override {
    if (outer)
      return CLASS_E_NOAGGREGATION;
    auto activator = Microsoft::WRL::Make<NotificationActivator>();
    return activator.CopyTo(riid, ppv);
  }
  IFACEMETHODIMP LockServer(BOOL) override { return S_OK; }

 private:
  std::atomic<ULONG> ref_count_;
};

NotificationActivatorFactory* g_factory_instance = nullptr;
bool g_registration_in_progress = false;

std::wstring GetExecutablePath() {
  wchar_t path[MAX_PATH];
  DWORD len = ::GetModuleFileNameW(nullptr, path, std::size(path));
  if (len == 0 || len == std::size(path))
    return L"";
  return std::wstring(path, len);
}

void EnsureCLSIDRegistry() {
  std::wstring exe = GetExecutablePath();
  if (exe.empty())
    return;
  std::wstring clsid = GetAppToastActivatorCLSID();
  std::wstring key_path = L"Software\\Classes\\CLSID\\" + clsid;
  base::win::RegKey key(HKEY_CURRENT_USER, key_path.c_str(), KEY_SET_VALUE);
  if (!key.Valid())
    return;
  key.WriteValue(nullptr, L"Electron Notification Activator");
  key.WriteValue(L"CustomActivator", 1U);

  std::wstring local_server = key_path + L"\\LocalServer32";
  base::win::RegKey server_key(HKEY_CURRENT_USER, local_server.c_str(),
                               KEY_SET_VALUE);
  if (!server_key.Valid())
    return;
  server_key.WriteValue(nullptr, exe.c_str());
}

bool ExistingShortcutValid(const base::FilePath& lnk_path, PCWSTR aumid) {
  if (!base::PathExists(lnk_path))
    return false;
  Microsoft::WRL::ComPtr<IShellLink> existing;
  if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&existing))))
    return false;
  Microsoft::WRL::ComPtr<IPersistFile> pf;
  if (FAILED(existing.As(&pf)) ||
      FAILED(pf->Load(lnk_path.value().c_str(), STGM_READ))) {
    return false;
  }
  Microsoft::WRL::ComPtr<IPropertyStore> store;
  if (FAILED(existing.As(&store)))
    return false;
  base::win::ScopedPropVariant pv_id;
  base::win::ScopedPropVariant pv_clsid;
  if (FAILED(store->GetValue(PKEY_AppUserModel_ID, pv_id.Receive())) ||
      FAILED(store->GetValue(PKEY_AppUserModel_ToastActivatorCLSID,
                             pv_clsid.Receive()))) {
    return false;
  }
  if (pv_id.get().vt == VT_LPWSTR && pv_clsid.get().vt == VT_CLSID &&
      pv_id.get().pwszVal && pv_clsid.get().puuid) {
    CLSID desired;
    if (SUCCEEDED(::CLSIDFromString(GetAppToastActivatorCLSID(), &desired))) {
      return _wcsicmp(pv_id.get().pwszVal, aumid) == 0 &&
             IsEqualGUID(*pv_clsid.get().puuid, desired);
    }
  }
  return false;
}

void EnsureShortcut() {
  PCWSTR aumid = GetRawAppUserModelID();
  if (!aumid || !*aumid)
    return;

  std::wstring exe = GetExecutablePath();
  if (exe.empty())
    return;

  PWSTR programs_path = nullptr;
  if (FAILED(
          SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programs_path)))
    return;
  base::FilePath programs(programs_path);
  CoTaskMemFree(programs_path);

  std::wstring product_name = base::UTF8ToWide(GetApplicationName());
  if (product_name.empty())
    product_name = L"ElectronApp";
  base::CreateDirectory(programs);
  base::FilePath lnk_path = programs.Append(product_name + L".lnk");
  if (base::PathExists(lnk_path)) {
    Microsoft::WRL::ComPtr<IShellLink> existing;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr,
                                   CLSCTX_INPROC_SERVER,
                                   IID_PPV_ARGS(&existing)))) {
      Microsoft::WRL::ComPtr<IPersistFile> pf;
      if (SUCCEEDED(existing.As(&pf)) &&
          SUCCEEDED(pf->Load(lnk_path.value().c_str(), STGM_READ))) {
        Microsoft::WRL::ComPtr<IPropertyStore> store;
        if (SUCCEEDED(existing.As(&store))) {
          base::win::ScopedPropVariant pv_clsid;
          if (SUCCEEDED(store->GetValue(PKEY_AppUserModel_ToastActivatorCLSID,
                                        pv_clsid.Receive())) &&
              pv_clsid.get().vt == VT_CLSID && pv_clsid.get().puuid) {
            wchar_t buf[64] = {0};
            if (StringFromGUID2(*pv_clsid.get().puuid, buf, std::size(buf)) >
                0) {
              SetAppToastActivatorCLSID(buf);
            }
          }
        }
      }
    }
  }

  if (ExistingShortcutValid(lnk_path, aumid))
    return;

  Microsoft::WRL::ComPtr<IShellLink> shell_link;
  if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&shell_link))))
    return;
  shell_link->SetPath(exe.c_str());
  shell_link->SetArguments(L"");
  shell_link->SetDescription(product_name.c_str());
  shell_link->SetWorkingDirectory(
      base::FilePath(exe).DirName().value().c_str());

  Microsoft::WRL::ComPtr<IPropertyStore> prop_store;
  if (SUCCEEDED(shell_link.As(&prop_store))) {
    PROPVARIANT pv_id;
    PropVariantInit(&pv_id);
    if (SUCCEEDED(InitPropVariantFromString(aumid, &pv_id))) {
      prop_store->SetValue(PKEY_AppUserModel_ID, pv_id);
      PropVariantClear(&pv_id);
    }
    PROPVARIANT pv_clsid;
    PropVariantInit(&pv_clsid);
    GUID clsid;
    if (SUCCEEDED(::CLSIDFromString(GetAppToastActivatorCLSID(), &clsid)) &&
        SUCCEEDED(InitPropVariantFromCLSID(clsid, &pv_clsid))) {
      prop_store->SetValue(PKEY_AppUserModel_ToastActivatorCLSID, pv_clsid);
      PropVariantClear(&pv_clsid);
    }
    prop_store->Commit();
  }

  Microsoft::WRL::ComPtr<IPersistFile> persist_file;
  if (SUCCEEDED(shell_link.As(&persist_file))) {
    persist_file->Save(lnk_path.value().c_str(), TRUE);
  }
}

}  // namespace

DWORD NotificationActivator::g_cookie_ = 0;
bool NotificationActivator::g_registered_ = false;

NotificationActivator::NotificationActivator() = default;
NotificationActivator::~NotificationActivator() = default;

// static

// static
void NotificationActivator::RegisterActivator() {
  if (g_registered_ || g_registration_in_progress)
    return;
  g_registration_in_progress = true;

  // For packaged (MSIX) apps, COM server registration is handled via the app
  // manifest (com:Extension and desktop:ToastNotificationActivation), so we
  // skip the registry and shortcut creation. We still need to call
  // CoRegisterClassObject to handle activations at runtime.
  //
  // For unpackaged apps, we need to create the Start Menu shortcut with
  // AUMID/CLSID properties and register the COM server in the registry.
  bool is_packaged = IsRunningInDesktopBridge();

  // Perform all blocking filesystem / registry work off the UI thread.
  base::ThreadPool::PostTask(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
      base::BindOnce(
          [](bool is_packaged) {
            // Skip shortcut and registry setup for packaged apps - these are
            // declared in the app manifest instead.
            if (!is_packaged) {
              EnsureShortcut();
              EnsureCLSIDRegistry();
            }
            content::GetUIThreadTaskRunner({})->PostTask(
                FROM_HERE, base::BindOnce([]() {
                  g_registration_in_progress = false;
                  if (g_registered_)
                    return;
                  CLSID clsid;
                  if (FAILED(::CLSIDFromString(GetAppToastActivatorCLSID(),
                                               &clsid))) {
                    LOG(ERROR) << "Invalid toast activator CLSID";
                    return;
                  }
                  if (!g_factory_instance)
                    g_factory_instance =
                        new (std::nothrow) NotificationActivatorFactory();
                  if (!g_factory_instance)
                    return;
                  DWORD cookie = 0;
                  HRESULT hr = ::CoRegisterClassObject(
                      clsid, static_cast<IClassFactory*>(g_factory_instance),
                      CLSCTX_LOCAL_SERVER,
                      REGCLS_MULTIPLEUSE | REGCLS_SUSPENDED, &cookie);
                  if (FAILED(hr))
                    return;
                  hr = ::CoResumeClassObjects();
                  if (FAILED(hr)) {
                    ::CoRevokeClassObject(cookie);
                    return;
                  }
                  g_cookie_ = cookie;
                  g_registered_ = true;
                }));
          },
          is_packaged));
}

// static
void NotificationActivator::UnregisterActivator() {
  if (!g_registered_)
    return;
  ::CoRevokeClassObject(g_cookie_);
  g_cookie_ = 0;
  g_registered_ = false;
  // Factory instance lifetime intentionally leaked after revoke to avoid
  // race; could be deleted here if ensured no pending COM calls.
}

IFACEMETHODIMP NotificationActivator::Activate(
    LPCWSTR app_user_model_id,
    LPCWSTR invoked_args,
    const NOTIFICATION_USER_INPUT_DATA* data,
    ULONG data_count) {
  std::wstring args = invoked_args ? invoked_args : L"";
  std::wstring aumid = app_user_model_id ? app_user_model_id : L"";

  std::vector<ActivationUserInput> copied_inputs;
  if (data && data_count) {
    std::vector<NOTIFICATION_USER_INPUT_DATA> temp;
    temp.resize(static_cast<size_t>(data_count));
    std::copy_n(data, static_cast<size_t>(data_count), temp.begin());
    copied_inputs.reserve(temp.size());
    for (const auto& entry : temp) {
      ActivationUserInput ui;
      if (entry.Key)
        ui.key = entry.Key;
      if (entry.Value)
        ui.value = entry.Value;
      copied_inputs.push_back(std::move(ui));
    }
  }

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(&HandleToastActivation, std::move(args),
                                std::move(copied_inputs)));
  return S_OK;
}

void HandleToastActivation(const std::wstring& invoked_args,
                           std::vector<ActivationUserInput> inputs) {
  // Expected invoked_args format:
  // type=<click|action|reply>&action=<index>&tag=<hash> Parse simple key=value
  // pairs separated by '&'.
  std::wstring args = invoked_args;
  std::wstring type;
  std::wstring action_index_str;
  std::wstring tag_str;

  for (const auto& token : base::SplitString(args, L"&", base::KEEP_WHITESPACE,
                                             base::SPLIT_WANT_NONEMPTY)) {
    auto kv = base::SplitString(token, L"=", base::KEEP_WHITESPACE,
                                base::SPLIT_WANT_NONEMPTY);
    if (kv.size() != 2)
      continue;
    if (kv[0] == L"type")
      type = kv[1];
    else if (kv[0] == L"action")
      action_index_str = kv[1];
    else if (kv[0] == L"tag")
      tag_str = kv[1];
  }

  int action_index = -1;
  if (!action_index_str.empty()) {
    action_index = std::stoi(action_index_str);
  }

  std::string reply_text;
  for (const auto& entry : inputs) {
    std::wstring_view key_view(entry.key);
    if (key_view == L"reply") {
      reply_text = base::WideToUTF8(entry.value);
    }
  }

  auto* browser_client =
      static_cast<ElectronBrowserClient*>(ElectronBrowserClient::Get());
  if (!browser_client)
    return;

  NotificationPresenter* presenter = browser_client->GetNotificationPresenter();
  if (!presenter)
    return;

  Notification* target = nullptr;
  for (auto* n : presenter->notifications()) {
    std::wstring tag_hash =
        base::NumberToWString(base::FastHash(n->notification_id()));
    if (tag_hash == tag_str) {
      target = n;
      break;
    }
  }

  if (!target)
    return;

  if (type == L"action" && target->delegate()) {
    int selection_index = -1;
    for (const auto& entry : inputs) {
      std::wstring_view key_view(entry.key);
      if (base::StartsWith(key_view, L"selection",
                           base::CompareCase::SENSITIVE)) {
        int parsed = -1;
        if (base::StringToInt(entry.value, &parsed) && parsed >= 0)
          selection_index = parsed;
        break;
      }
    }
    if (action_index >= 0)
      target->delegate()->NotificationAction(action_index, selection_index);
  } else if ((type == L"reply" || (!reply_text.empty() && type.empty())) &&
             target->delegate()) {
    target->delegate()->NotificationReplied(reply_text);
  } else if ((type == L"click" || type.empty()) && target->delegate()) {
    target->NotificationClicked();
  }
}

}  // namespace electron
