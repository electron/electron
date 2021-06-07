// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/tabs/tabs_api.h"

#include <iostream>
#include <memory>
#include <utility>

#include "chrome/browser/extensions/api/tabs/tabs_constants.h"
#include "extensions/browser/extension_api_frame_id_map.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/mojom/host_id.mojom.h"
#include "extensions/common/permissions/permissions_data.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/extensions/extension_tab_util.h"
#include "shell/browser/web_contents_zoom_controller.h"
#include "shell/common/extensions/api/tabs.h"
#include "third_party/blink/public/common/page/page_zoom.h"

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

electron::api::WebContents* GetActiveTab(ExtensionFunction* function) {
  auto* session =
      electron::api::Session::FromBrowserContext(function->browser_context());

  auto* sender_wc =
      electron::api::WebContents::From(function->GetSenderWebContents());
  if (!sender_wc)
    return nullptr;

  auto* web_contents = session->GetActiveTab(sender_wc);
  if (!web_contents)
    return nullptr;

  auto tab = session->GetExtensionTabDetails(web_contents);
  if (!tab)
    return nullptr;

  return web_contents;
}

bool GetTabById(int tab_id,
                ExtensionFunction* function,
                electron::api::WebContents** contents,
                electron::api::ExtensionTabDetails* out_tab,
                std::string* error_message) {
  auto* web_contents = ExtensionTabUtil::GetWebContentsById(tab_id);

  if (contents)
    *contents = web_contents;

  auto tab = ExtensionTabUtil::GetTabDetailsFromWebContents(web_contents);

  if (tab) {
    if (out_tab)
      *out_tab = tab.value();

    return true;
  }

  if (error_message) {
    *error_message = ErrorUtils::FormatErrorMessage(
        tabs_constants::kTabNotFoundError, base::NumberToString(tab_id));
  }

  return false;
}

// Gets the WebContents for |tab_id| if it is specified. Otherwise get the
// WebContents for the active tab in the |function|'s current window.
// Returns nullptr and fills |error| if failed.
electron::api::WebContents* GetTabsAPIDefaultWebContents(
    ExtensionFunction* function,
    int tab_id,
    std::string* error) {
  electron::api::WebContents* web_contents = nullptr;
  if (tab_id != -1) {
    // We assume this call leaves web_contents unchanged if it is unsuccessful.
    GetTabById(tab_id, function, &web_contents, nullptr, error);
  } else {
    web_contents = GetActiveTab(function);
    if (!web_contents)
      *error = tabs_constants::kNoSelectedTabError;
  }
  return web_contents;
}

std::unique_ptr<api::tabs::Tab> CreateTabObjectHelper(
    electron::api::WebContents* contents,
    electron::api::ExtensionTabDetails tab,
    const Extension* extension,
    Feature::Context context,
    int tab_index) {
  ExtensionTabUtil::ScrubTabBehavior scrub_tab_behavior =
      ExtensionTabUtil::GetScrubTabBehavior(extension, context,
                                            contents->web_contents());
  return ExtensionTabUtil::CreateTabObject(
      contents->web_contents(), tab, scrub_tab_behavior, extension, tab_index);
}

ExecuteCodeInTabFunction::ExecuteCodeInTabFunction() : execute_tab_id_(-1) {}

ExecuteCodeInTabFunction::~ExecuteCodeInTabFunction() = default;

ExecuteCodeFunction::InitResult ExecuteCodeInTabFunction::Init() {
  if (init_result_)
    return init_result_.value();

  // |tab_id| is optional so it's ok if it's not there.
  int tab_id = -1;
  if (args_->GetInteger(0, &tab_id) && tab_id < 0)
    return set_init_result(VALIDATION_FAILURE);

  // |details| are not optional.
  base::DictionaryValue* details_value = NULL;
  if (!args_->GetDictionary(1, &details_value))
    return set_init_result(VALIDATION_FAILURE);
  std::unique_ptr<InjectDetails> details(new InjectDetails());
  if (!InjectDetails::Populate(*details_value, details.get()))
    return set_init_result(VALIDATION_FAILURE);

  if (tab_id == -1) {
    electron::api::WebContents* web_contents = GetActiveTab(this);
    if (!web_contents)
      return set_init_result_error(tabs_constants::kNoTabInBrowserWindowError);

    tab_id = web_contents->ID();
  }

  execute_tab_id_ = tab_id;
  details_ = std::move(details);
  set_host_id(
      mojom::HostID(mojom::HostID::HostType::kExtensions, extension()->id()));
  return set_init_result(SUCCESS);
}

bool ExecuteCodeInTabFunction::CanExecuteScriptOnPage(std::string* error) {
  electron::api::WebContents* contents = nullptr;
  // If |tab_id| is specified, look for the tab. Otherwise default to selected
  // tab in the current window.
  CHECK_GE(execute_tab_id_, 0);
  if (!GetTabById(execute_tab_id_, this, &contents, nullptr, error)) {
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
  electron::api::WebContents* contents = nullptr;

  bool success =
      GetTabById(execute_tab_id_, this, &contents, nullptr, error) && contents;

  if (!success)
    return nullptr;

  return contents->script_executor();
}

bool ExecuteCodeInTabFunction::ShouldInsertCSS() const {
  return false;
}

bool ExecuteCodeInTabFunction::ShouldRemoveCSS() const {
  return false;
}

bool ExecuteCodeInTabFunction::IsWebView() const {
  return false;
}

const GURL& ExecuteCodeInTabFunction::GetWebViewSrc() const {
  return GURL::EmptyGURL();
}

bool TabsInsertCSSFunction::ShouldInsertCSS() const {
  return true;
}

bool TabsRemoveCSSFunction::ShouldRemoveCSS() const {
  return true;
}

ExtensionFunction::ResponseAction TabsGetFunction::Run() {
  std::unique_ptr<tabs::Get::Params> params(tabs::Get::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());
  int tab_id = params->tab_id;

  electron::api::WebContents* contents = nullptr;
  electron::api::ExtensionTabDetails tab;
  std::string error;
  if (!GetTabById(tab_id, this, &contents, &tab, &error)) {
    return RespondNow(Error(std::move(error)));
  }

  return RespondNow(
      ArgumentList(tabs::Get::Results::Create(*CreateTabObjectHelper(
          contents, tab, extension(), source_context_type(), 0))));
}

ExtensionFunction::ResponseAction TabsSetZoomFunction::Run() {
  std::unique_ptr<tabs::SetZoom::Params> params(
      tabs::SetZoom::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  std::string error;
  auto* contents = GetTabsAPIDefaultWebContents(this, tab_id, &error);
  if (!contents)
    return RespondNow(Error(std::move(error)));

  auto* web_contents = contents->web_contents();
  GURL url(web_contents->GetVisibleURL());
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
      tabs::GetZoom::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  std::string error;
  auto* contents = GetTabsAPIDefaultWebContents(this, tab_id, &error);
  if (!contents)
    return RespondNow(Error(std::move(error)));

  double zoom_level = contents->GetZoomController()->GetZoomLevel();
  double zoom_factor = blink::PageZoomLevelToZoomFactor(zoom_level);

  return RespondNow(ArgumentList(tabs::GetZoom::Results::Create(zoom_factor)));
}

ExtensionFunction::ResponseAction TabsGetZoomSettingsFunction::Run() {
  std::unique_ptr<tabs::GetZoomSettings::Params> params(
      tabs::GetZoomSettings::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  std::string error;
  auto* contents = GetTabsAPIDefaultWebContents(this, tab_id, &error);
  if (!contents)
    return RespondNow(Error(std::move(error)));

  auto* zoom_controller = contents->GetZoomController();
  WebContentsZoomController::ZoomMode zoom_mode =
      contents->GetZoomController()->zoom_mode();
  api::tabs::ZoomSettings zoom_settings;
  ZoomModeToZoomSettings(zoom_mode, &zoom_settings);
  zoom_settings.default_zoom_factor.reset(
      new double(blink::PageZoomLevelToZoomFactor(
          zoom_controller->GetDefaultZoomLevel())));

  return RespondNow(
      ArgumentList(api::tabs::GetZoomSettings::Results::Create(zoom_settings)));
}

ExtensionFunction::ResponseAction TabsSetZoomSettingsFunction::Run() {
  using api::tabs::ZoomSettings;

  std::unique_ptr<tabs::SetZoomSettings::Params> params(
      tabs::SetZoomSettings::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  int tab_id = params->tab_id ? *params->tab_id : -1;
  std::string error;
  auto* contents = GetTabsAPIDefaultWebContents(this, tab_id, &error);
  if (!contents)
    return RespondNow(Error(std::move(error)));

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

}  // namespace extensions
