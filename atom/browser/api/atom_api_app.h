// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/browser.h"
#include "atom/browser/browser_observer.h"
#include "atom/common/native_mate_converters/callback.h"
#include "base/process/process_iterator.h"
#include "base/task/cancelable_task_tracker.h"
#include "chrome/browser/icon_manager.h"
#include "chrome/browser/process_singleton.h"
#include "content/public/browser/browser_child_process_observer.h"
#include "content/public/browser/gpu_data_manager_observer.h"
#include "content/public/browser/render_process_host.h"
#include "native_mate/dictionary.h"
#include "native_mate/handle.h"
#include "net/base/completion_callback.h"

#if defined(USE_NSS_CERTS)
#include "chrome/browser/certificate_manager_model.h"
#endif

namespace base {
class FilePath;
}

namespace mate {
class Arguments;
}  // namespace mate

namespace atom {

#if defined(OS_WIN)
enum class JumpListResult : int;
#endif

struct ProcessMetric {
  int type;
  base::ProcessId pid;
  std::unique_ptr<base::ProcessMetrics> metrics;

  ProcessMetric(int type,
                base::ProcessId pid,
                std::unique_ptr<base::ProcessMetrics> metrics) {
    this->type = type;
    this->pid = pid;
    this->metrics = std::move(metrics);
  }
};

namespace api {

class App : public AtomBrowserClient::Delegate,
            public mate::EventEmitter<App>,
            public BrowserObserver,
            public content::GpuDataManagerObserver,
            public content::BrowserChildProcessObserver {
 public:
  using FileIconCallback = base::Callback<void(v8::Local<v8::Value>,
                                               const gfx::Image&)>;

  static mate::Handle<App> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  // Called when window with disposition needs to be created.
  void OnCreateWindow(
      const GURL& target_url,
      const std::string& frame_name,
      WindowOpenDisposition disposition,
      const std::vector<std::string>& features,
      const scoped_refptr<content::ResourceRequestBodyImpl>& body,
      int render_process_id,
      int render_frame_id);

#if defined(USE_NSS_CERTS)
  void OnCertificateManagerModelCreated(
      std::unique_ptr<base::DictionaryValue> options,
      const net::CompletionCallback& callback,
      std::unique_ptr<CertificateManagerModel> model);
#endif

  base::FilePath GetAppPath() const;
  void RenderProcessReady(content::RenderProcessHost* host);
  void RenderProcessDisconnected(base::ProcessId host_pid);

 protected:
  explicit App(v8::Isolate* isolate);
  ~App() override;

  // BrowserObserver:
  void OnBeforeQuit(bool* prevent_default) override;
  void OnWillQuit(bool* prevent_default) override;
  void OnWindowAllClosed() override;
  void OnQuit() override;
  void OnOpenFile(bool* prevent_default, const std::string& file_path) override;
  void OnOpenURL(const std::string& url) override;
  void OnActivate(bool has_visible_windows) override;
  void OnWillFinishLaunching() override;
  void OnFinishLaunching(const base::DictionaryValue& launch_info) override;
  void OnLogin(LoginHandler* login_handler,
               const base::DictionaryValue& request_details) override;
  void OnAccessibilitySupportChanged() override;
#if defined(OS_MACOSX)
  void OnContinueUserActivity(
      bool* prevent_default,
      const std::string& type,
      const base::DictionaryValue& user_info) override;
#endif

  // content::ContentBrowserClient:
  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool overridable,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      std::unique_ptr<content::ClientCertificateDelegate> delegate) override;

  // content::GpuDataManagerObserver:
  void OnGpuProcessCrashed(base::TerminationStatus status) override;

  // content::BrowserChildProcessObserver:
  void BrowserChildProcessLaunchedAndConnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessHostDisconnected(
      const content::ChildProcessData& data) override;
  void BrowserChildProcessCrashed(
      const content::ChildProcessData& data, int exit_code) override;
  void BrowserChildProcessKilled(
      const content::ChildProcessData& data, int exit_code) override;

 private:
  void SetAppPath(const base::FilePath& app_path);
  void ChildProcessLaunched(int process_type, base::ProcessHandle handle);
  void ChildProcessDisconnected(base::ProcessId pid);

  // Get/Set the pre-defined path in PathService.
  base::FilePath GetPath(mate::Arguments* args, const std::string& name);
  void SetPath(mate::Arguments* args,
               const std::string& name,
               const base::FilePath& path);

  void SetDesktopName(const std::string& desktop_name);
  std::string GetLocale();
  bool MakeSingleInstance(
      const ProcessSingleton::NotificationCallback& callback);
  void ReleaseSingleInstance();
  bool Relaunch(mate::Arguments* args);
  void DisableHardwareAcceleration(mate::Arguments* args);
  bool IsAccessibilitySupportEnabled();
  Browser::LoginItemSettings GetLoginItemSettings(mate::Arguments* args);
#if defined(USE_NSS_CERTS)
  void ImportCertificate(const base::DictionaryValue& options,
                         const net::CompletionCallback& callback);
#endif
  void GetFileIcon(const base::FilePath& path,
                   mate::Arguments* args);

  std::vector<mate::Dictionary> GetAppMetrics(v8::Isolate* isolate);
  v8::Local<v8::Value> GetGPUFeatureStatus(v8::Isolate* isolate);

#if defined(OS_WIN)
  // Get the current Jump List settings.
  v8::Local<v8::Value> GetJumpListSettings();

  // Set or remove a custom Jump List for the application.
  JumpListResult SetJumpList(v8::Local<v8::Value> val, mate::Arguments* args);
#endif  // defined(OS_WIN)

  std::unique_ptr<ProcessSingleton> process_singleton_;

#if defined(USE_NSS_CERTS)
  std::unique_ptr<CertificateManagerModel> certificate_manager_model_;
#endif

  // Tracks tasks requesting file icons.
  base::CancelableTaskTracker cancelable_task_tracker_;

  base::FilePath app_path_;

  using ProcessMetricMap =
      std::unordered_map<base::ProcessId,
                         std::unique_ptr<atom::ProcessMetric>>;
  ProcessMetricMap app_metrics_;

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
