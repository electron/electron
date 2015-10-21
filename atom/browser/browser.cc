// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/browser.h"

#include <string>

#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/window_list.h"
#include "base/message_loop/message_loop.h"
#include "base/path_service.h"
#include "brightray/browser/brightray_paths.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "net/ssl/ssl_cert_request_info.h"

namespace atom {

Browser::Browser()
    : is_quiting_(false),
      is_ready_(false),
      is_shutdown_(false),
      process_notify_callback_set_(false) {
  WindowList::AddObserver(this);
}

Browser::~Browser() {
  WindowList::RemoveObserver(this);
}

// static
Browser* Browser::Get() {
  return AtomBrowserMainParts::Get()->browser();
}

void Browser::Quit() {
  if (is_quiting_)
    return;

  is_quiting_ = HandleBeforeQuit();
  if (!is_quiting_)
    return;

  atom::WindowList* window_list = atom::WindowList::GetInstance();
  if (window_list->size() == 0)
    NotifyAndShutdown();

  window_list->CloseAllWindows();
}

void Browser::Shutdown() {
  if (is_shutdown_)
    return;

  is_shutdown_ = true;
  is_quiting_ = true;

  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnQuit());
  base::MessageLoop::current()->PostTask(
      FROM_HERE, base::MessageLoop::QuitWhenIdleClosure());
}

std::string Browser::GetVersion() const {
  if (version_override_.empty()) {
    std::string version = GetExecutableFileVersion();
    if (!version.empty())
      return version;
  }

  return version_override_;
}

void Browser::SetVersion(const std::string& version) {
  version_override_ = version;
}

std::string Browser::GetName() const {
  if (name_override_.empty()) {
    std::string name = GetExecutableFileProductName();
    if (!name.empty())
      return name;
  }

  return name_override_;
}

void Browser::SetName(const std::string& name) {
  name_override_ = name;

#if defined(OS_WIN)
  SetAppUserModelID(name);
#endif
}

bool Browser::OpenFile(const std::string& file_path) {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnOpenFile(&prevent_default, file_path));

  return prevent_default;
}

void Browser::OpenURL(const std::string& url) {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnOpenURL(url));
}

void Browser::Activate(bool has_visible_windows) {
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnActivate(has_visible_windows));
}

void Browser::WillFinishLaunching() {
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillFinishLaunching());
}

void Browser::DidFinishLaunching() {
  is_ready_ = true;

  if (process_notify_callback_set_) {
    process_singleton_->Unlock();
  }

  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnFinishLaunching());
}

void Browser::ClientCertificateSelector(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    scoped_ptr<content::ClientCertificateDelegate> delegate) {
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnSelectCertificate(web_contents,
                                        cert_request_info,
                                        delegate.Pass()));
}

void Browser::NotifyAndShutdown() {
  if (is_shutdown_)
    return;

  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWillQuit(&prevent_default));

  if (prevent_default) {
    is_quiting_ = false;
    return;
  }

  if (process_notify_callback_set_) {
    process_singleton_->Cleanup();
    process_notify_callback_.Reset();
  }

  Shutdown();
}

bool Browser::HandleBeforeQuit() {
  bool prevent_default = false;
  FOR_EACH_OBSERVER(BrowserObserver,
                    observers_,
                    OnBeforeQuit(&prevent_default));

  return !prevent_default;
}

void Browser::InitializeSingleInstance() {
  base::FilePath userDir;
  PathService::Get(brightray::DIR_USER_DATA, &userDir);

  auto no_refcount_this = base::Unretained(this);
  process_singleton_.reset(new AtomProcessSingleton(
    userDir,
    base::Bind(&Browser::OnProcessSingletonNotification, no_refcount_this)));

  process_notify_result_ = process_singleton_->NotifyOtherProcessOrCreate();

  if (is_ready_) {
    process_singleton_->Unlock();
  }
}

ProcessSingleton::NotifyResult Browser::GetSingleInstanceResult() {
  LOG(ERROR) << "Process Notify Result: " << process_notify_result_;
  return process_notify_result_;
}

void Browser::SetSingleInstanceCallback(
    ProcessSingleton::NotificationCallback callback) {
  process_notify_callback_ = callback;
  process_notify_callback_set_ = true;
}

void Browser::OnWindowCloseCancelled(NativeWindow* window) {
  if (is_quiting_)
    // Once a beforeunload handler has prevented the closing, we think the quit
    // is cancelled too.
    is_quiting_ = false;
}

void Browser::OnWindowAllClosed() {
  if (is_quiting_)
    NotifyAndShutdown();
  else
    FOR_EACH_OBSERVER(BrowserObserver, observers_, OnWindowAllClosed());
}

bool Browser::OnProcessSingletonNotification(
    const base::CommandLine& command_line,
    const base::FilePath& current_directory) {
  if (process_notify_callback_set_) {
    return process_notify_callback_.Run(command_line, current_directory);
  } else {
    return true;    // We'll handle this, not a different process
  }
}

}  // namespace atom
