// Copyright (c) 2020 Microsoft, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/ui/webui/accessibility_ui.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/strings/pattern.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/dev_ui_browser_resources.h"  // nogncheck
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/ax_event_notification_details.h"
#include "content/public/browser/ax_inspect_factory.h"
#include "content/public/browser/browser_accessibility_state.h"
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
#include "net/base/escape.h"
#include "shell/browser/native_window.h"
#include "shell/browser/window_list.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/accessibility/platform/ax_platform_node.h"
#include "ui/accessibility/platform/ax_platform_node_delegate.h"
#include "ui/base/webui/web_ui_util.h"

namespace {

static const char kTargetsDataFile[] = "targets-data.json";

static const char kAccessibilityModeField[] = "a11yMode";
static const char kBrowsersField[] = "browsers";
static const char kErrorField[] = "error";
static const char kFaviconUrlField[] = "faviconUrl";
static const char kNameField[] = "name";
static const char kPagesField[] = "pages";
static const char kPidField[] = "pid";
static const char kSessionIdField[] = "sessionId";
static const char kProcessIdField[] = "processId";
static const char kRequestTypeField[] = "requestType";
static const char kRoutingIdField[] = "routingId";
static const char kTypeField[] = "type";
static const char kUrlField[] = "url";
static const char kTreeField[] = "tree";

// Global flags
static const char kBrowser[] = "browser";
static const char kCopyTree[] = "copyTree";
static const char kHTML[] = "html";
static const char kInternal[] = "internal";
static const char kLabelImages[] = "labelImages";
static const char kNative[] = "native";
static const char kPage[] = "page";
static const char kPDF[] = "pdf";
static const char kScreenReader[] = "screenreader";
static const char kShowOrRefreshTree[] = "showOrRefreshTree";
static const char kText[] = "text";
static const char kWeb[] = "web";

// Possible global flag values
static const char kDisabled[] = "disabled";
static const char kOff[] = "off";
static const char kOn[] = "on";

std::unique_ptr<base::DictionaryValue> BuildTargetDescriptor(
    const GURL& url,
    const std::string& name,
    const GURL& favicon_url,
    int process_id,
    int routing_id,
    ui::AXMode accessibility_mode,
    base::ProcessHandle handle = base::kNullProcessHandle) {
  auto target_data = std::make_unique<base::DictionaryValue>();
  target_data->SetInteger(kProcessIdField, process_id);
  target_data->SetInteger(kRoutingIdField, routing_id);
  target_data->SetString(kUrlField, url.spec());
  target_data->SetString(kNameField, net::EscapeForHTML(name));
  target_data->SetInteger(kPidField, base::GetProcId(handle));
  target_data->SetString(kFaviconUrlField, favicon_url.spec());
  target_data->SetInteger(kAccessibilityModeField, accessibility_mode.mode());
  target_data->SetString(kTypeField, kPage);
  return target_data;
}

std::unique_ptr<base::DictionaryValue> BuildTargetDescriptor(
    content::RenderViewHost* rvh) {
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
                               rvh->GetProcess()->GetID(), rvh->GetRoutingID(),
                               accessibility_mode);
}

std::unique_ptr<base::DictionaryValue> BuildTargetDescriptor(
    electron::NativeWindow* window) {
  auto target_data = std::make_unique<base::DictionaryValue>();
  target_data->SetInteger(kSessionIdField, window->window_id());
  target_data->SetString(kNameField, window->GetTitle());
  target_data->SetString(kTypeField, kBrowser);
  return target_data;
}

bool ShouldHandleAccessibilityRequestCallback(const std::string& path) {
  return path == kTargetsDataFile;
}

// Add property filters to the property_filters vector for the given property
// filter type. The attributes are passed in as a string with each attribute
// separated by a space.
void AddPropertyFilters(std::vector<ui::AXPropertyFilter>* property_filters,
                        const std::string& attributes,
                        ui::AXPropertyFilter::Type type) {
  for (const std::string& attribute : base::SplitString(
           attributes, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY)) {
    property_filters->push_back(ui::AXPropertyFilter(attribute, type));
  }
}

bool MatchesPropertyFilters(
    const std::vector<ui::AXPropertyFilter>& property_filters,
    const std::string& text) {
  bool allow = false;
  for (const auto& filter : property_filters) {
    if (base::MatchPattern(text, filter.match_str)) {
      switch (filter.type) {
        case ui::AXPropertyFilter::ALLOW_EMPTY:
        case ui::AXPropertyFilter::SCRIPT:
          allow = true;
          break;
        case ui::AXPropertyFilter::ALLOW:
          allow = (!base::MatchPattern(text, "*=''"));
          break;
        case ui::AXPropertyFilter::DENY:
          allow = false;
          break;
      }
    }
  }
  return allow;
}

std::string RecursiveDumpAXPlatformNodeAsString(
    const ui::AXPlatformNode* node,
    int indent,
    const std::vector<ui::AXPropertyFilter>& property_filters) {
  if (!node)
    return "";
  std::string str(2 * indent, '+');
  const std::string line = node->GetDelegate()->GetData().ToString();
  const std::vector<std::string> attributes = base::SplitString(
      line, " ", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  for (const std::string& attribute : attributes) {
    if (MatchesPropertyFilters(property_filters, attribute)) {
      str += attribute + " ";
    }
  }
  str += "\n";
  for (int i = 0; i < node->GetDelegate()->GetChildCount(); i++) {
    gfx::NativeViewAccessible child = node->GetDelegate()->ChildAtIndex(i);
    const ui::AXPlatformNode* child_node =
        ui::AXPlatformNode::FromNativeViewAccessible(child);
    str += RecursiveDumpAXPlatformNodeAsString(child_node, indent + 1,
                                               property_filters);
  }
  return str;
}

bool IsValidJSValue(const std::string* str) {
  return str && str->length() < 5000U;
}

void HandleAccessibilityRequestCallback(
    content::BrowserContext* current_context,
    const std::string& path,
    content::WebUIDataSource::GotDataCallback callback) {
  DCHECK(ShouldHandleAccessibilityRequestCallback(path));

  base::DictionaryValue data;
  ui::AXMode mode =
      content::BrowserAccessibilityState::GetInstance()->GetAccessibilityMode();
  bool is_native_enabled = content::BrowserAccessibilityState::GetInstance()
                               ->IsRendererAccessibilityEnabled();
  bool native = mode.has_mode(ui::AXMode::kNativeAPIs);
  bool web = mode.has_mode(ui::AXMode::kWebContents);
  bool text = mode.has_mode(ui::AXMode::kInlineTextBoxes);
  bool screenreader = mode.has_mode(ui::AXMode::kScreenReader);
  bool html = mode.has_mode(ui::AXMode::kHTML);
  bool pdf = mode.has_mode(ui::AXMode::kPDF);

  // The "native" and "web" flags are disabled if
  // --disable-renderer-accessibility is set.
  data.SetString(kNative,
                 is_native_enabled ? (native ? kOn : kOff) : kDisabled);
  data.SetString(kWeb, is_native_enabled ? (web ? kOn : kOff) : kDisabled);

  // The "text", "screenreader" and "html" flags are only
  // meaningful if "web" is enabled.
  bool is_web_enabled = is_native_enabled && web;
  data.SetString(kText, is_web_enabled ? (text ? kOn : kOff) : kDisabled);
  data.SetString(kScreenReader,
                 is_web_enabled ? (screenreader ? kOn : kOff) : kDisabled);
  data.SetString(kHTML, is_web_enabled ? (html ? kOn : kOff) : kDisabled);

  // TODO(codebytere): enable use of this flag.
  //
  // The "labelImages" flag works only if "web" is enabled, the current profile
  // has the kAccessibilityImageLabelsEnabled preference set and the appropriate
  // command line switch has been used. Since this is so closely tied into user
  // prefs and causes bugs, we're disabling it for now.
  bool are_accessibility_image_labels_enabled = is_web_enabled;
  data.SetString(kLabelImages, kDisabled);

  // The "pdf" flag is independent of the others.
  data.SetString(kPDF, pdf ? kOn : kOff);

  // Always dump the Accessibility tree.
  data.SetString(kInternal, kOn);

  auto rvh_list = std::make_unique<base::ListValue>();
  std::unique_ptr<content::RenderWidgetHostIterator> widgets(
      content::RenderWidgetHost::GetRenderWidgetHosts());

  while (content::RenderWidgetHost* widget = widgets->GetNextHost()) {
    // Ignore processes that don't have a connection, such as crashed tabs.
    if (!widget->GetProcess()->IsInitializedAndNotDead())
      continue;
    content::RenderViewHost* rvh = content::RenderViewHost::From(widget);
    if (!rvh)
      continue;
    content::WebContents* web_contents =
        content::WebContents::FromRenderViewHost(rvh);
    content::WebContentsDelegate* delegate = web_contents->GetDelegate();
    if (!delegate)
      continue;
    // Ignore views that are never user-visible, like background pages.
    if (delegate->IsNeverComposited(web_contents))
      continue;
    content::BrowserContext* context = rvh->GetProcess()->GetBrowserContext();
    if (context != current_context)
      continue;

    std::unique_ptr<base::DictionaryValue> descriptor =
        BuildTargetDescriptor(rvh);
    descriptor->SetBoolean(kNative, is_native_enabled);
    descriptor->SetBoolean(kWeb, is_web_enabled);
    descriptor->SetBoolean(kLabelImages,
                           are_accessibility_image_labels_enabled);
    rvh_list->Append(std::move(descriptor));
  }

  data.Set(kPagesField, std::move(rvh_list));

  auto window_list = std::make_unique<base::ListValue>();
  for (auto* window : electron::WindowList::GetWindows()) {
    window_list->Append(BuildTargetDescriptor(window));
  }

  data.Set(kBrowsersField, std::move(window_list));

  std::string json_string;
  base::JSONWriter::Write(data, &json_string);

  std::move(callback).Run(base::RefCountedString::TakeString(&json_string));
}

}  // namespace

ElectronAccessibilityUI::ElectronAccessibilityUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Set up the chrome://accessibility source.
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIAccessibilityHost);

  // Add required resources.
  html_source->UseStringsJs();
  html_source->AddResourcePath("accessibility.css", IDR_ACCESSIBILITY_CSS);
  html_source->AddResourcePath("accessibility.js", IDR_ACCESSIBILITY_JS);
  html_source->SetDefaultResource(IDR_ACCESSIBILITY_HTML);
  html_source->SetRequestFilter(
      base::BindRepeating(&ShouldHandleAccessibilityRequestCallback),
      base::BindRepeating(&HandleAccessibilityRequestCallback,
                          web_ui->GetWebContents()->GetBrowserContext()));

  content::BrowserContext* browser_context =
      web_ui->GetWebContents()->GetBrowserContext();
  content::WebUIDataSource::Add(browser_context, html_source);

  web_ui->AddMessageHandler(
      std::make_unique<ElectronAccessibilityUIMessageHandler>());
}

ElectronAccessibilityUI::~ElectronAccessibilityUI() = default;

ElectronAccessibilityUIMessageHandler::ElectronAccessibilityUIMessageHandler() =
    default;

void ElectronAccessibilityUIMessageHandler::RequestNativeUITree(
    const base::ListValue* args) {
  const base::DictionaryValue* data;
  CHECK(args->GetDictionary(0, &data));

  int window_id = *data->FindIntPath(kSessionIdField);
  const std::string* request_type_p = data->FindStringPath(kRequestTypeField);
  CHECK(IsValidJSValue(request_type_p));
  std::string request_type = *request_type_p;
  CHECK(request_type == kShowOrRefreshTree || request_type == kCopyTree);
  request_type = "accessibility." + request_type;

  const std::string* allow_p = data->FindStringPath("filters.allow");
  CHECK(IsValidJSValue(allow_p));
  std::string allow = *allow_p;
  const std::string* allow_empty_p = data->FindStringPath("filters.allowEmpty");
  CHECK(IsValidJSValue(allow_empty_p));
  std::string allow_empty = *allow_empty_p;
  const std::string* deny_p = data->FindStringPath("filters.deny");
  CHECK(IsValidJSValue(deny_p));
  std::string deny = *deny_p;

  AllowJavascript();

  std::vector<ui::AXPropertyFilter> property_filters;
  AddPropertyFilters(&property_filters, allow, ui::AXPropertyFilter::ALLOW);
  AddPropertyFilters(&property_filters, allow_empty,
                     ui::AXPropertyFilter::ALLOW_EMPTY);
  AddPropertyFilters(&property_filters, deny, ui::AXPropertyFilter::DENY);

  for (auto* window : electron::WindowList::GetWindows()) {
    if (window->window_id() == window_id) {
      std::unique_ptr<base::DictionaryValue> result(
          BuildTargetDescriptor(window));
      gfx::NativeWindow native_window = window->GetNativeWindow();
      ui::AXPlatformNode* node =
          ui::AXPlatformNode::FromNativeWindow(native_window);
      result->SetKey(kTreeField,
                     base::Value(RecursiveDumpAXPlatformNodeAsString(
                         node, 0, property_filters)));
      CallJavascriptFunction(request_type, *(result.get()));
      return;
    }
  }

  // No browser with the specified |id| was found.
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetInteger(kSessionIdField, window_id);
  result->SetString(kTypeField, kBrowser);
  result->SetString(kErrorField, "Window no longer exists.");
  CallJavascriptFunction(request_type, *(result.get()));
}

void ElectronAccessibilityUIMessageHandler::RegisterMessages() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  web_ui()->RegisterDeprecatedMessageCallback(
      "toggleAccessibility",
      base::BindRepeating(&AccessibilityUIMessageHandler::ToggleAccessibility,
                          base::Unretained(this)));
  web_ui()->RegisterDeprecatedMessageCallback(
      "setGlobalFlag",
      base::BindRepeating(&AccessibilityUIMessageHandler::SetGlobalFlag,
                          base::Unretained(this)));
  web_ui()->RegisterDeprecatedMessageCallback(
      "requestWebContentsTree",
      base::BindRepeating(
          &AccessibilityUIMessageHandler::RequestWebContentsTree,
          base::Unretained(this)));
  web_ui()->RegisterDeprecatedMessageCallback(
      "requestNativeUITree",
      base::BindRepeating(
          &ElectronAccessibilityUIMessageHandler::RequestNativeUITree,
          base::Unretained(this)));
  web_ui()->RegisterDeprecatedMessageCallback(
      "requestAccessibilityEvents",
      base::BindRepeating(
          &AccessibilityUIMessageHandler::RequestAccessibilityEvents,
          base::Unretained(this)));
}
