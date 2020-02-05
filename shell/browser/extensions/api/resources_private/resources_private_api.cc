// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/resources_private/resources_private_api.h"

#include <memory>
#include <string>
#include <utility>

#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/ui/webui/webui_util.h"
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
#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/login/ui/login_display_host.h"
#endif  // defined(OS_CHROMEOS)
#endif  // BUILDFLAG(ENABLE_PDF)

// To add a new component to this API, simply:
// 1. Add your component to the Component enum in
//      chrome/common/extensions/api/resources_private.idl
// 2. Create an AddStringsForMyComponent(base::DictionaryValue * dict) method.
// 3. Tie in that method to the switch statement in Run()

namespace extensions {

namespace {

void AddStringsForIdentity(base::DictionaryValue* dict) {
  dict->SetString("window-title",
                  l10n_util::GetStringUTF16(IDS_EXTENSION_CONFIRM_PERMISSIONS));
}

void AddStringsForPdf(base::DictionaryValue* dict) {
#if BUILDFLAG(ENABLE_PDF)
  static constexpr webui::LocalizedString kPdfResources[] = {
    {"passwordDialogTitle", IDS_PDF_PASSWORD_DIALOG_TITLE},
    {"passwordPrompt", IDS_PDF_NEED_PASSWORD},
    {"passwordSubmit", IDS_PDF_PASSWORD_SUBMIT},
    {"passwordInvalid", IDS_PDF_PASSWORD_INVALID},
    {"pageLoading", IDS_PDF_PAGE_LOADING},
    {"pageLoadFailed", IDS_PDF_PAGE_LOAD_FAILED},
    {"errorDialogTitle", IDS_PDF_ERROR_DIALOG_TITLE},
    {"pageReload", IDS_PDF_PAGE_RELOAD_BUTTON},
    {"bookmarks", IDS_PDF_BOOKMARKS},
    {"labelPageNumber", IDS_PDF_LABEL_PAGE_NUMBER},
    {"tooltipRotateCW", IDS_PDF_TOOLTIP_ROTATE_CW},
    {"tooltipDownload", IDS_PDF_TOOLTIP_DOWNLOAD},
    {"tooltipPrint", IDS_PDF_TOOLTIP_PRINT},
    {"tooltipFitToPage", IDS_PDF_TOOLTIP_FIT_PAGE},
    {"tooltipFitToWidth", IDS_PDF_TOOLTIP_FIT_WIDTH},
    {"tooltipZoomIn", IDS_PDF_TOOLTIP_ZOOM_IN},
    {"tooltipZoomOut", IDS_PDF_TOOLTIP_ZOOM_OUT},
#if defined(OS_CHROMEOS)
    {"tooltipAnnotate", IDS_PDF_ANNOTATION_ANNOTATE},
    {"annotationDocumentTooLarge", IDS_PDF_ANNOTATION_DOCUMENT_TOO_LARGE},
    {"annotationDocumentProtected", IDS_PDF_ANNOTATION_DOCUMENT_PROTECTED},
    {"annotationDocumentRotated", IDS_PDF_ANNOTATION_DOCUMENT_ROTATED},
    {"annotationPen", IDS_PDF_ANNOTATION_PEN},
    {"annotationHighlighter", IDS_PDF_ANNOTATION_HIGHLIGHTER},
    {"annotationEraser", IDS_PDF_ANNOTATION_ERASER},
    {"annotationUndo", IDS_PDF_ANNOTATION_UNDO},
    {"annotationRedo", IDS_PDF_ANNOTATION_REDO},
    {"annotationExpand", IDS_PDF_ANNOTATION_EXPAND},
    {"annotationColorBlack", IDS_PDF_ANNOTATION_COLOR_BLACK},
    {"annotationColorRed", IDS_PDF_ANNOTATION_COLOR_RED},
    {"annotationColorYellow", IDS_PDF_ANNOTATION_COLOR_YELLOW},
    {"annotationColorGreen", IDS_PDF_ANNOTATION_COLOR_GREEN},
    {"annotationColorCyan", IDS_PDF_ANNOTATION_COLOR_CYAN},
    {"annotationColorPurple", IDS_PDF_ANNOTATION_COLOR_PURPLE},
    {"annotationColorBrown", IDS_PDF_ANNOTATION_COLOR_BROWN},
    {"annotationColorWhite", IDS_PDF_ANNOTATION_COLOR_WHITE},
    {"annotationColorCrimson", IDS_PDF_ANNOTATION_COLOR_CRIMSON},
    {"annotationColorAmber", IDS_PDF_ANNOTATION_COLOR_AMBER},
    {"annotationColorAvocadoGreen", IDS_PDF_ANNOTATION_COLOR_AVOCADO_GREEN},
    {"annotationColorCobaltBlue", IDS_PDF_ANNOTATION_COLOR_COBALT_BLUE},
    {"annotationColorDeepPurple", IDS_PDF_ANNOTATION_COLOR_DEEP_PURPLE},
    {"annotationColorDarkBrown", IDS_PDF_ANNOTATION_COLOR_DARK_BROWN},
    {"annotationColorDarkGrey", IDS_PDF_ANNOTATION_COLOR_DARK_GREY},
    {"annotationColorHotPink", IDS_PDF_ANNOTATION_COLOR_HOT_PINK},
    {"annotationColorOrange", IDS_PDF_ANNOTATION_COLOR_ORANGE},
    {"annotationColorLime", IDS_PDF_ANNOTATION_COLOR_LIME},
    {"annotationColorBlue", IDS_PDF_ANNOTATION_COLOR_BLUE},
    {"annotationColorViolet", IDS_PDF_ANNOTATION_COLOR_VIOLET},
    {"annotationColorTeal", IDS_PDF_ANNOTATION_COLOR_TEAL},
    {"annotationColorLightGrey", IDS_PDF_ANNOTATION_COLOR_LIGHT_GREY},
    {"annotationColorLightPink", IDS_PDF_ANNOTATION_COLOR_LIGHT_PINK},
    {"annotationColorLightOrange", IDS_PDF_ANNOTATION_COLOR_LIGHT_ORANGE},
    {"annotationColorLightGreen", IDS_PDF_ANNOTATION_COLOR_LIGHT_GREEN},
    {"annotationColorLightBlue", IDS_PDF_ANNOTATION_COLOR_LIGHT_BLUE},
    {"annotationColorLavender", IDS_PDF_ANNOTATION_COLOR_LAVENDER},
    {"annotationColorLightTeal", IDS_PDF_ANNOTATION_COLOR_LIGHT_TEAL},
    {"annotationSize1", IDS_PDF_ANNOTATION_SIZE1},
    {"annotationSize2", IDS_PDF_ANNOTATION_SIZE2},
    {"annotationSize3", IDS_PDF_ANNOTATION_SIZE3},
    {"annotationSize4", IDS_PDF_ANNOTATION_SIZE4},
    {"annotationSize8", IDS_PDF_ANNOTATION_SIZE8},
    {"annotationSize12", IDS_PDF_ANNOTATION_SIZE12},
    {"annotationSize16", IDS_PDF_ANNOTATION_SIZE16},
    {"annotationSize20", IDS_PDF_ANNOTATION_SIZE20},
    {"annotationFormWarningTitle", IDS_PDF_DISCARD_FORM_CHANGES},
    {"annotationFormWarningDetail", IDS_PDF_DISCARD_FORM_CHANGES_DETAIL},
    {"annotationFormWarningKeepEditing", IDS_PDF_KEEP_EDITING},
    {"annotationFormWarningDiscard", IDS_PDF_DISCARD},
#endif  // defined(OS_CHROMEOS)
  };
  for (const auto& resource : kPdfResources)
    dict->SetString(resource.name, l10n_util::GetStringUTF16(resource.id));

  dict->SetString("presetZoomFactors", zoom::GetPresetZoomFactorsAsJSON());
#endif  // BUILDFLAG(ENABLE_PDF)
}

void AddAdditionalDataForPdf(base::DictionaryValue* dict) {
#if BUILDFLAG(ENABLE_PDF)
  dict->SetKey("pdfFormSaveEnabled",
               base::Value(base::FeatureList::IsEnabled(
                   chrome_pdf::features::kSaveEditedPDFForm)));
  dict->SetKey("pdfAnnotationsEnabled",
               base::Value(base::FeatureList::IsEnabled(
                   chrome_pdf::features::kPDFAnnotations)));

  bool enable_printing = true;
#if defined(OS_CHROMEOS)
  // For Chrome OS, enable printing only if we are not at OOBE.
  enable_printing = !chromeos::LoginDisplayHost::default_host();
#endif  // defined(OS_CHROMEOS)
  dict->SetKey("printingEnabled", base::Value(enable_printing));
#endif  // BUILDFLAG(ENABLE_PDF)
}

}  // namespace

namespace get_strings = api::resources_private::GetStrings;

ResourcesPrivateGetStringsFunction::ResourcesPrivateGetStringsFunction() {}

ResourcesPrivateGetStringsFunction::~ResourcesPrivateGetStringsFunction() {}

ExtensionFunction::ResponseAction ResourcesPrivateGetStringsFunction::Run() {
  std::unique_ptr<get_strings::Params> params(
      get_strings::Params::Create(*args_));
  auto dict = std::make_unique<base::DictionaryValue>();

  api::resources_private::Component component = params->component;

  switch (component) {
    case api::resources_private::COMPONENT_IDENTITY:
      AddStringsForIdentity(dict.get());
      break;
    case api::resources_private::COMPONENT_PDF:
      AddStringsForPdf(dict.get());
      AddAdditionalDataForPdf(dict.get());
      break;
    case api::resources_private::COMPONENT_NONE:
      NOTREACHED();
  }

  const std::string& app_locale = g_browser_process->GetApplicationLocale();
  webui::SetLoadTimeDataDefaults(app_locale, dict.get());

  return RespondNow(OneArgument(std::move(dict)));
}

}  // namespace extensions
