// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#include <set>
#include <string>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/atom_javascript_dialog_manager.h"
#include "atom/browser/atom_security_state_model_client.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/web_dialog_helper.h"
#include "atom/common/atom_constants.h"
#include "base/files/file_util.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/security_style_explanation.h"
#include "content/public/browser/security_style_explanations.h"
#include "storage/browser/fileapi/isolated_context.h"

using content::BrowserThread;
using security_state::SecurityStateModel;

namespace atom {

namespace {

const char kRootName[] = "<root>";

struct FileSystem {
  FileSystem() {
  }
  FileSystem(const std::string& file_system_name,
             const std::string& root_url,
             const std::string& file_system_path)
    : file_system_name(file_system_name),
      root_url(root_url),
      file_system_path(file_system_path) {
  }

  std::string file_system_name;
  std::string root_url;
  std::string file_system_path;
};

std::string RegisterFileSystem(content::WebContents* web_contents,
                               const base::FilePath& path) {
  auto isolated_context = storage::IsolatedContext::GetInstance();
  std::string root_name(kRootName);
  std::string file_system_id = isolated_context->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal,
      std::string(),
      path,
      &root_name);

  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  content::RenderViewHost* render_view_host = web_contents->GetRenderViewHost();
  int renderer_id = render_view_host->GetProcess()->GetID();
  policy->GrantReadFileSystem(renderer_id, file_system_id);
  policy->GrantWriteFileSystem(renderer_id, file_system_id);
  policy->GrantCreateFileForFileSystem(renderer_id, file_system_id);
  policy->GrantDeleteFromFileSystem(renderer_id, file_system_id);

  if (!policy->CanReadFile(renderer_id, path))
    policy->GrantReadFile(renderer_id, path);

  return file_system_id;
}

FileSystem CreateFileSystemStruct(
    content::WebContents* web_contents,
    const std::string& file_system_id,
    const std::string& file_system_path) {
  const GURL origin = web_contents->GetURL().GetOrigin();
  std::string file_system_name =
      storage::GetIsolatedFileSystemName(origin, file_system_id);
  std::string root_url = storage::GetIsolatedFileSystemRootURIString(
      origin, file_system_id, kRootName);
  return FileSystem(file_system_name, root_url, file_system_path);
}

base::DictionaryValue* CreateFileSystemValue(const FileSystem& file_system) {
  auto* file_system_value = new base::DictionaryValue();
  file_system_value->SetString("fileSystemName", file_system.file_system_name);
  file_system_value->SetString("rootURL", file_system.root_url);
  file_system_value->SetString("fileSystemPath", file_system.file_system_path);
  return file_system_value;
}

void WriteToFile(const base::FilePath& path,
                 const std::string& content) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  DCHECK(!path.empty());

  base::WriteFile(path, content.data(), content.size());
}

void AppendToFile(const base::FilePath& path,
                  const std::string& content) {
  DCHECK_CURRENTLY_ON(BrowserThread::FILE);
  DCHECK(!path.empty());

  base::AppendToFile(path, content.data(), content.size());
}

PrefService* GetPrefService(content::WebContents* web_contents) {
  auto context = web_contents->GetBrowserContext();
  return static_cast<atom::AtomBrowserContext*>(context)->prefs();
}

std::set<std::string> GetAddedFileSystemPaths(
    content::WebContents* web_contents) {
  auto pref_service = GetPrefService(web_contents);
  const base::DictionaryValue* file_system_paths_value =
      pref_service->GetDictionary(prefs::kDevToolsFileSystemPaths);
  std::set<std::string> result;
  if (file_system_paths_value) {
    base::DictionaryValue::Iterator it(*file_system_paths_value);
    for (; !it.IsAtEnd(); it.Advance()) {
      result.insert(it.key());
    }
  }
  return result;
}

bool IsDevToolsFileSystemAdded(
    content::WebContents* web_contents,
    const std::string& file_system_path) {
  auto file_system_paths = GetAddedFileSystemPaths(web_contents);
  return file_system_paths.find(file_system_path) != file_system_paths.end();
}

content::SecurityStyle SecurityLevelToSecurityStyle(
    SecurityStateModel::SecurityLevel security_level) {
  switch (security_level) {
    case SecurityStateModel::NONE:
      return content::SECURITY_STYLE_UNAUTHENTICATED;
    case SecurityStateModel::SECURITY_WARNING:
    case SecurityStateModel::SECURITY_POLICY_WARNING:
      return content::SECURITY_STYLE_WARNING;
    case SecurityStateModel::EV_SECURE:
    case SecurityStateModel::SECURE:
      return content::SECURITY_STYLE_AUTHENTICATED;
    case SecurityStateModel::SECURITY_ERROR:
      return content::SECURITY_STYLE_AUTHENTICATION_BROKEN;
  }

  return content::SECURITY_STYLE_UNKNOWN;
}

}  // namespace

CommonWebContentsDelegate::CommonWebContentsDelegate()
    : html_fullscreen_(false),
      native_fullscreen_(false),
      devtools_file_system_indexer_(new DevToolsFileSystemIndexer) {
}

CommonWebContentsDelegate::~CommonWebContentsDelegate() {
}

void CommonWebContentsDelegate::InitWithWebContents(
    content::WebContents* web_contents,
    AtomBrowserContext* browser_context) {
  browser_context_ = browser_context;
  web_contents->SetDelegate(this);

  printing::PrintViewManagerBasic::CreateForWebContents(web_contents);
  printing::PrintPreviewMessageHandler::CreateForWebContents(web_contents);

  // Create InspectableWebContents.
  web_contents_.reset(brightray::InspectableWebContents::Create(web_contents));
  web_contents_->SetDelegate(this);
}

void CommonWebContentsDelegate::SetOwnerWindow(NativeWindow* owner_window) {
  SetOwnerWindow(GetWebContents(), owner_window);
}

void CommonWebContentsDelegate::SetOwnerWindow(
    content::WebContents* web_contents, NativeWindow* owner_window) {
  owner_window_ = owner_window->GetWeakPtr();
  NativeWindowRelay* relay = new NativeWindowRelay(owner_window_);
  web_contents->SetUserData(relay->key, relay);
}

void CommonWebContentsDelegate::DestroyWebContents() {
  web_contents_.reset();
}

content::WebContents* CommonWebContentsDelegate::GetWebContents() const {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetWebContents();
}

content::WebContents*
CommonWebContentsDelegate::GetDevToolsWebContents() const {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetDevToolsWebContents();
}

content::WebContents* CommonWebContentsDelegate::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  content::NavigationController::LoadURLParams load_url_params(params.url);
  load_url_params.referrer = params.referrer;
  load_url_params.transition_type = params.transition;
  load_url_params.extra_headers = params.extra_headers;
  load_url_params.should_replace_current_entry =
      params.should_replace_current_entry;
  load_url_params.is_renderer_initiated = params.is_renderer_initiated;
  load_url_params.should_clear_history_list = true;

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

bool CommonWebContentsDelegate::CanOverscrollContent() const {
  return false;
}

content::JavaScriptDialogManager*
CommonWebContentsDelegate::GetJavaScriptDialogManager(
    content::WebContents* source) {
  if (!dialog_manager_)
    dialog_manager_.reset(new AtomJavaScriptDialogManager);

  return dialog_manager_.get();
}

content::ColorChooser* CommonWebContentsDelegate::OpenColorChooser(
    content::WebContents* web_contents,
    SkColor color,
    const std::vector<content::ColorSuggestion>& suggestions) {
  return chrome::ShowColorChooser(web_contents, color);
}

void CommonWebContentsDelegate::RunFileChooser(
    content::WebContents* guest,
    const content::FileChooserParams& params) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(owner_window()));
  web_dialog_helper_->RunFileChooser(guest, params);
}

void CommonWebContentsDelegate::EnumerateDirectory(content::WebContents* guest,
                                                   int request_id,
                                                   const base::FilePath& path) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(owner_window()));
  web_dialog_helper_->EnumerateDirectory(guest, request_id, path);
}

void CommonWebContentsDelegate::EnterFullscreenModeForTab(
    content::WebContents* source, const GURL& origin) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(true);
  owner_window_->NotifyWindowEnterHtmlFullScreen();
  source->GetRenderViewHost()->GetWidget()->WasResized();
}

void CommonWebContentsDelegate::ExitFullscreenModeForTab(
    content::WebContents* source) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(false);
  owner_window_->NotifyWindowLeaveHtmlFullScreen();
  source->GetRenderViewHost()->GetWidget()->WasResized();
}

bool CommonWebContentsDelegate::IsFullscreenForTabOrPending(
    const content::WebContents* source) const {
  return html_fullscreen_;
}

content::SecurityStyle CommonWebContentsDelegate::GetSecurityStyle(
    content::WebContents* web_contents,
    content::SecurityStyleExplanations* explanations) {
  auto model_client =
      AtomSecurityStateModelClient::FromWebContents(web_contents);

  const SecurityStateModel::SecurityInfo& security_info =
      model_client->GetSecurityInfo();

  const content::SecurityStyle security_style =
      SecurityLevelToSecurityStyle(security_info.security_level);

  explanations->ran_insecure_content_style =
      SecurityLevelToSecurityStyle(
          SecurityStateModel::kRanInsecureContentLevel);
  explanations->displayed_insecure_content_style =
      SecurityLevelToSecurityStyle(
          SecurityStateModel::kDisplayedInsecureContentLevel);

  explanations->scheme_is_cryptographic = security_info.scheme_is_cryptographic;
  if (!security_info.scheme_is_cryptographic)
    return security_style;

  if (security_info.sha1_deprecation_status ==
      SecurityStateModel::DEPRECATED_SHA1_MAJOR) {
    explanations->broken_explanations.push_back(
        content::SecurityStyleExplanation(
            kSHA1Certificate,
            kSHA1MajorDescription,
            security_info.cert_id));
  } else if (security_info.sha1_deprecation_status ==
                SecurityStateModel::DEPRECATED_SHA1_MINOR) {
    explanations->unauthenticated_explanations.push_back(
        content::SecurityStyleExplanation(
            kSHA1Certificate,
            kSHA1MinorDescription,
            security_info.cert_id));
  }

  explanations->ran_insecure_content =
      security_info.mixed_content_status ==
          SecurityStateModel::RAN_MIXED_CONTENT ||
      security_info.mixed_content_status ==
          SecurityStateModel::RAN_AND_DISPLAYED_MIXED_CONTENT;
  explanations->displayed_insecure_content =
      security_info.mixed_content_status ==
          SecurityStateModel::DISPLAYED_MIXED_CONTENT ||
      security_info.mixed_content_status ==
          SecurityStateModel::RAN_AND_DISPLAYED_MIXED_CONTENT;

  if (net::IsCertStatusError(security_info.cert_status)) {
    std::string error_string = net::ErrorToString(
        net::MapCertStatusToNetError(security_info.cert_status));

    content::SecurityStyleExplanation explanation(
        kCertificateError,
        "There are issues with the site's certificate chain " + error_string,
        security_info.cert_id);

    if (net::IsCertStatusMinorError(security_info.cert_status))
      explanations->unauthenticated_explanations.push_back(explanation);
    else
      explanations->broken_explanations.push_back(explanation);
  } else {
    if (security_info.sha1_deprecation_status ==
        SecurityStateModel::NO_DEPRECATED_SHA1) {
      explanations->secure_explanations.push_back(
          content::SecurityStyleExplanation(
              kValidCertificate,
              kValidCertificateDescription,
              security_info.cert_id));
    }
  }

  if (security_info.is_secure_protocol_and_ciphersuite) {
    explanations->secure_explanations.push_back(
        content::SecurityStyleExplanation(
            kSecureProtocol,
            kSecureProtocolDescription));
  }

  return security_style;
}

void CommonWebContentsDelegate::DevToolsSaveToFile(
    const std::string& url, const std::string& content, bool save_as) {
  base::FilePath path;
  auto it = saved_files_.find(url);
  if (it != saved_files_.end() && !save_as) {
    path = it->second;
  } else {
    file_dialog::Filters filters;
    base::FilePath default_path(base::FilePath::FromUTF8Unsafe(url));
    if (!file_dialog::ShowSaveDialog(owner_window(), url, "", default_path,
                                     filters, &path)) {
      base::StringValue url_value(url);
      web_contents_->CallClientFunction(
          "DevToolsAPI.canceledSaveURL", &url_value, nullptr, nullptr);
      return;
    }
  }

  saved_files_[url] = path;
  BrowserThread::PostTaskAndReply(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&WriteToFile, path, content),
      base::Bind(&CommonWebContentsDelegate::OnDevToolsSaveToFile,
                 base::Unretained(this), url));
}

void CommonWebContentsDelegate::DevToolsAppendToFile(
    const std::string& url, const std::string& content) {
  auto it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;

  BrowserThread::PostTaskAndReply(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&AppendToFile, it->second, content),
      base::Bind(&CommonWebContentsDelegate::OnDevToolsAppendToFile,
                 base::Unretained(this), url));
}

void CommonWebContentsDelegate::DevToolsRequestFileSystems() {
  auto file_system_paths = GetAddedFileSystemPaths(GetDevToolsWebContents());
  if (file_system_paths.empty()) {
    base::ListValue empty_file_system_value;
    web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                      &empty_file_system_value,
                                      nullptr, nullptr);
    return;
  }

  std::vector<FileSystem> file_systems;
  for (auto file_system_path : file_system_paths) {
    base::FilePath path = base::FilePath::FromUTF8Unsafe(file_system_path);
    std::string file_system_id = RegisterFileSystem(GetDevToolsWebContents(),
                                                    path);
    FileSystem file_system = CreateFileSystemStruct(GetDevToolsWebContents(),
                                                    file_system_id,
                                                    file_system_path);
    file_systems.push_back(file_system);
  }

  base::ListValue file_system_value;
  for (auto& file_system : file_systems)
    file_system_value.Append(CreateFileSystemValue(file_system));
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                    &file_system_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsAddFileSystem(
    const base::FilePath& file_system_path) {
  base::FilePath path = file_system_path;
  if (path.empty()) {
    file_dialog::Filters filters;
    base::FilePath default_path;
    std::vector<base::FilePath> paths;
    int flag = file_dialog::FILE_DIALOG_OPEN_DIRECTORY;
    if (!file_dialog::ShowOpenDialog(owner_window(), "", "", default_path,
                                     filters, flag, &paths))
      return;

    path = paths[0];
  }

  std::string file_system_id = RegisterFileSystem(GetDevToolsWebContents(),
                                                  path);
  if (IsDevToolsFileSystemAdded(GetDevToolsWebContents(), path.AsUTF8Unsafe()))
    return;

  FileSystem file_system = CreateFileSystemStruct(GetDevToolsWebContents(),
                                                 file_system_id,
                                                 path.AsUTF8Unsafe());
  std::unique_ptr<base::DictionaryValue> file_system_value(
      CreateFileSystemValue(file_system));

  auto pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->SetWithoutPathExpansion(
      path.AsUTF8Unsafe(), base::Value::CreateNullValue());

  web_contents_->CallClientFunction("DevToolsAPI.fileSystemAdded",
                                    file_system_value.get(),
                                    nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsRemoveFileSystem(
    const base::FilePath& file_system_path) {
  if (!web_contents_)
    return;

  std::string path = file_system_path.AsUTF8Unsafe();
  storage::IsolatedContext::GetInstance()->
      RevokeFileSystemByPath(file_system_path);

  auto pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->RemoveWithoutPathExpansion(path, nullptr);

  base::StringValue file_system_path_value(path);
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemRemoved",
                                    &file_system_path_value,
                                    nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsIndexPath(
    int request_id,
    const std::string& file_system_path) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsIndexingDone(request_id, file_system_path);
    return;
  }
  if (devtools_indexing_jobs_.count(request_id) != 0)
    return;
  devtools_indexing_jobs_[request_id] =
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>(
          devtools_file_system_indexer_->IndexPath(
              file_system_path,
              base::Bind(
                  &CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated,
                  base::Unretained(this),
                  request_id,
                  file_system_path),
              base::Bind(&CommonWebContentsDelegate::OnDevToolsIndexingWorked,
                         base::Unretained(this),
                         request_id,
                         file_system_path),
              base::Bind(&CommonWebContentsDelegate::OnDevToolsIndexingDone,
                         base::Unretained(this),
                         request_id,
                         file_system_path)));
}

void CommonWebContentsDelegate::DevToolsStopIndexing(int request_id) {
  auto it = devtools_indexing_jobs_.find(request_id);
  if (it == devtools_indexing_jobs_.end())
    return;
  it->second->Stop();
  devtools_indexing_jobs_.erase(it);
}

void CommonWebContentsDelegate::DevToolsSearchInPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& query) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsSearchCompleted(request_id,
                              file_system_path,
                              std::vector<std::string>());
    return;
  }
  devtools_file_system_indexer_->SearchInPath(
      file_system_path,
      query,
      base::Bind(&CommonWebContentsDelegate::OnDevToolsSearchCompleted,
                 base::Unretained(this),
                 request_id,
                 file_system_path));
}

void CommonWebContentsDelegate::OnDevToolsSaveToFile(
    const std::string& url) {
  // Notify DevTools.
  base::StringValue url_value(url);
  web_contents_->CallClientFunction(
      "DevToolsAPI.savedURL", &url_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::OnDevToolsAppendToFile(
    const std::string& url) {
  // Notify DevTools.
  base::StringValue url_value(url);
  web_contents_->CallClientFunction(
      "DevToolsAPI.appendedToURL", &url_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated(
    int request_id,
    const std::string& file_system_path,
    int total_work) {
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  base::FundamentalValue total_work_value(total_work);
  web_contents_->CallClientFunction("DevToolsAPI.indexingTotalWorkCalculated",
                                    &request_id_value,
                                    &file_system_path_value,
                                    &total_work_value);
}

void CommonWebContentsDelegate::OnDevToolsIndexingWorked(
    int request_id,
    const std::string& file_system_path,
    int worked) {
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  base::FundamentalValue worked_value(worked);
  web_contents_->CallClientFunction("DevToolsAPI.indexingWorked",
                                    &request_id_value,
                                    &file_system_path_value,
                                    &worked_value);
}

void CommonWebContentsDelegate::OnDevToolsIndexingDone(
    int request_id,
    const std::string& file_system_path) {
  devtools_indexing_jobs_.erase(request_id);
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  web_contents_->CallClientFunction("DevToolsAPI.indexingDone",
                                    &request_id_value,
                                    &file_system_path_value,
                                    nullptr);
}

void CommonWebContentsDelegate::OnDevToolsSearchCompleted(
    int request_id,
    const std::string& file_system_path,
    const std::vector<std::string>& file_paths) {
  base::ListValue file_paths_value;
  for (const auto& file_path : file_paths) {
    file_paths_value.AppendString(file_path);
  }
  base::FundamentalValue request_id_value(request_id);
  base::StringValue file_system_path_value(file_system_path);
  web_contents_->CallClientFunction("DevToolsAPI.searchCompleted",
                                    &request_id_value,
                                    &file_system_path_value,
                                    &file_paths_value);
}

void CommonWebContentsDelegate::SetHtmlApiFullscreen(bool enter_fullscreen) {
  // Window is already in fullscreen mode, save the state.
  if (enter_fullscreen && owner_window_->IsFullscreen()) {
    native_fullscreen_ = true;
    html_fullscreen_ = true;
    return;
  }

  // Exit html fullscreen state but not window's fullscreen mode.
  if (!enter_fullscreen && native_fullscreen_) {
    html_fullscreen_ = false;
    return;
  }

  owner_window_->SetFullScreen(enter_fullscreen);
  html_fullscreen_ = enter_fullscreen;
  native_fullscreen_ = false;
}

}  // namespace atom
