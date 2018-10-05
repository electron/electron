// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/browser/web_dialog_helper.h"
#include "atom/common/atom_constants.h"
#include "atom/common/options_switches.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/security_state/content/content_utils.h"
#include "components/security_state/core/security_state.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/security_style_explanation.h"
#include "content/public/browser/security_style_explanations.h"
#include "storage/browser/fileapi/isolated_context.h"

using content::BrowserThread;

namespace atom {

namespace {

const char kRootName[] = "<root>";

struct FileSystem {
  FileSystem() {}
  FileSystem(const std::string& type,
             const std::string& file_system_name,
             const std::string& root_url,
             const std::string& file_system_path)
      : type(type),
        file_system_name(file_system_name),
        root_url(root_url),
        file_system_path(file_system_path) {}

  std::string type;
  std::string file_system_name;
  std::string root_url;
  std::string file_system_path;
};

std::string RegisterFileSystem(content::WebContents* web_contents,
                               const base::FilePath& path) {
  auto* isolated_context = storage::IsolatedContext::GetInstance();
  std::string root_name(kRootName);
  std::string file_system_id = isolated_context->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal, std::string(), path, &root_name);

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

FileSystem CreateFileSystemStruct(content::WebContents* web_contents,
                                  const std::string& file_system_id,
                                  const std::string& file_system_path,
                                  const std::string& type) {
  const GURL origin = web_contents->GetURL().GetOrigin();
  std::string file_system_name =
      storage::GetIsolatedFileSystemName(origin, file_system_id);
  std::string root_url = storage::GetIsolatedFileSystemRootURIString(
      origin, file_system_id, kRootName);
  return FileSystem(type, file_system_name, root_url, file_system_path);
}

std::unique_ptr<base::DictionaryValue> CreateFileSystemValue(
    const FileSystem& file_system) {
  std::unique_ptr<base::DictionaryValue> file_system_value(
      new base::DictionaryValue());
  file_system_value->SetString("type", file_system.type);
  file_system_value->SetString("fileSystemName", file_system.file_system_name);
  file_system_value->SetString("rootURL", file_system.root_url);
  file_system_value->SetString("fileSystemPath", file_system.file_system_path);
  return file_system_value;
}

void WriteToFile(const base::FilePath& path, const std::string& content) {
  base::AssertBlockingAllowed();
  DCHECK(!path.empty());

  base::WriteFile(path, content.data(), content.size());
}

void AppendToFile(const base::FilePath& path, const std::string& content) {
  base::AssertBlockingAllowed();
  DCHECK(!path.empty());

  base::AppendToFile(path, content.data(), content.size());
}

PrefService* GetPrefService(content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  return static_cast<atom::AtomBrowserContext*>(context)->prefs();
}

std::map<std::string, std::string> GetAddedFileSystemPaths(
    content::WebContents* web_contents) {
  auto* pref_service = GetPrefService(web_contents);
  const base::DictionaryValue* file_system_paths_value =
      pref_service->GetDictionary(prefs::kDevToolsFileSystemPaths);
  std::map<std::string, std::string> result;
  if (file_system_paths_value) {
    base::DictionaryValue::Iterator it(*file_system_paths_value);
    for (; !it.IsAtEnd(); it.Advance()) {
      std::string type =
          it.value().is_string() ? it.value().GetString() : std::string();
      result[it.key()] = type;
    }
  }
  return result;
}

bool IsDevToolsFileSystemAdded(content::WebContents* web_contents,
                               const std::string& file_system_path) {
  auto file_system_paths = GetAddedFileSystemPaths(web_contents);
  return file_system_paths.find(file_system_path) != file_system_paths.end();
}

}  // namespace

CommonWebContentsDelegate::CommonWebContentsDelegate()
    : devtools_file_system_indexer_(new DevToolsFileSystemIndexer),
      file_task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      weak_factory_(this) {}

CommonWebContentsDelegate::~CommonWebContentsDelegate() {}

void CommonWebContentsDelegate::InitWithWebContents(
    content::WebContents* web_contents,
    AtomBrowserContext* browser_context,
    bool is_guest) {
  browser_context_ = browser_context;
  web_contents->SetDelegate(this);

  printing::PrintViewManagerBasic::CreateForWebContents(web_contents);
  printing::PrintPreviewMessageHandler::CreateForWebContents(web_contents);

  // Determien whether the WebContents is offscreen.
  auto* web_preferences = WebContentsPreferences::From(web_contents);
  offscreen_ =
      !web_preferences || web_preferences->IsEnabled(options::kOffscreen);

  // Create InspectableWebContents.
  web_contents_.reset(brightray::InspectableWebContents::Create(
      web_contents, browser_context->prefs(), is_guest));
  web_contents_->SetDelegate(this);
}

void CommonWebContentsDelegate::SetOwnerWindow(NativeWindow* owner_window) {
  SetOwnerWindow(GetWebContents(), owner_window);
}

void CommonWebContentsDelegate::SetOwnerWindow(
    content::WebContents* web_contents,
    NativeWindow* owner_window) {
  if (owner_window) {
    owner_window_ = owner_window->GetWeakPtr();
#if defined(TOOLKIT_VIEWS) && !defined(OS_MACOSX)
    autofill_popup_.reset(new AutofillPopup());
#endif
    NativeWindowRelay::CreateForWebContents(web_contents,
                                            owner_window->GetWeakPtr());
  } else {
    owner_window_ = nullptr;
    web_contents->RemoveUserData(
        NativeWindowRelay::kNativeWindowRelayUserDataKey);
  }
}

void CommonWebContentsDelegate::ResetManagedWebContents(bool async) {
  if (async) {
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE,
                                                    web_contents_.release());
  } else {
    web_contents_.reset();
  }
}

content::WebContents* CommonWebContentsDelegate::GetWebContents() const {
  if (!web_contents_)
    return nullptr;
  return web_contents_->GetWebContents();
}

content::WebContents* CommonWebContentsDelegate::GetDevToolsWebContents()
    const {
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

content::ColorChooser* CommonWebContentsDelegate::OpenColorChooser(
    content::WebContents* web_contents,
    SkColor color,
    const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions) {
  return chrome::ShowColorChooser(web_contents, color);
}

void CommonWebContentsDelegate::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const content::FileChooserParams& params) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(owner_window(), offscreen_));
  web_dialog_helper_->RunFileChooser(render_frame_host, params);
}

void CommonWebContentsDelegate::EnumerateDirectory(content::WebContents* guest,
                                                   int request_id,
                                                   const base::FilePath& path) {
  if (!web_dialog_helper_)
    web_dialog_helper_.reset(new WebDialogHelper(owner_window(), offscreen_));
  web_dialog_helper_->EnumerateDirectory(guest, request_id, path);
}

void CommonWebContentsDelegate::EnterFullscreenModeForTab(
    content::WebContents* source,
    const GURL& origin,
    const blink::WebFullscreenOptions& options) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(true);
  owner_window_->NotifyWindowEnterHtmlFullScreen();
  source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
}

void CommonWebContentsDelegate::ExitFullscreenModeForTab(
    content::WebContents* source) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(false);
  owner_window_->NotifyWindowLeaveHtmlFullScreen();
  source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
}

bool CommonWebContentsDelegate::IsFullscreenForTabOrPending(
    const content::WebContents* source) const {
  return html_fullscreen_;
}

blink::WebSecurityStyle CommonWebContentsDelegate::GetSecurityStyle(
    content::WebContents* web_contents,
    content::SecurityStyleExplanations* security_style_explanations) {
  SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  DCHECK(helper);
  security_state::SecurityInfo security_info;
  helper->GetSecurityInfo(&security_info);
  return security_state::GetSecurityStyle(security_info,
                                          security_style_explanations);
}

void CommonWebContentsDelegate::DevToolsSaveToFile(const std::string& url,
                                                   const std::string& content,
                                                   bool save_as) {
  base::FilePath path;
  auto it = saved_files_.find(url);
  if (it != saved_files_.end() && !save_as) {
    path = it->second;
  } else {
    file_dialog::DialogSettings settings;
    settings.parent_window = owner_window();
    settings.force_detached = offscreen_;
    settings.title = url;
    settings.default_path = base::FilePath::FromUTF8Unsafe(url);
    if (!file_dialog::ShowSaveDialog(settings, &path)) {
      base::Value url_value(url);
      web_contents_->CallClientFunction("DevToolsAPI.canceledSaveURL",
                                        &url_value, nullptr, nullptr);
      return;
    }
  }

  saved_files_[url] = path;
  // Notify DevTools.
  base::Value url_value(url);
  base::Value file_system_path_value(path.AsUTF8Unsafe());
  web_contents_->CallClientFunction("DevToolsAPI.savedURL", &url_value,
                                    &file_system_path_value, nullptr);
  file_task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(&WriteToFile, path, content));
}

void CommonWebContentsDelegate::DevToolsAppendToFile(
    const std::string& url,
    const std::string& content) {
  auto it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;

  // Notify DevTools.
  base::Value url_value(url);
  web_contents_->CallClientFunction("DevToolsAPI.appendedToURL", &url_value,
                                    nullptr, nullptr);
  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AppendToFile, it->second, content));
}

void CommonWebContentsDelegate::DevToolsRequestFileSystems() {
  auto file_system_paths = GetAddedFileSystemPaths(GetDevToolsWebContents());
  if (file_system_paths.empty()) {
    base::ListValue empty_file_system_value;
    web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                      &empty_file_system_value, nullptr,
                                      nullptr);
    return;
  }

  std::vector<FileSystem> file_systems;
  for (const auto& file_system_path : file_system_paths) {
    base::FilePath path =
        base::FilePath::FromUTF8Unsafe(file_system_path.first);
    std::string file_system_id =
        RegisterFileSystem(GetDevToolsWebContents(), path);
    FileSystem file_system =
        CreateFileSystemStruct(GetDevToolsWebContents(), file_system_id,
                               file_system_path.first, file_system_path.second);
    file_systems.push_back(file_system);
  }

  base::ListValue file_system_value;
  for (const auto& file_system : file_systems)
    file_system_value.Append(CreateFileSystemValue(file_system));
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemsLoaded",
                                    &file_system_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsAddFileSystem(
    const std::string& type,
    const base::FilePath& file_system_path) {
  base::FilePath path = file_system_path;
  if (path.empty()) {
    std::vector<base::FilePath> paths;
    file_dialog::DialogSettings settings;
    settings.parent_window = owner_window();
    settings.force_detached = offscreen_;
    settings.properties = file_dialog::FILE_DIALOG_OPEN_DIRECTORY;
    if (!file_dialog::ShowOpenDialog(settings, &paths))
      return;

    path = paths[0];
  }

  std::string file_system_id =
      RegisterFileSystem(GetDevToolsWebContents(), path);
  if (IsDevToolsFileSystemAdded(GetDevToolsWebContents(), path.AsUTF8Unsafe()))
    return;

  FileSystem file_system = CreateFileSystemStruct(
      GetDevToolsWebContents(), file_system_id, path.AsUTF8Unsafe(), type);
  std::unique_ptr<base::DictionaryValue> file_system_value(
      CreateFileSystemValue(file_system));

  auto* pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->SetWithoutPathExpansion(path.AsUTF8Unsafe(),
                                        std::make_unique<base::Value>(type));
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemAdded", nullptr,
                                    file_system_value.get(), nullptr);
}

void CommonWebContentsDelegate::DevToolsRemoveFileSystem(
    const base::FilePath& file_system_path) {
  if (!web_contents_)
    return;

  std::string path = file_system_path.AsUTF8Unsafe();
  storage::IsolatedContext::GetInstance()->RevokeFileSystemByPath(
      file_system_path);

  auto* pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->RemoveWithoutPathExpansion(path, nullptr);

  base::Value file_system_path_value(path);
  web_contents_->CallClientFunction("DevToolsAPI.fileSystemRemoved",
                                    &file_system_path_value, nullptr, nullptr);
}

void CommonWebContentsDelegate::DevToolsIndexPath(
    int request_id,
    const std::string& file_system_path,
    const std::string& excluded_folders_message) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsIndexingDone(request_id, file_system_path);
    return;
  }
  if (devtools_indexing_jobs_.count(request_id) != 0)
    return;
  std::vector<std::string> excluded_folders;
  std::unique_ptr<base::Value> parsed_excluded_folders =
      base::JSONReader::Read(excluded_folders_message);
  if (parsed_excluded_folders && parsed_excluded_folders->is_list()) {
    const std::vector<base::Value>& folder_paths =
        parsed_excluded_folders->GetList();
    for (const base::Value& folder_path : folder_paths) {
      if (folder_path.is_string())
        excluded_folders.push_back(folder_path.GetString());
    }
  }
  devtools_indexing_jobs_[request_id] =
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>(
          devtools_file_system_indexer_->IndexPath(
              file_system_path, excluded_folders,
              base::Bind(
                  &CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated,
                  weak_factory_.GetWeakPtr(), request_id, file_system_path),
              base::Bind(&CommonWebContentsDelegate::OnDevToolsIndexingWorked,
                         weak_factory_.GetWeakPtr(), request_id,
                         file_system_path),
              base::Bind(&CommonWebContentsDelegate::OnDevToolsIndexingDone,
                         weak_factory_.GetWeakPtr(), request_id,
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
    OnDevToolsSearchCompleted(request_id, file_system_path,
                              std::vector<std::string>());
    return;
  }
  devtools_file_system_indexer_->SearchInPath(
      file_system_path, query,
      base::Bind(&CommonWebContentsDelegate::OnDevToolsSearchCompleted,
                 weak_factory_.GetWeakPtr(), request_id, file_system_path));
}

void CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated(
    int request_id,
    const std::string& file_system_path,
    int total_work) {
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  base::Value total_work_value(total_work);
  web_contents_->CallClientFunction("DevToolsAPI.indexingTotalWorkCalculated",
                                    &request_id_value, &file_system_path_value,
                                    &total_work_value);
}

void CommonWebContentsDelegate::OnDevToolsIndexingWorked(
    int request_id,
    const std::string& file_system_path,
    int worked) {
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  base::Value worked_value(worked);
  web_contents_->CallClientFunction("DevToolsAPI.indexingWorked",
                                    &request_id_value, &file_system_path_value,
                                    &worked_value);
}

void CommonWebContentsDelegate::OnDevToolsIndexingDone(
    int request_id,
    const std::string& file_system_path) {
  devtools_indexing_jobs_.erase(request_id);
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  web_contents_->CallClientFunction("DevToolsAPI.indexingDone",
                                    &request_id_value, &file_system_path_value,
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
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  web_contents_->CallClientFunction("DevToolsAPI.searchCompleted",
                                    &request_id_value, &file_system_path_value,
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
