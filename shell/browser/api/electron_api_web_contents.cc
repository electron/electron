// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_contents.h"

#include <limits>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/containers/id_map.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/current_thread.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_restrictions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/exclusive_access/exclusive_access_manager.h"
#include "chrome/browser/ui/views/eye_dropper/eye_dropper.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/security_state/content/content_utils.h"
#include "components/security_state/core/security_state.h"
#include "content/browser/renderer_host/frame_tree_node.h"  // nogncheck
#include "content/browser/renderer_host/render_frame_host_manager.h"  // nogncheck
#include "content/browser/renderer_host/render_widget_host_impl.h"  // nogncheck
#include "content/browser/renderer_host/render_widget_host_view_base.h"  // nogncheck
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/context_menu_params.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/browser/desktop_streams_registry.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/file_select_listener.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "content/public/browser/navigation_details.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/service_worker_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/referrer_type_converters.h"
#include "content/public/common/webplugininfo.h"
#include "electron/buildflags/buildflags.h"
#include "electron/shell/common/api/api.mojom.h"
#include "gin/arguments.h"
#include "gin/data_object_builder.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "ppapi/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "printing/print_job_constants.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "shell/browser/api/electron_api_browser_window.h"
#include "shell/browser/api/electron_api_debugger.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/api/message_port.h"
#include "shell/browser/browser.h"
#include "shell/browser/child_web_contents_tracker.h"
#include "shell/browser/electron_autofill_driver_factory.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/electron_browser_main_parts.h"
#include "shell/browser/electron_javascript_dialog_manager.h"
#include "shell/browser/electron_navigation_throttle.h"
#include "shell/browser/file_select_helper.h"
#include "shell/browser/native_window.h"
#include "shell/browser/session_preferences.h"
#include "shell/browser/ui/drag_util.h"
#include "shell/browser/ui/file_dialog.h"
#include "shell/browser/ui/inspectable_web_contents.h"
#include "shell/browser/ui/inspectable_web_contents_view.h"
#include "shell/browser/web_contents_permission_helper.h"
#include "shell/browser/web_contents_preferences.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/browser/web_view_guest_delegate.h"
#include "shell/browser/web_view_manager.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/api/electron_bindings.h"
#include "shell/common/color_util.h"
#include "shell/common/electron_constants.h"
#include "shell/common/gin_converters/base_converter.h"
#include "shell/common/gin_converters/blink_converter.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/content_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/frame_converter.h"
#include "shell/common/gin_converters/gfx_converter.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/image_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/language_util.h"
#include "shell/common/mouse_util.h"
#include "shell/common/node_includes.h"
#include "shell/common/options_switches.h"
#include "shell/common/process_util.h"
#include "shell/common/v8_value_serializer.h"
#include "storage/browser/file_system/isolated_context.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/messaging/transferable_message_mojom_traits.h"
#include "third_party/blink/public/common/page/page_zoom.h"
#include "third_party/blink/public/mojom/frame/find_in_page.mojom.h"
#include "third_party/blink/public/mojom/frame/fullscreen.mojom.h"
#include "third_party/blink/public/mojom/messaging/transferable_message.mojom.h"
#include "third_party/blink/public/mojom/renderer_preferences.mojom.h"
#include "ui/base/cursor/cursor.h"
#include "ui/base/cursor/mojom/cursor_type.mojom-shared.h"
#include "ui/display/screen.h"
#include "ui/events/base_event_utils.h"

#if BUILDFLAG(ENABLE_OSR)
#include "shell/browser/osr/osr_render_widget_host_view.h"
#include "shell/browser/osr/osr_web_contents_view.h"
#endif

#if !BUILDFLAG(IS_MAC)
#include "ui/aura/window.h"
#else
#include "ui/base/cocoa/defaults_utils.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "ui/views/linux_ui/linux_ui.h"
#endif

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
#include "ui/gfx/font_render_params.h"
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
#include "extensions/browser/script_executor.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/mojom/view_type.mojom.h"
#include "shell/browser/extensions/electron_extension_web_contents_observer.h"
#endif

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/printing/browser/print_manager_utils.h"
#include "printing/backend/print_backend.h"  // nogncheck
#include "printing/mojom/print.mojom.h"      // nogncheck
#include "shell/browser/printing/print_preview_message_handler.h"
#include "shell/browser/printing/print_view_manager_electron.h"

#if BUILDFLAG(IS_WIN)
#include "printing/backend/win_helper.h"
#endif
#endif

#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
#include "chrome/browser/picture_in_picture/picture_in_picture_window_manager.h"
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "components/pdf/browser/pdf_web_contents_helper.h"  // nogncheck
#include "shell/browser/electron_pdf_web_contents_helper_client.h"
#endif

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#endif

#ifndef MAS_BUILD
#include "chrome/browser/hang_monitor/hang_crash_dump.h"  // nogncheck
#endif

namespace gin {

#if BUILDFLAG(ENABLE_PRINTING)
template <>
struct Converter<printing::mojom::MarginType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     printing::mojom::MarginType* out) {
    std::string type;
    if (ConvertFromV8(isolate, val, &type)) {
      if (type == "default") {
        *out = printing::mojom::MarginType::kDefaultMargins;
        return true;
      }
      if (type == "none") {
        *out = printing::mojom::MarginType::kNoMargins;
        return true;
      }
      if (type == "printableArea") {
        *out = printing::mojom::MarginType::kPrintableAreaMargins;
        return true;
      }
      if (type == "custom") {
        *out = printing::mojom::MarginType::kCustomMargins;
        return true;
      }
    }
    return false;
  }
};

template <>
struct Converter<printing::mojom::DuplexMode> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     printing::mojom::DuplexMode* out) {
    std::string mode;
    if (ConvertFromV8(isolate, val, &mode)) {
      if (mode == "simplex") {
        *out = printing::mojom::DuplexMode::kSimplex;
        return true;
      }
      if (mode == "longEdge") {
        *out = printing::mojom::DuplexMode::kLongEdge;
        return true;
      }
      if (mode == "shortEdge") {
        *out = printing::mojom::DuplexMode::kShortEdge;
        return true;
      }
    }
    return false;
  }
};

#endif

template <>
struct Converter<WindowOpenDisposition> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   WindowOpenDisposition val) {
    std::string disposition = "other";
    switch (val) {
      case WindowOpenDisposition::CURRENT_TAB:
        disposition = "default";
        break;
      case WindowOpenDisposition::NEW_FOREGROUND_TAB:
        disposition = "foreground-tab";
        break;
      case WindowOpenDisposition::NEW_BACKGROUND_TAB:
        disposition = "background-tab";
        break;
      case WindowOpenDisposition::NEW_POPUP:
      case WindowOpenDisposition::NEW_WINDOW:
        disposition = "new-window";
        break;
      case WindowOpenDisposition::SAVE_TO_DISK:
        disposition = "save-to-disk";
        break;
      default:
        break;
    }
    return gin::ConvertToV8(isolate, disposition);
  }
};

template <>
struct Converter<content::SavePageType> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     content::SavePageType* out) {
    std::string save_type;
    if (!ConvertFromV8(isolate, val, &save_type))
      return false;
    save_type = base::ToLowerASCII(save_type);
    if (save_type == "htmlonly") {
      *out = content::SAVE_PAGE_TYPE_AS_ONLY_HTML;
    } else if (save_type == "htmlcomplete") {
      *out = content::SAVE_PAGE_TYPE_AS_COMPLETE_HTML;
    } else if (save_type == "mhtml") {
      *out = content::SAVE_PAGE_TYPE_AS_MHTML;
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<electron::api::WebContents::Type> {
  static v8::Local<v8::Value> ToV8(v8::Isolate* isolate,
                                   electron::api::WebContents::Type val) {
    using Type = electron::api::WebContents::Type;
    std::string type;
    switch (val) {
      case Type::kBackgroundPage:
        type = "backgroundPage";
        break;
      case Type::kBrowserWindow:
        type = "window";
        break;
      case Type::kBrowserView:
        type = "browserView";
        break;
      case Type::kRemote:
        type = "remote";
        break;
      case Type::kWebView:
        type = "webview";
        break;
      case Type::kOffScreen:
        type = "offscreen";
        break;
      default:
        break;
    }
    return gin::ConvertToV8(isolate, type);
  }

  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     electron::api::WebContents::Type* out) {
    using Type = electron::api::WebContents::Type;
    std::string type;
    if (!ConvertFromV8(isolate, val, &type))
      return false;
    if (type == "backgroundPage") {
      *out = Type::kBackgroundPage;
    } else if (type == "browserView") {
      *out = Type::kBrowserView;
    } else if (type == "webview") {
      *out = Type::kWebView;
#if BUILDFLAG(ENABLE_OSR)
    } else if (type == "offscreen") {
      *out = Type::kOffScreen;
#endif
    } else {
      return false;
    }
    return true;
  }
};

template <>
struct Converter<scoped_refptr<content::DevToolsAgentHost>> {
  static v8::Local<v8::Value> ToV8(
      v8::Isolate* isolate,
      const scoped_refptr<content::DevToolsAgentHost>& val) {
    gin_helper::Dictionary dict(isolate, v8::Object::New(isolate));
    dict.Set("id", val->GetId());
    dict.Set("url", val->GetURL().spec());
    return dict.GetHandle();
  }
};

}  // namespace gin

namespace electron {

namespace api {

namespace {

base::IDMap<WebContents*>& GetAllWebContents() {
  static base::NoDestructor<base::IDMap<WebContents*>> s_all_web_contents;
  return *s_all_web_contents;
}

// Called when CapturePage is done.
void OnCapturePageDone(gin_helper::Promise<gfx::Image> promise,
                       const SkBitmap& bitmap) {
  // Hack to enable transparency in captured image
  promise.Resolve(gfx::Image::CreateFrom1xBitmap(bitmap));
}

absl::optional<base::TimeDelta> GetCursorBlinkInterval() {
#if BUILDFLAG(IS_MAC)
  absl::optional<base::TimeDelta> system_value(
      ui::TextInsertionCaretBlinkPeriodFromDefaults());
  if (system_value)
    return *system_value;
#elif BUILDFLAG(IS_LINUX)
  if (auto* linux_ui = views::LinuxUI::instance())
    return linux_ui->GetCursorBlinkInterval();
#elif BUILDFLAG(IS_WIN)
  const auto system_msec = ::GetCaretBlinkTime();
  if (system_msec != 0) {
    return (system_msec == INFINITE) ? base::TimeDelta()
                                     : base::Milliseconds(system_msec);
  }
#endif
  return absl::nullopt;
}

#if BUILDFLAG(ENABLE_PRINTING)
// This will return false if no printer with the provided device_name can be
// found on the network. We need to check this because Chromium does not do
// sanity checking of device_name validity and so will crash on invalid names.
bool IsDeviceNameValid(const std::u16string& device_name) {
#if BUILDFLAG(IS_MAC)
  base::ScopedCFTypeRef<CFStringRef> new_printer_id(
      base::SysUTF16ToCFStringRef(device_name));
  PMPrinter new_printer = PMPrinterCreateFromPrinterID(new_printer_id.get());
  bool printer_exists = new_printer != nullptr;
  PMRelease(new_printer);
  return printer_exists;
#elif BUILDFLAG(IS_WIN)
  printing::ScopedPrinterHandle printer;
  return printer.OpenPrinterWithName(base::as_wcstr(device_name));
#else
  return true;
#endif
}

std::pair<std::string, std::u16string> GetDefaultPrinterAsync() {
#if BUILDFLAG(IS_WIN)
  // Blocking is needed here because Windows printer drivers are oftentimes
  // not thread-safe and have to be accessed on the UI thread.
  base::ThreadRestrictions::ScopedAllowIO allow_io;
#endif

  scoped_refptr<printing::PrintBackend> print_backend =
      printing::PrintBackend::CreateInstance(
          g_browser_process->GetApplicationLocale());
  std::string printer_name;
  printing::mojom::ResultCode code =
      print_backend->GetDefaultPrinterName(printer_name);

  // We don't want to return if this fails since some devices won't have a
  // default printer.
  if (code != printing::mojom::ResultCode::kSuccess)
    LOG(ERROR) << "Failed to get default printer name";

  // Check for existing printers and pick the first one should it exist.
  if (printer_name.empty()) {
    printing::PrinterList printers;
    if (print_backend->EnumeratePrinters(&printers) !=
        printing::mojom::ResultCode::kSuccess)
      return std::make_pair("Failed to enumerate printers", std::u16string());
    if (!printers.empty())
      printer_name = printers.front().printer_name;
  }

  return std::make_pair(std::string(), base::UTF8ToUTF16(printer_name));
}

// Copied from
// chrome/browser/ui/webui/print_preview/local_printer_handler_default.cc:L36-L54
scoped_refptr<base::TaskRunner> CreatePrinterHandlerTaskRunner() {
  // USER_VISIBLE because the result is displayed in the print preview dialog.
#if !BUILDFLAG(IS_WIN)
  static constexpr base::TaskTraits kTraits = {
      base::MayBlock(), base::TaskPriority::USER_VISIBLE};
#endif

#if defined(USE_CUPS)
  // CUPS is thread safe.
  return base::ThreadPool::CreateTaskRunner(kTraits);
#elif BUILDFLAG(IS_WIN)
  // Windows drivers are likely not thread-safe and need to be accessed on the
  // UI thread.
  return content::GetUIThreadTaskRunner(
      {base::MayBlock(), base::TaskPriority::USER_VISIBLE});
#else
  // Be conservative on unsupported platforms.
  return base::ThreadPool::CreateSingleThreadTaskRunner(kTraits);
#endif
}
#endif

struct UserDataLink : public base::SupportsUserData::Data {
  explicit UserDataLink(base::WeakPtr<WebContents> contents)
      : web_contents(contents) {}

  base::WeakPtr<WebContents> web_contents;
};
const void* kElectronApiWebContentsKey = &kElectronApiWebContentsKey;

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
          storage::kFileSystemTypeLocal, std::string(), path, &root_name);

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
  const GURL origin = web_contents->GetURL().DeprecatedGetOriginAsURL();
  std::string file_system_name =
      storage::GetIsolatedFileSystemName(origin, file_system_id);
  std::string root_url = storage::GetIsolatedFileSystemRootURIString(
      origin, file_system_id, kRootName);
  return FileSystem(type, file_system_name, root_url, file_system_path);
}

std::unique_ptr<base::DictionaryValue> CreateFileSystemValue(
    const FileSystem& file_system) {
  auto file_system_value = std::make_unique<base::DictionaryValue>();
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

  base::AppendToFile(path, content);
}

PrefService* GetPrefService(content::WebContents* web_contents) {
  auto* context = web_contents->GetBrowserContext();
  return static_cast<electron::ElectronBrowserContext*>(context)->prefs();
}

std::map<std::string, std::string> GetAddedFileSystemPaths(
    content::WebContents* web_contents) {
  auto* pref_service = GetPrefService(web_contents);
  const base::Value* file_system_paths_value =
      pref_service->GetDictionary(prefs::kDevToolsFileSystemPaths);
  std::map<std::string, std::string> result;
  if (file_system_paths_value) {
    const base::DictionaryValue* file_system_paths_dict;
    file_system_paths_value->GetAsDictionary(&file_system_paths_dict);

    for (auto it : file_system_paths_dict->DictItems()) {
      std::string type =
          it.second.is_string() ? it.second.GetString() : std::string();
      result[it.first] = type;
    }
  }
  return result;
}

bool IsDevToolsFileSystemAdded(content::WebContents* web_contents,
                               const std::string& file_system_path) {
  auto file_system_paths = GetAddedFileSystemPaths(web_contents);
  return file_system_paths.find(file_system_path) != file_system_paths.end();
}

void SetBackgroundColor(content::RenderWidgetHostView* rwhv, SkColor color) {
  rwhv->SetBackgroundColor(color);
  static_cast<content::RenderWidgetHostViewBase*>(rwhv)
      ->SetContentBackgroundColor(color);
}

}  // namespace

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)

WebContents::Type GetTypeFromViewType(extensions::mojom::ViewType view_type) {
  switch (view_type) {
    case extensions::mojom::ViewType::kExtensionBackgroundPage:
      return WebContents::Type::kBackgroundPage;

    case extensions::mojom::ViewType::kAppWindow:
    case extensions::mojom::ViewType::kComponent:
    case extensions::mojom::ViewType::kExtensionDialog:
    case extensions::mojom::ViewType::kExtensionPopup:
    case extensions::mojom::ViewType::kBackgroundContents:
    case extensions::mojom::ViewType::kExtensionGuest:
    case extensions::mojom::ViewType::kTabContents:
    case extensions::mojom::ViewType::kInvalid:
      return WebContents::Type::kRemote;
  }
}

#endif

WebContents::WebContents(v8::Isolate* isolate,
                         content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      type_(Type::kRemote),
      id_(GetAllWebContents().Add(this)),
      devtools_file_system_indexer_(
          base::MakeRefCounted<DevToolsFileSystemIndexer>()),
      exclusive_access_manager_(std::make_unique<ExclusiveAccessManager>(this)),
      file_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}))
#if BUILDFLAG(ENABLE_PRINTING)
      ,
      print_task_runner_(CreatePrinterHandlerTaskRunner())
#endif
{
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // WebContents created by extension host will have valid ViewType set.
  extensions::mojom::ViewType view_type = extensions::GetViewType(web_contents);
  if (view_type != extensions::mojom::ViewType::kInvalid) {
    InitWithExtensionView(isolate, web_contents, view_type);
  }

  extensions::ElectronExtensionWebContentsObserver::CreateForWebContents(
      web_contents);
  script_executor_ = std::make_unique<extensions::ScriptExecutor>(web_contents);
#endif

  auto session = Session::CreateFrom(isolate, GetBrowserContext());
  session_.Reset(isolate, session.ToV8());

  web_contents->SetUserAgentOverride(blink::UserAgentOverride::UserAgentOnly(
                                         GetBrowserContext()->GetUserAgent()),
                                     false);
  web_contents->SetUserData(kElectronApiWebContentsKey,
                            std::make_unique<UserDataLink>(GetWeakPtr()));
  InitZoomController(web_contents, gin::Dictionary::CreateEmpty(isolate));
}

WebContents::WebContents(v8::Isolate* isolate,
                         std::unique_ptr<content::WebContents> web_contents,
                         Type type)
    : content::WebContentsObserver(web_contents.get()),
      type_(type),
      id_(GetAllWebContents().Add(this)),
      devtools_file_system_indexer_(
          base::MakeRefCounted<DevToolsFileSystemIndexer>()),
      exclusive_access_manager_(std::make_unique<ExclusiveAccessManager>(this)),
      file_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}))
#if BUILDFLAG(ENABLE_PRINTING)
      ,
      print_task_runner_(CreatePrinterHandlerTaskRunner())
#endif
{
  DCHECK(type != Type::kRemote)
      << "Can't take ownership of a remote WebContents";
  auto session = Session::CreateFrom(isolate, GetBrowserContext());
  session_.Reset(isolate, session.ToV8());
  InitWithSessionAndOptions(isolate, std::move(web_contents), session,
                            gin::Dictionary::CreateEmpty(isolate));
}

WebContents::WebContents(v8::Isolate* isolate,
                         const gin_helper::Dictionary& options)
    : id_(GetAllWebContents().Add(this)),
      devtools_file_system_indexer_(
          base::MakeRefCounted<DevToolsFileSystemIndexer>()),
      exclusive_access_manager_(std::make_unique<ExclusiveAccessManager>(this)),
      file_task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}))
#if BUILDFLAG(ENABLE_PRINTING)
      ,
      print_task_runner_(CreatePrinterHandlerTaskRunner())
#endif
{
  // Read options.
  options.Get("backgroundThrottling", &background_throttling_);

  // Get type
  options.Get("type", &type_);

#if BUILDFLAG(ENABLE_OSR)
  bool b = false;
  if (options.Get(options::kOffscreen, &b) && b)
    type_ = Type::kOffScreen;
#endif

  // Init embedder earlier
  options.Get("embedder", &embedder_);

  // Whether to enable DevTools.
  options.Get("devTools", &enable_devtools_);

  // BrowserViews are not attached to a window initially so they should start
  // off as hidden. This is also important for compositor recycling. See:
  // https://github.com/electron/electron/pull/21372
  bool initially_shown = type_ != Type::kBrowserView;
  options.Get(options::kShow, &initially_shown);

  // Obtain the session.
  std::string partition;
  gin::Handle<api::Session> session;
  if (options.Get("session", &session) && !session.IsEmpty()) {
  } else if (options.Get("partition", &partition)) {
    session = Session::FromPartition(isolate, partition);
  } else {
    // Use the default session if not specified.
    session = Session::FromPartition(isolate, "");
  }
  session_.Reset(isolate, session.ToV8());

  std::unique_ptr<content::WebContents> web_contents;
  if (IsGuest()) {
    scoped_refptr<content::SiteInstance> site_instance =
        content::SiteInstance::CreateForURL(session->browser_context(),
                                            GURL("chrome-guest://fake-host"));
    content::WebContents::CreateParams params(session->browser_context(),
                                              site_instance);
    guest_delegate_ =
        std::make_unique<WebViewGuestDelegate>(embedder_->web_contents(), this);
    params.guest_delegate = guest_delegate_.get();

#if BUILDFLAG(ENABLE_OSR)
    if (embedder_ && embedder_->IsOffScreen()) {
      auto* view = new OffScreenWebContentsView(
          false,
          base::BindRepeating(&WebContents::OnPaint, base::Unretained(this)));
      params.view = view;
      params.delegate_view = view;

      web_contents = content::WebContents::Create(params);
      view->SetWebContents(web_contents.get());
    } else {
#endif
      web_contents = content::WebContents::Create(params);
#if BUILDFLAG(ENABLE_OSR)
    }
  } else if (IsOffScreen()) {
    // webPreferences does not have a transparent option, so if the window needs
    // to be transparent, that will be set at electron_api_browser_window.cc#L57
    // and we then need to pull it back out and check it here.
    std::string background_color;
    options.GetHidden(options::kBackgroundColor, &background_color);
    bool transparent = ParseCSSColor(background_color) == SK_ColorTRANSPARENT;

    content::WebContents::CreateParams params(session->browser_context());
    auto* view = new OffScreenWebContentsView(
        transparent,
        base::BindRepeating(&WebContents::OnPaint, base::Unretained(this)));
    params.view = view;
    params.delegate_view = view;

    web_contents = content::WebContents::Create(params);
    view->SetWebContents(web_contents.get());
#endif
  } else {
    content::WebContents::CreateParams params(session->browser_context());
    params.initially_hidden = !initially_shown;
    web_contents = content::WebContents::Create(params);
  }

  InitWithSessionAndOptions(isolate, std::move(web_contents), session, options);
}

void WebContents::InitZoomController(content::WebContents* web_contents,
                                     const gin_helper::Dictionary& options) {
  WebContentsZoomController::CreateForWebContents(web_contents);
  zoom_controller_ = WebContentsZoomController::FromWebContents(web_contents);
  double zoom_factor;
  if (options.Get(options::kZoomFactor, &zoom_factor))
    zoom_controller_->SetDefaultZoomFactor(zoom_factor);
}

void WebContents::InitWithSessionAndOptions(
    v8::Isolate* isolate,
    std::unique_ptr<content::WebContents> owned_web_contents,
    gin::Handle<api::Session> session,
    const gin_helper::Dictionary& options) {
  Observe(owned_web_contents.get());
  InitWithWebContents(std::move(owned_web_contents), session->browser_context(),
                      IsGuest());

  inspectable_web_contents_->GetView()->SetDelegate(this);

  auto* prefs = web_contents()->GetMutableRendererPrefs();

  // Collect preferred languages from OS and browser process. accept_languages
  // effects HTTP header, navigator.languages, and CJK fallback font selection.
  //
  // Note that an application locale set to the browser process might be
  // different with the one set to the preference list.
  // (e.g. overridden with --lang)
  std::string accept_languages =
      g_browser_process->GetApplicationLocale() + ",";
  for (auto const& language : electron::GetPreferredLanguages()) {
    if (language == g_browser_process->GetApplicationLocale())
      continue;
    accept_languages += language + ",";
  }
  accept_languages.pop_back();
  prefs->accept_languages = accept_languages;

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
  // Update font settings.
  static const gfx::FontRenderParams params(
      gfx::GetFontRenderParams(gfx::FontRenderParamsQuery(), nullptr));
  prefs->should_antialias_text = params.antialiasing;
  prefs->use_subpixel_positioning = params.subpixel_positioning;
  prefs->hinting = params.hinting;
  prefs->use_autohinter = params.autohinter;
  prefs->use_bitmaps = params.use_bitmaps;
  prefs->subpixel_rendering = params.subpixel_rendering;
#endif

  // Honor the system's cursor blink rate settings
  if (auto interval = GetCursorBlinkInterval())
    prefs->caret_blink_interval = *interval;

  // Save the preferences in C++.
  // If there's already a WebContentsPreferences object, we created it as part
  // of the webContents.setWindowOpenHandler path, so don't overwrite it.
  if (!WebContentsPreferences::From(web_contents())) {
    new WebContentsPreferences(web_contents(), options);
  }
  // Trigger re-calculation of webkit prefs.
  web_contents()->NotifyPreferencesChanged();

  WebContentsPermissionHelper::CreateForWebContents(web_contents());
  InitZoomController(web_contents(), options);
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  extensions::ElectronExtensionWebContentsObserver::CreateForWebContents(
      web_contents());
  script_executor_ =
      std::make_unique<extensions::ScriptExecutor>(web_contents());
#endif

  AutofillDriverFactory::CreateForWebContents(web_contents());

  web_contents()->SetUserAgentOverride(blink::UserAgentOverride::UserAgentOnly(
                                           GetBrowserContext()->GetUserAgent()),
                                       false);

  if (IsGuest()) {
    NativeWindow* owner_window = nullptr;
    if (embedder_) {
      // New WebContents's owner_window is the embedder's owner_window.
      auto* relay =
          NativeWindowRelay::FromWebContents(embedder_->web_contents());
      if (relay)
        owner_window = relay->GetNativeWindow();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);
  }

  web_contents()->SetUserData(kElectronApiWebContentsKey,
                              std::make_unique<UserDataLink>(GetWeakPtr()));
}

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
void WebContents::InitWithExtensionView(v8::Isolate* isolate,
                                        content::WebContents* web_contents,
                                        extensions::mojom::ViewType view_type) {
  // Must reassign type prior to calling `Init`.
  type_ = GetTypeFromViewType(view_type);
  if (type_ == Type::kRemote)
    return;
  if (type_ == Type::kBackgroundPage)
    // non-background-page WebContents are retained by other classes. We need
    // to pin here to prevent background-page WebContents from being GC'd.
    // The background page api::WebContents will live until the underlying
    // content::WebContents is destroyed.
    Pin(isolate);

  // Allow toggling DevTools for background pages
  Observe(web_contents);
  InitWithWebContents(std::unique_ptr<content::WebContents>(web_contents),
                      GetBrowserContext(), IsGuest());
  inspectable_web_contents_->GetView()->SetDelegate(this);
}
#endif

void WebContents::InitWithWebContents(
    std::unique_ptr<content::WebContents> web_contents,
    ElectronBrowserContext* browser_context,
    bool is_guest) {
  browser_context_ = browser_context;
  web_contents->SetDelegate(this);

#if BUILDFLAG(ENABLE_PRINTING)
  PrintPreviewMessageHandler::CreateForWebContents(web_contents.get());
  PrintViewManagerElectron::CreateForWebContents(web_contents.get());
  printing::CreateCompositeClientIfNeeded(web_contents.get(),
                                          browser_context->GetUserAgent());
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
  pdf::PDFWebContentsHelper::CreateForWebContentsWithClient(
      web_contents.get(),
      std::make_unique<ElectronPDFWebContentsHelperClient>());
#endif

  // Determine whether the WebContents is offscreen.
  auto* web_preferences = WebContentsPreferences::From(web_contents.get());
  offscreen_ = web_preferences && web_preferences->IsOffscreen();

  // Create InspectableWebContents.
  inspectable_web_contents_ = std::make_unique<InspectableWebContents>(
      std::move(web_contents), browser_context->prefs(), is_guest);
  inspectable_web_contents_->SetDelegate(this);
}

WebContents::~WebContents() {
  // clear out objects that have been granted permissions so that when
  // WebContents::RenderFrameDeleted is called as a result of WebContents
  // destruction it doesn't try to clear out a granted_devices_
  // on a destructed object.
  granted_devices_.clear();

  if (!inspectable_web_contents_) {
    WebContentsDestroyed();
    return;
  }

  inspectable_web_contents_->GetView()->SetDelegate(nullptr);

  // This event is only for internal use, which is emitted when WebContents is
  // being destroyed.
  Emit("will-destroy");

  // For guest view based on OOPIF, the WebContents is released by the embedder
  // frame, and we need to clear the reference to the memory.
  bool not_owned_by_this = IsGuest() && attached_;
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // And background pages are owned by extensions::ExtensionHost.
  if (type_ == Type::kBackgroundPage)
    not_owned_by_this = true;
#endif
  if (not_owned_by_this) {
    inspectable_web_contents_->ReleaseWebContents();
    WebContentsDestroyed();
  }

  // InspectableWebContents will be automatically destroyed.
}

void WebContents::DeleteThisIfAlive() {
  // It is possible that the FirstWeakCallback has been called but the
  // SecondWeakCallback has not, in this case the garbage collection of
  // WebContents has already started and we should not |delete this|.
  // Calling |GetWrapper| can detect this corner case.
  auto* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;
  delete this;
}

void WebContents::Destroy() {
  // The content::WebContents should be destroyed asyncronously when possible
  // as user may choose to destroy WebContents during an event of it.
  if (Browser::Get()->is_shutting_down() || IsGuest()) {
    DeleteThisIfAlive();
  } else {
    base::PostTask(FROM_HERE, {content::BrowserThread::UI},
                   base::BindOnce(
                       [](base::WeakPtr<WebContents> contents) {
                         if (contents)
                           contents->DeleteThisIfAlive();
                       },
                       GetWeakPtr()));
  }
}

bool WebContents::DidAddMessageToConsole(
    content::WebContents* source,
    blink::mojom::ConsoleMessageLevel level,
    const std::u16string& message,
    int32_t line_no,
    const std::u16string& source_id) {
  return Emit("console-message", static_cast<int32_t>(level), message, line_no,
              source_id);
}

void WebContents::OnCreateWindow(
    const GURL& target_url,
    const content::Referrer& referrer,
    const std::string& frame_name,
    WindowOpenDisposition disposition,
    const std::string& features,
    const scoped_refptr<network::ResourceRequestBody>& body) {
  Emit("-new-window", target_url, frame_name, disposition, features, referrer,
       body);
}

void WebContents::WebContentsCreatedWithFullParams(
    content::WebContents* source_contents,
    int opener_render_process_id,
    int opener_render_frame_id,
    const content::mojom::CreateNewWindowParams& params,
    content::WebContents* new_contents) {
  ChildWebContentsTracker::CreateForWebContents(new_contents);
  auto* tracker = ChildWebContentsTracker::FromWebContents(new_contents);
  tracker->url = params.target_url;
  tracker->frame_name = params.frame_name;
  tracker->referrer = params.referrer.To<content::Referrer>();
  tracker->raw_features = params.raw_features;
  tracker->body = params.body;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);

  gin_helper::Dictionary dict;
  gin::ConvertFromV8(isolate, pending_child_web_preferences_.Get(isolate),
                     &dict);
  pending_child_web_preferences_.Reset();

  // Associate the preferences passed in via `setWindowOpenHandler` with the
  // content::WebContents that was just created for the child window. These
  // preferences will be picked up by the RenderWidgetHost via its call to the
  // delegate's OverrideWebkitPrefs.
  new WebContentsPreferences(new_contents, dict);
}

bool WebContents::IsWebContentsCreationOverridden(
    content::SiteInstance* source_site_instance,
    content::mojom::WindowContainerType window_container_type,
    const GURL& opener_url,
    const content::mojom::CreateNewWindowParams& params) {
  bool default_prevented = Emit(
      "-will-add-new-contents", params.target_url, params.frame_name,
      params.raw_features, params.disposition, *params.referrer, params.body);
  // If the app prevented the default, redirect to CreateCustomWebContents,
  // which always returns nullptr, which will result in the window open being
  // prevented (window.open() will return null in the renderer).
  return default_prevented;
}

void WebContents::SetNextChildWebPreferences(
    const gin_helper::Dictionary preferences) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  // Store these prefs for when Chrome calls WebContentsCreatedWithFullParams
  // with the new child contents.
  pending_child_web_preferences_.Reset(isolate, preferences.GetHandle());
}

content::WebContents* WebContents::CreateCustomWebContents(
    content::RenderFrameHost* opener,
    content::SiteInstance* source_site_instance,
    bool is_new_browsing_instance,
    const GURL& opener_url,
    const std::string& frame_name,
    const GURL& target_url,
    const content::StoragePartitionConfig& partition_config,
    content::SessionStorageNamespace* session_storage_namespace) {
  return nullptr;
}

void WebContents::AddNewContents(
    content::WebContents* source,
    std::unique_ptr<content::WebContents> new_contents,
    const GURL& target_url,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture,
    bool* was_blocked) {
  auto* tracker = ChildWebContentsTracker::FromWebContents(new_contents.get());
  DCHECK(tracker);

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();

  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  auto api_web_contents =
      CreateAndTake(isolate, std::move(new_contents), Type::kBrowserWindow);

  // We call RenderFrameCreated here as at this point the empty "about:blank"
  // render frame has already been created.  If the window never navigates again
  // RenderFrameCreated won't be called and certain prefs like
  // "kBackgroundColor" will not be applied.
  auto* frame = api_web_contents->MainFrame();
  if (frame) {
    api_web_contents->HandleNewRenderFrame(frame);
  }

  if (Emit("-add-new-contents", api_web_contents, disposition, user_gesture,
           initial_rect.x(), initial_rect.y(), initial_rect.width(),
           initial_rect.height(), tracker->url, tracker->frame_name,
           tracker->referrer, tracker->raw_features, tracker->body)) {
    api_web_contents->Destroy();
  }
}

content::WebContents* WebContents::OpenURLFromTab(
    content::WebContents* source,
    const content::OpenURLParams& params) {
  auto weak_this = GetWeakPtr();
  if (params.disposition != WindowOpenDisposition::CURRENT_TAB) {
    Emit("-new-window", params.url, "", params.disposition, "", params.referrer,
         params.post_data);
    return nullptr;
  }

  if (!weak_this || !web_contents())
    return nullptr;

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

void WebContents::BeforeUnloadFired(content::WebContents* tab,
                                    bool proceed,
                                    bool* proceed_to_fire_unload) {
  if (type_ == Type::kBrowserWindow || type_ == Type::kOffScreen ||
      type_ == Type::kBrowserView)
    *proceed_to_fire_unload = proceed;
  else
    *proceed_to_fire_unload = true;
  // Note that Chromium does not emit this for navigations.
  Emit("before-unload-fired", proceed);
}

void WebContents::SetContentsBounds(content::WebContents* source,
                                    const gfx::Rect& rect) {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnSetContentBounds(rect);
}

void WebContents::CloseContents(content::WebContents* source) {
  Emit("close");

  auto* autofill_driver_factory =
      AutofillDriverFactory::FromWebContents(web_contents());
  if (autofill_driver_factory) {
    autofill_driver_factory->CloseAllPopups();
  }

  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnCloseContents();
}

void WebContents::ActivateContents(content::WebContents* source) {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnActivateContents();
}

void WebContents::UpdateTargetURL(content::WebContents* source,
                                  const GURL& url) {
  Emit("update-target-url", url);
}

bool WebContents::HandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (type_ == Type::kWebView && embedder_) {
    // Send the unhandled keyboard events back to the embedder.
    return embedder_->HandleKeyboardEvent(source, event);
  } else {
    return PlatformHandleKeyboardEvent(source, event);
  }
}

#if !BUILDFLAG(IS_MAC)
// NOTE: The macOS version of this function is found in
// electron_api_web_contents_mac.mm, as it requires calling into objective-C
// code.
bool WebContents::PlatformHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  // Escape exits tabbed fullscreen mode.
  if (event.windows_key_code == ui::VKEY_ESCAPE && is_html_fullscreen()) {
    ExitFullscreenModeForTab(source);
    return true;
  }

  // Check if the webContents has preferences and to ignore shortcuts
  auto* web_preferences = WebContentsPreferences::From(source);
  if (web_preferences && web_preferences->ShouldIgnoreMenuShortcuts())
    return false;

  // Let the NativeWindow handle other parts.
  if (owner_window()) {
    owner_window()->HandleKeyboardEvent(source, event);
    return true;
  }

  return false;
}
#endif

content::KeyboardEventProcessingResult WebContents::PreHandleKeyboardEvent(
    content::WebContents* source,
    const content::NativeWebKeyboardEvent& event) {
  if (exclusive_access_manager_->HandleUserKeyEvent(event))
    return content::KeyboardEventProcessingResult::HANDLED;

  if (event.GetType() == blink::WebInputEvent::Type::kRawKeyDown ||
      event.GetType() == blink::WebInputEvent::Type::kKeyUp) {
    bool prevent_default = Emit("before-input-event", event);
    if (prevent_default) {
      return content::KeyboardEventProcessingResult::HANDLED;
    }
  }

  return content::KeyboardEventProcessingResult::NOT_HANDLED;
}

void WebContents::ContentsZoomChange(bool zoom_in) {
  Emit("zoom-changed", zoom_in ? "in" : "out");
}

Profile* WebContents::GetProfile() {
  return nullptr;
}

bool WebContents::IsFullscreen() const {
  return owner_window_ && owner_window_->IsFullscreen();
}

void WebContents::EnterFullscreen(const GURL& url,
                                  ExclusiveAccessBubbleType bubble_type,
                                  const int64_t display_id) {}

void WebContents::ExitFullscreen() {}

void WebContents::UpdateExclusiveAccessExitBubbleContent(
    const GURL& url,
    ExclusiveAccessBubbleType bubble_type,
    ExclusiveAccessBubbleHideCallback bubble_first_hide_callback,
    bool force_update) {}

void WebContents::OnExclusiveAccessUserInput() {}

content::WebContents* WebContents::GetActiveWebContents() {
  return web_contents();
}

bool WebContents::CanUserExitFullscreen() const {
  return true;
}

bool WebContents::IsExclusiveAccessBubbleDisplayed() const {
  return false;
}

void WebContents::EnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options) {
  auto* source = content::WebContents::FromRenderFrameHost(requesting_frame);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(source);
  auto callback =
      base::BindRepeating(&WebContents::OnEnterFullscreenModeForTab,
                          base::Unretained(this), requesting_frame, options);
  permission_helper->RequestFullscreenPermission(callback);
  exclusive_access_manager_->fullscreen_controller()->EnterFullscreenModeForTab(
      requesting_frame, options.display_id);
}

void WebContents::OnEnterFullscreenModeForTab(
    content::RenderFrameHost* requesting_frame,
    const blink::mojom::FullscreenOptions& options,
    bool allowed) {
  if (!allowed || !owner_window_)
    return;

  auto* source = content::WebContents::FromRenderFrameHost(requesting_frame);
  if (IsFullscreenForTabOrPending(source)) {
    DCHECK_EQ(fullscreen_frame_, source->GetFocusedFrame());
    return;
  }

  SetHtmlApiFullscreen(true);

  if (native_fullscreen_) {
    // Explicitly trigger a view resize, as the size is not actually changing if
    // the browser is fullscreened, too.
    source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
  }
}

void WebContents::ExitFullscreenModeForTab(content::WebContents* source) {
  if (!owner_window_)
    return;

  // This needs to be called before we exit fullscreen on the native window,
  // or the controller will incorrectly think we weren't fullscreen and bail.
  exclusive_access_manager_->fullscreen_controller()->ExitFullscreenModeForTab(
      source);

  SetHtmlApiFullscreen(false);

  if (native_fullscreen_) {
    // Explicitly trigger a view resize, as the size is not actually changing if
    // the browser is fullscreened, too. Chrome does this indirectly from
    // `chrome/browser/ui/exclusive_access/fullscreen_controller.cc`.
    source->GetRenderViewHost()->GetWidget()->SynchronizeVisualProperties();
  }
}

void WebContents::RendererUnresponsive(
    content::WebContents* source,
    content::RenderWidgetHost* render_widget_host,
    base::RepeatingClosure hang_monitor_restarter) {
  Emit("unresponsive");
}

void WebContents::RendererResponsive(
    content::WebContents* source,
    content::RenderWidgetHost* render_widget_host) {
  Emit("responsive");
}

bool WebContents::HandleContextMenu(content::RenderFrameHost& render_frame_host,
                                    const content::ContextMenuParams& params) {
  Emit("context-menu", std::make_pair(params, &render_frame_host));

  return true;
}

bool WebContents::OnGoToEntryOffset(int offset) {
  GoToOffset(offset);
  return false;
}

void WebContents::FindReply(content::WebContents* web_contents,
                            int request_id,
                            int number_of_matches,
                            const gfx::Rect& selection_rect,
                            int active_match_ordinal,
                            bool final_update) {
  if (!final_update)
    return;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary result = gin::Dictionary::CreateEmpty(isolate);
  result.Set("requestId", request_id);
  result.Set("matches", number_of_matches);
  result.Set("selectionArea", selection_rect);
  result.Set("activeMatchOrdinal", active_match_ordinal);
  result.Set("finalUpdate", final_update);  // Deprecate after 2.0
  Emit("found-in-page", result.GetHandle());
}

void WebContents::RequestExclusivePointerAccess(
    content::WebContents* web_contents,
    bool user_gesture,
    bool last_unlocked_by_target,
    bool allowed) {
  if (allowed) {
    exclusive_access_manager_->mouse_lock_controller()->RequestToLockMouse(
        web_contents, user_gesture, last_unlocked_by_target);
  } else {
    web_contents->GotResponseToLockMouseRequest(
        blink::mojom::PointerLockResult::kPermissionDenied);
  }
}

void WebContents::RequestToLockMouse(content::WebContents* web_contents,
                                     bool user_gesture,
                                     bool last_unlocked_by_target) {
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestPointerLockPermission(
      user_gesture, last_unlocked_by_target,
      base::BindOnce(&WebContents::RequestExclusivePointerAccess,
                     base::Unretained(this)));
}

void WebContents::LostMouseLock() {
  exclusive_access_manager_->mouse_lock_controller()->LostMouseLock();
}

void WebContents::RequestKeyboardLock(content::WebContents* web_contents,
                                      bool esc_key_locked) {
  exclusive_access_manager_->keyboard_lock_controller()->RequestKeyboardLock(
      web_contents, esc_key_locked);
}

void WebContents::CancelKeyboardLockRequest(
    content::WebContents* web_contents) {
  exclusive_access_manager_->keyboard_lock_controller()
      ->CancelKeyboardLockRequest(web_contents);
}

bool WebContents::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    blink::mojom::MediaStreamType type) {
  auto* web_contents =
      content::WebContents::FromRenderFrameHost(render_frame_host);
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  return permission_helper->CheckMediaAccessPermission(security_origin, type);
}

void WebContents::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    content::MediaResponseCallback callback) {
  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  permission_helper->RequestMediaAccessPermission(request, std::move(callback));
}

content::JavaScriptDialogManager* WebContents::GetJavaScriptDialogManager(
    content::WebContents* source) {
  if (!dialog_manager_)
    dialog_manager_ = std::make_unique<ElectronJavaScriptDialogManager>();

  return dialog_manager_.get();
}

void WebContents::OnAudioStateChanged(bool audible) {
  Emit("-audio-state-changed", audible);
}

void WebContents::BeforeUnloadFired(bool proceed,
                                    const base::TimeTicks& proceed_time) {
  // Do nothing, we override this method just to avoid compilation error since
  // there are two virtual functions named BeforeUnloadFired.
}

void WebContents::HandleNewRenderFrame(
    content::RenderFrameHost* render_frame_host) {
  auto* rwhv = render_frame_host->GetView();
  if (!rwhv)
    return;

  // Set the background color of RenderWidgetHostView.
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (web_preferences) {
    absl::optional<SkColor> maybe_color = web_preferences->GetBackgroundColor();
    web_contents()->SetPageBaseBackgroundColor(maybe_color);

    bool guest = IsGuest() || type_ == Type::kBrowserView;
    SkColor color =
        maybe_color.value_or(guest ? SK_ColorTRANSPARENT : SK_ColorWHITE);
    SetBackgroundColor(rwhv, color);
  }

  if (!background_throttling_)
    render_frame_host->GetRenderViewHost()->SetSchedulerThrottling(false);

  auto* rwh_impl =
      static_cast<content::RenderWidgetHostImpl*>(rwhv->GetRenderWidgetHost());
  if (rwh_impl)
    rwh_impl->disable_hidden_ = !background_throttling_;

  auto* web_frame = WebFrameMain::FromRenderFrameHost(render_frame_host);
  if (web_frame)
    web_frame->Connect();
}

void WebContents::OnBackgroundColorChanged() {
  absl::optional<SkColor> color = web_contents()->GetBackgroundColor();
  if (color.has_value()) {
    auto* const view = web_contents()->GetRenderWidgetHostView();
    static_cast<content::RenderWidgetHostViewBase*>(view)
        ->SetContentBackgroundColor(color.value());
  }
}

void WebContents::RenderFrameCreated(
    content::RenderFrameHost* render_frame_host) {
  HandleNewRenderFrame(render_frame_host);

  // RenderFrameCreated is called for speculative frames which may not be
  // used in certain cross-origin navigations. Invoking
  // RenderFrameHost::GetLifecycleState currently crashes when called for
  // speculative frames so we need to filter it out for now. Check
  // https://crbug.com/1183639 for details on when this can be removed.
  auto* rfh_impl =
      static_cast<content::RenderFrameHostImpl*>(render_frame_host);
  if (rfh_impl->lifecycle_state() ==
      content::RenderFrameHostImpl::LifecycleStateImpl::kSpeculative) {
    return;
  }

  content::RenderFrameHost::LifecycleState lifecycle_state =
      render_frame_host->GetLifecycleState();
  if (lifecycle_state == content::RenderFrameHost::LifecycleState::kActive) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin_helper::Dictionary details =
        gin_helper::Dictionary::CreateEmpty(isolate);
    details.SetGetter("frame", render_frame_host);
    Emit("frame-created", details);
  }
}

void WebContents::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  // A RenderFrameHost can be deleted when:
  // - A WebContents is removed and its containing frames are disposed.
  // - An <iframe> is removed from the DOM.
  // - Cross-origin navigation creates a new RFH in a separate process which
  //   is swapped by content::RenderFrameHostManager.
  //

  // clear out objects that have been granted permissions
  if (!granted_devices_.empty()) {
    granted_devices_.erase(render_frame_host->GetFrameTreeNodeId());
  }

  // WebFrameMain::FromRenderFrameHost(rfh) will use the RFH's FrameTreeNode ID
  // to find an existing instance of WebFrameMain. During a cross-origin
  // navigation, the deleted RFH will be the old host which was swapped out. In
  // this special case, we need to also ensure that WebFrameMain's internal RFH
  // matches before marking it as disposed.
  auto* web_frame = WebFrameMain::FromRenderFrameHost(render_frame_host);
  if (web_frame && web_frame->render_frame_host() == render_frame_host)
    web_frame->MarkRenderFrameDisposed();
}

void WebContents::RenderFrameHostChanged(content::RenderFrameHost* old_host,
                                         content::RenderFrameHost* new_host) {
  // During cross-origin navigation, a FrameTreeNode will swap out its RFH.
  // If an instance of WebFrameMain exists, it will need to have its RFH
  // swapped as well.
  //
  // |old_host| can be a nullptr so we use |new_host| for looking up the
  // WebFrameMain instance.
  auto* web_frame =
      WebFrameMain::FromFrameTreeNodeId(new_host->GetFrameTreeNodeId());
  if (web_frame) {
    web_frame->UpdateRenderFrameHost(new_host);
  }
}

void WebContents::FrameDeleted(int frame_tree_node_id) {
  auto* web_frame = WebFrameMain::FromFrameTreeNodeId(frame_tree_node_id);
  if (web_frame)
    web_frame->Destroyed();
}

void WebContents::RenderViewDeleted(content::RenderViewHost* render_view_host) {
  // This event is necessary for tracking any states with respect to
  // intermediate render view hosts aka speculative render view hosts. Currently
  // used by object-registry.js to ref count remote objects.
  Emit("render-view-deleted", render_view_host->GetProcess()->GetID());

  if (web_contents()->GetRenderViewHost() == render_view_host) {
    // When the RVH that has been deleted is the current RVH it means that the
    // the web contents are being closed. This is communicated by this event.
    // Currently tracked by guest-window-manager.ts to destroy the
    // BrowserWindow.
    Emit("current-render-view-deleted",
         render_view_host->GetProcess()->GetID());
  }
}

void WebContents::PrimaryMainFrameRenderProcessGone(
    base::TerminationStatus status) {
  auto weak_this = GetWeakPtr();
  Emit("crashed", status == base::TERMINATION_STATUS_PROCESS_WAS_KILLED);

  // User might destroy WebContents in the crashed event.
  if (!weak_this || !web_contents())
    return;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  gin_helper::Dictionary details = gin_helper::Dictionary::CreateEmpty(isolate);
  details.Set("reason", status);
  details.Set("exitCode", web_contents()->GetCrashedErrorCode());
  Emit("render-process-gone", details);
}

void WebContents::PluginCrashed(const base::FilePath& plugin_path,
                                base::ProcessId plugin_pid) {
#if BUILDFLAG(ENABLE_PLUGINS)
  content::WebPluginInfo info;
  auto* plugin_service = content::PluginService::GetInstance();
  plugin_service->GetPluginInfoByPath(plugin_path, &info);
  Emit("plugin-crashed", info.name, info.version);
#endif  // BUILDFLAG(ENABLE_PLUGINS)
}

void WebContents::MediaStartedPlaying(const MediaPlayerInfo& video_type,
                                      const content::MediaPlayerId& id) {
  Emit("media-started-playing");
}

void WebContents::MediaStoppedPlaying(
    const MediaPlayerInfo& video_type,
    const content::MediaPlayerId& id,
    content::WebContentsObserver::MediaStoppedReason reason) {
  Emit("media-paused");
}

void WebContents::DidChangeThemeColor() {
  auto theme_color = web_contents()->GetThemeColor();
  if (theme_color) {
    Emit("did-change-theme-color", electron::ToRGBHex(theme_color.value()));
  } else {
    Emit("did-change-theme-color", nullptr);
  }
}

void WebContents::DidAcquireFullscreen(content::RenderFrameHost* rfh) {
  set_fullscreen_frame(rfh);
}

void WebContents::OnWebContentsFocused(
    content::RenderWidgetHost* render_widget_host) {
  Emit("focus");
}

void WebContents::OnWebContentsLostFocus(
    content::RenderWidgetHost* render_widget_host) {
  Emit("blur");
}

void WebContents::DOMContentLoaded(
    content::RenderFrameHost* render_frame_host) {
  auto* web_frame = WebFrameMain::FromRenderFrameHost(render_frame_host);
  if (web_frame)
    web_frame->DOMContentLoaded();

  if (!render_frame_host->GetParent())
    Emit("dom-ready");
}

void WebContents::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                                const GURL& validated_url) {
  bool is_main_frame = !render_frame_host->GetParent();
  int frame_process_id = render_frame_host->GetProcess()->GetID();
  int frame_routing_id = render_frame_host->GetRoutingID();
  auto weak_this = GetWeakPtr();
  Emit("did-frame-finish-load", is_main_frame, frame_process_id,
       frame_routing_id);

  // ⚠️WARNING!⚠️
  // Emit() triggers JS which can call destroy() on |this|. It's not safe to
  // assume that |this| points to valid memory at this point.
  if (is_main_frame && weak_this && web_contents())
    Emit("did-finish-load");
}

void WebContents::DidFailLoad(content::RenderFrameHost* render_frame_host,
                              const GURL& url,
                              int error_code) {
  bool is_main_frame = !render_frame_host->GetParent();
  int frame_process_id = render_frame_host->GetProcess()->GetID();
  int frame_routing_id = render_frame_host->GetRoutingID();
  Emit("did-fail-load", error_code, "", url, is_main_frame, frame_process_id,
       frame_routing_id);
}

void WebContents::DidStartLoading() {
  Emit("did-start-loading");
}

void WebContents::DidStopLoading() {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (web_preferences && web_preferences->ShouldUsePreferredSizeMode())
    web_contents()->GetRenderViewHost()->EnablePreferredSizeMode();

  Emit("did-stop-loading");
}

bool WebContents::EmitNavigationEvent(
    const std::string& event,
    content::NavigationHandle* navigation_handle) {
  bool is_main_frame = navigation_handle->IsInMainFrame();
  int frame_tree_node_id = navigation_handle->GetFrameTreeNodeId();
  content::FrameTreeNode* frame_tree_node =
      content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);
  content::RenderFrameHostManager* render_manager =
      frame_tree_node->render_manager();
  content::RenderFrameHost* frame_host = nullptr;
  if (render_manager) {
    frame_host = render_manager->speculative_frame_host();
    if (!frame_host)
      frame_host = render_manager->current_frame_host();
  }
  int frame_process_id = -1, frame_routing_id = -1;
  if (frame_host) {
    frame_process_id = frame_host->GetProcess()->GetID();
    frame_routing_id = frame_host->GetRoutingID();
  }
  bool is_same_document = navigation_handle->IsSameDocument();
  auto url = navigation_handle->GetURL();
  return Emit(event, url, is_same_document, is_main_frame, frame_process_id,
              frame_routing_id);
}

void WebContents::Message(bool internal,
                          const std::string& channel,
                          blink::CloneableMessage arguments,
                          content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::Message", "channel", channel);
  // webContents.emit('-ipc-message', new Event(), internal, channel,
  // arguments);
  EmitWithSender("-ipc-message", render_frame_host,
                 electron::mojom::ElectronApiIPC::InvokeCallback(), internal,
                 channel, std::move(arguments));
}

void WebContents::Invoke(
    bool internal,
    const std::string& channel,
    blink::CloneableMessage arguments,
    electron::mojom::ElectronApiIPC::InvokeCallback callback,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::Invoke", "channel", channel);
  // webContents.emit('-ipc-invoke', new Event(), internal, channel, arguments);
  EmitWithSender("-ipc-invoke", render_frame_host, std::move(callback),
                 internal, channel, std::move(arguments));
}

void WebContents::OnFirstNonEmptyLayout(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host == web_contents()->GetMainFrame()) {
    Emit("ready-to-show");
  }
}

void WebContents::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message,
    content::RenderFrameHost* render_frame_host) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope handle_scope(isolate);
  auto wrapped_ports =
      MessagePort::EntanglePorts(isolate, std::move(message.ports));
  v8::Local<v8::Value> message_value =
      electron::DeserializeV8Value(isolate, message);
  EmitWithSender("-ipc-ports", render_frame_host,
                 electron::mojom::ElectronApiIPC::InvokeCallback(), false,
                 channel, message_value, std::move(wrapped_ports));
}

void WebContents::MessageSync(
    bool internal,
    const std::string& channel,
    blink::CloneableMessage arguments,
    electron::mojom::ElectronApiIPC::MessageSyncCallback callback,
    content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::MessageSync", "channel", channel);
  // webContents.emit('-ipc-message-sync', new Event(sender, message), internal,
  // channel, arguments);
  EmitWithSender("-ipc-message-sync", render_frame_host, std::move(callback),
                 internal, channel, std::move(arguments));
}

void WebContents::MessageTo(int32_t web_contents_id,
                            const std::string& channel,
                            blink::CloneableMessage arguments) {
  TRACE_EVENT1("electron", "WebContents::MessageTo", "channel", channel);
  auto* target_web_contents = FromID(web_contents_id);

  if (target_web_contents) {
    content::RenderFrameHost* frame = target_web_contents->MainFrame();
    DCHECK(frame);

    v8::HandleScope handle_scope(JavascriptEnvironment::GetIsolate());
    gin::Handle<WebFrameMain> web_frame_main =
        WebFrameMain::From(JavascriptEnvironment::GetIsolate(), frame);

    if (!web_frame_main->CheckRenderFrame())
      return;

    int32_t sender_id = ID();
    web_frame_main->GetRendererApi()->Message(false /* internal */, channel,
                                              std::move(arguments), sender_id);
  }
}

void WebContents::MessageHost(const std::string& channel,
                              blink::CloneableMessage arguments,
                              content::RenderFrameHost* render_frame_host) {
  TRACE_EVENT1("electron", "WebContents::MessageHost", "channel", channel);
  // webContents.emit('ipc-message-host', new Event(), channel, args);
  EmitWithSender("ipc-message-host", render_frame_host,
                 electron::mojom::ElectronApiIPC::InvokeCallback(), channel,
                 std::move(arguments));
}

void WebContents::UpdateDraggableRegions(
    std::vector<mojom::DraggableRegionPtr> regions) {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnDraggableRegionsUpdated(regions);
}

void WebContents::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  EmitNavigationEvent("did-start-navigation", navigation_handle);
}

void WebContents::DidRedirectNavigation(
    content::NavigationHandle* navigation_handle) {
  EmitNavigationEvent("did-redirect-navigation", navigation_handle);
}

void WebContents::ReadyToCommitNavigation(
    content::NavigationHandle* navigation_handle) {
  // Don't focus content in an inactive window.
  if (!owner_window())
    return;
#if BUILDFLAG(IS_MAC)
  if (!owner_window()->IsActive())
    return;
#else
  if (!owner_window()->widget()->IsActive())
    return;
#endif
  // Don't focus content after subframe navigations.
  if (!navigation_handle->IsInMainFrame())
    return;
  // Only focus for top-level contents.
  if (type_ != Type::kBrowserWindow)
    return;
  web_contents()->SetInitialFocus();
}

void WebContents::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (owner_window_) {
    owner_window_->NotifyLayoutWindowControlsOverlay();
  }

  if (!navigation_handle->HasCommitted())
    return;
  bool is_main_frame = navigation_handle->IsInMainFrame();
  content::RenderFrameHost* frame_host =
      navigation_handle->GetRenderFrameHost();
  int frame_process_id = -1, frame_routing_id = -1;
  if (frame_host) {
    frame_process_id = frame_host->GetProcess()->GetID();
    frame_routing_id = frame_host->GetRoutingID();
  }
  if (!navigation_handle->IsErrorPage()) {
    // FIXME: All the Emit() calls below could potentially result in |this|
    // being destroyed (by JS listening for the event and calling
    // webContents.destroy()).
    auto url = navigation_handle->GetURL();
    bool is_same_document = navigation_handle->IsSameDocument();
    if (is_same_document) {
      Emit("did-navigate-in-page", url, is_main_frame, frame_process_id,
           frame_routing_id);
    } else {
      const net::HttpResponseHeaders* http_response =
          navigation_handle->GetResponseHeaders();
      std::string http_status_text;
      int http_response_code = -1;
      if (http_response) {
        http_status_text = http_response->GetStatusText();
        http_response_code = http_response->response_code();
      }
      Emit("did-frame-navigate", url, http_response_code, http_status_text,
           is_main_frame, frame_process_id, frame_routing_id);
      if (is_main_frame) {
        Emit("did-navigate", url, http_response_code, http_status_text);
      }
    }
    if (IsGuest())
      Emit("load-commit", url, is_main_frame);
  } else {
    auto url = navigation_handle->GetURL();
    int code = navigation_handle->GetNetErrorCode();
    auto description = net::ErrorToShortString(code);
    Emit("did-fail-provisional-load", code, description, url, is_main_frame,
         frame_process_id, frame_routing_id);

    // Do not emit "did-fail-load" for canceled requests.
    if (code != net::ERR_ABORTED) {
      EmitWarning(
          node::Environment::GetCurrent(JavascriptEnvironment::GetIsolate()),
          "Failed to load URL: " + url.possibly_invalid_spec() +
              " with error: " + description,
          "electron");
      Emit("did-fail-load", code, description, url, is_main_frame,
           frame_process_id, frame_routing_id);
    }
  }
  content::NavigationEntry* entry = navigation_handle->GetNavigationEntry();

  // This check is needed due to an issue in Chromium
  // Check the Chromium issue to keep updated:
  // https://bugs.chromium.org/p/chromium/issues/detail?id=1178663
  // If a history entry has been made and the forward/back call has been made,
  // proceed with setting the new title
  if (entry && (entry->GetTransitionType() & ui::PAGE_TRANSITION_FORWARD_BACK))
    WebContents::TitleWasSet(entry);
}

void WebContents::TitleWasSet(content::NavigationEntry* entry) {
  std::u16string final_title;
  bool explicit_set = true;
  if (entry) {
    auto title = entry->GetTitle();
    auto url = entry->GetURL();
    if (url.SchemeIsFile() && title.empty()) {
      final_title = base::UTF8ToUTF16(url.ExtractFileName());
      explicit_set = false;
    } else {
      final_title = title;
    }
  } else {
    final_title = web_contents()->GetTitle();
  }
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnPageTitleUpdated(final_title, explicit_set);
  Emit("page-title-updated", final_title, explicit_set);
}

void WebContents::DidUpdateFaviconURL(
    content::RenderFrameHost* render_frame_host,
    const std::vector<blink::mojom::FaviconURLPtr>& urls) {
  std::set<GURL> unique_urls;
  for (const auto& iter : urls) {
    if (iter->icon_type != blink::mojom::FaviconIconType::kFavicon)
      continue;
    const GURL& url = iter->icon_url;
    if (url.is_valid())
      unique_urls.insert(url);
  }
  Emit("page-favicon-updated", unique_urls);
}

void WebContents::DevToolsReloadPage() {
  Emit("devtools-reload-page");
}

void WebContents::DevToolsFocused() {
  Emit("devtools-focused");
}

void WebContents::DevToolsOpened() {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  DCHECK(inspectable_web_contents_);
  DCHECK(inspectable_web_contents_->GetDevToolsWebContents());
  auto handle = FromOrCreate(
      isolate, inspectable_web_contents_->GetDevToolsWebContents());
  devtools_web_contents_.Reset(isolate, handle.ToV8());

  // Set inspected tabID.
  base::Value tab_id(ID());
  inspectable_web_contents_->CallClientFunction("DevToolsAPI.setInspectedTabId",
                                                &tab_id, nullptr, nullptr);

  // Inherit owner window in devtools when it doesn't have one.
  auto* devtools = inspectable_web_contents_->GetDevToolsWebContents();
  bool has_window = devtools->GetUserData(NativeWindowRelay::UserDataKey());
  if (owner_window() && !has_window)
    handle->SetOwnerWindow(devtools, owner_window());

  Emit("devtools-opened");
}

void WebContents::DevToolsClosed() {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::Locker locker(isolate);
  v8::HandleScope handle_scope(isolate);
  devtools_web_contents_.Reset();

  Emit("devtools-closed");
}

void WebContents::DevToolsResized() {
  for (ExtendedWebContentsObserver& observer : observers_)
    observer.OnDevToolsResized();
}

void WebContents::SetOwnerWindow(NativeWindow* owner_window) {
  SetOwnerWindow(GetWebContents(), owner_window);
}

void WebContents::SetOwnerWindow(content::WebContents* web_contents,
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

content::WebContents* WebContents::GetWebContents() const {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents_->GetWebContents();
}

content::WebContents* WebContents::GetDevToolsWebContents() const {
  if (!inspectable_web_contents_)
    return nullptr;
  return inspectable_web_contents_->GetDevToolsWebContents();
}

void WebContents::WebContentsDestroyed() {
  // Clear the pointer stored in wrapper.
  if (GetAllWebContents().Lookup(id_))
    GetAllWebContents().Remove(id_);
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  v8::Local<v8::Object> wrapper;
  if (!GetWrapper(isolate).ToLocal(&wrapper))
    return;
  wrapper->SetAlignedPointerInInternalField(0, nullptr);

  // Tell WebViewGuestDelegate that the WebContents has been destroyed.
  if (guest_delegate_)
    guest_delegate_->WillDestroy();

  Observe(nullptr);
  Emit("destroyed");
}

void WebContents::NavigationEntryCommitted(
    const content::LoadCommittedDetails& details) {
  Emit("navigation-entry-committed", details.entry->GetURL(),
       details.is_same_document, details.did_replace_entry);
}

bool WebContents::GetBackgroundThrottling() const {
  return background_throttling_;
}

void WebContents::SetBackgroundThrottling(bool allowed) {
  background_throttling_ = allowed;

  auto* rfh = web_contents()->GetMainFrame();
  if (!rfh)
    return;

  auto* rwhv = rfh->GetView();
  if (!rwhv)
    return;

  auto* rwh_impl =
      static_cast<content::RenderWidgetHostImpl*>(rwhv->GetRenderWidgetHost());
  if (!rwh_impl)
    return;

  rwh_impl->disable_hidden_ = !background_throttling_;
  web_contents()->GetRenderViewHost()->SetSchedulerThrottling(allowed);

  if (rwh_impl->is_hidden()) {
    rwh_impl->WasShown({});
  }
}

int WebContents::GetProcessID() const {
  return web_contents()->GetMainFrame()->GetProcess()->GetID();
}

base::ProcessId WebContents::GetOSProcessID() const {
  base::ProcessHandle process_handle =
      web_contents()->GetMainFrame()->GetProcess()->GetProcess().Handle();
  return base::GetProcId(process_handle);
}

WebContents::Type WebContents::GetType() const {
  return type_;
}

bool WebContents::Equal(const WebContents* web_contents) const {
  return ID() == web_contents->ID();
}

GURL WebContents::GetURL() const {
  return web_contents()->GetLastCommittedURL();
}

void WebContents::LoadURL(const GURL& url,
                          const gin_helper::Dictionary& options) {
  if (!url.is_valid() || url.spec().size() > url::kMaxURLChars) {
    Emit("did-fail-load", static_cast<int>(net::ERR_INVALID_URL),
         net::ErrorToShortString(net::ERR_INVALID_URL),
         url.possibly_invalid_spec(), true);
    return;
  }

  content::NavigationController::LoadURLParams params(url);

  if (!options.Get("httpReferrer", &params.referrer)) {
    GURL http_referrer;
    if (options.Get("httpReferrer", &http_referrer))
      params.referrer =
          content::Referrer(http_referrer.GetAsReferrer(),
                            network::mojom::ReferrerPolicy::kDefault);
  }

  std::string user_agent;
  if (options.Get("userAgent", &user_agent))
    web_contents()->SetUserAgentOverride(
        blink::UserAgentOverride::UserAgentOnly(user_agent), false);

  std::string extra_headers;
  if (options.Get("extraHeaders", &extra_headers))
    params.extra_headers = extra_headers;

  scoped_refptr<network::ResourceRequestBody> body;
  if (options.Get("postData", &body)) {
    params.post_data = body;
    params.load_type = content::NavigationController::LOAD_TYPE_HTTP_POST;
  }

  GURL base_url_for_data_url;
  if (options.Get("baseURLForDataURL", &base_url_for_data_url)) {
    params.base_url_for_data_url = base_url_for_data_url;
    params.load_type = content::NavigationController::LOAD_TYPE_DATA;
  }

  bool reload_ignoring_cache = false;
  if (options.Get("reloadIgnoringCache", &reload_ignoring_cache) &&
      reload_ignoring_cache) {
    params.reload_type = content::ReloadType::BYPASSING_CACHE;
  }

  // Calling LoadURLWithParams() can trigger JS which destroys |this|.
  auto weak_this = GetWeakPtr();

  params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_ADDRESS_BAR);
  params.override_user_agent = content::NavigationController::UA_OVERRIDE_TRUE;
  // Discord non-committed entries to ensure that we don't re-use a pending
  // entry
  web_contents()->GetController().DiscardNonCommittedEntries();
  web_contents()->GetController().LoadURLWithParams(params);

  // ⚠️WARNING!⚠️
  // LoadURLWithParams() triggers JS events which can call destroy() on |this|.
  // It's not safe to assume that |this| points to valid memory at this point.
  if (!weak_this || !web_contents())
    return;

  // Required to make beforeunload handler work.
  NotifyUserActivation();
}

// TODO(MarshallOfSound): Figure out what we need to do with post data here, I
// believe the default behavior when we pass "true" is to phone out to the
// delegate and then the controller expects this method to be called again with
// "false" if the user approves the reload.  For now this would result in
// ".reload()" calls on POST data domains failing silently.  Passing false would
// result in them succeeding, but reposting which although more correct could be
// considering a breaking change.
void WebContents::Reload() {
  web_contents()->GetController().Reload(content::ReloadType::NORMAL,
                                         /* check_for_repost */ true);
}

void WebContents::ReloadIgnoringCache() {
  web_contents()->GetController().Reload(content::ReloadType::BYPASSING_CACHE,
                                         /* check_for_repost */ true);
}

void WebContents::DownloadURL(const GURL& url) {
  auto* browser_context = web_contents()->GetBrowserContext();
  auto* download_manager = browser_context->GetDownloadManager();
  std::unique_ptr<download::DownloadUrlParameters> download_params(
      content::DownloadRequestUtils::CreateDownloadForWebContentsMainFrame(
          web_contents(), url, MISSING_TRAFFIC_ANNOTATION));
  download_manager->DownloadUrl(std::move(download_params));
}

std::u16string WebContents::GetTitle() const {
  return web_contents()->GetTitle();
}

bool WebContents::IsLoading() const {
  return web_contents()->IsLoading();
}

bool WebContents::IsLoadingMainFrame() const {
  return web_contents()->ShouldShowLoadingUI();
}

bool WebContents::IsWaitingForResponse() const {
  return web_contents()->IsWaitingForResponse();
}

void WebContents::Stop() {
  web_contents()->Stop();
}

bool WebContents::CanGoBack() const {
  return web_contents()->GetController().CanGoBack();
}

void WebContents::GoBack() {
  if (CanGoBack())
    web_contents()->GetController().GoBack();
}

bool WebContents::CanGoForward() const {
  return web_contents()->GetController().CanGoForward();
}

void WebContents::GoForward() {
  if (CanGoForward())
    web_contents()->GetController().GoForward();
}

bool WebContents::CanGoToOffset(int offset) const {
  return web_contents()->GetController().CanGoToOffset(offset);
}

void WebContents::GoToOffset(int offset) {
  if (CanGoToOffset(offset))
    web_contents()->GetController().GoToOffset(offset);
}

bool WebContents::CanGoToIndex(int index) const {
  return index >= 0 && index < GetHistoryLength();
}

void WebContents::GoToIndex(int index) {
  if (CanGoToIndex(index))
    web_contents()->GetController().GoToIndex(index);
}

int WebContents::GetActiveIndex() const {
  return web_contents()->GetController().GetCurrentEntryIndex();
}

void WebContents::ClearHistory() {
  // In some rare cases (normally while there is no real history) we are in a
  // state where we can't prune navigation entries
  if (web_contents()->GetController().CanPruneAllButLastCommitted()) {
    web_contents()->GetController().PruneAllButLastCommitted();
  }
}

int WebContents::GetHistoryLength() const {
  return web_contents()->GetController().GetEntryCount();
}

const std::string WebContents::GetWebRTCIPHandlingPolicy() const {
  return web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy;
}

void WebContents::SetWebRTCIPHandlingPolicy(
    const std::string& webrtc_ip_handling_policy) {
  if (GetWebRTCIPHandlingPolicy() == webrtc_ip_handling_policy)
    return;
  web_contents()->GetMutableRendererPrefs()->webrtc_ip_handling_policy =
      webrtc_ip_handling_policy;

  web_contents()->SyncRendererPrefs();
}

std::string WebContents::GetMediaSourceID(
    content::WebContents* request_web_contents) {
  auto* frame_host = web_contents()->GetMainFrame();
  if (!frame_host)
    return std::string();

  content::DesktopMediaID media_id(
      content::DesktopMediaID::TYPE_WEB_CONTENTS,
      content::DesktopMediaID::kNullId,
      content::WebContentsMediaCaptureId(frame_host->GetProcess()->GetID(),
                                         frame_host->GetRoutingID()));

  auto* request_frame_host = request_web_contents->GetMainFrame();
  if (!request_frame_host)
    return std::string();

  std::string id =
      content::DesktopStreamsRegistry::GetInstance()->RegisterStream(
          request_frame_host->GetProcess()->GetID(),
          request_frame_host->GetRoutingID(),
          url::Origin::Create(request_frame_host->GetLastCommittedURL()
                                  .DeprecatedGetOriginAsURL()),
          media_id, "", content::kRegistryStreamTypeTab);

  return id;
}

bool WebContents::IsCrashed() const {
  return web_contents()->IsCrashed();
}

void WebContents::ForcefullyCrashRenderer() {
  content::RenderWidgetHostView* view =
      web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;

  content::RenderWidgetHost* rwh = view->GetRenderWidgetHost();
  if (!rwh)
    return;

  content::RenderProcessHost* rph = rwh->GetProcess();
  if (rph) {
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)
    // A generic |CrashDumpHungChildProcess()| is not implemented for Linux.
    // Instead we send an explicit IPC to crash on the renderer's IO thread.
    rph->ForceCrash();
#else
    // Try to generate a crash report for the hung process.
#ifndef MAS_BUILD
    CrashDumpHungChildProcess(rph->GetProcess().Handle());
#endif
    rph->Shutdown(content::RESULT_CODE_HUNG);
#endif
  }
}

void WebContents::SetUserAgent(const std::string& user_agent) {
  web_contents()->SetUserAgentOverride(
      blink::UserAgentOverride::UserAgentOnly(user_agent), false);
}

std::string WebContents::GetUserAgent() {
  return web_contents()->GetUserAgentOverride().ua_string_override;
}

v8::Local<v8::Promise> WebContents::SavePage(
    const base::FilePath& full_file_path,
    const content::SavePageType& save_type) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!full_file_path.IsAbsolute()) {
    promise.RejectWithErrorMessage("Path must be absolute");
    return handle;
  }

  auto* handler = new SavePageHandler(web_contents(), std::move(promise));
  handler->Handle(full_file_path, save_type);

  return handle;
}

void WebContents::OpenDevTools(gin::Arguments* args) {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  std::string state;
  if (type_ == Type::kWebView || type_ == Type::kBackgroundPage ||
      !owner_window()) {
    state = "detach";
  }
  bool activate = true;
  if (args && args->Length() == 1) {
    gin_helper::Dictionary options;
    if (args->GetNext(&options)) {
      options.Get("mode", &state);
      options.Get("activate", &activate);
    }
  }

  DCHECK(inspectable_web_contents_);
  inspectable_web_contents_->SetDockState(state);
  inspectable_web_contents_->ShowDevTools(activate);
}

void WebContents::CloseDevTools() {
  if (type_ == Type::kRemote)
    return;

  DCHECK(inspectable_web_contents_);
  inspectable_web_contents_->CloseDevTools();
}

bool WebContents::IsDevToolsOpened() {
  if (type_ == Type::kRemote)
    return false;

  DCHECK(inspectable_web_contents_);
  return inspectable_web_contents_->IsDevToolsViewShowing();
}

bool WebContents::IsDevToolsFocused() {
  if (type_ == Type::kRemote)
    return false;

  DCHECK(inspectable_web_contents_);
  return inspectable_web_contents_->GetView()->IsDevToolsViewFocused();
}

void WebContents::EnableDeviceEmulation(
    const blink::DeviceEmulationParams& params) {
  if (type_ == Type::kRemote)
    return;

  DCHECK(web_contents());
  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    auto* widget_host_impl = static_cast<content::RenderWidgetHostImpl*>(
        frame_host->GetView()->GetRenderWidgetHost());
    if (widget_host_impl) {
      auto& frame_widget = widget_host_impl->GetAssociatedFrameWidget();
      frame_widget->EnableDeviceEmulation(params);
    }
  }
}

void WebContents::DisableDeviceEmulation() {
  if (type_ == Type::kRemote)
    return;

  DCHECK(web_contents());
  auto* frame_host = web_contents()->GetMainFrame();
  if (frame_host) {
    auto* widget_host_impl = static_cast<content::RenderWidgetHostImpl*>(
        frame_host->GetView()->GetRenderWidgetHost());
    if (widget_host_impl) {
      auto& frame_widget = widget_host_impl->GetAssociatedFrameWidget();
      frame_widget->DisableDeviceEmulation();
    }
  }
}

void WebContents::ToggleDevTools() {
  if (IsDevToolsOpened())
    CloseDevTools();
  else
    OpenDevTools(nullptr);
}

void WebContents::InspectElement(int x, int y) {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  DCHECK(inspectable_web_contents_);
  if (!inspectable_web_contents_->GetDevToolsWebContents())
    OpenDevTools(nullptr);
  inspectable_web_contents_->InspectElement(x, y);
}

void WebContents::InspectSharedWorkerById(const std::string& workerId) {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeSharedWorker) {
      if (agent_host->GetId() == workerId) {
        OpenDevTools(nullptr);
        inspectable_web_contents_->AttachTo(agent_host);
        break;
      }
    }
  }
}

std::vector<scoped_refptr<content::DevToolsAgentHost>>
WebContents::GetAllSharedWorkers() {
  std::vector<scoped_refptr<content::DevToolsAgentHost>> shared_workers;

  if (type_ == Type::kRemote)
    return shared_workers;

  if (!enable_devtools_)
    return shared_workers;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeSharedWorker) {
      shared_workers.push_back(agent_host);
    }
  }
  return shared_workers;
}

void WebContents::InspectSharedWorker() {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeSharedWorker) {
      OpenDevTools(nullptr);
      inspectable_web_contents_->AttachTo(agent_host);
      break;
    }
  }
}

void WebContents::InspectServiceWorker() {
  if (type_ == Type::kRemote)
    return;

  if (!enable_devtools_)
    return;

  for (const auto& agent_host : content::DevToolsAgentHost::GetOrCreateAll()) {
    if (agent_host->GetType() ==
        content::DevToolsAgentHost::kTypeServiceWorker) {
      OpenDevTools(nullptr);
      inspectable_web_contents_->AttachTo(agent_host);
      break;
    }
  }
}

void WebContents::SetIgnoreMenuShortcuts(bool ignore) {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  DCHECK(web_preferences);
  web_preferences->SetIgnoreMenuShortcuts(ignore);
}

void WebContents::SetAudioMuted(bool muted) {
  web_contents()->SetAudioMuted(muted);
}

bool WebContents::IsAudioMuted() {
  return web_contents()->IsAudioMuted();
}

bool WebContents::IsCurrentlyAudible() {
  return web_contents()->IsCurrentlyAudible();
}

#if BUILDFLAG(ENABLE_PRINTING)
void WebContents::OnGetDefaultPrinter(
    base::Value print_settings,
    printing::CompletionCallback print_callback,
    std::u16string device_name,
    bool silent,
    // <error, default_printer>
    std::pair<std::string, std::u16string> info) {
  // The content::WebContents might be already deleted at this point, and the
  // PrintViewManagerElectron class does not do null check.
  if (!web_contents()) {
    if (print_callback)
      std::move(print_callback).Run(false, "failed");
    return;
  }

  if (!info.first.empty()) {
    if (print_callback)
      std::move(print_callback).Run(false, info.first);
    return;
  }

  // If the user has passed a deviceName use it, otherwise use default printer.
  std::u16string printer_name = device_name.empty() ? info.second : device_name;

  // If there are no valid printers available on the network, we bail.
  if (printer_name.empty() || !IsDeviceNameValid(printer_name)) {
    if (print_callback)
      std::move(print_callback).Run(false, "no valid printers available");
    return;
  }

  print_settings.SetStringKey(printing::kSettingDeviceName, printer_name);

  auto* print_view_manager =
      PrintViewManagerElectron::FromWebContents(web_contents());
  if (!print_view_manager)
    return;

  auto* focused_frame = web_contents()->GetFocusedFrame();
  auto* rfh = focused_frame && focused_frame->HasSelection()
                  ? focused_frame
                  : web_contents()->GetMainFrame();

  print_view_manager->PrintNow(rfh, silent, std::move(print_settings),
                               std::move(print_callback));
}

void WebContents::Print(gin::Arguments* args) {
  gin_helper::Dictionary options =
      gin::Dictionary::CreateEmpty(args->isolate());
  base::Value settings(base::Value::Type::DICTIONARY);

  if (args->Length() >= 1 && !args->GetNext(&options)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("webContents.print(): Invalid print settings specified.");
    return;
  }

  printing::CompletionCallback callback;
  if (args->Length() == 2 && !args->GetNext(&callback)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("webContents.print(): Invalid optional callback provided.");
    return;
  }

  // Set optional silent printing
  bool silent = false;
  options.Get("silent", &silent);

  bool print_background = false;
  options.Get("printBackground", &print_background);
  settings.SetBoolKey(printing::kSettingShouldPrintBackgrounds,
                      print_background);

  // Set custom margin settings
  gin_helper::Dictionary margins =
      gin::Dictionary::CreateEmpty(args->isolate());
  if (options.Get("margins", &margins)) {
    printing::mojom::MarginType margin_type =
        printing::mojom::MarginType::kDefaultMargins;
    margins.Get("marginType", &margin_type);
    settings.SetIntKey(printing::kSettingMarginsType,
                       static_cast<int>(margin_type));

    if (margin_type == printing::mojom::MarginType::kCustomMargins) {
      base::Value custom_margins(base::Value::Type::DICTIONARY);
      int top = 0;
      margins.Get("top", &top);
      custom_margins.SetIntKey(printing::kSettingMarginTop, top);
      int bottom = 0;
      margins.Get("bottom", &bottom);
      custom_margins.SetIntKey(printing::kSettingMarginBottom, bottom);
      int left = 0;
      margins.Get("left", &left);
      custom_margins.SetIntKey(printing::kSettingMarginLeft, left);
      int right = 0;
      margins.Get("right", &right);
      custom_margins.SetIntKey(printing::kSettingMarginRight, right);
      settings.SetPath(printing::kSettingMarginsCustom,
                       std::move(custom_margins));
    }
  } else {
    settings.SetIntKey(
        printing::kSettingMarginsType,
        static_cast<int>(printing::mojom::MarginType::kDefaultMargins));
  }

  // Set whether to print color or greyscale
  bool print_color = true;
  options.Get("color", &print_color);
  auto const color_model = print_color ? printing::mojom::ColorModel::kColor
                                       : printing::mojom::ColorModel::kGray;
  settings.SetIntKey(printing::kSettingColor, static_cast<int>(color_model));

  // Is the orientation landscape or portrait.
  bool landscape = false;
  options.Get("landscape", &landscape);
  settings.SetBoolKey(printing::kSettingLandscape, landscape);

  // We set the default to the system's default printer and only update
  // if at the Chromium level if the user overrides.
  // Printer device name as opened by the OS.
  std::u16string device_name;
  options.Get("deviceName", &device_name);
  if (!device_name.empty() && !IsDeviceNameValid(device_name)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("webContents.print(): Invalid deviceName provided.");
    return;
  }

  int scale_factor = 100;
  options.Get("scaleFactor", &scale_factor);
  settings.SetIntKey(printing::kSettingScaleFactor, scale_factor);

  int pages_per_sheet = 1;
  options.Get("pagesPerSheet", &pages_per_sheet);
  settings.SetIntKey(printing::kSettingPagesPerSheet, pages_per_sheet);

  // True if the user wants to print with collate.
  bool collate = true;
  options.Get("collate", &collate);
  settings.SetBoolKey(printing::kSettingCollate, collate);

  // The number of individual copies to print
  int copies = 1;
  options.Get("copies", &copies);
  settings.SetIntKey(printing::kSettingCopies, copies);

  // Strings to be printed as headers and footers if requested by the user.
  std::string header;
  options.Get("header", &header);
  std::string footer;
  options.Get("footer", &footer);

  if (!(header.empty() && footer.empty())) {
    settings.SetBoolKey(printing::kSettingHeaderFooterEnabled, true);

    settings.SetStringKey(printing::kSettingHeaderFooterTitle, header);
    settings.SetStringKey(printing::kSettingHeaderFooterURL, footer);
  } else {
    settings.SetBoolKey(printing::kSettingHeaderFooterEnabled, false);
  }

  // We don't want to allow the user to enable these settings
  // but we need to set them or a CHECK is hit.
  settings.SetIntKey(printing::kSettingPrinterType,
                     static_cast<int>(printing::mojom::PrinterType::kLocal));
  settings.SetBoolKey(printing::kSettingShouldPrintSelectionOnly, false);
  settings.SetBoolKey(printing::kSettingRasterizePdf, false);

  // Set custom page ranges to print
  std::vector<gin_helper::Dictionary> page_ranges;
  if (options.Get("pageRanges", &page_ranges)) {
    base::Value page_range_list(base::Value::Type::LIST);
    for (auto& range : page_ranges) {
      int from, to;
      if (range.Get("from", &from) && range.Get("to", &to)) {
        base::Value range(base::Value::Type::DICTIONARY);
        // Chromium uses 1-based page ranges, so increment each by 1.
        range.SetIntKey(printing::kSettingPageRangeFrom, from + 1);
        range.SetIntKey(printing::kSettingPageRangeTo, to + 1);
        page_range_list.Append(std::move(range));
      } else {
        continue;
      }
    }
    if (!page_range_list.GetListDeprecated().empty())
      settings.SetPath(printing::kSettingPageRange, std::move(page_range_list));
  }

  // Duplex type user wants to use.
  printing::mojom::DuplexMode duplex_mode =
      printing::mojom::DuplexMode::kSimplex;
  options.Get("duplexMode", &duplex_mode);
  settings.SetIntKey(printing::kSettingDuplexMode,
                     static_cast<int>(duplex_mode));

  // We've already done necessary parameter sanitization at the
  // JS level, so we can simply pass this through.
  base::Value media_size(base::Value::Type::DICTIONARY);
  if (options.Get("mediaSize", &media_size))
    settings.SetKey(printing::kSettingMediaSize, std::move(media_size));

  // Set custom dots per inch (dpi)
  gin_helper::Dictionary dpi_settings;
  int dpi = 72;
  if (options.Get("dpi", &dpi_settings)) {
    int horizontal = 72;
    dpi_settings.Get("horizontal", &horizontal);
    settings.SetIntKey(printing::kSettingDpiHorizontal, horizontal);
    int vertical = 72;
    dpi_settings.Get("vertical", &vertical);
    settings.SetIntKey(printing::kSettingDpiVertical, vertical);
  } else {
    settings.SetIntKey(printing::kSettingDpiHorizontal, dpi);
    settings.SetIntKey(printing::kSettingDpiVertical, dpi);
  }

  print_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GetDefaultPrinterAsync),
      base::BindOnce(&WebContents::OnGetDefaultPrinter,
                     weak_factory_.GetWeakPtr(), std::move(settings),
                     std::move(callback), device_name, silent));
}

v8::Local<v8::Promise> WebContents::PrintToPDF(base::DictionaryValue settings) {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  gin_helper::Promise<v8::Local<v8::Value>> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();
  PrintPreviewMessageHandler::FromWebContents(web_contents())
      ->PrintToPDF(std::move(settings), std::move(promise));
  return handle;
}
#endif

void WebContents::AddWorkSpace(gin::Arguments* args,
                               const base::FilePath& path) {
  if (path.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("path cannot be empty");
    return;
  }
  DevToolsAddFileSystem(std::string(), path);
}

void WebContents::RemoveWorkSpace(gin::Arguments* args,
                                  const base::FilePath& path) {
  if (path.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("path cannot be empty");
    return;
  }
  DevToolsRemoveFileSystem(path);
}

void WebContents::Undo() {
  web_contents()->Undo();
}

void WebContents::Redo() {
  web_contents()->Redo();
}

void WebContents::Cut() {
  web_contents()->Cut();
}

void WebContents::Copy() {
  web_contents()->Copy();
}

void WebContents::Paste() {
  web_contents()->Paste();
}

void WebContents::PasteAndMatchStyle() {
  web_contents()->PasteAndMatchStyle();
}

void WebContents::Delete() {
  web_contents()->Delete();
}

void WebContents::SelectAll() {
  web_contents()->SelectAll();
}

void WebContents::Unselect() {
  web_contents()->CollapseSelection();
}

void WebContents::Replace(const std::u16string& word) {
  web_contents()->Replace(word);
}

void WebContents::ReplaceMisspelling(const std::u16string& word) {
  web_contents()->ReplaceMisspelling(word);
}

uint32_t WebContents::FindInPage(gin::Arguments* args) {
  std::u16string search_text;
  if (!args->GetNext(&search_text) || search_text.empty()) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("Must provide a non-empty search content");
    return 0;
  }

  uint32_t request_id = ++find_in_page_request_id_;
  gin_helper::Dictionary dict;
  auto options = blink::mojom::FindOptions::New();
  if (args->GetNext(&dict)) {
    dict.Get("forward", &options->forward);
    dict.Get("matchCase", &options->match_case);
    dict.Get("findNext", &options->new_session);
  }

  web_contents()->Find(request_id, search_text, std::move(options));
  return request_id;
}

void WebContents::StopFindInPage(content::StopFindAction action) {
  web_contents()->StopFinding(action);
}

void WebContents::ShowDefinitionForSelection() {
#if BUILDFLAG(IS_MAC)
  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (view)
    view->ShowDefinitionForSelection();
#endif
}

void WebContents::CopyImageAt(int x, int y) {
  auto* const host = web_contents()->GetMainFrame();
  if (host)
    host->CopyImageAt(x, y);
}

void WebContents::Focus() {
  // Focusing on WebContents does not automatically focus the window on macOS
  // and Linux, do it manually to match the behavior on Windows.
#if BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  if (owner_window())
    owner_window()->Focus(true);
#endif
  web_contents()->Focus();
}

#if !BUILDFLAG(IS_MAC)
bool WebContents::IsFocused() const {
  auto* view = web_contents()->GetRenderWidgetHostView();
  if (!view)
    return false;

  if (GetType() != Type::kBackgroundPage) {
    auto* window = web_contents()->GetNativeView()->GetToplevelWindow();
    if (window && !window->IsVisible())
      return false;
  }

  return view->HasFocus();
}
#endif

void WebContents::SendInputEvent(v8::Isolate* isolate,
                                 v8::Local<v8::Value> input_event) {
  content::RenderWidgetHostView* view =
      web_contents()->GetRenderWidgetHostView();
  if (!view)
    return;

  content::RenderWidgetHost* rwh = view->GetRenderWidgetHost();
  blink::WebInputEvent::Type type =
      gin::GetWebInputEventType(isolate, input_event);
  if (blink::WebInputEvent::IsMouseEventType(type)) {
    blink::WebMouseEvent mouse_event;
    if (gin::ConvertFromV8(isolate, input_event, &mouse_event)) {
      if (IsOffScreen()) {
#if BUILDFLAG(ENABLE_OSR)
        GetOffScreenRenderWidgetHostView()->SendMouseEvent(mouse_event);
#endif
      } else {
        rwh->ForwardMouseEvent(mouse_event);
      }
      return;
    }
  } else if (blink::WebInputEvent::IsKeyboardEventType(type)) {
    content::NativeWebKeyboardEvent keyboard_event(
        blink::WebKeyboardEvent::Type::kRawKeyDown,
        blink::WebInputEvent::Modifiers::kNoModifiers, ui::EventTimeForNow());
    if (gin::ConvertFromV8(isolate, input_event, &keyboard_event)) {
      rwh->ForwardKeyboardEvent(keyboard_event);
      return;
    }
  } else if (type == blink::WebInputEvent::Type::kMouseWheel) {
    blink::WebMouseWheelEvent mouse_wheel_event;
    if (gin::ConvertFromV8(isolate, input_event, &mouse_wheel_event)) {
      if (IsOffScreen()) {
#if BUILDFLAG(ENABLE_OSR)
        GetOffScreenRenderWidgetHostView()->SendMouseWheelEvent(
            mouse_wheel_event);
#endif
      } else {
        // Chromium expects phase info in wheel events (and applies a
        // DCHECK to verify it). See: https://crbug.com/756524.
        mouse_wheel_event.phase = blink::WebMouseWheelEvent::kPhaseBegan;
        mouse_wheel_event.dispatch_type =
            blink::WebInputEvent::DispatchType::kBlocking;
        rwh->ForwardWheelEvent(mouse_wheel_event);

        // Send a synthetic wheel event with phaseEnded to finish scrolling.
        mouse_wheel_event.has_synthetic_phase = true;
        mouse_wheel_event.delta_x = 0;
        mouse_wheel_event.delta_y = 0;
        mouse_wheel_event.phase = blink::WebMouseWheelEvent::kPhaseEnded;
        mouse_wheel_event.dispatch_type =
            blink::WebInputEvent::DispatchType::kEventNonBlocking;
        rwh->ForwardWheelEvent(mouse_wheel_event);
      }
      return;
    }
  }

  isolate->ThrowException(
      v8::Exception::Error(gin::StringToV8(isolate, "Invalid event object")));
}

void WebContents::BeginFrameSubscription(gin::Arguments* args) {
  bool only_dirty = false;
  FrameSubscriber::FrameCaptureCallback callback;

  if (args->Length() > 1) {
    if (!args->GetNext(&only_dirty)) {
      args->ThrowError();
      return;
    }
  }
  if (!args->GetNext(&callback)) {
    args->ThrowError();
    return;
  }

  frame_subscriber_ =
      std::make_unique<FrameSubscriber>(web_contents(), callback, only_dirty);
}

void WebContents::EndFrameSubscription() {
  frame_subscriber_.reset();
}

void WebContents::StartDrag(const gin_helper::Dictionary& item,
                            gin::Arguments* args) {
  base::FilePath file;
  std::vector<base::FilePath> files;
  if (!item.Get("files", &files) && item.Get("file", &file)) {
    files.push_back(file);
  }

  v8::Local<v8::Value> icon_value;
  if (!item.Get("icon", &icon_value)) {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("'icon' parameter is required");
    return;
  }

  NativeImage* icon = nullptr;
  if (!NativeImage::TryConvertNativeImage(args->isolate(), icon_value, &icon) ||
      icon->image().IsEmpty()) {
    return;
  }

  // Start dragging.
  if (!files.empty()) {
    base::CurrentThread::ScopedNestableTaskAllower allow;
    DragFileItems(files, icon->image(), web_contents()->GetNativeView());
  } else {
    gin_helper::ErrorThrower(args->isolate())
        .ThrowError("Must specify either 'file' or 'files' option");
  }
}

v8::Local<v8::Promise> WebContents::CapturePage(gin::Arguments* args) {
  gfx::Rect rect;
  gin_helper::Promise<gfx::Image> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // get rect arguments if they exist
  args->GetNext(&rect);

  auto* const view = web_contents()->GetRenderWidgetHostView();
  if (!view) {
    promise.Resolve(gfx::Image());
    return handle;
  }

#if !BUILDFLAG(IS_MAC)
  // If the view's renderer is suspended this may fail on Windows/Linux -
  // bail if so. See CopyFromSurface in
  // content/public/browser/render_widget_host_view.h.
  auto* rfh = web_contents()->GetMainFrame();
  if (rfh &&
      rfh->GetVisibilityState() == blink::mojom::PageVisibilityState::kHidden) {
    promise.Resolve(gfx::Image());
    return handle;
  }
#endif  // BUILDFLAG(IS_MAC)

  // Capture full page if user doesn't specify a |rect|.
  const gfx::Size view_size =
      rect.IsEmpty() ? view->GetViewBounds().size() : rect.size();

  // By default, the requested bitmap size is the view size in screen
  // coordinates.  However, if there's more pixel detail available on the
  // current system, increase the requested bitmap size to capture it all.
  gfx::Size bitmap_size = view_size;
  const gfx::NativeView native_view = view->GetNativeView();
  const float scale = display::Screen::GetScreen()
                          ->GetDisplayNearestView(native_view)
                          .device_scale_factor();
  if (scale > 1.0f)
    bitmap_size = gfx::ScaleToCeiledSize(view_size, scale);

  view->CopyFromSurface(gfx::Rect(rect.origin(), view_size), bitmap_size,
                        base::BindOnce(&OnCapturePageDone, std::move(promise)));
  return handle;
}

void WebContents::IncrementCapturerCount(gin::Arguments* args) {
  gfx::Size size;
  bool stay_hidden = false;
  bool stay_awake = false;

  // get size arguments if they exist
  args->GetNext(&size);
  // get stayHidden arguments if they exist
  args->GetNext(&stay_hidden);
  // get stayAwake arguments if they exist
  args->GetNext(&stay_awake);

  std::ignore = web_contents()
                    ->IncrementCapturerCount(size, stay_hidden, stay_awake)
                    .Release();
}

void WebContents::DecrementCapturerCount(gin::Arguments* args) {
  bool stay_hidden = false;
  bool stay_awake = false;

  // get stayHidden arguments if they exist
  args->GetNext(&stay_hidden);
  // get stayAwake arguments if they exist
  args->GetNext(&stay_awake);

  web_contents()->DecrementCapturerCount(stay_hidden, stay_awake);
}

bool WebContents::IsBeingCaptured() {
  return web_contents()->IsBeingCaptured();
}

void WebContents::OnCursorChanged(const content::WebCursor& webcursor) {
  const ui::Cursor& cursor = webcursor.cursor();

  if (cursor.type() == ui::mojom::CursorType::kCustom) {
    Emit("cursor-changed", CursorTypeToString(cursor),
         gfx::Image::CreateFrom1xBitmap(cursor.custom_bitmap()),
         cursor.image_scale_factor(),
         gfx::Size(cursor.custom_bitmap().width(),
                   cursor.custom_bitmap().height()),
         cursor.custom_hotspot());
  } else {
    Emit("cursor-changed", CursorTypeToString(cursor));
  }
}

bool WebContents::IsGuest() const {
  return type_ == Type::kWebView;
}

void WebContents::AttachToIframe(content::WebContents* embedder_web_contents,
                                 int embedder_frame_id) {
  attached_ = true;
  if (guest_delegate_)
    guest_delegate_->AttachToIframe(embedder_web_contents, embedder_frame_id);
}

bool WebContents::IsOffScreen() const {
#if BUILDFLAG(ENABLE_OSR)
  return type_ == Type::kOffScreen;
#else
  return false;
#endif
}

#if BUILDFLAG(ENABLE_OSR)
void WebContents::OnPaint(const gfx::Rect& dirty_rect, const SkBitmap& bitmap) {
  Emit("paint", dirty_rect, gfx::Image::CreateFrom1xBitmap(bitmap));
}

void WebContents::StartPainting() {
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetPainting(true);
}

void WebContents::StopPainting() {
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetPainting(false);
}

bool WebContents::IsPainting() const {
  auto* osr_wcv = GetOffScreenWebContentsView();
  return osr_wcv && osr_wcv->IsPainting();
}

void WebContents::SetFrameRate(int frame_rate) {
  auto* osr_wcv = GetOffScreenWebContentsView();
  if (osr_wcv)
    osr_wcv->SetFrameRate(frame_rate);
}

int WebContents::GetFrameRate() const {
  auto* osr_wcv = GetOffScreenWebContentsView();
  return osr_wcv ? osr_wcv->GetFrameRate() : 0;
}
#endif

void WebContents::Invalidate() {
  if (IsOffScreen()) {
#if BUILDFLAG(ENABLE_OSR)
    auto* osr_rwhv = GetOffScreenRenderWidgetHostView();
    if (osr_rwhv)
      osr_rwhv->Invalidate();
#endif
  } else {
    auto* const window = owner_window();
    if (window)
      window->Invalidate();
  }
}

gfx::Size WebContents::GetSizeForNewRenderView(content::WebContents* wc) {
  if (IsOffScreen() && wc == web_contents()) {
    auto* relay = NativeWindowRelay::FromWebContents(web_contents());
    if (relay) {
      auto* owner_window = relay->GetNativeWindow();
      return owner_window ? owner_window->GetSize() : gfx::Size();
    }
  }

  return gfx::Size();
}

void WebContents::SetZoomLevel(double level) {
  zoom_controller_->SetZoomLevel(level);
}

double WebContents::GetZoomLevel() const {
  return zoom_controller_->GetZoomLevel();
}

void WebContents::SetZoomFactor(gin_helper::ErrorThrower thrower,
                                double factor) {
  if (factor < std::numeric_limits<double>::epsilon()) {
    thrower.ThrowError("'zoomFactor' must be a double greater than 0.0");
    return;
  }

  auto level = blink::PageZoomFactorToZoomLevel(factor);
  SetZoomLevel(level);
}

double WebContents::GetZoomFactor() const {
  auto level = GetZoomLevel();
  return blink::PageZoomLevelToZoomFactor(level);
}

void WebContents::SetTemporaryZoomLevel(double level) {
  zoom_controller_->SetTemporaryZoomLevel(level);
}

void WebContents::DoGetZoomLevel(
    electron::mojom::ElectronWebContentsUtility::DoGetZoomLevelCallback
        callback) {
  std::move(callback).Run(GetZoomLevel());
}

std::vector<base::FilePath> WebContents::GetPreloadPaths() const {
  auto result = SessionPreferences::GetValidPreloads(GetBrowserContext());

  if (auto* web_preferences = WebContentsPreferences::From(web_contents())) {
    base::FilePath preload;
    if (web_preferences->GetPreloadPath(&preload)) {
      result.emplace_back(preload);
    }
  }

  return result;
}

v8::Local<v8::Value> WebContents::GetLastWebPreferences(
    v8::Isolate* isolate) const {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  if (!web_preferences)
    return v8::Null(isolate);
  return gin::ConvertToV8(isolate, *web_preferences->last_preference());
}

v8::Local<v8::Value> WebContents::GetOwnerBrowserWindow(
    v8::Isolate* isolate) const {
  if (owner_window())
    return BrowserWindow::From(isolate, owner_window());
  else
    return v8::Null(isolate);
}

v8::Local<v8::Value> WebContents::Session(v8::Isolate* isolate) {
  return v8::Local<v8::Value>::New(isolate, session_);
}

content::WebContents* WebContents::HostWebContents() const {
  if (!embedder_)
    return nullptr;
  return embedder_->web_contents();
}

void WebContents::SetEmbedder(const WebContents* embedder) {
  if (embedder) {
    NativeWindow* owner_window = nullptr;
    auto* relay = NativeWindowRelay::FromWebContents(embedder->web_contents());
    if (relay) {
      owner_window = relay->GetNativeWindow();
    }
    if (owner_window)
      SetOwnerWindow(owner_window);

    content::RenderWidgetHostView* rwhv =
        web_contents()->GetRenderWidgetHostView();
    if (rwhv) {
      rwhv->Hide();
      rwhv->Show();
    }
  }
}

void WebContents::SetDevToolsWebContents(const WebContents* devtools) {
  if (inspectable_web_contents_)
    inspectable_web_contents_->SetDevToolsWebContents(devtools->web_contents());
}

v8::Local<v8::Value> WebContents::GetNativeView(v8::Isolate* isolate) const {
  gfx::NativeView ptr = web_contents()->GetNativeView();
  auto buffer = node::Buffer::Copy(isolate, reinterpret_cast<char*>(&ptr),
                                   sizeof(gfx::NativeView));
  if (buffer.IsEmpty())
    return v8::Null(isolate);
  else
    return buffer.ToLocalChecked();
}

v8::Local<v8::Value> WebContents::DevToolsWebContents(v8::Isolate* isolate) {
  if (devtools_web_contents_.IsEmpty())
    return v8::Null(isolate);
  else
    return v8::Local<v8::Value>::New(isolate, devtools_web_contents_);
}

v8::Local<v8::Value> WebContents::Debugger(v8::Isolate* isolate) {
  if (debugger_.IsEmpty()) {
    auto handle = electron::api::Debugger::Create(isolate, web_contents());
    debugger_.Reset(isolate, handle.ToV8());
  }
  return v8::Local<v8::Value>::New(isolate, debugger_);
}

content::RenderFrameHost* WebContents::MainFrame() {
  return web_contents()->GetMainFrame();
}

void WebContents::NotifyUserActivation() {
  content::RenderFrameHost* frame = web_contents()->GetMainFrame();
  if (frame)
    frame->NotifyUserActivation(
        blink::mojom::UserActivationNotificationType::kInteraction);
}

void WebContents::SetImageAnimationPolicy(const std::string& new_policy) {
  auto* web_preferences = WebContentsPreferences::From(web_contents());
  web_preferences->SetImageAnimationPolicy(new_policy);
  web_contents()->OnWebPreferencesChanged();
}

v8::Local<v8::Promise> WebContents::GetProcessMemoryInfo(v8::Isolate* isolate) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  auto* frame_host = web_contents()->GetMainFrame();
  if (!frame_host) {
    promise.RejectWithErrorMessage("Failed to create memory dump");
    return handle;
  }

  auto pid = frame_host->GetProcess()->GetProcess().Pid();
  v8::Global<v8::Context> context(isolate, isolate->GetCurrentContext());
  memory_instrumentation::MemoryInstrumentation::GetInstance()
      ->RequestGlobalDumpForPid(
          pid, std::vector<std::string>(),
          base::BindOnce(&ElectronBindings::DidReceiveMemoryDump,
                         std::move(context), std::move(promise), pid));
  return handle;
}

v8::Local<v8::Promise> WebContents::TakeHeapSnapshot(
    v8::Isolate* isolate,
    const base::FilePath& file_path) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::ThreadRestrictions::ScopedAllowIO allow_io;
  base::File file(file_path,
                  base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE);
  if (!file.IsValid()) {
    promise.RejectWithErrorMessage("takeHeapSnapshot failed");
    return handle;
  }

  auto* frame_host = web_contents()->GetMainFrame();
  if (!frame_host) {
    promise.RejectWithErrorMessage("takeHeapSnapshot failed");
    return handle;
  }

  if (!frame_host->IsRenderFrameCreated()) {
    promise.RejectWithErrorMessage("takeHeapSnapshot failed");
    return handle;
  }

  // This dance with `base::Owned` is to ensure that the interface stays alive
  // until the callback is called. Otherwise it would be closed at the end of
  // this function.
  auto electron_renderer =
      std::make_unique<mojo::Remote<mojom::ElectronRenderer>>();
  frame_host->GetRemoteInterfaces()->GetInterface(
      electron_renderer->BindNewPipeAndPassReceiver());
  auto* raw_ptr = electron_renderer.get();
  (*raw_ptr)->TakeHeapSnapshot(
      mojo::WrapPlatformFile(base::ScopedPlatformFile(file.TakePlatformFile())),
      base::BindOnce(
          [](mojo::Remote<mojom::ElectronRenderer>* ep,
             gin_helper::Promise<void> promise, bool success) {
            if (success) {
              promise.Resolve();
            } else {
              promise.RejectWithErrorMessage("takeHeapSnapshot failed");
            }
          },
          base::Owned(std::move(electron_renderer)), std::move(promise)));
  return handle;
}

void WebContents::GrantDevicePermission(
    const url::Origin& origin,
    const base::Value* device,
    blink::PermissionType permissionType,
    content::RenderFrameHost* render_frame_host) {
  granted_devices_[render_frame_host->GetFrameTreeNodeId()][permissionType]
                  [origin]
                      .push_back(
                          std::make_unique<base::Value>(device->Clone()));
}

std::vector<base::Value> WebContents::GetGrantedDevices(
    const url::Origin& origin,
    blink::PermissionType permissionType,
    content::RenderFrameHost* render_frame_host) {
  const auto& devices_for_frame_host_it =
      granted_devices_.find(render_frame_host->GetFrameTreeNodeId());
  if (devices_for_frame_host_it == granted_devices_.end())
    return {};

  const auto& current_devices_it =
      devices_for_frame_host_it->second.find(permissionType);
  if (current_devices_it == devices_for_frame_host_it->second.end())
    return {};

  const auto& origin_devices_it = current_devices_it->second.find(origin);
  if (origin_devices_it == current_devices_it->second.end())
    return {};

  std::vector<base::Value> results;
  for (const auto& object : origin_devices_it->second)
    results.push_back(object->Clone());

  return results;
}

void WebContents::UpdatePreferredSize(content::WebContents* web_contents,
                                      const gfx::Size& pref_size) {
  Emit("preferred-size-changed", pref_size);
}

bool WebContents::CanOverscrollContent() {
  return false;
}

std::unique_ptr<content::EyeDropper> WebContents::OpenEyeDropper(
    content::RenderFrameHost* frame,
    content::EyeDropperListener* listener) {
  return ShowEyeDropper(frame, listener);
}

void WebContents::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  FileSelectHelper::RunFileChooser(render_frame_host, std::move(listener),
                                   params);
}

void WebContents::EnumerateDirectory(
    content::WebContents* web_contents,
    scoped_refptr<content::FileSelectListener> listener,
    const base::FilePath& path) {
  FileSelectHelper::EnumerateDirectory(web_contents, std::move(listener), path);
}

bool WebContents::IsFullscreenForTabOrPending(
    const content::WebContents* source) {
  return html_fullscreen_;
}

bool WebContents::TakeFocus(content::WebContents* source, bool reverse) {
  if (source && source->GetOutermostWebContents() == source) {
    // If this is the outermost web contents and the user has tabbed or
    // shift + tabbed through all the elements, reset the focus back to
    // the first or last element so that it doesn't stay in the body.
    source->FocusThroughTabTraversal(reverse);
    return true;
  }

  return false;
}

content::PictureInPictureResult WebContents::EnterPictureInPicture(
    content::WebContents* web_contents) {
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  return PictureInPictureWindowManager::GetInstance()
      ->EnterVideoPictureInPicture(web_contents);
#else
  return content::PictureInPictureResult::kNotSupported;
#endif
}

void WebContents::ExitPictureInPicture() {
#if BUILDFLAG(ENABLE_PICTURE_IN_PICTURE)
  PictureInPictureWindowManager::GetInstance()->ExitPictureInPicture();
#endif
}

void WebContents::DevToolsSaveToFile(const std::string& url,
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
      inspectable_web_contents_->CallClientFunction(
          "DevToolsAPI.canceledSaveURL", &url_value, nullptr, nullptr);
      return;
    }
  }

  saved_files_[url] = path;
  // Notify DevTools.
  base::Value url_value(url);
  base::Value file_system_path_value(path.AsUTF8Unsafe());
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.savedURL", &url_value, &file_system_path_value, nullptr);
  file_task_runner_->PostTask(FROM_HERE,
                              base::BindOnce(&WriteToFile, path, content));
}

void WebContents::DevToolsAppendToFile(const std::string& url,
                                       const std::string& content) {
  auto it = saved_files_.find(url);
  if (it == saved_files_.end())
    return;

  // Notify DevTools.
  base::Value url_value(url);
  inspectable_web_contents_->CallClientFunction("DevToolsAPI.appendedToURL",
                                                &url_value, nullptr, nullptr);
  file_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&AppendToFile, it->second, content));
}

void WebContents::DevToolsRequestFileSystems() {
  auto file_system_paths = GetAddedFileSystemPaths(GetDevToolsWebContents());
  if (file_system_paths.empty()) {
    base::ListValue empty_file_system_value;
    inspectable_web_contents_->CallClientFunction(
        "DevToolsAPI.fileSystemsLoaded", &empty_file_system_value, nullptr,
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
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.fileSystemsLoaded", &file_system_value, nullptr, nullptr);
}

void WebContents::DevToolsAddFileSystem(
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
  update.Get()->SetKey(path.AsUTF8Unsafe(), base::Value(type));
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.fileSystemAdded", nullptr, file_system_value.get(), nullptr);
}

void WebContents::DevToolsRemoveFileSystem(
    const base::FilePath& file_system_path) {
  if (!inspectable_web_contents_)
    return;

  std::string path = file_system_path.AsUTF8Unsafe();
  storage::IsolatedContext::GetInstance()->RevokeFileSystemByPath(
      file_system_path);

  auto* pref_service = GetPrefService(GetDevToolsWebContents());
  DictionaryPrefUpdate update(pref_service, prefs::kDevToolsFileSystemPaths);
  update.Get()->RemoveKey(path);

  base::Value file_system_path_value(path);
  inspectable_web_contents_->CallClientFunction("DevToolsAPI.fileSystemRemoved",
                                                &file_system_path_value,
                                                nullptr, nullptr);
}

void WebContents::DevToolsIndexPath(
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
    for (const base::Value& folder_path :
         parsed_excluded_folders->GetListDeprecated()) {
      if (folder_path.is_string())
        excluded_folders.push_back(folder_path.GetString());
    }
  }
  devtools_indexing_jobs_[request_id] =
      scoped_refptr<DevToolsFileSystemIndexer::FileSystemIndexingJob>(
          devtools_file_system_indexer_->IndexPath(
              file_system_path, excluded_folders,
              base::BindRepeating(
                  &WebContents::OnDevToolsIndexingWorkCalculated,
                  weak_factory_.GetWeakPtr(), request_id, file_system_path),
              base::BindRepeating(&WebContents::OnDevToolsIndexingWorked,
                                  weak_factory_.GetWeakPtr(), request_id,
                                  file_system_path),
              base::BindRepeating(&WebContents::OnDevToolsIndexingDone,
                                  weak_factory_.GetWeakPtr(), request_id,
                                  file_system_path)));
}

void WebContents::DevToolsStopIndexing(int request_id) {
  auto it = devtools_indexing_jobs_.find(request_id);
  if (it == devtools_indexing_jobs_.end())
    return;
  it->second->Stop();
  devtools_indexing_jobs_.erase(it);
}

void WebContents::DevToolsSearchInPath(int request_id,
                                       const std::string& file_system_path,
                                       const std::string& query) {
  if (!IsDevToolsFileSystemAdded(GetDevToolsWebContents(), file_system_path)) {
    OnDevToolsSearchCompleted(request_id, file_system_path,
                              std::vector<std::string>());
    return;
  }
  devtools_file_system_indexer_->SearchInPath(
      file_system_path, query,
      base::BindRepeating(&WebContents::OnDevToolsSearchCompleted,
                          weak_factory_.GetWeakPtr(), request_id,
                          file_system_path));
}

void WebContents::DevToolsSetEyeDropperActive(bool active) {
  auto* web_contents = GetWebContents();
  if (!web_contents)
    return;

  if (active) {
    eye_dropper_ = std::make_unique<DevToolsEyeDropper>(
        web_contents, base::BindRepeating(&WebContents::ColorPickedInEyeDropper,
                                          base::Unretained(this)));
  } else {
    eye_dropper_.reset();
  }
}

void WebContents::ColorPickedInEyeDropper(int r, int g, int b, int a) {
  base::DictionaryValue color;
  color.SetInteger("r", r);
  color.SetInteger("g", g);
  color.SetInteger("b", b);
  color.SetInteger("a", a);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.eyeDropperPickedColor", &color, nullptr, nullptr);
}

#if defined(TOOLKIT_VIEWS) && !BUILDFLAG(IS_MAC)
ui::ImageModel WebContents::GetDevToolsWindowIcon() {
  return owner_window() ? owner_window()->GetWindowAppIcon() : ui::ImageModel{};
}
#endif

#if BUILDFLAG(IS_LINUX)
void WebContents::GetDevToolsWindowWMClass(std::string* name,
                                           std::string* class_name) {
  *class_name = Browser::Get()->GetName();
  *name = base::ToLowerASCII(*class_name);
}
#endif

void WebContents::OnDevToolsIndexingWorkCalculated(
    int request_id,
    const std::string& file_system_path,
    int total_work) {
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  base::Value total_work_value(total_work);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.indexingTotalWorkCalculated", &request_id_value,
      &file_system_path_value, &total_work_value);
}

void WebContents::OnDevToolsIndexingWorked(int request_id,
                                           const std::string& file_system_path,
                                           int worked) {
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  base::Value worked_value(worked);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.indexingWorked", &request_id_value, &file_system_path_value,
      &worked_value);
}

void WebContents::OnDevToolsIndexingDone(int request_id,
                                         const std::string& file_system_path) {
  devtools_indexing_jobs_.erase(request_id);
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.indexingDone", &request_id_value, &file_system_path_value,
      nullptr);
}

void WebContents::OnDevToolsSearchCompleted(
    int request_id,
    const std::string& file_system_path,
    const std::vector<std::string>& file_paths) {
  base::ListValue file_paths_value;
  for (const auto& file_path : file_paths) {
    file_paths_value.Append(file_path);
  }
  base::Value request_id_value(request_id);
  base::Value file_system_path_value(file_system_path);
  inspectable_web_contents_->CallClientFunction(
      "DevToolsAPI.searchCompleted", &request_id_value, &file_system_path_value,
      &file_paths_value);
}

void WebContents::SetHtmlApiFullscreen(bool enter_fullscreen) {
  // Window is already in fullscreen mode, save the state.
  if (enter_fullscreen && owner_window_->IsFullscreen()) {
    native_fullscreen_ = true;
    UpdateHtmlApiFullscreen(true);
    return;
  }

  // Exit html fullscreen state but not window's fullscreen mode.
  if (!enter_fullscreen && native_fullscreen_) {
    UpdateHtmlApiFullscreen(false);
    return;
  }

  // Set fullscreen on window if allowed.
  auto* web_preferences = WebContentsPreferences::From(GetWebContents());
  bool html_fullscreenable =
      web_preferences
          ? !web_preferences->ShouldDisableHtmlFullscreenWindowResize()
          : true;

  if (html_fullscreenable) {
    owner_window_->SetFullScreen(enter_fullscreen);
  }

  UpdateHtmlApiFullscreen(enter_fullscreen);
  native_fullscreen_ = false;
}

void WebContents::UpdateHtmlApiFullscreen(bool fullscreen) {
  if (fullscreen == is_html_fullscreen())
    return;

  html_fullscreen_ = fullscreen;

  // Notify renderer of the html fullscreen change.
  web_contents()
      ->GetRenderViewHost()
      ->GetWidget()
      ->SynchronizeVisualProperties();

  // The embedder WebContents is separated from the frame tree of webview, so
  // we must manually sync their fullscreen states.
  if (embedder_)
    embedder_->SetHtmlApiFullscreen(fullscreen);

  if (fullscreen) {
    Emit("enter-html-full-screen");
    owner_window_->NotifyWindowEnterHtmlFullScreen();
  } else {
    Emit("leave-html-full-screen");
    owner_window_->NotifyWindowLeaveHtmlFullScreen();
  }

  // Make sure all child webviews quit html fullscreen.
  if (!fullscreen && !IsGuest()) {
    auto* manager = WebViewManager::GetWebViewManager(web_contents());
    manager->ForEachGuest(
        web_contents(), base::BindRepeating([](content::WebContents* guest) {
          WebContents* api_web_contents = WebContents::From(guest);
          api_web_contents->SetHtmlApiFullscreen(false);
          return false;
        }));
  }
}

// static
v8::Local<v8::ObjectTemplate> WebContents::FillObjectTemplate(
    v8::Isolate* isolate,
    v8::Local<v8::ObjectTemplate> templ) {
  gin::InvokerOptions options;
  options.holder_is_first_argument = true;
  options.holder_type = "WebContents";
  templ->Set(
      gin::StringToSymbol(isolate, "isDestroyed"),
      gin::CreateFunctionTemplate(
          isolate, base::BindRepeating(&gin_helper::Destroyable::IsDestroyed),
          options));
  // We use gin_helper::ObjectTemplateBuilder instead of
  // gin::ObjectTemplateBuilder here to handle the fact that WebContents is
  // destroyable.
  return gin_helper::ObjectTemplateBuilder(isolate, templ)
      .SetMethod("destroy", &WebContents::Destroy)
      .SetMethod("getBackgroundThrottling",
                 &WebContents::GetBackgroundThrottling)
      .SetMethod("setBackgroundThrottling",
                 &WebContents::SetBackgroundThrottling)
      .SetMethod("getProcessId", &WebContents::GetProcessID)
      .SetMethod("getOSProcessId", &WebContents::GetOSProcessID)
      .SetMethod("equal", &WebContents::Equal)
      .SetMethod("_loadURL", &WebContents::LoadURL)
      .SetMethod("reload", &WebContents::Reload)
      .SetMethod("reloadIgnoringCache", &WebContents::ReloadIgnoringCache)
      .SetMethod("downloadURL", &WebContents::DownloadURL)
      .SetMethod("getURL", &WebContents::GetURL)
      .SetMethod("getTitle", &WebContents::GetTitle)
      .SetMethod("isLoading", &WebContents::IsLoading)
      .SetMethod("isLoadingMainFrame", &WebContents::IsLoadingMainFrame)
      .SetMethod("isWaitingForResponse", &WebContents::IsWaitingForResponse)
      .SetMethod("stop", &WebContents::Stop)
      .SetMethod("canGoBack", &WebContents::CanGoBack)
      .SetMethod("goBack", &WebContents::GoBack)
      .SetMethod("canGoForward", &WebContents::CanGoForward)
      .SetMethod("goForward", &WebContents::GoForward)
      .SetMethod("canGoToOffset", &WebContents::CanGoToOffset)
      .SetMethod("goToOffset", &WebContents::GoToOffset)
      .SetMethod("canGoToIndex", &WebContents::CanGoToIndex)
      .SetMethod("goToIndex", &WebContents::GoToIndex)
      .SetMethod("getActiveIndex", &WebContents::GetActiveIndex)
      .SetMethod("clearHistory", &WebContents::ClearHistory)
      .SetMethod("length", &WebContents::GetHistoryLength)
      .SetMethod("isCrashed", &WebContents::IsCrashed)
      .SetMethod("forcefullyCrashRenderer",
                 &WebContents::ForcefullyCrashRenderer)
      .SetMethod("setUserAgent", &WebContents::SetUserAgent)
      .SetMethod("getUserAgent", &WebContents::GetUserAgent)
      .SetMethod("savePage", &WebContents::SavePage)
      .SetMethod("openDevTools", &WebContents::OpenDevTools)
      .SetMethod("closeDevTools", &WebContents::CloseDevTools)
      .SetMethod("isDevToolsOpened", &WebContents::IsDevToolsOpened)
      .SetMethod("isDevToolsFocused", &WebContents::IsDevToolsFocused)
      .SetMethod("enableDeviceEmulation", &WebContents::EnableDeviceEmulation)
      .SetMethod("disableDeviceEmulation", &WebContents::DisableDeviceEmulation)
      .SetMethod("toggleDevTools", &WebContents::ToggleDevTools)
      .SetMethod("inspectElement", &WebContents::InspectElement)
      .SetMethod("setIgnoreMenuShortcuts", &WebContents::SetIgnoreMenuShortcuts)
      .SetMethod("setAudioMuted", &WebContents::SetAudioMuted)
      .SetMethod("isAudioMuted", &WebContents::IsAudioMuted)
      .SetMethod("isCurrentlyAudible", &WebContents::IsCurrentlyAudible)
      .SetMethod("undo", &WebContents::Undo)
      .SetMethod("redo", &WebContents::Redo)
      .SetMethod("cut", &WebContents::Cut)
      .SetMethod("copy", &WebContents::Copy)
      .SetMethod("paste", &WebContents::Paste)
      .SetMethod("pasteAndMatchStyle", &WebContents::PasteAndMatchStyle)
      .SetMethod("delete", &WebContents::Delete)
      .SetMethod("selectAll", &WebContents::SelectAll)
      .SetMethod("unselect", &WebContents::Unselect)
      .SetMethod("replace", &WebContents::Replace)
      .SetMethod("replaceMisspelling", &WebContents::ReplaceMisspelling)
      .SetMethod("findInPage", &WebContents::FindInPage)
      .SetMethod("stopFindInPage", &WebContents::StopFindInPage)
      .SetMethod("focus", &WebContents::Focus)
      .SetMethod("isFocused", &WebContents::IsFocused)
      .SetMethod("sendInputEvent", &WebContents::SendInputEvent)
      .SetMethod("beginFrameSubscription", &WebContents::BeginFrameSubscription)
      .SetMethod("endFrameSubscription", &WebContents::EndFrameSubscription)
      .SetMethod("startDrag", &WebContents::StartDrag)
      .SetMethod("attachToIframe", &WebContents::AttachToIframe)
      .SetMethod("detachFromOuterFrame", &WebContents::DetachFromOuterFrame)
      .SetMethod("isOffscreen", &WebContents::IsOffScreen)
#if BUILDFLAG(ENABLE_OSR)
      .SetMethod("startPainting", &WebContents::StartPainting)
      .SetMethod("stopPainting", &WebContents::StopPainting)
      .SetMethod("isPainting", &WebContents::IsPainting)
      .SetMethod("setFrameRate", &WebContents::SetFrameRate)
      .SetMethod("getFrameRate", &WebContents::GetFrameRate)
#endif
      .SetMethod("invalidate", &WebContents::Invalidate)
      .SetMethod("setZoomLevel", &WebContents::SetZoomLevel)
      .SetMethod("getZoomLevel", &WebContents::GetZoomLevel)
      .SetMethod("setZoomFactor", &WebContents::SetZoomFactor)
      .SetMethod("getZoomFactor", &WebContents::GetZoomFactor)
      .SetMethod("getType", &WebContents::GetType)
      .SetMethod("_getPreloadPaths", &WebContents::GetPreloadPaths)
      .SetMethod("getLastWebPreferences", &WebContents::GetLastWebPreferences)
      .SetMethod("getOwnerBrowserWindow", &WebContents::GetOwnerBrowserWindow)
      .SetMethod("inspectServiceWorker", &WebContents::InspectServiceWorker)
      .SetMethod("inspectSharedWorker", &WebContents::InspectSharedWorker)
      .SetMethod("inspectSharedWorkerById",
                 &WebContents::InspectSharedWorkerById)
      .SetMethod("getAllSharedWorkers", &WebContents::GetAllSharedWorkers)
#if BUILDFLAG(ENABLE_PRINTING)
      .SetMethod("_print", &WebContents::Print)
      .SetMethod("_printToPDF", &WebContents::PrintToPDF)
#endif
      .SetMethod("_setNextChildWebPreferences",
                 &WebContents::SetNextChildWebPreferences)
      .SetMethod("addWorkSpace", &WebContents::AddWorkSpace)
      .SetMethod("removeWorkSpace", &WebContents::RemoveWorkSpace)
      .SetMethod("showDefinitionForSelection",
                 &WebContents::ShowDefinitionForSelection)
      .SetMethod("copyImageAt", &WebContents::CopyImageAt)
      .SetMethod("capturePage", &WebContents::CapturePage)
      .SetMethod("setEmbedder", &WebContents::SetEmbedder)
      .SetMethod("setDevToolsWebContents", &WebContents::SetDevToolsWebContents)
      .SetMethod("getNativeView", &WebContents::GetNativeView)
      .SetMethod("incrementCapturerCount", &WebContents::IncrementCapturerCount)
      .SetMethod("decrementCapturerCount", &WebContents::DecrementCapturerCount)
      .SetMethod("isBeingCaptured", &WebContents::IsBeingCaptured)
      .SetMethod("setWebRTCIPHandlingPolicy",
                 &WebContents::SetWebRTCIPHandlingPolicy)
      .SetMethod("getMediaSourceId", &WebContents::GetMediaSourceID)
      .SetMethod("getWebRTCIPHandlingPolicy",
                 &WebContents::GetWebRTCIPHandlingPolicy)
      .SetMethod("takeHeapSnapshot", &WebContents::TakeHeapSnapshot)
      .SetMethod("setImageAnimationPolicy",
                 &WebContents::SetImageAnimationPolicy)
      .SetMethod("_getProcessMemoryInfo", &WebContents::GetProcessMemoryInfo)
      .SetProperty("id", &WebContents::ID)
      .SetProperty("session", &WebContents::Session)
      .SetProperty("hostWebContents", &WebContents::HostWebContents)
      .SetProperty("devToolsWebContents", &WebContents::DevToolsWebContents)
      .SetProperty("debugger", &WebContents::Debugger)
      .SetProperty("mainFrame", &WebContents::MainFrame)
      .Build();
}

const char* WebContents::GetTypeName() {
  return "WebContents";
}

ElectronBrowserContext* WebContents::GetBrowserContext() const {
  return static_cast<ElectronBrowserContext*>(
      web_contents()->GetBrowserContext());
}

// static
gin::Handle<WebContents> WebContents::New(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& options) {
  gin::Handle<WebContents> handle =
      gin::CreateHandle(isolate, new WebContents(isolate, options));
  v8::TryCatch try_catch(isolate);
  gin_helper::CallMethod(isolate, handle.get(), "_init");
  if (try_catch.HasCaught()) {
    node::errors::TriggerUncaughtException(isolate, try_catch);
  }
  return handle;
}

// static
gin::Handle<WebContents> WebContents::CreateAndTake(
    v8::Isolate* isolate,
    std::unique_ptr<content::WebContents> web_contents,
    Type type) {
  gin::Handle<WebContents> handle = gin::CreateHandle(
      isolate, new WebContents(isolate, std::move(web_contents), type));
  v8::TryCatch try_catch(isolate);
  gin_helper::CallMethod(isolate, handle.get(), "_init");
  if (try_catch.HasCaught()) {
    node::errors::TriggerUncaughtException(isolate, try_catch);
  }
  return handle;
}

// static
WebContents* WebContents::From(content::WebContents* web_contents) {
  if (!web_contents)
    return nullptr;
  auto* data = static_cast<UserDataLink*>(
      web_contents->GetUserData(kElectronApiWebContentsKey));
  return data ? data->web_contents.get() : nullptr;
}

// static
gin::Handle<WebContents> WebContents::FromOrCreate(
    v8::Isolate* isolate,
    content::WebContents* web_contents) {
  WebContents* api_web_contents = From(web_contents);
  if (!api_web_contents) {
    api_web_contents = new WebContents(isolate, web_contents);
    v8::TryCatch try_catch(isolate);
    gin_helper::CallMethod(isolate, api_web_contents, "_init");
    if (try_catch.HasCaught()) {
      node::errors::TriggerUncaughtException(isolate, try_catch);
    }
  }
  return gin::CreateHandle(isolate, api_web_contents);
}

// static
gin::Handle<WebContents> WebContents::CreateFromWebPreferences(
    v8::Isolate* isolate,
    const gin_helper::Dictionary& web_preferences) {
  // Check if webPreferences has |webContents| option.
  gin::Handle<WebContents> web_contents;
  if (web_preferences.GetHidden("webContents", &web_contents) &&
      !web_contents.IsEmpty()) {
    // Set webPreferences from options if using an existing webContents.
    // These preferences will be used when the webContent launches new
    // render processes.
    auto* existing_preferences =
        WebContentsPreferences::From(web_contents->web_contents());
    gin_helper::Dictionary web_preferences_dict;
    if (gin::ConvertFromV8(isolate, web_preferences.GetHandle(),
                           &web_preferences_dict)) {
      existing_preferences->SetFromDictionary(web_preferences_dict);
      absl::optional<SkColor> color =
          existing_preferences->GetBackgroundColor();
      web_contents->web_contents()->SetPageBaseBackgroundColor(color);
      // Because web preferences don't recognize transparency,
      // only set rwhv background color if a color exists
      auto* rwhv = web_contents->web_contents()->GetRenderWidgetHostView();
      if (rwhv && color.has_value())
        SetBackgroundColor(rwhv, color.value());
    }
  } else {
    // Create one if not.
    web_contents = WebContents::New(isolate, web_preferences);
  }

  return web_contents;
}

// static
WebContents* WebContents::FromID(int32_t id) {
  return GetAllWebContents().Lookup(id);
}

// static
gin::WrapperInfo WebContents::kWrapperInfo = {gin::kEmbedderNativeGin};

}  // namespace api

}  // namespace electron

namespace {

using electron::api::GetAllWebContents;
using electron::api::WebContents;

gin::Handle<WebContents> WebContentsFromID(v8::Isolate* isolate, int32_t id) {
  WebContents* contents = WebContents::FromID(id);
  return contents ? gin::CreateHandle(isolate, contents)
                  : gin::Handle<WebContents>();
}

gin::Handle<WebContents> WebContentsFromDevToolsTargetID(
    v8::Isolate* isolate,
    std::string target_id) {
  auto agent_host = content::DevToolsAgentHost::GetForId(target_id);
  WebContents* contents =
      agent_host ? WebContents::From(agent_host->GetWebContents()) : nullptr;
  return contents ? gin::CreateHandle(isolate, contents)
                  : gin::Handle<WebContents>();
}

std::vector<gin::Handle<WebContents>> GetAllWebContentsAsV8(
    v8::Isolate* isolate) {
  std::vector<gin::Handle<WebContents>> list;
  for (auto iter = base::IDMap<WebContents*>::iterator(&GetAllWebContents());
       !iter.IsAtEnd(); iter.Advance()) {
    list.push_back(gin::CreateHandle(isolate, iter.GetCurrentValue()));
  }
  return list;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  gin_helper::Dictionary dict(isolate, exports);
  dict.Set("WebContents", WebContents::GetConstructor(context));
  dict.SetMethod("fromId", &WebContentsFromID);
  dict.SetMethod("fromDevToolsTargetId", &WebContentsFromDevToolsTargetID);
  dict.SetMethod("getAllWebContents", &GetAllWebContentsAsV8);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_web_contents, Initialize)
