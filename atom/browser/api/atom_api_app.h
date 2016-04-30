// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ATOM_BROWSER_API_ATOM_API_APP_H_
#define ATOM_BROWSER_API_ATOM_API_APP_H_

#include <string>
#include <map>

#include "atom/browser/api/event_emitter.h"
#include "atom/browser/atom_browser_client.h"
#include "atom/browser/browser_observer.h"
#include "atom/common/native_mate_converters/callback.h"
#include "chrome/browser/process_singleton.h"
#include "content/public/browser/gpu_data_manager_observer.h"
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
}

namespace atom {

namespace api {

class App : public AtomBrowserClient::Delegate,
            public mate::EventEmitter<App>,
            public BrowserObserver,
            public content::GpuDataManagerObserver {
 public:
  static mate::Handle<App> Create(v8::Isolate* isolate);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::ObjectTemplate> prototype);

  // Called when window with disposition needs to be created.
  void OnCreateWindow(const GURL& target_url,
                      const std::string& frame_name,
                      WindowOpenDisposition disposition,
                      int render_process_id,
                      int render_frame_id);

#if defined(USE_NSS_CERTS)
  void OnCertificateManagerModelCreated(
      scoped_ptr<base::DictionaryValue> options,
      const net::CompletionCallback& callback,
      scoped_ptr<CertificateManagerModel> model);
#endif

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
  void OnFinishLaunching() override;
  void OnLogin(LoginHandler* login_handler) override;
  void OnContinueUserActivity(bool* prevent_default,
    const std::string& type,
    const std::map<std::string, std::string>& user_info) override;

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
      const base::Callback<void(bool)>& callback,
      content::CertificateRequestResultType* request) override;
  void SelectClientCertificate(
      content::WebContents* web_contents,
      net::SSLCertRequestInfo* cert_request_info,
      scoped_ptr<content::ClientCertificateDelegate> delegate) override;

  // content::GpuDataManagerObserver:
  void OnGpuProcessCrashed(base::TerminationStatus exit_code) override;

 private:
  // Get/Set the pre-defined path in PathService.
  base::FilePath GetPath(mate::Arguments* args, const std::string& name);
  void SetPath(mate::Arguments* args,
               const std::string& name,
               const base::FilePath& path);

  void SetDesktopName(const std::string& desktop_name);
  void AllowNTLMCredentialsForAllDomains(bool should_allow);
  bool MakeSingleInstance(
      const ProcessSingleton::NotificationCallback& callback);
  std::string GetLocale();

#if defined(USE_NSS_CERTS)
  void ImportCertificate(const base::DictionaryValue& options,
                         const net::CompletionCallback& callback);
#endif

  scoped_ptr<ProcessSingleton> process_singleton_;

#if defined(USE_NSS_CERTS)
  scoped_ptr<CertificateManagerModel> certificate_manager_model_;
#endif

  DISALLOW_COPY_AND_ASSIGN(App);
};

}  // namespace api

}  // namespace atom

#endif  // ATOM_BROWSER_API_ATOM_API_APP_H_
