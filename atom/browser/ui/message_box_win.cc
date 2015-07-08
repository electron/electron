// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/message_box.h"

#include <windows.h>
#include <commctrl.h>

#include "atom/browser/native_window_views.h"
#include "base/callback.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread.h"
#include "content/public/browser/browser_thread.h"

namespace atom {

namespace {

int ShowMessageBoxUTF16(HWND parent,
                        const std::vector<base::string16>& buttons,
                        int cancel_id,
                        const base::string16& title,
                        const base::string16& message,
                        const base::string16& detail) {
  std::vector<TASKDIALOG_BUTTON> dialog_buttons;
  for (size_t i = 0; i < buttons.size(); ++i)
    dialog_buttons.push_back({i, buttons[i].c_str()});

  TASKDIALOGCONFIG config = { 0 };
  config.cbSize             = sizeof(config);
  config.hwndParent         = parent;
  config.hInstance          = GetModuleHandle(NULL);
  config.dwFlags            = TDF_SIZE_TO_CONTENT;
  config.pszWindowTitle     = title.c_str();
  config.pszMainInstruction = message.c_str();
  config.pszContent         = detail.c_str();
  config.pButtons           = &dialog_buttons.front();
  config.cButtons           = dialog_buttons.size();

  int id = 0;
  TaskDialogIndirect(&config, &id, NULL, NULL);
  return id;
}

void RunMessageBoxInNewThread(base::Thread* thread,
                              NativeWindow* parent,
                              MessageBoxType type,
                              const std::vector<std::string>& buttons,
                              int cancel_id,
                              const std::string& title,
                              const std::string& message,
                              const std::string& detail,
                              const gfx::ImageSkia& icon,
                              const MessageBoxCallback& callback) {
  int result = ShowMessageBox(parent, type, buttons, cancel_id, title, message,
                              detail, icon);
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
                             utf16_buttons,
                             cancel_id,
                             base::UTF8ToUTF16(title),
                             base::UTF8ToUTF16(message),
                             base::UTF8ToUTF16(detail));
}

void ShowMessageBox(NativeWindow* parent,
                    MessageBoxType type,
                    const std::vector<std::string>& buttons,
                    int cancel_id,
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
                 parent, type, buttons, cancel_id, title, message, detail, icon,
                 callback));
}

void ShowErrorBox(const base::string16& title, const base::string16& content) {
}

}  // namespace atom
