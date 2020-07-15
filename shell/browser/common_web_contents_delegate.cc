// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/common_web_contents_delegate.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/browser/ssl/security_state_tab_helper.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/security_state/content/content_utils.h"
#include "components/security_state/core/security_state.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"  // nogncheck
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/security_style_explanation.h"
#include "content/public/browser/security_style_explanations.h"
#include "printing/buildflags/buildflags.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/web_dialog_helper.h"
#include "shell/common/electron_constants.h"
#include "shell/common/options_switches.h"
#include "storage/browser/file_system/isolated_context.h"

#if BUILDFLAG(ENABLE_COLOR_CHOOSER)
#include "chrome/browser/ui/color_chooser.h"
#endif

#if BUILDFLAG(ENABLE_OSR)
#include "shell/browser/osr/osr_web_contents_view.h"
#endif

#if BUILDFLAG(ENABLE_PRINTING)
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "components/printing/browser/print_manager_utils.h"
#include "shell/browser/printing/print_preview_message_handler.h"
#endif

#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
#include "chrome/browser/picture_in_picture/picture_in_picture_window_manager.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "components/pdf/browser/pdf_web_contents_helper.h"  // nogncheck
#include "shell/browser/electron_pdf_web_contents_helper_client.h"
#endif

using content::BrowserThread;

namespace electron {

namespace {

const char kRootName[] = "<root>";

struct FileSystem {
  FileSystem() = default;
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
  storage::IsolatedContext::ScopedFSHandle file_system =
      isolated_context->RegisterFileSystemForPath(
          storage::kFileSystemTypeNativeLocal, std::string(), path, &root_name);

  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  content::RenderViewHost* render_view_host = web_contents->GetRenderViewHost();
  int renderer_id = render_view_host->GetProcess()->GetID();
  policy->GrantReadFileSystem(renderer_id, file_system.id());
  policy->GrantWriteFileSystem(renderer_id, file_system.id());
  policy->GrantCreateFileForFileSystem(renderer_id, file_system.id());
  policy->GrantDeleteFromFileSystem(renderer_id, file_system.id());

  if (!policy->CanReadFile(renderer_id, path))
    policy->GrantReadFile(renderer_id, path);

  return file_system.id();
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
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK(!path.empty());

  base::WriteFile(path, content.data(), content.size());
}

void AppendToFile(const base::FilePath& path, const std::string& content) {
  base::ScopedBlockingCall scoped_blocking_call(FROM_HERE,
                                                base::BlockingType::WILL_BLOCK);
  DCHECK(!path.empty());

  base::AppendToFile(path, content.data(), content.size());
}

PrefService* GetPrefService(content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  return static_cast<electron::ElectronBrowserContext*>(context)->prefs();
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
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      weak_factory_(this) {}

CommonWebContentsDelegate::~CommonWebContentsDelegate() = default;

void CommonWebContentsDelegate::InitWithWebContents(
    content::WebContents* web_contents,
    ElectronBrowserContext* browser_context,
    bool is_guest) {
  browser_context_ = browser_context;
  web_contents->SetDelegate(this);

#if BUILDFLAG(ENABLE_PRINTING)
  PrintPreviewMessageHandler::CreateForWebContents(web_contents);
  printing::PrintViewManagerBasic::CreateForWebContents(web_contents);
  printing::CreateCompositeClientIfNeeded(web_contents,
                                          browser_context->GetUserAgent());
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  pdf::PDFWebContentsHelper::CreateForWebContentsWithClient(
      web_contents, std::make_unique<ElectronPDFWebContentsHelperClient>());
#endif

  // Determien whether the WebContents is offscreen.
  auto* web_preferences = WebContentsPreferences::From(web_contents);
  offscreen_ =
      web_preferences && web_preferences->IsEnabled(options::kOffscreen);

  // Create InspectableWebContents.
  web_contents_.reset(new InspectableWebContents(
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
    NativeWindowRelay::CreateForWebContents(web_contents,
                                            owner_window->GetWeakPtr());
  } else {
    owner_window_ = nullptr;
    web_contents->RemoveUserData(NativeWindowRelay::UserDataKey());
  }
#if BUILDFLAG(ENABLE_OSR)
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetNativeWindow(owner_window);
#endif
}

void CommonWebContentsDelegate::ResetManagedWebContents(bool async) {
  if (async) {
    // Browser context should be destroyed only after the WebContents,
    // this is guaranteed in the sync mode by the order of declaration,
    // in the async version we maintain a reference until the WebContents
    // is destroyed.
    // //electron/patches/chromium/content_browser_main_loop.patch
    // is required to get the right quit closure for the main message loop.
    base::ThreadTaskRunnerHandle::Get()->PostNonNestableTask(
        FROM_HERE,
        base::BindOnce(
            [](scoped_refptr<ElectronBrowserContext> browser_context,
               std::unique_ptr<InspectableWebContents> web_contents) {
              web_contents.reset();
            },
            base::RetainedRef(browser_context_), std::move(web_contents_)));
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

#if BUILDFLAG(ENABLE_OSR)
OffScreenWebContentsView*
CommonWebContentsDelegate::GetOffScreenWebContentsView() const {
  return nullptr;
}
#endif

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
  load_url_params.started_from_context_menu = params.started_from_context_menu;
  load_url_params.initiator_origin = params.initiator_origin;
  load_url_params.source_site_instance = params.source_site_instance;
  load_url_params.frame_tree_node_id = params.frame_tree_node_id;
  load_url_params.redirect_chain = params.redirect_chain;
  load_url_params.has_user_gesture = params.user_gesture;
  load_url_params.blob_url_loader_factory = params.blob_url_loader_factory;
  load_url_params.href_translate = params.href_translate;
  load_url_params.reload_type = params.reload_type;

  if (params.post_data) {
    load_url_params.load_type =
        content::NavigationController::LOAD_TYPE_HTTP_POST;
    load_url_params.post_data = params.post_data;
  }

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

bool CommonWebContentsDelegate::CanOverscrollContent() {
  return false;
}

content::ColorChooser* CommonWebContentsDelegate::OpenColorChooser(
    content::WebContents* web_contents,
    SkColor color,
    const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions) {
#if BUILDFLAG(ENABLE_COLOR_CHOOSER)
  return chrome::ShowColorChooser(web_contents, color);
#else
  return nullptr;
#endif
}

void CommonWebContentsDelegate::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  if (!web_dialog_helper_)
    web_dialog_helper_ =
        std::make_unique<WebDialogHelper>(owner_window(), offscreen_);
  web_dialog_helper_->RunFileChooser(render_frame_host, std::move(listener),
                                     params);
}

void CommonWebContentsDelegate::EnumerateDirectory(
    content::WebContents* guest,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  if (!web_dialog_helper_)
    web_dialog_helper_ =
        std::make_unique<WebDialogHelper>(owner_window(), offscreen_);
  web_dialog_helper_->EnumerateDirectory(guest, std::move(listener), path);
}

void CommonWebContentsDelegate::EnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options) {
  if (!owner_window_)
    return;
  auto* source = content::WebContents::FromRenderFrameHost(requesting_frame);
  if (IsFullscreenForTabOrPending(source)) {
    DCHECK_EQ(fullscreen_frame_, source->GetFocusedFrame());
    return;
  }
  SetHtmlApiFullscreen(true);
  owner_window_->NotifyWindowEnterHtmlFullScreen();

  if (native_fullscreen_) {
    // Explicitly trigger a view resize, as the size is not actually changing if
    // the browser is fullscreened, too.
    source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
  }
}

void CommonWebContentsDelegate::ExitFullscreenModeForTab(
    content::WebContents* source) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(false);
  owner_window_->NotifyWindowLeaveHtmlFullScreen();

  if (native_fullscreen_) {
    // Explicitly trigger a view resize, as the size is not actually changing if
    // the browser is fullscreened, too. Chrome does this indirectly from
    // `chrome/browser/ui/exclusive_access/fullscreen_controller.cc`.
    source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
  }
}

bool CommonWebContentsDelegate::IsFullscreenForTabOrPending(
    const content::WebContents* source) {
  return html_fullscreen_;
}

blink::SecurityStyle CommonWebContentsDelegate::GetSecurityStyle(
    content::WebContents* web_contents,
    content::SecurityStyleExplanations* security_style_explanations) {
  SecurityStateTabHelper* helper =
      SecurityStateTabHelper::FromWebContents(web_contents);
  DCHECK(helper);
  return security_state::GetSecurityStyle(helper->GetSecurityLevel(),
                                          *helper->GetVisibleSecurityState(),
                                          security_style_explanations);
}

bool CommonWebContentsDelegate::TakeFocus(content::WebContents* source,
                                          bool reverse) {
  if (source && source->GetOutermostWebContents() == source) {
    // If this is the outermost web contents and the user has tabbed or
    // shift + tabbed through all the elements, reset the focus back to
    // the first or last element so that it doesn't stay in the body.
    source->FocusThroughTabTraversal(reverse);
    return true;
  }

  return false;
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
    if (!file_dialog::ShowSaveDialogSync(settings, &path)) {
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
    settings.properties = file_dialog::OPEN_DIALOG_OPEN_DIRECTORY;
    if (!file_dialog::ShowOpenDialogSync(settings, &paths))
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
      base::JSONReader::ReadDeprecated(excluded_folders_message);
  if (parsed_excluded_folders && parsed_excluded_folders->is_list()) {
    for (const base::Value& folder_path : parsed_excluded_folders->GetList()) {
      if (folder_path.is_string())
        excluded_folders.push_back(folder_path.GetString());
    }
  }
  devtools_indexing_jobs_[request_id] =
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>(
          devtools_file_system_indexer_->IndexPath(
              file_system_path, excluded_folders,
              base::BindRepeating(
                  &CommonWebContentsDelegate::OnDevToolsIndexingWorkCalculated,
                  weak_factory_.GetWeakPtr(), request_id, file_system_path),
              base::BindRepeating(
                  &CommonWebContentsDelegate::OnDevToolsIndexingWorked,
                  weak_factory_.GetWeakPtr(), request_id, file_system_path),
              base::BindRepeating(
                  &CommonWebContentsDelegate::OnDevToolsIndexingDone,
                  weak_factory_.GetWeakPtr(), request_id, file_system_path)));
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
      base::BindRepeating(&CommonWebContentsDelegate::OnDevToolsSearchCompleted,
                          weak_factory_.GetWeakPtr(), request_id,
                          file_system_path));
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

  // Set fullscreen on window if allowed.
  auto* web_preferences = WebContentsPreferences::From(GetWebContents());
  bool html_fullscreenable =
      web_preferences ? !web_preferences->IsEnabled(
                            options::kDisableHtmlFullscreenWindowResize)
                      : true;

  if (html_fullscreenable) {
    owner_window_->SetFullScreen(enter_fullscreen);
  }

  html_fullscreen_ = enter_fullscreen;
  native_fullscreen_ = false;
}

content::PictureInPictureResult
CommonWebContentsDelegate::EnterPictureInPicture(
    content::WebContents* web_contents,
    const viz::SurfaceId& surface_id,
    const gfx::Size& natural_size) {
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  return PictureInPictureWindowManager::GetInstance()->EnterPictureInPicture(
      web_contents, surface_id, natural_size);
#else
  return content::PictureInPictureResult::kNotSupported;
#endif
}

void CommonWebContentsDelegate::ExitPictureInPicture() {
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  PictureInPictureWindowManager::GetInstance()->ExitPictureInPicture();
#endif
}

}  // namespace electron
