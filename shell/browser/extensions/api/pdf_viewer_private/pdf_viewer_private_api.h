// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_EXTENSIONS_API_PDF_VIEWER_PRIVATE_PDF_VIEWER_PRIVATE_API_H_
#define ELECTRON_SHELL_BROWSER_EXTENSIONS_API_PDF_VIEWER_PRIVATE_PDF_VIEWER_PRIVATE_API_H_

#include "chrome/common/extensions/api/pdf_viewer_private.h"
#include "extensions/browser/extension_function.h"
#include "pdf/buildflags.h"

namespace extensions {

class PdfViewerPrivateGetStreamInfoFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pdfViewerPrivate.getStreamInfo",
                             PDFVIEWERPRIVATE_GETSTREAMINFO)

  PdfViewerPrivateGetStreamInfoFunction();
  PdfViewerPrivateGetStreamInfoFunction(
      const PdfViewerPrivateGetStreamInfoFunction&) = delete;
  PdfViewerPrivateGetStreamInfoFunction& operator=(
      const PdfViewerPrivateGetStreamInfoFunction&) = delete;

 protected:
  ~PdfViewerPrivateGetStreamInfoFunction() override;

  // Override from ExtensionFunction:
  ResponseAction Run() override;
};

class PdfViewerPrivateIsAllowedLocalFileAccessFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pdfViewerPrivate.isAllowedLocalFileAccess",
                             PDFVIEWERPRIVATE_ISALLOWEDLOCALFILEACCESS)

  PdfViewerPrivateIsAllowedLocalFileAccessFunction();
  PdfViewerPrivateIsAllowedLocalFileAccessFunction(
      const PdfViewerPrivateIsAllowedLocalFileAccessFunction&) = delete;
  PdfViewerPrivateIsAllowedLocalFileAccessFunction& operator=(
      const PdfViewerPrivateIsAllowedLocalFileAccessFunction&) = delete;

 protected:
  ~PdfViewerPrivateIsAllowedLocalFileAccessFunction() override;

  // Override from ExtensionFunction:
  ResponseAction Run() override;
};

class PdfViewerPrivateSaveToDriveFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pdfViewerPrivate.saveToDrive",
                             PDFVIEWERPRIVATE_SAVETODRIVE)

  PdfViewerPrivateSaveToDriveFunction();
  PdfViewerPrivateSaveToDriveFunction(
      const PdfViewerPrivateSaveToDriveFunction&) = delete;
  PdfViewerPrivateSaveToDriveFunction& operator=(
      const PdfViewerPrivateSaveToDriveFunction&) = delete;

 protected:
  ~PdfViewerPrivateSaveToDriveFunction() override;

  // Override from ExtensionFunction:
  ResponseAction Run() override;

#if BUILDFLAG(ENABLE_PDF_SAVE_TO_DRIVE)
 private:
  ResponseAction RunSaveToDriveFlow(
      api::pdf_viewer_private::SaveRequestType request_type);

  ResponseAction StopSaveToDriveFlow();
#endif  // BUILDFLAG(ENABLE_PDF_SAVE_TO_DRIVE)
};

class PdfViewerPrivateSetPdfDocumentTitleFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pdfViewerPrivate.setPdfDocumentTitle",
                             PDFVIEWERPRIVATE_SETPDFDOCUMENTTITLE)

  PdfViewerPrivateSetPdfDocumentTitleFunction();
  PdfViewerPrivateSetPdfDocumentTitleFunction(
      const PdfViewerPrivateSetPdfDocumentTitleFunction&) = delete;
  PdfViewerPrivateSetPdfDocumentTitleFunction& operator=(
      const PdfViewerPrivateSetPdfDocumentTitleFunction&) = delete;

 protected:
  ~PdfViewerPrivateSetPdfDocumentTitleFunction() override;

  // Override from ExtensionFunction:
  ResponseAction Run() override;
};

class PdfViewerPrivateSetPdfPluginAttributesFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("pdfViewerPrivate.setPdfPluginAttributes",
                             PDFVIEWERPRIVATE_SETPDFPLUGINATTRIBUTES)

  PdfViewerPrivateSetPdfPluginAttributesFunction();
  PdfViewerPrivateSetPdfPluginAttributesFunction(
      const PdfViewerPrivateSetPdfPluginAttributesFunction&) = delete;
  PdfViewerPrivateSetPdfPluginAttributesFunction& operator=(
      const PdfViewerPrivateSetPdfPluginAttributesFunction&) = delete;

 protected:
  ~PdfViewerPrivateSetPdfPluginAttributesFunction() override;

  // Override from ExtensionFunction:
  ResponseAction Run() override;
};

}  // namespace extensions

#endif  // ELECTRON_SHELL_BROWSER_EXTENSIONS_API_PDF_VIEWER_PRIVATE_PDF_VIEWER_PRIVATE_API_H_
