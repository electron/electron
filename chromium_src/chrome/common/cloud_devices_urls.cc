// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/cloud_devices_urls.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/url_util.h"

namespace cloud_devices {

namespace {

// Url must not be matched by "urls" section of
// cloud_print_app/manifest.json. If it's matched, print driver dialog will
// open sign-in page in separate window.
const char kCloudPrintURL[] = "https://www.google.com/cloudprint";

}

// Returns the root service URL for the cloud print service.  The default is to
// point at the Google Cloud Print service.  This can be overridden by the
// command line or by the user preferences.
GURL GetCloudPrintURL() {
  return GURL(kCloudPrintURL);
}

GURL GetCloudPrintRelativeURL(const std::string& relative_path) {
  GURL url = GetCloudPrintURL();
  std::string path;
  static const char kURLPathSeparator[] = "/";
  base::TrimString(url.path(), kURLPathSeparator, &path);
  std::string trimmed_path;
  base::TrimString(relative_path, kURLPathSeparator, &trimmed_path);
  path += kURLPathSeparator;
  path += trimmed_path;
  GURL::Replacements replacements;
  replacements.SetPathStr(path);
  return url.ReplaceComponents(replacements);
}

}  // namespace cloud_devices
