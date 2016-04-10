// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/api/atom_api_app.h"

#include <string>
#include <vector>

#include "atom/browser/api/atom_api_menu.h"
#include "atom/browser/api/atom_api_session.h"
#include "atom/browser/api/atom_api_web_contents.h"
#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_browser_main_parts.h"
#include "atom/browser/browser.h"
#include "atom/browser/login_handler.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

#if defined(OS_WIN)
#include "base/strings/utf_string_conversions.h"
#include "ui/base/win/shell.h"
#endif

#if defined(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/one_shot_event.h"
#endif

using atom::Browser;

namespace mate {

#if defined(OS_WIN)
template<>
struct Converter<Browser::UserTask> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     Browser::UserTask* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;
    if (!dict.Get("program", &(out->program)) ||
        !dict.Get("title", &(out->title)))
      return false;
    if (dict.Get("iconPath", &(out->icon_path)) &&
        !dict.Get("iconIndex", &(out->icon_index)))
      return false;
    dict.Get("arguments", &(out->arguments));
    dict.Get("description", &(out->description));
    return true;
  }
};
#endif

}  // namespace mate


namespace atom {

namespace api {

namespace {

// Return the path constant from string.
int GetPathConstant(const std::string& name) {
  if (name == "appData")
    return brightray::DIR_APP_DATA;
  else if (name == "userData")
    return brightray::DIR_USER_DATA;
  else if (name == "cache")
    return brightray::DIR_CACHE;
  else if (name == "userCache")
    return brightray::DIR_USER_CACHE;
  else if (name == "home")
    return base::DIR_HOME;
  else if (name == "temp")
    return base::DIR_TEMP;
  else if (name == "userDesktop" || name == "desktop")
    return base::DIR_USER_DESKTOP;
  else if (name == "exe")
    return base::FILE_EXE;
  else if (name == "module")
    return base::FILE_MODULE;
  else if (name == "documents")
    return chrome::DIR_USER_DOCUMENTS;
  else if (name == "downloads")
    return chrome::DIR_DEFAULT_DOWNLOADS;
  else if (name == "music")
    return chrome::DIR_USER_MUSIC;
  else if (name == "pictures")
    return chrome::DIR_USER_PICTURES;
  else if (name == "videos")
    return chrome::DIR_USER_VIDEOS;
  else
    return -1;
}

bool NotificationCallbackWrapper(
    const ProcessSingleton::NotificationCallback& callback,
    const base::CommandLine::StringVector& cmd,
    const base::FilePath& cwd) {
  // Make sure the callback is called after app gets ready.
  if (Browser::Get()->is_ready()) {
    callback.Run(cmd, cwd);
  } else {
    scoped_refptr<base::SingleThreadTaskRunner> task_runner(
        base::ThreadTaskRunnerHandle::Get());
    task_runner->PostTask(
        FROM_HERE, base::Bind(base::IgnoreResult(callback), cmd, cwd));
  }
  // ProcessSingleton needs to know whether current process is quiting.
  return !Browser::Get()->is_shutting_down();
}

void OnClientCertificateSelected(
    v8::Isolate* isolate,
    std::shared_ptr<content::ClientCertificateDelegate> delegate,
    mate::Arguments* args) {
  mate::Dictionary cert_data;
  if (!args->GetNext(&cert_data)) {
    args->ThrowError();
    return;
  }

  v8::Local<v8::Object> data;
  if (!cert_data.Get("data", &data))
    return;

  auto certs = net::X509Certificate::CreateCertificateListFromBytes(
      node::Buffer::Data(data), node::Buffer::Length(data),
      net::X509Certificate::FORMAT_AUTO);
  if (certs.size() > 0)
    delegate->ContinueWithCertificate(certs[0].get());
}

void PassLoginInformation(scoped_refptr<LoginHandler> login_handler,
                          mate::Arguments* args) {
  base::string16 username, password;
  if (args->GetNext(&username) && args->GetNext(&password))
    login_handler->Login(username, password);
  else
    login_handler->CancelAuth();
}

}  // namespace

App::App() {
  static_cast<AtomBrowserClient*>(AtomBrowserClient::Get())->set_delegate(this);
  Browser::Get()->AddObserver(this);
  content::GpuDataManager::GetInstance()->AddObserver(this);
  registrar_.Add(this,
                 content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED,
                 content::NotificationService::AllBrowserContextsAndSources());
}

void App::Observe(
    int type, const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  switch (type) {
    case content::NOTIFICATION_WEB_CONTENTS_RENDER_VIEW_HOST_CREATED: {
#if defined(ENABLE_EXTENSIONS)
      content::WebContents* web_contents =
          content::Source<content::WebContents>(source).ptr();
      auto browser_context = web_contents->GetBrowserContext();
      auto url = web_contents->GetURL();

      // make sure background pages get a webcontents
      // api wrapper so they can communicate via IPC
      if (extensions::ExtensionSystem::Get(browser_context)
          ->ready().is_signaled()) {
        const extensions::Extension* extension =
            extensions::ExtensionRegistry::Get(browser_context)->
                enabled_extensions().GetExtensionOrAppByURL(url);
        if (extension &&
            url == extensions::BackgroundInfo::GetBackgroundURL(extension)) {
          v8::Locker locker(isolate());
          v8::HandleScope handle_scope(isolate());
          WebContents::CreateFrom(isolate(), web_contents);
        }
      }
#endif
      break;
    }
  }
}

App::~App() {
  static_cast<AtomBrowserClient*>(AtomBrowserClient::Get())->set_delegate(
      nullptr);
  Browser::Get()->RemoveObserver(this);
  content::GpuDataManager::GetInstance()->RemoveObserver(this);
}

void App::OnBeforeQuit(bool* prevent_default) {
  *prevent_default = Emit("before-quit");
}

void App::OnWillQuit(bool* prevent_default) {
  *prevent_default = Emit("will-quit");
}

void App::OnWindowAllClosed() {
  Emit("window-all-closed");
}

void App::OnQuit() {
  int exitCode = AtomBrowserMainParts::Get()->GetExitCode();
  Emit("quit", exitCode);

  if (process_singleton_.get()) {
    process_singleton_->Cleanup();
    process_singleton_.reset();
  }
}

void App::OnOpenFile(bool* prevent_default, const std::string& file_path) {
  *prevent_default = Emit("open-file", file_path);
}

void App::OnOpenURL(const std::string& url) {
  Emit("open-url", url);
}

void App::OnActivate(bool has_visible_windows) {
  Emit("activate", has_visible_windows);
}

void App::OnWillFinishLaunching() {
  Emit("will-finish-launching");
}

void App::OnFinishLaunching() {
  Emit("ready");
}

void App::OnLogin(LoginHandler* login_handler) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  bool prevent_default = Emit(
      "login",
      WebContents::CreateFrom(isolate(), login_handler->GetWebContents()),
      login_handler->request(),
      login_handler->auth_info(),
      base::Bind(&PassLoginInformation, make_scoped_refptr(login_handler)));

  // Default behavior is to always cancel the auth.
  if (!prevent_default)
    login_handler->CancelAuth();
}

bool App::CanCreateWindow(const GURL& opener_url,
                     const GURL& opener_top_level_frame_url,
                     const GURL& source_origin,
                     WindowContainerType container_type,
                     const std::string& frame_name,
                     const GURL& target_url,
                     const content::Referrer& referrer,
                     WindowOpenDisposition disposition,
                     const blink::WebWindowFeatures& features,
                     bool user_gesture,
                     bool opener_suppressed,
                     content::ResourceContext* context,
                     int render_process_id,
                     int opener_render_view_id,
                     int opener_render_frame_id,
                     bool* no_javascript_access) {
  // just a reminder that we are on the IO thread
  // and need to be careful about v8 isolate usage
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  *no_javascript_access = false;

  if (container_type == WINDOW_CONTAINER_TYPE_BACKGROUND) {
    return true;
  }

  // this will override allowpopups we need some way to integerate
  // it so we can turn popup blocking off if desired
  if (!user_gesture) {
    return false;
  }

  return true;
}

void App::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool overridable,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(bool)>& callback,
    content::CertificateRequestResultType* request) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  bool prevent_default = Emit("certificate-error",
                              WebContents::CreateFrom(isolate(), web_contents),
                              request_url,
                              net::ErrorToString(cert_error),
                              ssl_info.cert,
                              callback);

  // Deny the certificate by default.
  if (!prevent_default)
    *request = content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY;
}

void App::SelectClientCertificate(
    content::WebContents* web_contents,
    net::SSLCertRequestInfo* cert_request_info,
    scoped_ptr<content::ClientCertificateDelegate> delegate) {
  std::shared_ptr<content::ClientCertificateDelegate>
      shared_delegate(delegate.release());
  bool prevent_default =
      Emit("select-client-certificate",
           WebContents::CreateFrom(isolate(), web_contents),
           cert_request_info->host_and_port.ToString(),
           cert_request_info->client_certs,
           base::Bind(&OnClientCertificateSelected,
                      isolate(),
                      shared_delegate));

  // Default to first certificate from the platform store.
  if (!prevent_default)
    shared_delegate->ContinueWithCertificate(
        cert_request_info->client_certs[0].get());
}

void App::OnGpuProcessCrashed(base::TerminationStatus exit_code) {
  Emit("gpu-process-crashed");
}

#if defined(OS_MACOSX)
void App::OnPlatformThemeChanged() {
  Emit("platform-theme-changed");
}
#endif

base::FilePath App::GetPath(mate::Arguments* args, const std::string& name) {
  bool succeed = false;
  base::FilePath path;
  int key = GetPathConstant(name);
  if (key >= 0)
    succeed = PathService::Get(key, &path);
  if (!succeed)
    args->ThrowError("Failed to get path");
  return path;
}

void App::SetPath(mate::Arguments* args,
                  const std::string& name,
                  const base::FilePath& path) {
  bool succeed = false;
  int key = GetPathConstant(name);
  if (key >= 0)
    succeed = PathService::Override(key, path);
  if (!succeed)
    args->ThrowError("Failed to set path");
}

void App::SetDesktopName(const std::string& desktop_name) {
#if defined(OS_LINUX)
  scoped_ptr<base::Environment> env(base::Environment::Create());
  env->SetVar("CHROME_DESKTOP", desktop_name);
#endif
}

void App::AllowNTLMCredentialsForAllDomains(bool should_allow) {
  auto browser_context = static_cast<AtomBrowserContext*>(
        AtomBrowserMainParts::Get()->browser_context());
  browser_context->AllowNTLMCredentialsForAllDomains(should_allow);
}

std::string App::GetLocale() {
  return l10n_util::GetApplicationLocale("");
}

#if defined(OS_WIN)
bool App::IsAeroGlassEnabled() {
  return ui::win::IsAeroGlassEnabled();
}
#endif

bool App::MakeSingleInstance(
    const ProcessSingleton::NotificationCallback& callback) {
  if (process_singleton_.get())
    return false;

  base::FilePath user_dir;
  PathService::Get(brightray::DIR_USER_DATA, &user_dir);
  process_singleton_.reset(new ProcessSingleton(
      user_dir, base::Bind(NotificationCallbackWrapper, callback)));

  switch (process_singleton_->NotifyOtherProcessOrCreate()) {
    case ProcessSingleton::NotifyResult::LOCK_ERROR:
    case ProcessSingleton::NotifyResult::PROFILE_IN_USE:
    case ProcessSingleton::NotifyResult::PROCESS_NOTIFIED:
      process_singleton_.reset();
      return true;
    case ProcessSingleton::NotifyResult::PROCESS_NONE:
    default:  // Shouldn't be needed, but VS warns if it is not there.
      return false;
  }
}

mate::ObjectTemplateBuilder App::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  auto browser = base::Unretained(Browser::Get());
  return mate::ObjectTemplateBuilder(isolate)
      .SetMethod("quit", base::Bind(&Browser::Quit, browser))
      .SetMethod("exit", base::Bind(&Browser::Exit, browser))
      .SetMethod("focus", base::Bind(&Browser::Focus, browser))
      .SetMethod("getVersion", base::Bind(&Browser::GetVersion, browser))
      .SetMethod("setVersion", base::Bind(&Browser::SetVersion, browser))
      .SetMethod("getName", base::Bind(&Browser::GetName, browser))
      .SetMethod("setName", base::Bind(&Browser::SetName, browser))
      .SetMethod("isReady", base::Bind(&Browser::is_ready, browser))
      .SetMethod("addRecentDocument",
                 base::Bind(&Browser::AddRecentDocument, browser))
      .SetMethod("clearRecentDocuments",
                 base::Bind(&Browser::ClearRecentDocuments, browser))
      .SetMethod("setAppUserModelId",
                 base::Bind(&Browser::SetAppUserModelID, browser))
#if defined(OS_MACOSX)
      .SetMethod("hide", base::Bind(&Browser::Hide, browser))
      .SetMethod("show", base::Bind(&Browser::Show, browser))
      .SetMethod("isDarkMode",
                 base::Bind(&Browser::IsDarkMode, browser))
#endif
#if defined(OS_WIN)
      .SetMethod("setUserTasks",
                 base::Bind(&Browser::SetUserTasks, browser))
      .SetMethod("isAeroGlassEnabled", &App::IsAeroGlassEnabled)
#endif
      .SetMethod("setPath", &App::SetPath)
      .SetMethod("getPath", &App::GetPath)
      .SetMethod("setDesktopName", &App::SetDesktopName)
      .SetMethod("allowNTLMCredentialsForAllDomains",
                 &App::AllowNTLMCredentialsForAllDomains)
      .SetMethod("getLocale", &App::GetLocale)
      .SetMethod("makeSingleInstance", &App::MakeSingleInstance);
}

// static
mate::Handle<App> App::Create(v8::Isolate* isolate) {
  return CreateHandle(isolate, new App);
}

}  // namespace api

}  // namespace atom


namespace {

void AppendSwitch(const std::string& switch_string, mate::Arguments* args) {
  auto command_line = base::CommandLine::ForCurrentProcess();

  if (switch_string == atom::switches::kPpapiFlashPath ||
      switch_string == atom::switches::kClientCertificate ||
      switch_string == switches::kLogNetLog) {
    base::FilePath path;
    args->GetNext(&path);
    command_line->AppendSwitchPath(switch_string, path);
    return;
  }

  std::string value;
  if (args->GetNext(&value))
    command_line->AppendSwitchASCII(switch_string, value);
  else
    command_line->AppendSwitch(switch_string);
}

#if defined(OS_MACOSX)
int DockBounce(const std::string& type) {
  int request_id = -1;
  if (type == "critical")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_CRITICAL);
  else if (type == "informational")
    request_id = Browser::Get()->DockBounce(Browser::BOUNCE_INFORMATIONAL);
  return request_id;
}

void DockSetMenu(atom::api::Menu* menu) {
  Browser::Get()->DockSetMenu(menu->model());
}
#endif

void Initialize(v8::Local<v8::Object> exports, v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context, void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  auto command_line = base::CommandLine::ForCurrentProcess();

  mate::Dictionary dict(isolate, exports);
  dict.Set("app", atom::api::App::Create(isolate));
  dict.SetMethod("appendSwitch", &AppendSwitch);
  dict.SetMethod("appendArgument",
                 base::Bind(&base::CommandLine::AppendArg,
                            base::Unretained(command_line)));
#if defined(OS_MACOSX)
  auto browser = base::Unretained(Browser::Get());
  dict.SetMethod("dockBounce", &DockBounce);
  dict.SetMethod("dockCancelBounce",
                 base::Bind(&Browser::DockCancelBounce, browser));
  dict.SetMethod("dockSetBadgeText",
                 base::Bind(&Browser::DockSetBadgeText, browser));
  dict.SetMethod("dockGetBadgeText",
                 base::Bind(&Browser::DockGetBadgeText, browser));
  dict.SetMethod("dockHide", base::Bind(&Browser::DockHide, browser));
  dict.SetMethod("dockShow", base::Bind(&Browser::DockShow, browser));
  dict.SetMethod("dockSetMenu", &DockSetMenu);
  dict.SetMethod("dockSetIcon", base::Bind(&Browser::DockSetIcon, browser));
#endif
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_app, Initialize)
