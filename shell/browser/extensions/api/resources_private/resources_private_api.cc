// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/resources_private/resources_private_api.h"

#include <memory>
#include <string>
#include <utility>

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/extensions/api/resources_private.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/zoom/page_zoom_constants.h"
#include "pdf/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

#if BUILDFLAG(ENABLE_PDF)
#include "pdf/pdf_features.h"
#endif  // BUILDFLAG(ENABLE_PDF)

// To add a new component to this API, simply:
// 1. Add your component to the Component enum in
//      chrome/common/extensions/api/resources_private.idl
// 2. Create an AddStringsForMyComponent(base::DictionaryValue * dict) method.
// 3. Tie in that method to the switch statement in Run()

namespace extensions {

namespace {

void AddStringsForPdf(base::DictionaryValue* dict) {
#if BUILDFLAG(ENABLE_PDF)
  static constexpr webui::LocalizedString kPdfResources[] = {
      {"passwordDialogTitle", IDS_PDF_PASSWORD_DIALOG_TITLE},
      {"passwordPrompt", IDS_PDF_NEED_PASSWORD},
      {"passwordSubmit", IDS_PDF_PASSWORD_SUBMIT},
      {"thumbnailPageAriaLabel", IDS_PDF_THUMBNAIL_PAGE_ARIA_LABEL},
      {"passwordInvalid", IDS_PDF_PASSWORD_INVALID},
      {"pageLoading", IDS_PDF_PAGE_LOADING},
      {"pageLoadFailed", IDS_PDF_PAGE_LOAD_FAILED},
      {"errorDialogTitle", IDS_PDF_ERROR_DIALOG_TITLE},
      {"pageReload", IDS_PDF_PAGE_RELOAD_BUTTON},
      {"bookmarks", IDS_PDF_BOOKMARKS},
      {"labelPageNumber", IDS_PDF_LABEL_PAGE_NUMBER},
      {"tooltipDownload", IDS_PDF_TOOLTIP_DOWNLOAD},
      {"tooltipPrint", IDS_PDF_TOOLTIP_PRINT},
      {"tooltipFitToPage", IDS_PDF_TOOLTIP_FIT_PAGE},
      {"tooltipFitToWidth", IDS_PDF_TOOLTIP_FIT_WIDTH},
      {"tooltipZoomIn", IDS_PDF_TOOLTIP_ZOOM_IN},
      {"tooltipZoomOut", IDS_PDF_TOOLTIP_ZOOM_OUT},
  };
  for (const auto& resource : kPdfResources)
    dict->SetString(resource.name, l10n_util::GetStringUTF16(resource.id));

  dict->SetString("presetZoomFactors", zoom::GetPresetZoomFactorsAsJSON());
#endif  // BUILDFLAG(ENABLE_PDF)
}

void AddAdditionalDataForPdf(base::DictionaryValue* dict) {
#if BUILDFLAG(ENABLE_PDF)
  dict->SetKey("pdfAnnotationsEnabled", base::Value(false));
  dict->SetKey("printingEnabled", base::Value(true));
#endif  // BUILDFLAG(ENABLE_PDF)
}

}  // namespace

namespace get_strings = api::resources_private::GetStrings;

ResourcesPrivateGetStringsFunction::ResourcesPrivateGetStringsFunction() =
    default;

ResourcesPrivateGetStringsFunction::~ResourcesPrivateGetStringsFunction() =
    default;

ExtensionFunction::ResponseAction ResourcesPrivateGetStringsFunction::Run() {
  std::unique_ptr<get_strings::Params> params(
      get_strings::Params::Create(args()));
  auto dict = std::make_unique<base::DictionaryValue>();

  api::resources_private::Component component = params->component;

  switch (component) {
    case api::resources_private::COMPONENT_PDF:
      AddStringsForPdf(dict.get());
      AddAdditionalDataForPdf(dict.get());
      break;
    case api::resources_private::COMPONENT_IDENTITY:
      NOTREACHED();
      break;
    case api::resources_private::COMPONENT_NONE:
      NOTREACHED();
      break;
  }

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, dict.get());

  return RespondNow(
      OneArgument(base::Value::FromUniquePtrValue(std::move(dict))));
}

}  // namespace extensions
