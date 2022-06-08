// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/tabs/tabs_api.h"

#include <memory>
#include <utility>

#include "chrome/common/url_constants.h"
#include "components/url_formatter/url_fixer.h"
#include "content/public/browser/navigation_entry.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/mojom/host_id.mojom.h"
#include "extensions/common/permissions/permissions_data.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/common/extensions/api/tabs.h"
#include "third_party/blink/public/common/page/page_zoom.h"
#include "url/gurl.h"

using electron::WebContentsZoomController;

namespace extensions {

namespace tabs = api::tabs;

const char kFrameNotFoundError[] = "No frame with id * in tab *.";
const char kPerOriginOnlyInAutomaticError[] =
    "Can only set scope to "
    "\"per-origin\" in \"automatic\" mode.";

using api::extension_types::InjectDetails;

namespace {
void ZoomModeToZoomSettings(WebContentsZoomController::ZoomMode zoom_mode,
                            api::tabs::ZoomSettings* zoom_settings) {
  DCHECK(zoom_settings);
  switch (zoom_mode) {
    case WebContentsZoomController::ZoomMode::kDefault:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN;
      break;
    case WebContentsZoomController::ZoomMode::kIsolated:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_AUTOMATIC;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
    case WebContentsZoomController::ZoomMode::kManual:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_MANUAL;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
    case WebContentsZoomController::ZoomMode::kDisabled:
      zoom_settings->mode = api::tabs::ZOOM_SETTINGS_MODE_DISABLED;
      zoom_settings->scope = api::tabs::ZOOM_SETTINGS_SCOPE_PER_TAB;
      break;
  }
}
}  // namespace

ExecuteCodeInTabFunction::ExecuteCodeInTabFunction() : execute_tab_id_(-1) {}

ExecuteCodeInTabFunction::~ExecuteCodeInTabFunction() = default;

ExecuteCodeFunction::InitResult ExecuteCodeInTabFunction::Init() {
  if (init_result_)
    return init_result_.value();

  if (args().size() < 2)
    return set_init_result(VALIDATION_FAILURE);

  const auto& tab_id_value = args()[0];
  // |tab_id| is optional so it's ok if it's not there.
  int tab_id = -1;
  if (tab_id_value.is_int()) {
    // But if it is present, it needs to be non-negative.
    tab_id = tab_id_value.GetInt();
    if (tab_id < 0) {
      return set_init_result(VALIDATION_FAILURE);
    }
  }

  // |details| are not optional.
  const base::Value& details_value = args()[1];
  if (!details_value.is_dict())
    return set_init_result(VALIDATION_FAILURE);
  std::unique_ptr<InjectDetails> details(new InjectDetails());
  if (!InjectDetails::Populate(details_value, details.get()))
    return set_init_result(VALIDATION_FAILURE);

  if (tab_id == -1) {
    // There's no useful concept of a "default tab" in Electron.
    // TODO(nornagon): we could potentially kick this to an event to allow the
    // app to decide what "default tab" means for them?
    return set_init_result(VALIDATION_FAILURE);
  }

  execute_tab_id_ = tab_id;
  details_ = std::move(details);
  set_host_id(
      mojom::HostID(mojom::HostID::HostType::kExtensions, extension()->id()));
  return set_init_result(SUCCESS);
}

bool ExecuteCodeInTabFunction::CanExecuteScriptOnPage(std::string* error) {
  // If |tab_id| is specified, look for the tab. Otherwise default to selected
  // tab in the current window.
  CHECK_GE(execute_tab_id_, 0);
  auto* contents = electron::api::WebContents::FromID(execute_tab_id_);
  if (!contents) {
    return false;
  }

  int frame_id = details_->frame_id ? *details_->frame_id
                                    : ExtensionApiFrameIdMap::kTopFrameId;
  content::RenderFrameHost* rfh =
      ExtensionApiFrameIdMap::GetRenderFrameHostById(contents->web_contents(),
                                                     frame_id);
  if (!rfh) {
    *error = ErrorUtils::FormatErrorMessage(
        kFrameNotFoundError, base::NumberToString(frame_id),
        base::NumberToString(execute_tab_id_));
    return false;
  }

  // Content scripts declared in manifest.json can access frames at about:-URLs
  // if the extension has permission to access the frame's origin, so also allow
  // programmatic content scripts at about:-URLs for allowed origins.
  GURL effective_document_url(rfh->GetLastCommittedURL());
  bool is_about_url = effective_document_url.SchemeIs(url::kAboutScheme);
  if (is_about_url && details_->match_about_blank &&
      *details_->match_about_blank) {
    effective_document_url = GURL(rfh->GetLastCommittedOrigin().Serialize());
  }

  if (!effective_document_url.is_valid()) {
    // Unknown URL, e.g. because no load was committed yet. Allow for now, the
    // renderer will check again and fail the injection if needed.
    return true;
  }

  // NOTE: This can give the wrong answer due to race conditions, but it is OK,
  // we check again in the renderer.
  if (!extension()->permissions_data()->CanAccessPage(effective_document_url,
                                                      execute_tab_id_, error)) {
    if (is_about_url &&
        extension()->permissions_data()->active_permissions().HasAPIPermission(
            extensions::mojom::APIPermissionID::kTab)) {
      *error = ErrorUtils::FormatErrorMessage(
          manifest_errors::kCannotAccessAboutUrl,
          rfh->GetLastCommittedURL().spec(),
          rfh->GetLastCommittedOrigin().Serialize());
    }
    return false;
  }

  return true;
}

ScriptExecutor* ExecuteCodeInTabFunction::GetScriptExecutor(
    std::string* error) {
  auto* contents = electron::api::WebContents::FromID(execute_tab_id_);
  if (!contents)
    return nullptr;
  return contents->script_executor();
}

bool ExecuteCodeInTabFunction::IsWebView() const {
  return false;
}

const GURL& ExecuteCodeInTabFunction::GetWebViewSrc() const {
  return GURL::EmptyGURL();
}

bool TabsExecuteScriptFunction::ShouldInsertCSS() const {
  return false;
}

bool TabsExecuteScriptFunction::ShouldRemoveCSS() const {
  return false;
}

ExtensionFunction::ResponseAction TabsReloadFunction::Run() {
  std::unique_ptr<tabs::Reload::Params> params(
      tabs::Reload::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  bool bypass_cache = false;
  if (params->reload_properties.get() &&
      params->reload_properties->bypass_cache.get()) {
    bypass_cache = *params->reload_properties->bypass_cache;
  }

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  contents->web_contents()->GetController().Reload(
      bypass_cache ? content::ReloadType::BYPASSING_CACHE
                   : content::ReloadType::NORMAL,
      true);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction TabsGetFunction::Run() {
  std::unique_ptr<tabs::Get::Params> params(tabs::Get::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->tab_id;

  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  tabs::Tab tab;

  tab.id = std::make_unique<int>(tab_id);
  // TODO(nornagon): in Chrome, the tab URL is only available to extensions
  // that have the "tabs" (or "activeTab") permission. We should do the same
  // permission check here.
  tab.url = std::make_unique<std::string>(
      contents->web_contents()->GetLastCommittedURL().spec());

  tab.active = contents->IsFocused();

  return RespondNow(ArgumentList(tabs::Get::Results::Create(std::move(tab))));
}

ExtensionFunction::ResponseAction TabsSetZoomFunction::Run() {
  std::unique_ptr<tabs::SetZoom::Params> params(
      tabs::SetZoom::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  auto* web_contents = contents->web_contents();
  GURL url(web_contents->GetVisibleURL());
  std::string error;
  if (extension()->permissions_data()->IsRestrictedUrl(url, &error))
    return RespondNow(Error(error));

  auto* zoom_controller = contents->GetZoomController();
  double zoom_level =
      params->zoom_factor > 0
          ? blink::PageZoomFactorToZoomLevel(params->zoom_factor)
          : blink::PageZoomFactorToZoomLevel(
                zoom_controller->GetDefaultZoomFactor());

  zoom_controller->SetZoomLevel(zoom_level);

  return RespondNow(NoArguments());
}

ExtensionFunction::ResponseAction TabsGetZoomFunction::Run() {
  std::unique_ptr<tabs::GetZoom::Params> params(
      tabs::GetZoom::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  double zoom_level = contents->GetZoomController()->GetZoomLevel();
  double zoom_factor = blink::PageZoomLevelToZoomFactor(zoom_level);

  return RespondNow(ArgumentList(tabs::GetZoom::Results::Create(zoom_factor)));
}

ExtensionFunction::ResponseAction TabsGetZoomSettingsFunction::Run() {
  std::unique_ptr<tabs::GetZoomSettings::Params> params(
      tabs::GetZoomSettings::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  auto* zoom_controller = contents->GetZoomController();
  WebContentsZoomController::ZoomMode zoom_mode =
      contents->GetZoomController()->zoom_mode();
  tabs::ZoomSettings zoom_settings;
  ZoomModeToZoomSettings(zoom_mode, &zoom_settings);
  zoom_settings.default_zoom_factor = std::make_unique<double>(
      blink::PageZoomLevelToZoomFactor(zoom_controller->GetDefaultZoomLevel()));

  return RespondNow(
      ArgumentList(tabs::GetZoomSettings::Results::Create(zoom_settings)));
}

ExtensionFunction::ResponseAction TabsSetZoomSettingsFunction::Run() {
  using tabs::ZoomSettings;

  std::unique_ptr<tabs::SetZoomSettings::Params> params(
      tabs::SetZoomSettings::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  std::string error;
  GURL url(contents->web_contents()->GetVisibleURL());
  if (extension()->permissions_data()->IsRestrictedUrl(url, &error))
    return RespondNow(Error(error));

  // "per-origin" scope is only available in "automatic" mode.
  if (params->zoom_settings.scope == tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN &&
      params->zoom_settings.mode != tabs::ZOOM_SETTINGS_MODE_AUTOMATIC &&
      params->zoom_settings.mode != tabs::ZOOM_SETTINGS_MODE_NONE) {
    return RespondNow(Error(kPerOriginOnlyInAutomaticError));
  }

  // Determine the correct internal zoom mode to set |web_contents| to from the
  // user-specified |zoom_settings|.
  WebContentsZoomController::ZoomMode zoom_mode =
      WebContentsZoomController::ZoomMode::kDefault;
  switch (params->zoom_settings.mode) {
    case tabs::ZOOM_SETTINGS_MODE_NONE:
    case tabs::ZOOM_SETTINGS_MODE_AUTOMATIC:
      switch (params->zoom_settings.scope) {
        case tabs::ZOOM_SETTINGS_SCOPE_NONE:
        case tabs::ZOOM_SETTINGS_SCOPE_PER_ORIGIN:
          zoom_mode = WebContentsZoomController::ZoomMode::kDefault;
          break;
        case tabs::ZOOM_SETTINGS_SCOPE_PER_TAB:
          zoom_mode = WebContentsZoomController::ZoomMode::kIsolated;
      }
      break;
    case tabs::ZOOM_SETTINGS_MODE_MANUAL:
      zoom_mode = WebContentsZoomController::ZoomMode::kManual;
      break;
    case tabs::ZOOM_SETTINGS_MODE_DISABLED:
      zoom_mode = WebContentsZoomController::ZoomMode::kDisabled;
  }

  contents->GetZoomController()->SetZoomMode(zoom_mode);

  return RespondNow(NoArguments());
}

bool IsKillURL(const GURL& url) {
#if DCHECK_IS_ON()
  // Caller should ensure that |url| is already "fixed up" by
  // url_formatter::FixupURL, which (among many other things) takes care
  // of rewriting about:kill into chrome://kill/.
  if (url.SchemeIs(url::kAboutScheme))
    DCHECK(url.IsAboutBlank() || url.IsAboutSrcdoc());
#endif

  static const char* const kill_hosts[] = {
      chrome::kChromeUICrashHost,         chrome::kChromeUIDelayedHangUIHost,
      chrome::kChromeUIHangUIHost,        chrome::kChromeUIKillHost,
      chrome::kChromeUIQuitHost,          chrome::kChromeUIRestartHost,
      content::kChromeUIBrowserCrashHost, content::kChromeUIMemoryExhaustHost,
  };

  if (!url.SchemeIs(content::kChromeUIScheme))
    return false;

  return base::Contains(kill_hosts, url.host_piece());
}

GURL ResolvePossiblyRelativeURL(const std::string& url_string,
                                const Extension* extension) {
  GURL url = GURL(url_string);
  if (!url.is_valid() && extension)
    url = extension->GetResourceURL(url_string);

  return url;
}
bool PrepareURLForNavigation(const std::string& url_string,
                             const Extension* extension,
                             GURL* return_url,
                             std::string* error) {
  GURL url = ResolvePossiblyRelativeURL(url_string, extension);

  // Ideally, the URL would only be "fixed" for user input (e.g. for URLs
  // entered into the Omnibox), but some extensions rely on the legacy behavior
  // where all navigations were subject to the "fixing".  See also
  // https://crbug.com/1145381.
  url = url_formatter::FixupURL(url.spec(), "" /* = desired_tld */);

  // Reject invalid URLs.
  if (!url.is_valid()) {
    const char kInvalidUrlError[] = "Invalid url: \"*\".";
    *error = ErrorUtils::FormatErrorMessage(kInvalidUrlError, url_string);
    return false;
  }

  // Don't let the extension crash the browser or renderers.
  if (IsKillURL(url)) {
    const char kNoCrashBrowserError[] =
        "I'm sorry. I'm afraid I can't do that.";
    *error = kNoCrashBrowserError;
    return false;
  }

  // Don't let the extension navigate directly to devtools scheme pages, unless
  // they have applicable permissions.
  if (url.SchemeIs(content::kChromeDevToolsScheme) &&
      !(extension->permissions_data()->HasAPIPermission(
            extensions::mojom::APIPermissionID::kDevtools) ||
        extension->permissions_data()->HasAPIPermission(
            extensions::mojom::APIPermissionID::kDebugger))) {
    const char kCannotNavigateToDevtools[] =
        "Cannot navigate to a devtools:// page without either the devtools or "
        "debugger permission.";
    *error = kCannotNavigateToDevtools;
    return false;
  }

  return_url->Swap(&url);
  return true;
}

TabsUpdateFunction::TabsUpdateFunction() : web_contents_(nullptr) {}

ExtensionFunction::ResponseAction TabsUpdateFunction::Run() {
  std::unique_ptr<tabs::Update::Params> params(
      tabs::Update::Params::Create(args()));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  int tab_id = params->tab_id ? *params->tab_id : -1;
  auto* contents = electron::api::WebContents::FromID(tab_id);
  if (!contents)
    return RespondNow(Error("No such tab"));

  web_contents_ = contents->web_contents();

  // Navigate the tab to a new location if the url is different.
  std::string error;
  if (params->update_properties.url.get()) {
    std::string updated_url = *params->update_properties.url;
    if (!UpdateURL(updated_url, tab_id, &error))
      return RespondNow(Error(std::move(error)));
  }

  if (params->update_properties.muted.get()) {
    contents->SetAudioMuted(*params->update_properties.muted);
  }

  return RespondNow(GetResult());
}

bool TabsUpdateFunction::UpdateURL(const std::string& url_string,
                                   int tab_id,
                                   std::string* error) {
  GURL url;
  if (!PrepareURLForNavigation(url_string, extension(), &url, error)) {
    return false;
  }

  const bool is_javascript_scheme = url.SchemeIs(url::kJavaScriptScheme);
  // JavaScript URLs are forbidden in chrome.tabs.update().
  if (is_javascript_scheme) {
    const char kJavaScriptUrlsNotAllowedInTabsUpdate[] =
        "JavaScript URLs are not allowed in chrome.tabs.update. Use "
        "chrome.tabs.executeScript instead.";
    *error = kJavaScriptUrlsNotAllowedInTabsUpdate;
    return false;
  }

  content::NavigationController::LoadURLParams load_params(url);

  // Treat extension-initiated navigations as renderer-initiated so that the URL
  // does not show in the omnibox until it commits.  This avoids URL spoofs
  // since URLs can be opened on behalf of untrusted content.
  load_params.is_renderer_initiated = true;
  // All renderer-initiated navigations need to have an initiator origin.
  load_params.initiator_origin = extension()->origin();
  // |source_site_instance| needs to be set so that a renderer process
  // compatible with |initiator_origin| is picked by Site Isolation.
  load_params.source_site_instance = content::SiteInstance::CreateForURL(
      web_contents_->GetBrowserContext(),
      load_params.initiator_origin->GetURL());

  // Marking the navigation as initiated via an API means that the focus
  // will stay in the omnibox - see https://crbug.com/1085779.
  load_params.transition_type = ui::PAGE_TRANSITION_FROM_API;

  web_contents_->GetController().LoadURLWithParams(load_params);

  DCHECK_EQ(url,
            web_contents_->GetController().GetPendingEntry()->GetVirtualURL());

  return true;
}

ExtensionFunction::ResponseValue TabsUpdateFunction::GetResult() {
  if (!has_callback())
    return NoArguments();

  tabs::Tab tab;

  auto* api_web_contents = electron::api::WebContents::From(web_contents_);
  tab.id =
      std::make_unique<int>(api_web_contents ? api_web_contents->ID() : -1);
  // TODO(nornagon): in Chrome, the tab URL is only available to extensions
  // that have the "tabs" (or "activeTab") permission. We should do the same
  // permission check here.
  tab.url = std::make_unique<std::string>(
      web_contents_->GetLastCommittedURL().spec());

  return ArgumentList(tabs::Get::Results::Create(std::move(tab)));
}

void TabsUpdateFunction::OnExecuteCodeFinished(
    const std::string& error,
    const GURL& url,
    const base::ListValue& script_result) {
  if (!error.empty()) {
    Respond(Error(error));
    return;
  }

  return Respond(GetResult());
}

}  // namespace extensions
