// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/electron_constants.h"

namespace electron {

#if BUILDFLAG(ENABLE_PDF_VIEWER)
const char kPDFExtensionPluginName[] = "Chromium PDF Viewer";
const char kPDFInternalPluginName[] = "Chromium PDF Plugin";
const base::FilePath::CharType kPdfPluginPath[] =
    FILE_PATH_LITERAL("internal-pdf-viewer");
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

}  // namespace electron
