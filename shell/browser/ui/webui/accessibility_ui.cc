// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/webui/accessibility_ui.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_writer.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/accessibility_resources.h"      // nogncheck
#include "chrome/grit/accessibility_resources_map.h"  // nogncheck
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/ax_inspect_factory.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/render_widget_host_iterator.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_delegate.h"
#include "content/public/browser/web_ui_data_source.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "ui/accessibility/accessibility_features.h"
#include "ui/accessibility/ax_updates_and_events.h"
#include "ui/accessibility/platform/ax_platform.h"
#include "ui/accessibility/platform/ax_platform_node.h"
#include "ui/accessibility/platform/ax_platform_node_delegate.h"
#include "ui/base/webui/web_ui_util.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

#if BUILDFLAG(IS_WIN)
#include "ui/accessibility/platform/ax_platform_node_win.h"
#endif

namespace {

constexpr std::string_view kTargetsDataFile = "targets-data.json";

constexpr std::string_view kAccessibilityModeField = "a11yMode";
constexpr std::string_view kBrowsersField = "browsers";
constexpr std::string_view kErrorField = "error";
constexpr std::string_view kFaviconUrlField = "faviconUrl";
constexpr std::string_view kNameField = "name";
constexpr std::string_view kPagesField = "pages";
constexpr std::string_view kPidField = "pid";
constexpr std::string_view kProcessIdField = "processId";
constexpr std::string_view kRequestTypeField = "requestType";
constexpr std::string_view kRoutingIdField = "routingId";
constexpr std::string_view kSessionIdField = "sessionId";
constexpr std::string_view kSupportedApiTypesField = "supportedApiTypes";
constexpr std::string_view kTreeField = "tree";
constexpr std::string_view kTypeField = "type";
constexpr std::string_view kUrlField = "url";
constexpr std::string_view kApiTypeField = "apiType";

// Global flags
constexpr std::string_view kBrowser = "browser";
constexpr std::string_view kLockedPlatformModes = "lockedPlatformModes";
constexpr std::string_view kIsolate = "isolate";
constexpr std::string_view kCopyTree = "copyTree";
constexpr std::string_view kHTML = "html";
constexpr std::string_view kLocked = "locked";
constexpr std::string_view kNative = "native";
constexpr std::string_view kPage = "page";
constexpr std::string_view kPDFPrinting = "pdfPrinting";
constexpr std::string_view kExtendedProperties = "extendedProperties";
constexpr std::string_view kScreenReader = "screenReader";
constexpr std::string_view kShowOrRefreshTree = "showOrRefreshTree";
constexpr std::string_view kText = "text";
constexpr std::string_view kWeb = "web";

// Screen reader detection.
static const char kDetectedATName[] = "detectedATName";
static const char kIsScreenReaderActive[] = "isScreenReaderActive";

base::DictValue BuildTargetDescriptor(
    const GURL& url,
    const std::string& name,
    const GURL& favicon_url,
    int process_id,
    int routing_id,
    ui::AXMode accessibility_mode,
    base::ProcessHandle handle = base::kNullProcessHandle) {
  base::DictValue target_data;
  target_data.Set(kProcessIdField, process_id);
  target_data.Set(kRoutingIdField, routing_id);
  target_data.Set(kUrlField, url.spec());
  target_data.Set(kNameField, base::EscapeForHTML(name));
  target_data.Set(kPidField, static_cast<int>(base::GetProcId(handle)));
  target_data.Set(kFaviconUrlField, favicon_url.spec());
  target_data.Set(kAccessibilityModeField,
                  static_cast<int>(accessibility_mode.flags()));
  target_data.Set(kTypeField, kPage);
  return target_data;
}

base::DictValue BuildTargetDescriptor(content::RenderViewHost* rvh) {
  content::WebContents* web_contents =
      content::WebContents::FromRenderViewHost(rvh);
  ui::AXMode accessibility_mode;

  std::string title;
  GURL url;
  GURL favicon_url;
  if (web_contents) {
    url = web_contents->GetURL();
    title = base::UTF16ToUTF8(web_contents->GetTitle());
    content::NavigationController& controller = web_contents->GetController();
    content::NavigationEntry* entry = controller.GetVisibleEntry();
    if (entry != nullptr && entry->GetURL().is_valid()) {
      gfx::Image favicon_image = entry->GetFavicon().image;
      if (!favicon_image.IsEmpty()) {
        const SkBitmap* favicon_bitmap = favicon_image.ToSkBitmap();
        favicon_url = GURL(webui::GetBitmapDataUrl(*favicon_bitmap));
      }
    }
    accessibility_mode = web_contents->GetAccessibilityMode();
  }

  return BuildTargetDescriptor(url, title, favicon_url,
                               rvh->GetProcess()->GetDeprecatedID(),
                               rvh->GetRoutingID(), accessibility_mode);
}

base::DictValue BuildTargetDescriptor(electron::NativeWindow* window) {
  base::DictValue target_data;
  target_data.Set(kSessionIdField, window->window_id());
  target_data.Set(kNameField, window->GetTitle());
  target_data.Set(kTypeField, kBrowser);
  return target_data;
}

bool ShouldHandleAccessibilityRequestCallback(const std::string& path) {
  return path == kTargetsDataFile;
}

// Sets boolean values in `data` for each bit in `new_ax_mode` that differs from
// that in `last_ax_mode`. Returns `true` if `data` was modified.
void SetProcessModeBools(ui::AXMode ax_mode, base::DictValue& data) {
  data.Set(kNative, ax_mode.has_mode(ui::AXMode::kNativeAPIs));
  data.Set(kWeb, ax_mode.has_mode(ui::AXMode::kWebContents));
  data.Set(kText, ax_mode.has_mode(ui::AXMode::kInlineTextBoxes));
  data.Set(kExtendedProperties,
           ax_mode.has_mode(ui::AXMode::kExtendedProperties));
  data.Set(kHTML, ax_mode.has_mode(ui::AXMode::kHTML));
  data.Set(kScreenReader, ax_mode.has_mode(ui::AXMode::kScreenReader));
}

#if BUILDFLAG(IS_WIN)
// Sets values in `data` for the platform node counts in `counts`.
void SetNodeCounts(const ui::AXPlatformNodeWin::Counts& counts,
                   base::DictValue& data) {
  data.Set("dormantCount", base::NumberToString(counts.dormant_nodes));
  data.Set("liveCount", base::NumberToString(counts.live_nodes));
  data.Set("ghostCount", base::NumberToString(counts.ghost_nodes));
}
#endif

void HandleAccessibilityRequestCallback(
    content::BrowserContext* current_context,
    ui::AXMode initial_process_mode,
    const std::string& path,
    content::WebUIDataSource::GotDataCallback callback) {
  DCHECK(ShouldHandleAccessibilityRequestCallback(path));

  auto& browser_accessibility_state =
      *content::BrowserAccessibilityState::GetInstance();
  base::DictValue data;
  PrefService* pref =
      static_cast<electron::ElectronBrowserContext*>(current_context)->prefs();
  ui::AXMode mode =
      content::BrowserAccessibilityState::GetInstance()->GetAccessibilityMode();
  bool native = mode.has_mode(ui::AXMode::kNativeAPIs);
  bool web = mode.has_mode(ui::AXMode::kWebContents);
  bool text = mode.has_mode(ui::AXMode::kInlineTextBoxes);
  bool extended_properties = mode.has_mode(ui::AXMode::kExtendedProperties);
  bool screen_reader = mode.has_mode(ui::AXMode::kScreenReader);
  bool html = mode.has_mode(ui::AXMode::kHTML);
  bool pdf_printing = mode.has_mode(ui::AXMode::kPDFPrinting);
  bool allow_platform_activation =
      browser_accessibility_state.IsActivationFromPlatformEnabled();

  ui::AssistiveTech assistive_tech =
      ui::AXPlatform::GetInstance().active_assistive_tech();
  bool is_screen_reader_active =
      ui::AXPlatform::GetInstance().IsScreenReaderActive();

  // The "native" and "web" flags are disabled if
  // --disable-renderer-accessibility is set.
  data.Set(kNative, native);
  data.Set(kWeb, web);

  // The "text", "extendedProperties" and "html" flags are only
  // meaningful if "web" is enabled.
  data.Set(kText, text);
  data.Set(kExtendedProperties, extended_properties);
  data.Set(kScreenReader, screen_reader);
  data.Set(kHTML, html);

  // The "pdfPrinting" flag is independent of the others.
  data.Set(kPDFPrinting, pdf_printing);

  // Identify the mode checkboxes that were turned on via platform API
  // interactions and therefore cannot be unchecked unless the #isolate checkbox
  // is checked.
  data.Set(
      kLockedPlatformModes,
      base::DictValue()
          .Set(kNative,
               allow_platform_activation && native &&
                   initial_process_mode.has_mode(ui::AXMode::kNativeAPIs))
          .Set(kWeb,
               allow_platform_activation && web &&
                   initial_process_mode.has_mode(ui::AXMode::kWebContents))
          .Set(kText,
               allow_platform_activation && text &&
                   initial_process_mode.has_mode(ui::AXMode::kInlineTextBoxes))
          .Set(kExtendedProperties, allow_platform_activation &&
                                        extended_properties &&
                                        initial_process_mode.has_mode(
                                            ui::AXMode::kExtendedProperties))
          .Set(kHTML, allow_platform_activation &&
                          initial_process_mode.has_mode(ui::AXMode::kHTML)));

  data.Set(kDetectedATName, ui::GetAssistiveTechString(assistive_tech));
  data.Set(kIsScreenReaderActive, is_screen_reader_active);

  std::string pref_api_type =
      std::string(pref->GetString(prefs::kShownAccessibilityApiType));
  bool pref_api_type_supported = false;

  std::vector<ui::AXApiType::Type> supported_api_types =
      content::AXInspectFactory::SupportedApis();
  base::ListValue supported_api_list;
  supported_api_list.reserve(supported_api_types.size());
  for (ui::AXApiType::Type type : supported_api_types) {
    supported_api_list.Append(std::string_view(type));
    if (type == ui::AXApiType::From(pref_api_type)) {
      pref_api_type_supported = true;
    }
  }
  data.Set(kSupportedApiTypesField, std::move(supported_api_list));

  // If the saved API type is not supported, use the default platform formatter
  // API type.
  if (!pref_api_type_supported) {
    pref_api_type = std::string_view(
        content::AXInspectFactory::DefaultPlatformFormatterType());
  }
  data.Set(kApiTypeField, pref_api_type);

  data.Set(kIsolate, !allow_platform_activation);

  data.Set(kLocked, !browser_accessibility_state.IsAXModeChangeAllowed());

  base::ListValue page_list;
  std::unique_ptr<content::RenderWidgetHostIterator> widget_iter(
      content::RenderWidgetHost::GetRenderWidgetHosts());

  while (content::RenderWidgetHost* widget = widget_iter->GetNextHost()) {
    // Ignore processes that don't have a connection, such as crashed tabs.
    if (!widget->GetProcess()->IsInitializedAndNotDead()) {
      continue;
    }
    content::RenderViewHost* rvh = content::RenderViewHost::From(widget);
    if (!rvh) {
      continue;
    }
    content::WebContents* web_contents =
        content::WebContents::FromRenderViewHost(rvh);
    content::WebContentsDelegate* delegate = web_contents->GetDelegate();
    if (!delegate) {
      continue;
    }
    if (web_contents->GetPrimaryMainFrame()->GetRenderViewHost() != rvh) {
      continue;
    }
    // Ignore views that are never user-visible, like background pages.
    if (web_contents->IsNeverComposited()) {
      continue;
    }
    content::BrowserContext* context = rvh->GetProcess()->GetBrowserContext();
    if (context != current_context) {
      continue;
    }

    base::DictValue descriptor = BuildTargetDescriptor(rvh);
    descriptor.Set(kNative, native);
    descriptor.Set(kExtendedProperties, extended_properties);
    descriptor.Set(kScreenReader, screen_reader);
    descriptor.Set(kWeb, web);
    page_list.Append(std::move(descriptor));
  }
  data.Set(kPagesField, std::move(page_list));

  base::ListValue window_list;
  for (auto* window : electron::WindowList::GetWindows()) {
    window_list.Append(BuildTargetDescriptor(window));
  }
  data.Set(kBrowsersField, std::move(window_list));

#if BUILDFLAG(IS_WIN)
  SetNodeCounts(ui::AXPlatformNodeWin::GetCounts(), data);
#endif

  std::move(callback).Run(base::MakeRefCounted<base::RefCountedString>(
      base::WriteJson(data).value_or("")));
}

std::string RecursiveDumpAXPlatformNodeAsString(
    ui::AXPlatformNode* node,
    int indent,
    const std::vector<ui::AXPropertyFilter>& property_filters) {
  if (!node) {
    return "";
  }
  std::string str(2 * indent, '+');
  ui::AXPlatformNodeDelegate* const node_delegate = node->GetDelegate();
  std::string line = node_delegate->GetData().ToString();
  std::vector<std::string> attributes = base::SplitString(
      line, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const std::string& attribute : attributes) {
    if (ui::AXTreeFormatter::MatchesPropertyFilters(property_filters, attribute,
                                                    false)) {
      str += attribute + " ";
    }
  }
  str += "\n";
  for (size_t i = 0, child_count = node_delegate->GetChildCount();
       i < child_count; i++) {
    gfx::NativeViewAccessible child = node_delegate->ChildAtIndex(i);
    ui::AXPlatformNode* child_node =
        ui::AXPlatformNode::FromNativeViewAccessible(child);
    str += RecursiveDumpAXPlatformNodeAsString(child_node, indent + 1,
                                               property_filters);
  }
  return str;
}

// Add property filters to the property_filters vector for the given property
// filter type. The attributes are passed in as a string with each attribute
// separated by a space.
void AddPropertyFilters(std::vector<ui::AXPropertyFilter>& property_filters,
                        const std::string& attributes,
                        ui::AXPropertyFilter::Type type) {
  for (const std::string& attribute : base::SplitString(
           attributes, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    property_filters.emplace_back(attribute, type);
  }
}

bool IsValidJSValue(const std::string* str) {
  return str && str->length() < 5000U;
}

const std::string& CheckJSValue(const std::string* str) {
  CHECK(IsValidJSValue(str));
  return *str;
}

}  // namespace

ElectronAccessibilityUI::ElectronAccessibilityUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  auto* const browser_context = web_ui->GetWebContents()->GetBrowserContext();
  // Set up the chrome://accessibility source.
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::CreateAndAdd(
          browser_context, chrome::kChromeUIAccessibilityHost);

  // The process-wide accessibility mode when the UI page is initially launched.
  ui::AXMode initial_process_mode =
      content::BrowserAccessibilityState::GetInstance()->GetAccessibilityMode();

  // Add required resources.
  html_source->UseStringsJs();
  html_source->AddResourcePaths(kAccessibilityResources);
  html_source->SetDefaultResource(IDR_ACCESSIBILITY_ACCESSIBILITY_HTML);
  html_source->SetRequestFilter(
      base::BindRepeating(&ShouldHandleAccessibilityRequestCallback),
      base::BindRepeating(&HandleAccessibilityRequestCallback, browser_context,
                          initial_process_mode));
  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes,
      "trusted-types parse-html-subset sanitize-inner-html;");

  web_ui->AddMessageHandler(
      std::make_unique<ElectronAccessibilityUIMessageHandler>());
}

ElectronAccessibilityUI::~ElectronAccessibilityUI() = default;

ElectronAccessibilityUIMessageHandler::ElectronAccessibilityUIMessageHandler()
    : update_display_timer_(
          FROM_HERE,
          base::Seconds(1),
          base::BindRepeating(
              &ElectronAccessibilityUIMessageHandler::OnUpdateDisplayTimer,
              base::Unretained(this))) {}

void ElectronAccessibilityUIMessageHandler::GetRequestTypeAndFilters(
    const base::DictValue& data,
    std::string& request_type,
    std::string& allow,
    std::string& allow_empty,
    std::string& deny) {
  request_type = CheckJSValue(data.FindString(kRequestTypeField));
  CHECK(request_type == kShowOrRefreshTree || request_type == kCopyTree);
  allow = CheckJSValue(data.FindStringByDottedPath("filters.allow"));
  allow_empty = CheckJSValue(data.FindStringByDottedPath("filters.allowEmpty"));
  deny = CheckJSValue(data.FindStringByDottedPath("filters.deny"));
}

void ElectronAccessibilityUIMessageHandler::RequestNativeUITree(
    const base::ListValue& args) {
  const base::DictValue& data = args.front().GetDict();

  std::string request_type, allow, allow_empty, deny;
  GetRequestTypeAndFilters(data, request_type, allow, allow_empty, deny);

  int window_id = *data.FindInt(kSessionIdField);

  AllowJavascript();

  std::vector<ui::AXPropertyFilter> property_filters;
  AddPropertyFilters(property_filters, allow, ui::AXPropertyFilter::ALLOW);
  AddPropertyFilters(property_filters, allow_empty,
                     ui::AXPropertyFilter::ALLOW_EMPTY);
  AddPropertyFilters(property_filters, deny, ui::AXPropertyFilter::DENY);

  for (auto* window : electron::WindowList::GetWindows()) {
    if (window->window_id() == window_id) {
      base::DictValue result = BuildTargetDescriptor(window);
      gfx::NativeWindow native_window = window->GetNativeWindow();
      ui::AXPlatformNode* node =
          ui::AXPlatformNode::FromNativeWindow(native_window);
      result.Set(kTreeField, RecursiveDumpAXPlatformNodeAsString(
                                 node, 0, property_filters));
      FireWebUIListener(request_type, result);
      return;
    }
  }

  // No browser with the specified |id| was found.
  base::DictValue result;
  result.Set(kSessionIdField, window_id);
  result.Set(kTypeField, kBrowser);
  result.Set(kErrorField, "Window no longer exists.");
  FireWebUIListener(request_type, result);
}

void ElectronAccessibilityUIMessageHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  web_ui()->RegisterMessageCallback(
      "toggleAccessibility",
      base::BindRepeating(
          &AccessibilityUIMessageHandler::ToggleAccessibilityForWebContents,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setGlobalFlag",
      base::BindRepeating(&AccessibilityUIMessageHandler::SetGlobalFlag,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "setGlobalString",
      base::BindRepeating(&AccessibilityUIMessageHandler::SetGlobalString,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "requestWebContentsTree",
      base::BindRepeating(
          &AccessibilityUIMessageHandler::RequestWebContentsTree,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "requestNativeUITree",
      base::BindRepeating(
          &ElectronAccessibilityUIMessageHandler::RequestNativeUITree,
          base::Unretained(this)));
#if defined(USE_AURA)
  web_ui()->RegisterMessageCallback(
      "requestWidgetsTree",
      base::BindRepeating(&AccessibilityUIMessageHandler::RequestWidgetsTree,
                          base::Unretained(this)));
#endif
  web_ui()->RegisterMessageCallback(
      "requestAccessibilityEvents",
      base::BindRepeating(
          &AccessibilityUIMessageHandler::RequestAccessibilityEvents,
          base::Unretained(this)));

  auto* web_contents = web_ui()->GetWebContents();
  Observe(web_contents);
  OnVisibilityChanged(web_contents->GetVisibility());
}

// static
void ElectronAccessibilityUIMessageHandler::RegisterPrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  const std::string_view default_api_type =
      std::string_view(ui::AXApiType::Type(ui::AXApiType::kBlink));
  registry->RegisterStringPref(prefs::kShownAccessibilityApiType,
                               std::string(default_api_type));
}

void ElectronAccessibilityUIMessageHandler::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::HIDDEN) {
    update_display_timer_.Stop();
  } else {
    update_display_timer_.Reset();
  }
}

void ElectronAccessibilityUIMessageHandler::OnUpdateDisplayTimer() {
  // Collect the current state.
  base::DictValue data;

  SetProcessModeBools(
      content::BrowserAccessibilityState::GetInstance()->GetAccessibilityMode(),
      data);

#if BUILDFLAG(IS_WIN)
  SetNodeCounts(ui::AXPlatformNodeWin::GetCounts(), data);
#endif  // BUILDFLAG(IS_WIN)

  // Compute the delta from the last transmission.
  for (auto scan = data.begin(); scan != data.end();) {
    const auto& [new_key, new_value] = *scan;
    if (const auto* old_value = last_data_.Find(new_key);
        !old_value || *old_value != new_value) {
      // This is a new value; remember it for the future.
      last_data_.Set(new_key, new_value.Clone());
      ++scan;
    } else {
      // This is the same as the last value; forget about it.
      scan = data.erase(scan);
    }
  }

  // Transmit any new values to the UI.
  if (!data.empty()) {
    AllowJavascript();
    FireWebUIListener("updateDisplay", data);
  }
}
