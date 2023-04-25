// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/extensions/api/pdf_viewer_private/pdf_viewer_private_api.h"

#include <string>

#include "base/values.h"
#include "chrome/common/extensions/api/pdf_viewer_private.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "shell/browser/electron_browser_context.h"
#include "url/url_constants.h"

namespace extensions {

namespace {

namespace IsAllowedLocalFileAccess =
    api::pdf_viewer_private::IsAllowedLocalFileAccess;

namespace SetPdfOcrPref = api::pdf_viewer_private::SetPdfOcrPref;

// Check if the current URL is allowed based on a list of allowlisted domains.
bool IsUrlAllowedToEmbedLocalFiles(
    const GURL& current_url,
    const base::Value::List& allowlisted_domains) {
  if (!current_url.is_valid() || !current_url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  for (auto const& value : allowlisted_domains) {
    const std::string* domain = value.GetIfString();
    if (!domain) {
      continue;
    }

    if (current_url.DomainIs(*domain)) {
      return true;
    }
  }
  return false;
}

}  // namespace

PdfViewerPrivateIsAllowedLocalFileAccessFunction::
    PdfViewerPrivateIsAllowedLocalFileAccessFunction() = default;

PdfViewerPrivateIsAllowedLocalFileAccessFunction::
    ~PdfViewerPrivateIsAllowedLocalFileAccessFunction() = default;

ExtensionFunction::ResponseAction
PdfViewerPrivateIsAllowedLocalFileAccessFunction::Run() {
  absl::optional<IsAllowedLocalFileAccess::Params> params =
      IsAllowedLocalFileAccess::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);

  return RespondNow(WithArguments(
      IsUrlAllowedToEmbedLocalFiles(GURL(params->url), base::Value::List())));
}

PdfViewerPrivateIsPdfOcrAlwaysActiveFunction::
    PdfViewerPrivateIsPdfOcrAlwaysActiveFunction() = default;

PdfViewerPrivateIsPdfOcrAlwaysActiveFunction::
    ~PdfViewerPrivateIsPdfOcrAlwaysActiveFunction() = default;

// TODO(codebytere): enable when https://crbug.com/1393069 works properly.
ExtensionFunction::ResponseAction
PdfViewerPrivateIsPdfOcrAlwaysActiveFunction::Run() {
  return RespondNow(WithArguments(false));
}

PdfViewerPrivateSetPdfOcrPrefFunction::PdfViewerPrivateSetPdfOcrPrefFunction() =
    default;

PdfViewerPrivateSetPdfOcrPrefFunction::
    ~PdfViewerPrivateSetPdfOcrPrefFunction() = default;

// TODO(codebytere): enable when https://crbug.com/1393069 works properly.
ExtensionFunction::ResponseAction PdfViewerPrivateSetPdfOcrPrefFunction::Run() {
  absl::optional<SetPdfOcrPref::Params> params =
      SetPdfOcrPref::Params::Create(args());
  EXTENSION_FUNCTION_VALIDATE(params);
  return RespondNow(WithArguments(false));
}

}  // namespace extensions
