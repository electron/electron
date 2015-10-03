// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/common_web_contents_delegate.h"

#include <string>
#include <vector>

#include "atom/browser/atom_javascript_dialog_manager.h"
#include "atom/browser/native_window.h"
#include "atom/browser/ui/file_dialog.h"
#include "atom/browser/web_dialog_helper.h"
#include "base/files/file_util.h"
#include "chrome/browser/printing/print_preview_message_handler.h"
#include "chrome/browser/printing/print_view_manager_basic.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "storage/browser/fileapi/isolated_context.h"

#if defined(TOOLKIT_VIEWS)
#include "atom/browser/native_window_views.h"
#endif

#if defined(USE_X11)
#include "atom/browser/browser.h"
#endif

using content::BrowserThread;

namespace atom {

namespace {

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
                               const base::FilePath& path,
                               std::string* registered_name) {
  auto isolated_context = storage::IsolatedContext::GetInstance();
  std::string file_system_id = isolated_context->RegisterFileSystemForPath(
      storage::kFileSystemTypeNativeLocal,
      std::string(),
      path,
      registered_name);

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
    const std::string& registered_name,
    const std::string& file_system_path) {
  const GURL origin = web_contents->GetURL().GetOrigin();
  std::string file_system_name =
      storage::GetIsolatedFileSystemName(origin, file_system_id);
  std::string root_url = storage::GetIsolatedFileSystemRootURIString(
      origin, file_system_id, registered_name);
  return FileSystem(file_system_name, root_url, file_system_path);
}

base::DictionaryValue* CreateFileSystemValue(const FileSystem& file_system) {
  base::DictionaryValue* file_system_value = new base::DictionaryValue();
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

}  // namespace

CommonWebContentsDelegate::CommonWebContentsDelegate()
    : html_fullscreen_(false),
      native_fullscreen_(false) {
}

CommonWebContentsDelegate::~CommonWebContentsDelegate() {
}

void CommonWebContentsDelegate::InitWithWebContents(
    content::WebContents* web_contents) {
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
  load_url_params.transferred_global_request_id =
      params.transferred_global_request_id;
  load_url_params.should_clear_history_list = true;

  source->GetController().LoadURLWithParams(load_url_params);
  return source;
}

void CommonWebContentsDelegate::RequestToLockMouse(
    content::WebContents* web_contents,
    bool user_gesture,
    bool last_unlocked_by_target) {
  GetWebContents()->GotResponseToLockMouseRequest(true);
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
  source->GetRenderViewHost()->WasResized();
}

void CommonWebContentsDelegate::ExitFullscreenModeForTab(
    content::WebContents* source) {
  if (!owner_window_)
    return;
  SetHtmlApiFullscreen(false);
  owner_window_->NotifyWindowLeaveHtmlFullScreen();
  source->GetRenderViewHost()->WasResized();
}

bool CommonWebContentsDelegate::IsFullscreenForTabOrPending(
    const content::WebContents* source) const {
  return html_fullscreen_;
}

void CommonWebContentsDelegate::DevToolsSaveToFile(
    const std::string& url, const std::string& content, bool save_as) {
  base::FilePath path;
  PathsMap::iterator it = saved_files_.find(url);
  if (it != saved_files_.end() && !save_as) {
    path = it->second;
  } else {
    file_dialog::Filters filters;
    base::FilePath default_path(base::FilePath::FromUTF8Unsafe(url));
    if (!file_dialog::ShowSaveDialog(owner_window(), url, default_path,
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
  PathsMap::iterator it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;

  BrowserThread::PostTaskAndReply(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&AppendToFile, it->second, content),
      base::Bind(&CommonWebContentsDelegate::OnDevToolsAppendToFile,
                 base::Unretained(this), url));
}

void CommonWebContentsDelegate::DevToolsAddFileSystem(
    const base::FilePath& file_system_path) {
  base::FilePath path = file_system_path;
  if (path.empty()) {
    file_dialog::Filters filters;
    base::FilePath default_path;
    std::vector<base::FilePath> paths;
    int flag = file_dialog::FILE_DIALOG_OPEN_DIRECTORY;
    if (!file_dialog::ShowOpenDialog(owner_window(), "", default_path,
                                     filters, flag, &paths))
      return;

    path = paths[0];
  }

  std::string registered_name;
  std::string file_system_id = RegisterFileSystem(GetDevToolsWebContents(),
                                                  path,
                                                  &registered_name);

  WorkspaceMap::iterator it = saved_paths_.find(file_system_id);
  if (it != saved_paths_.end())
    return;

  saved_paths_[file_system_id] = path;

  FileSystem file_system = CreateFileSystemStruct(GetDevToolsWebContents(),
                                                  file_system_id,
                                                  registered_name,
                                                  path.AsUTF8Unsafe());

  scoped_ptr<base::StringValue> error_string_value(
      new base::StringValue(std::string()));
  scoped_ptr<base::DictionaryValue> file_system_value;
  if (!file_system.file_system_path.empty())
    file_system_value.reset(CreateFileSystemValue(file_system));
  web_contents_->CallClientFunction(
      "DevToolsAPI.fileSystemAdded",
      error_string_value.get(),
      file_system_value.get(),
      nullptr);
}

void CommonWebContentsDelegate::DevToolsRemoveFileSystem(
    const base::FilePath& file_system_path) {
  if (!web_contents_)
    return;

  storage::IsolatedContext::GetInstance()->
      RevokeFileSystemByPath(file_system_path);

  for (auto it = saved_paths_.begin(); it != saved_paths_.end(); ++it)
    if (it->second == file_system_path) {
      saved_paths_.erase(it);
      break;
    }

  base::StringValue file_system_path_value(file_system_path.AsUTF8Unsafe());
  web_contents_->CallClientFunction(
      "DevToolsAPI.fileSystemRemoved",
       &file_system_path_value,
       nullptr,
       nullptr);
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

#if defined(TOOLKIT_VIEWS)
gfx::ImageSkia CommonWebContentsDelegate::GetDevToolsWindowIcon() {
  if (!owner_window())
    return gfx::ImageSkia();
  return static_cast<views::WidgetDelegate*>(static_cast<NativeWindowViews*>(
      owner_window()))->GetWindowAppIcon();
}
#endif

#if defined(USE_X11)
void CommonWebContentsDelegate::GetDevToolsWindowWMClass(
    std::string* name, std::string* class_name) {
  *class_name = Browser::Get()->GetName();
  *name = base::StringToLowerASCII(*class_name);
}
#endif

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
