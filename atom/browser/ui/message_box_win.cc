// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include <windows.h>
#include <commctrl.h>

#include <map>
#include <vector>

#include "atom/browser/browser.h"
#include "atom/browser/native_window_views.h"
#include "base/callback.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "base/win/scoped_gdi_object.h"
#include "content/public/browser/browser_thread.h"
#include "ui/gfx/icon_util.h"

namespace atom {

namespace {

// Small command ID values are already taken by Windows, we have to start from
// a large number to avoid conflicts with Windows.
const int kIDStart = 100;

// Get the common ID from button's name.
struct CommonButtonID {
  int button;
  int id;
};
CommonButtonID GetCommonID(const base::string16& button) {
  base::string16 lower = base::StringToLowerASCII(button);
  if (lower == L"ok")
    return { TDCBF_OK_BUTTON, IDOK };
  else if (lower == L"yes")
    return { TDCBF_YES_BUTTON, IDYES };
  else if (lower == L"no")
    return { TDCBF_NO_BUTTON, IDNO };
  else if (lower == L"cancel")
    return { TDCBF_CANCEL_BUTTON, IDCANCEL };
  else if (lower == L"retry")
    return { TDCBF_RETRY_BUTTON, IDRETRY };
  else if (lower == L"close")
    return { TDCBF_CLOSE_BUTTON, IDCLOSE };
  return { -1, -1 };
}

// Determine whether the buttons are common buttons, if so map common ID
// to button ID.
void MapToCommonID(const std::vector<base::string16>& buttons,
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
      dialog_buttons->push_back({i + kIDStart, buttons[i].c_str()});
    }
  }
}

int ShowMessageBoxUTF16(HWND parent,
                        MessageBoxType type,
                        const std::vector<base::string16>& buttons,
                        int cancel_id,
                        int options,
                        const base::string16& title,
                        const base::string16& message,
                        const base::string16& detail,
                        const gfx::ImageSkia& icon) {
  TASKDIALOG_FLAGS flags =
      TDF_SIZE_TO_CONTENT |  // Show all content.
      TDF_ALLOW_DIALOG_CANCELLATION;  // Allow canceling the dialog.

  TASKDIALOGCONFIG config = { 0 };
  config.cbSize     = sizeof(config);
  config.hwndParent = parent;
  config.hInstance  = GetModuleHandle(NULL);
  config.dwFlags    = flags;

  // TaskDialogIndirect doesn't allow empty name, if we set empty title it
  // will show "electron.exe" in title.
  base::string16 app_name = base::UTF8ToUTF16(Browser::Get()->GetName());
  if (title.empty())
    config.pszWindowTitle = app_name.c_str();
  else
    config.pszWindowTitle = title.c_str();

  base::win::ScopedHICON hicon;
  if (!icon.isNull()) {
    hicon.Set(IconUtil::CreateHICONFromSkBitmap(*icon.bitmap()));
    config.dwFlags |= TDF_USE_HICON_MAIN;
    config.hMainIcon = hicon.Get();
  } else {
    // Show icon according to dialog's type.
    switch (type) {
      case MESSAGE_BOX_TYPE_INFORMATION:
      case MESSAGE_BOX_TYPE_QUESTION:
        config.pszMainIcon = TD_INFORMATION_ICON;
        break;
      case MESSAGE_BOX_TYPE_WARNING:
        config.pszMainIcon = TD_WARNING_ICON;
        break;
      case MESSAGE_BOX_TYPE_ERROR:
        config.pszMainIcon = TD_ERROR_ICON;
        break;
    }
  }

  // If "detail" is empty then don't make message hilighted.
  if (detail.empty()) {
    config.pszContent = message.c_str();
  } else {
    config.pszMainInstruction = message.c_str();
    config.pszContent = detail.c_str();
  }

  // Iterate through the buttons, put common buttons in dwCommonButtons
  // and custom buttons in pButtons.
  std::map<int, int> id_map;
  std::vector<TASKDIALOG_BUTTON> dialog_buttons;
  if (options & MESSAGE_BOX_NO_LINK) {
    for (size_t i = 0; i < buttons.size(); ++i)
      dialog_buttons.push_back({i + kIDStart, buttons[i].c_str()});
  } else {
    MapToCommonID(buttons, &id_map, &config.dwCommonButtons, &dialog_buttons);
  }
  if (dialog_buttons.size() > 0) {
    config.pButtons = &dialog_buttons.front();
    config.cButtons = dialog_buttons.size();
    if (!(options & MESSAGE_BOX_NO_LINK))
      config.dwFlags |= TDF_USE_COMMAND_LINKS;  // custom buttons as links.
  }

  int id = 0;
  TaskDialogIndirect(&config, &id, NULL, NULL);
  if (id_map.find(id) != id_map.end())  // common button.
    return id_map[id];
  else if (id >= kIDStart)  // custom button.
    return id - kIDStart;
  else
    return cancel_id;
}

void RunMessageBoxInNewThread(base::Thread* thread,
                              NativeWindow* parent,
                              MessageBoxType type,
                              const std::vector<std::string>& buttons,
                              int cancel_id,
                              int options,
                              const std::string& title,
                              const std::string& message,
                              const std::string& detail,
                              const gfx::ImageSkia& icon,
                              const MessageBoxCallback& callback) {
  int result = ShowMessageBox(parent, type, buttons, cancel_id, options, title,
                              message, detail, icon);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE, base::Bind(callback, result));
  content::BrowserThread::DeleteSoon(
      content::BrowserThread::UI, FROM_HERE, thread);
}

}  // namespace

int ShowMessageBox(NativeWindow* parent,
                   MessageBoxType type,
                   const std::vector<std::string>& buttons,
                   int cancel_id,
                   int options,
                   const std::string& title,
                   const std::string& message,
                   const std::string& detail,
                   const gfx::ImageSkia& icon) {
  std::vector<base::string16> utf16_buttons;
  for (const auto& button : buttons)
    utf16_buttons.push_back(base::UTF8ToUTF16(button));

  HWND hwnd_parent = parent ?
      static_cast<atom::NativeWindowViews*>(parent)->GetAcceleratedWidget() :
      NULL;

  NativeWindow::DialogScope dialog_scope(parent);
  return ShowMessageBoxUTF16(hwnd_parent,
                             type,
                             utf16_buttons,
                             cancel_id,
                             options,
                             base::UTF8ToUTF16(title),
                             base::UTF8ToUTF16(message),
                             base::UTF8ToUTF16(detail),
                             icon);
}

void ShowMessageBox(NativeWindow* parent,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    int cancel_id,
                    int options,
                    const std::string& title,
                    const std::string& message,
                    const std::string& detail,
                    const gfx::ImageSkia& icon,
                    const MessageBoxCallback& callback) {
  scoped_ptr<base::Thread> thread(
      new base::Thread(ATOM_PRODUCT_NAME "MessageBoxThread"));
  thread->init_com_with_mta(false);
  if (!thread->Start()) {
    callback.Run(cancel_id);
    return;
  }

  base::Thread* unretained = thread.release();
  unretained->message_loop()->PostTask(
      FROM_HERE,
      base::Bind(&RunMessageBoxInNewThread, base::Unretained(unretained),
                 parent, type, buttons, cancel_id, options, title, message,
                 detail, icon, callback));
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
  ShowMessageBoxUTF16(NULL, MESSAGE_BOX_TYPE_ERROR, {}, 0, 0, L"Error", title,
                      content, gfx::ImageSkia());
}

}  // namespace atom
