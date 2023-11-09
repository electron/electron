// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/message_box.h"

#include <windows.h>  // windows.h must be included first

#include <commctrl.h>

#include <map>
#include <vector>

#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/win/scoped_gdi_object.h"
#include "shell/browser/browser.h"
#include "shell/browser/native_window_views.h"
#include "shell/browser/ui/win/dialog_thread.h"
#include "ui/gfx/icon_util.h"
#include "ui/gfx/image/image_skia.h"

namespace electron {

MessageBoxSettings::MessageBoxSettings() = default;
MessageBoxSettings::MessageBoxSettings(const MessageBoxSettings&) = default;
MessageBoxSettings::~MessageBoxSettings() = default;

namespace {

struct DialogResult {
  int button_id;
  bool verification_flag_checked;
};

// <ID, messageBox> map.
//
// Note that the HWND is stored in a unique_ptr, because the pointer of HWND
// will be passed between threads and we need to ensure the memory of HWND is
// not changed while dialogs map is modified.
std::map<int, std::unique_ptr<HWND>>& GetDialogsMap() {
  static base::NoDestructor<std::map<int, std::unique_ptr<HWND>>> dialogs;
  return *dialogs;
}

// Special HWND used by the dialogs map.
//
// - ID is used but window has not been created yet.
const HWND kHwndReserve = reinterpret_cast<HWND>(-1);
// - Notification to cancel message box.
const HWND kHwndCancel = reinterpret_cast<HWND>(-2);

// Lock used for modifying HWND between threads.
//
// Note that there might be multiple dialogs being opened at the same time, but
// we only use one lock for them all, because each dialog is independent from
// each other and there is no need to use different lock for each one.
// Also note that the |GetDialogsMap| is only used in the main thread, what is
// shared between threads is the memory of HWND, so there is no need to use lock
// when accessing dialogs map.
base::Lock& GetHWNDLock() {
  static base::NoDestructor<base::Lock> lock;
  return *lock;
}

// Small command ID values are already taken by Windows, we have to start from
// a large number to avoid conflicts with Windows.
const int kIDStart = 100;

// Get the common ID from button's name.
struct CommonButtonID {
  int button;
  int id;
};
CommonButtonID GetCommonID(const std::wstring& button) {
  std::wstring lower = base::ToLowerASCII(button);
  if (lower == L"ok")
    return {TDCBF_OK_BUTTON, IDOK};
  else if (lower == L"yes")
    return {TDCBF_YES_BUTTON, IDYES};
  else if (lower == L"no")
    return {TDCBF_NO_BUTTON, IDNO};
  else if (lower == L"cancel")
    return {TDCBF_CANCEL_BUTTON, IDCANCEL};
  else if (lower == L"retry")
    return {TDCBF_RETRY_BUTTON, IDRETRY};
  else if (lower == L"close")
    return {TDCBF_CLOSE_BUTTON, IDCLOSE};
  return {-1, -1};
}

// Determine whether the buttons are common buttons, if so map common ID
// to button ID.
void MapToCommonID(const std::vector<std::wstring>& buttons,
                   std::map<int, int>* id_map,
                   TASKDIALOG_COMMON_BUTTON_FLAGS* button_flags,
                   std::vector<TASKDIALOG_BUTTON>* dialog_buttons) {
  for (size_t i = 0; i < buttons.size(); ++i) {
    auto common = GetCommonID(buttons[i]);
    if (common.button != -1) {
      // It is a common button.
      (*id_map)[common.id] = i;
      (*button_flags) |= common.button;
    } else {
      // It is a custom button.
      dialog_buttons->push_back(
          {static_cast<int>(i + kIDStart), buttons[i].c_str()});
    }
  }
}

// Callback of the task dialog. The TaskDialogIndirect API does not provide the
// HWND of the dialog, and we have to listen to the TDN_CREATED message to get
// it.
// Note that this callback runs in dialog thread instead of main thread, so it
// is possible for CloseMessageBox to be called before or all after the dialog
// window is created.
HRESULT CALLBACK
TaskDialogCallback(HWND hwnd, UINT msg, WPARAM, LPARAM, LONG_PTR data) {
  if (msg == TDN_CREATED) {
    HWND* target = reinterpret_cast<HWND*>(data);
    // Lock since CloseMessageBox might be called.
    base::AutoLock lock(GetHWNDLock());
    if (*target == kHwndCancel) {
      // The dialog is cancelled before it is created, close it directly.
      ::PostMessage(hwnd, WM_CLOSE, 0, 0);
    } else if (*target == kHwndReserve) {
      // Otherwise save the hwnd.
      *target = hwnd;
    } else {
      NOTREACHED();
    }
  }
  return S_OK;
}

DialogResult ShowTaskDialogWstr(gfx::AcceleratedWidget parent,
                                MessageBoxType type,
                                const std::vector<std::wstring>& buttons,
                                int default_id,
                                int cancel_id,
                                bool no_link,
                                const std::u16string& title,
                                const std::u16string& message,
                                const std::u16string& detail,
                                const std::u16string& checkbox_label,
                                bool checkbox_checked,
                                const gfx::ImageSkia& icon,
                                HWND* hwnd) {
  TASKDIALOG_FLAGS flags =
      TDF_SIZE_TO_CONTENT |           // Show all content.
      TDF_ALLOW_DIALOG_CANCELLATION;  // Allow canceling the dialog.

  TASKDIALOGCONFIG config = {0};
  config.cbSize = sizeof(config);
  config.hInstance = GetModuleHandle(nullptr);
  config.dwFlags = flags;

  if (parent) {
    config.hwndParent = parent;
  }

  if (default_id > 0)
    config.nDefaultButton = kIDStart + default_id;

  // TaskDialogIndirect doesn't allow empty name, if we set empty title it
  // will show "electron.exe" in title.
  std::wstring app_name;
  if (title.empty()) {
    app_name = base::UTF8ToWide(Browser::Get()->GetName());
    config.pszWindowTitle = app_name.c_str();
  } else {
    config.pszWindowTitle = base::as_wcstr(title);
  }

  base::win::ScopedHICON hicon;
  if (!icon.isNull()) {
    hicon = IconUtil::CreateHICONFromSkBitmap(*icon.bitmap());
    config.dwFlags |= TDF_USE_HICON_MAIN;
    config.hMainIcon = hicon.get();
  } else {
    // Show icon according to dialog's type.
    switch (type) {
      case MessageBoxType::kInformation:
      case MessageBoxType::kQuestion:
        config.pszMainIcon = TD_INFORMATION_ICON;
        break;
      case MessageBoxType::kWarning:
        config.pszMainIcon = TD_WARNING_ICON;
        break;
      case MessageBoxType::kError:
        config.pszMainIcon = TD_ERROR_ICON;
        break;
      case MessageBoxType::kNone:
        break;
    }
  }

  // If "detail" is empty then don't make message highlighted.
  if (detail.empty()) {
    config.pszContent = base::as_wcstr(message);
  } else {
    config.pszMainInstruction = base::as_wcstr(message);
    config.pszContent = base::as_wcstr(detail);
  }

  if (!checkbox_label.empty()) {
    config.pszVerificationText = base::as_wcstr(checkbox_label);
    if (checkbox_checked)
      config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
  }

  // Iterate through the buttons, put common buttons in dwCommonButtons
  // and custom buttons in pButtons.
  std::map<int, int> id_map;
  std::vector<TASKDIALOG_BUTTON> dialog_buttons;
  if (no_link) {
    for (size_t i = 0; i < buttons.size(); ++i)
      dialog_buttons.push_back(
          {static_cast<int>(i + kIDStart), buttons[i].c_str()});
  } else {
    MapToCommonID(buttons, &id_map, &config.dwCommonButtons, &dialog_buttons);
  }
  if (!dialog_buttons.empty()) {
    config.pButtons = &dialog_buttons.front();
    config.cButtons = dialog_buttons.size();
    if (!no_link)
      config.dwFlags |= TDF_USE_COMMAND_LINKS;  // custom buttons as links.
  }

  // Pass a callback to receive the HWND of the message box.
  if (hwnd) {
    config.pfCallback = &TaskDialogCallback;
    config.lpCallbackData = reinterpret_cast<LONG_PTR>(hwnd);
  }

  int id = 0;
  BOOL verification_flag_checked = FALSE;
  TaskDialogIndirect(&config, &id, nullptr, &verification_flag_checked);

  int button_id;
  if (base::Contains(id_map, id))  // common button.
    button_id = id_map[id];
  else if (id >= kIDStart)  // custom button.
    button_id = id - kIDStart;
  else
    button_id = cancel_id;

  return {button_id, static_cast<bool>(verification_flag_checked)};
}

DialogResult ShowTaskDialogUTF8(const MessageBoxSettings& settings,
                                gfx::AcceleratedWidget parent_widget,
                                HWND* hwnd) {
  std::vector<std::wstring> buttons;
  for (const auto& button : settings.buttons)
    buttons.push_back(base::UTF8ToWide(button));

  const std::u16string title = base::UTF8ToUTF16(settings.title);
  const std::u16string message = base::UTF8ToUTF16(settings.message);
  const std::u16string detail = base::UTF8ToUTF16(settings.detail);
  const std::u16string checkbox_label =
      base::UTF8ToUTF16(settings.checkbox_label);

  return ShowTaskDialogWstr(
      parent_widget, settings.type, buttons, settings.default_id,
      settings.cancel_id, settings.no_link, title, message, detail,
      checkbox_label, settings.checkbox_checked, settings.icon, hwnd);
}

}  // namespace

int ShowMessageBoxSync(const MessageBoxSettings& settings) {
  gfx::AcceleratedWidget parent_widget =
      settings.parent_window
          ? static_cast<electron::NativeWindowViews*>(settings.parent_window)
                ->GetAcceleratedWidget()
          : nullptr;
  DialogResult result = ShowTaskDialogUTF8(settings, parent_widget, nullptr);
  return result.button_id;
}

void ShowMessageBox(const MessageBoxSettings& settings,
                    MessageBoxCallback callback) {
  // The dialog is created in a new thread so we don't know its HWND yet, put
  // kHwndReserve in the dialogs map for now.
  HWND* hwnd = nullptr;
  if (settings.id) {
    if (base::Contains(GetDialogsMap(), *settings.id))
      CloseMessageBox(*settings.id);
    auto it = GetDialogsMap().emplace(*settings.id,
                                      std::make_unique<HWND>(kHwndReserve));
    hwnd = it.first->second.get();
  }

  gfx::AcceleratedWidget parent_widget =
      settings.parent_window
          ? static_cast<electron::NativeWindowViews*>(settings.parent_window)
                ->GetAcceleratedWidget()
          : nullptr;
  dialog_thread::Run(base::BindOnce(&ShowTaskDialogUTF8, settings,
                                    parent_widget, base::Unretained(hwnd)),
                     base::BindOnce(
                         [](MessageBoxCallback callback, absl::optional<int> id,
                            DialogResult result) {
                           if (id)
                             GetDialogsMap().erase(*id);
                           std::move(callback).Run(
                               result.button_id,
                               result.verification_flag_checked);
                         },
                         std::move(callback), settings.id));
}

void CloseMessageBox(int id) {
  auto it = GetDialogsMap().find(id);
  if (it == GetDialogsMap().end()) {
    LOG(ERROR) << "CloseMessageBox called with nonexistent ID";
    return;
  }
  HWND* hwnd = it->second.get();
  // Lock since the TaskDialogCallback might be saving the dialog's HWND.
  base::AutoLock lock(GetHWNDLock());
  DCHECK(*hwnd != kHwndCancel);
  if (*hwnd == kHwndReserve) {
    // If the dialog window has not been created yet, tell it to cancel.
    *hwnd = kHwndCancel;
  } else {
    // Otherwise send a message to close it.
    ::PostMessage(*hwnd, WM_CLOSE, 0, 0);
  }
}

void ShowErrorBox(const std::u16string& title, const std::u16string& content) {
  ShowTaskDialogWstr(nullptr, MessageBoxType::kError, {}, -1, 0, false,
                     u"Error", title, content, u"", false, gfx::ImageSkia(),
                     nullptr);
}

}  // namespace electron
