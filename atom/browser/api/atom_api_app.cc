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
#include "atom/browser/relauncher.h"
#include "atom/common/atom_command_line.h"
#include "atom/common/native_mate_converters/callback.h"
#include "atom/common/native_mate_converters/file_path_converter.h"
#include "atom/common/native_mate_converters/gurl_converter.h"
#include "atom/common/native_mate_converters/image_converter.h"
#include "atom/common/native_mate_converters/net_converter.h"
#include "atom/common/native_mate_converters/value_converter.h"
#include "atom/common/node_includes.h"
#include "atom/common/options_switches.h"
#include "base/command_line.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "brightray/browser/brightray_paths.h"
#include "chrome/common/chrome_paths.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/client_certificate_delegate.h"
#include "content/public/browser/gpu_data_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"
#include "native_mate/dictionary.h"
#include "native_mate/object_template_builder.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/image/image.h"

#if defined(OS_WIN)
#include "atom/browser/ui/win/jump_list.h"
#include "base/strings/utf_string_conversions.h"
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

using atom::JumpListItem;
using atom::JumpListCategory;
using atom::JumpListResult;

template<>
struct Converter<JumpListItem::Type> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     JumpListItem::Type* out) {
    std::string item_type;
    if (!ConvertFromV8(isolate, val, &item_type))
      return false;

    if (item_type == "task")
      *out = JumpListItem::Type::TASK;
    else if (item_type == "separator")
      *out = JumpListItem::Type::SEPARATOR;
    else if (item_type == "file")
      *out = JumpListItem::Type::FILE;
    else
      return false;

    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   JumpListItem::Type val) {
    std::string item_type;
    switch (val) {
      case JumpListItem::Type::TASK:
        item_type = "task";
        break;

      case JumpListItem::Type::SEPARATOR:
        item_type = "separator";
        break;

      case JumpListItem::Type::FILE:
        item_type = "file";
        break;
    }
    return mate::ConvertToV8(isolate, item_type);
  }
};

template<>
struct Converter<JumpListItem> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     JumpListItem* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;

    if (!dict.Get("type", &(out->type)))
      return false;

    switch (out->type) {
      case JumpListItem::Type::TASK:
        if (!dict.Get("program", &(out->path)) ||
            !dict.Get("title", &(out->title)))
          return false;

        if (dict.Get("iconPath", &(out->icon_path)) &&
            !dict.Get("iconIndex", &(out->icon_index)))
          return false;

        dict.Get("args", &(out->arguments));
        dict.Get("description", &(out->description));
        return true;

      case JumpListItem::Type::SEPARATOR:
        return true;

      case JumpListItem::Type::FILE:
        return dict.Get("path", &(out->path));
    }

    assert(false);
    return false;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   const JumpListItem& val) {
    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    dict.Set("type", val.type);

    switch (val.type) {
      case JumpListItem::Type::TASK:
        dict.Set("program", val.path);
        dict.Set("args", val.arguments);
        dict.Set("title", val.title);
        dict.Set("iconPath", val.icon_path);
        dict.Set("iconIndex", val.icon_index);
        dict.Set("description", val.description);
        break;

      case JumpListItem::Type::SEPARATOR:
        break;

      case JumpListItem::Type::FILE:
        dict.Set("path", val.path);
        break;
    }
    return dict.GetHandle();
  }
};

template<>
struct Converter<JumpListCategory::Type> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     JumpListCategory::Type* out) {
    std::string category_type;
    if (!ConvertFromV8(isolate, val, &category_type))
      return false;

    if (category_type == "tasks")
      *out = JumpListCategory::Type::TASKS;
    else if (category_type == "frequent")
      *out = JumpListCategory::Type::FREQUENT;
    else if (category_type == "recent")
      *out = JumpListCategory::Type::RECENT;
    else if (category_type == "custom")
      *out = JumpListCategory::Type::CUSTOM;
    else
      return false;

    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   JumpListCategory::Type val) {
    std::string category_type;
    switch (val) {
      case JumpListCategory::Type::TASKS:
        category_type = "tasks";
        break;

      case JumpListCategory::Type::FREQUENT:
        category_type = "frequent";
        break;

      case JumpListCategory::Type::RECENT:
        category_type = "recent";
        break;

      case JumpListCategory::Type::CUSTOM:
        category_type = "custom";
        break;
    }
    return mate::ConvertToV8(isolate, category_type);
  }
};

template<>
struct Converter<JumpListCategory> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     JumpListCategory* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;

    if (dict.Get("name", &(out->name)) && out->name.empty())
      return false;

    if (!dict.Get("type", &(out->type))) {
      if (out->name.empty())
        out->type = JumpListCategory::Type::TASKS;
      else
        out->type = JumpListCategory::Type::CUSTOM;
    }

    if ((out->type == JumpListCategory::Type::TASKS) ||
        (out->type == JumpListCategory::Type::CUSTOM)) {
      if (!dict.Get("items", &(out->items)))
        return false;
    }

    return true;
  }
};

// static
template<>
struct Converter<JumpListResult> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate, JumpListResult val) {
    std::string result_code;
    switch (val) {
      case JumpListResult::SUCCESS:
        result_code = "ok";
        break;

      case JumpListResult::ARGUMENT_ERROR:
        result_code = "argumentError";
        break;

      case JumpListResult::GENERIC_ERROR:
        result_code = "error";
        break;

      case JumpListResult::CUSTOM_CATEGORY_SEPARATOR_ERROR:
        result_code = "invalidSeparatorError";
        break;

      case JumpListResult::MISSING_FILE_TYPE_REGISTRATION_ERROR:
        result_code = "fileTypeRegistrationError";
        break;

      case JumpListResult::CUSTOM_CATEGORY_ACCESS_DENIED_ERROR:
        result_code = "customCategoryAccessDeniedError";
        break;
    }
    return ConvertToV8(isolate, result_code);
  }
};
#endif

template<>
struct Converter<Browser::LoginItemSettings> {
  static bool FromV8(v8::Isolate* isolate, v8::Local<v8::Value> val,
                     Browser::LoginItemSettings* out) {
    mate::Dictionary dict;
    if (!ConvertFromV8(isolate, val, &dict))
      return false;

    dict.Get("openAtLogin", &(out->open_at_login));
    dict.Get("openAsHidden", &(out->open_as_hidden));
    return true;
  }

  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   Browser::LoginItemSettings val) {
    mate::Dictionary dict = mate::Dictionary::CreateEmpty(isolate);
    dict.Set("openAtLogin", val.open_at_login);
    dict.Set("openAsHidden", val.open_as_hidden);
    dict.Set("restoreState", val.restore_state);
    dict.Set("wasOpenedAtLogin", val.opened_at_login);
    dict.Set("wasOpenedAsHidden", val.opened_as_hidden);
    return dict.GetHandle();
  }
};
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
  else if (name == "pepperFlashSystemPlugin")
    return chrome::FILE_PEPPER_FLASH_SYSTEM_PLUGIN;
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

  std::string data;
  if (!cert_data.Get("data", &data))
    return;

  auto certs = net::X509Certificate::CreateCertificateListFromBytes(
      data.c_str(), data.length(), net::X509Certificate::FORMAT_AUTO);
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

#if defined(USE_NSS_CERTS)
int ImportIntoCertStore(
    CertificateManagerModel* model,
    const base::DictionaryValue& options) {
  std::string file_data, cert_path;
  base::string16 password;
  net::CertificateList imported_certs;
  int rv = -1;
  options.GetString("certificate", &cert_path);
  options.GetString("password", &password);

  if (!cert_path.empty()) {
    if (base::ReadFileToString(base::FilePath(cert_path), &file_data)) {
      auto module = model->cert_db()->GetPublicModule();
      rv = model->ImportFromPKCS12(module,
                                   file_data,
                                   password,
                                   true,
                                   &imported_certs);
      if (imported_certs.size() > 1) {
        auto it = imported_certs.begin();
        ++it;  // skip first which would  be the client certificate.
        for (; it != imported_certs.end(); ++it)
          rv &= model->SetCertTrust(it->get(),
                                    net::CA_CERT,
                                    net::NSSCertDatabase::TRUSTED_SSL);
      }
    }
  }
  return rv;
}
#endif

}  // namespace

App::App(v8::Isolate* isolate) {
  static_cast<AtomBrowserClient*>(AtomBrowserClient::Get())->set_delegate(this);
  Browser::Get()->AddObserver(this);
  content::GpuDataManager::GetInstance()->AddObserver(this);
  Init(isolate);
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

void App::OnFinishLaunching(const base::DictionaryValue& launch_info) {
  Emit("ready", launch_info);
}

void App::OnAccessibilitySupportChanged() {
  Emit("accessibility-support-changed", IsAccessibilitySupportEnabled());
}

#if defined(OS_MACOSX)
void App::OnContinueUserActivity(
    bool* prevent_default,
    const std::string& type,
    const base::DictionaryValue& user_info) {
  *prevent_default = Emit("continue-activity", type, user_info);
}
#endif

void App::OnLogin(LoginHandler* login_handler,
                  const base::DictionaryValue& request_details) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  bool prevent_default = Emit(
      "login",
      WebContents::CreateFrom(isolate(), login_handler->GetWebContents()),
      request_details,
      login_handler->auth_info(),
      base::Bind(&PassLoginInformation, make_scoped_refptr(login_handler)));

  // Default behavior is to always cancel the auth.
  if (!prevent_default)
    login_handler->CancelAuth();
}

void App::OnCreateWindow(const GURL& target_url,
                         const std::string& frame_name,
                         WindowOpenDisposition disposition,
                         int render_process_id,
                         int render_frame_id) {
  v8::Locker locker(isolate());
  v8::HandleScope handle_scope(isolate());
  content::RenderFrameHost* rfh =
      content::RenderFrameHost::FromID(render_process_id, render_frame_id);
  content::WebContents* web_contents =
      content::WebContents::FromRenderFrameHost(rfh);
  if (web_contents) {
    auto api_web_contents = WebContents::CreateFrom(isolate(), web_contents);
    api_web_contents->OnCreateWindow(target_url, frame_name, disposition);
  }
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
    std::unique_ptr<content::ClientCertificateDelegate> delegate) {
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

void App::OnGpuProcessCrashed(base::TerminationStatus status) {
  Emit("gpu-process-crashed",
    status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED);
}

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
  if (!path.IsAbsolute()) {
    args->ThrowError("path must be absolute");
    return;
  }

  bool succeed = false;
  int key = GetPathConstant(name);
  if (key >= 0)
    succeed = PathService::OverrideAndCreateIfNeeded(key, path, true, false);
  if (!succeed)
    args->ThrowError("Failed to set path");
}

void App::SetDesktopName(const std::string& desktop_name) {
#if defined(OS_LINUX)
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  env->SetVar("CHROME_DESKTOP", desktop_name);
#endif
}

std::string App::GetLocale() {
  return l10n_util::GetApplicationLocale("");
}

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

void App::ReleaseSingleInstance() {
  if (process_singleton_.get()) {
    process_singleton_->Cleanup();
    process_singleton_.reset();
  }
}

bool App::Relaunch(mate::Arguments* js_args) {
  // Parse parameters.
  bool override_argv = false;
  base::FilePath exec_path;
  relauncher::StringVector args;

  mate::Dictionary options;
  if (js_args->GetNext(&options)) {
    if (options.Get("execPath", &exec_path) | options.Get("args", &args))
      override_argv = true;
  }

  if (!override_argv) {
#if defined(OS_WIN)
    const relauncher::StringVector& argv = atom::AtomCommandLine::wargv();
#else
    const relauncher::StringVector& argv = atom::AtomCommandLine::argv();
#endif
    return relauncher::RelaunchApp(argv);
  }

  relauncher::StringVector argv;
  argv.reserve(1 + args.size());

  if (exec_path.empty()) {
    base::FilePath current_exe_path;
    PathService::Get(base::FILE_EXE, &current_exe_path);
    argv.push_back(current_exe_path.value());
  } else {
    argv.push_back(exec_path.value());
  }

  argv.insert(argv.end(), args.begin(), args.end());

  return relauncher::RelaunchApp(argv);
}

void App::DisableHardwareAcceleration(mate::Arguments* args) {
  if (Browser::Get()->is_ready()) {
    args->ThrowError("app.disableHardwareAcceleration() can only be called "
                     "before app is ready");
    return;
  }
  content::GpuDataManager::GetInstance()->DisableHardwareAcceleration();
}

bool App::IsAccessibilitySupportEnabled() {
  auto ax_state = content::BrowserAccessibilityState::GetInstance();
  return ax_state->IsAccessibleBrowser();
}

#if defined(USE_NSS_CERTS)
void App::ImportCertificate(
    const base::DictionaryValue& options,
    const net::CompletionCallback& callback) {
  auto browser_context = AtomBrowserContext::From("", false);
  if (!certificate_manager_model_) {
    std::unique_ptr<base::DictionaryValue> copy = options.CreateDeepCopy();
    CertificateManagerModel::Create(
        browser_context.get(),
        base::Bind(&App::OnCertificateManagerModelCreated,
                   base::Unretained(this),
                   base::Passed(&copy),
                   callback));
    return;
  }

  int rv = ImportIntoCertStore(certificate_manager_model_.get(), options);
  callback.Run(rv);
}

void App::OnCertificateManagerModelCreated(
    std::unique_ptr<base::DictionaryValue> options,
    const net::CompletionCallback& callback,
    std::unique_ptr<CertificateManagerModel> model) {
  certificate_manager_model_ = std::move(model);
  int rv = ImportIntoCertStore(certificate_manager_model_.get(),
                               *(options.get()));
  callback.Run(rv);
}
#endif

#if defined(OS_WIN)
v8::Local<v8::Value> App::GetJumpListSettings() {
  JumpList jump_list(Browser::Get()->GetAppUserModelID());

  int min_items = 10;
  std::vector<JumpListItem> removed_items;
  if (jump_list.Begin(&min_items, &removed_items)) {
    // We don't actually want to change anything, so abort the transaction.
    jump_list.Abort();
  } else {
    LOG(ERROR) << "Failed to begin Jump List transaction.";
  }

  auto dict = mate::Dictionary::CreateEmpty(isolate());
  dict.Set("minItems", min_items);
  dict.Set("removedItems", mate::ConvertToV8(isolate(), removed_items));
  return dict.GetHandle();
}

JumpListResult App::SetJumpList(v8::Local<v8::Value> val,
                                mate::Arguments* args) {
  std::vector<JumpListCategory> categories;
  bool delete_jump_list = val->IsNull();
  if (!delete_jump_list &&
    !mate::ConvertFromV8(args->isolate(), val, &categories)) {
    args->ThrowError("Argument must be null or an array of categories");
    return JumpListResult::ARGUMENT_ERROR;
  }

  JumpList jump_list(Browser::Get()->GetAppUserModelID());

  if (delete_jump_list) {
    return jump_list.Delete()
      ? JumpListResult::SUCCESS
      : JumpListResult::GENERIC_ERROR;
  }

  // Start a transaction that updates the JumpList of this application.
  if (!jump_list.Begin())
    return JumpListResult::GENERIC_ERROR;

  JumpListResult result = jump_list.AppendCategories(categories);
  // AppendCategories may have failed to add some categories, but it's better
  // to have something than nothing so try to commit the changes anyway.
  if (!jump_list.Commit()) {
    LOG(ERROR) << "Failed to commit changes to custom Jump List.";
    // It's more useful to return the earlier error code that might give
    // some indication as to why the transaction actually failed, so don't
    // overwrite it with a "generic error" code here.
    if (result == JumpListResult::SUCCESS)
      result = JumpListResult::GENERIC_ERROR;
  }

  return result;
}
#endif  // defined(OS_WIN)

// static
mate::Handle<App> App::Create(v8::Isolate* isolate) {
  return mate::CreateHandle(isolate, new App(isolate));
}

// static
void App::BuildPrototype(
    v8::Isolate* isolate, v8::Local<v8::FunctionTemplate> prototype) {
  prototype->SetClassName(mate::StringToV8(isolate, "App"));
  auto browser = base::Unretained(Browser::Get());
  mate::ObjectTemplateBuilder(isolate, prototype->PrototypeTemplate())
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
      .SetMethod("isDefaultProtocolClient",
                 base::Bind(&Browser::IsDefaultProtocolClient, browser))
      .SetMethod("setAsDefaultProtocolClient",
                 base::Bind(&Browser::SetAsDefaultProtocolClient, browser))
      .SetMethod("removeAsDefaultProtocolClient",
                 base::Bind(&Browser::RemoveAsDefaultProtocolClient, browser))
      .SetMethod("setBadgeCount", base::Bind(&Browser::SetBadgeCount, browser))
      .SetMethod("getBadgeCount", base::Bind(&Browser::GetBadgeCount, browser))
      .SetMethod("getLoginItemSettings",
                 base::Bind(&Browser::GetLoginItemSettings, browser))
      .SetMethod("setLoginItemSettings",
                 base::Bind(&Browser::SetLoginItemSettings, browser))
#if defined(OS_MACOSX)
      .SetMethod("hide", base::Bind(&Browser::Hide, browser))
      .SetMethod("show", base::Bind(&Browser::Show, browser))
      .SetMethod("setUserActivity",
                 base::Bind(&Browser::SetUserActivity, browser))
      .SetMethod("getCurrentActivityType",
                 base::Bind(&Browser::GetCurrentActivityType, browser))
#endif
#if defined(OS_WIN)
      .SetMethod("setUserTasks", base::Bind(&Browser::SetUserTasks, browser))
      .SetMethod("getJumpListSettings", &App::GetJumpListSettings)
      .SetMethod("setJumpList", &App::SetJumpList)
#endif
#if defined(OS_LINUX)
      .SetMethod("isUnityRunning",
                 base::Bind(&Browser::IsUnityRunning, browser))
#endif
      .SetMethod("setPath", &App::SetPath)
      .SetMethod("getPath", &App::GetPath)
      .SetMethod("setDesktopName", &App::SetDesktopName)
      .SetMethod("getLocale", &App::GetLocale)
#if defined(USE_NSS_CERTS)
      .SetMethod("importCertificate", &App::ImportCertificate)
#endif
      .SetMethod("makeSingleInstance", &App::MakeSingleInstance)
      .SetMethod("releaseSingleInstance", &App::ReleaseSingleInstance)
      .SetMethod("relaunch", &App::Relaunch)
      .SetMethod("isAccessibilitySupportEnabled",
                 &App::IsAccessibilitySupportEnabled)
      .SetMethod("disableHardwareAcceleration",
                 &App::DisableHardwareAcceleration);
}

}  // namespace api

}  // namespace atom


namespace {

void AppendSwitch(const std::string& switch_string, mate::Arguments* args) {
  auto command_line = base::CommandLine::ForCurrentProcess();

  if (base::EndsWith(switch_string, "-path",
                     base::CompareCase::INSENSITIVE_ASCII) ||
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
  dict.Set("App", atom::api::App::GetConstructor(isolate)->GetFunction());
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
  dict.SetMethod("dockDownloadFinished",
                 base::Bind(&Browser::DockDownloadFinished, browser));
  dict.SetMethod("dockSetBadgeText",
                 base::Bind(&Browser::DockSetBadgeText, browser));
  dict.SetMethod("dockGetBadgeText",
                 base::Bind(&Browser::DockGetBadgeText, browser));
  dict.SetMethod("dockHide", base::Bind(&Browser::DockHide, browser));
  dict.SetMethod("dockShow", base::Bind(&Browser::DockShow, browser));
  dict.SetMethod("dockIsVisible", base::Bind(&Browser::DockIsVisible, browser));
  dict.SetMethod("dockSetMenu", &DockSetMenu);
  dict.SetMethod("dockSetIcon", base::Bind(&Browser::DockSetIcon, browser));
#endif
}

}  // namespace

NODE_MODULE_CONTEXT_AWARE_BUILTIN(atom_browser_app, Initialize)
