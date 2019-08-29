// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/util_gtk.h"

#include <gtk/gtk.h>

namespace util_gtk {

// Copied from L40-L55 in
// https://cs.chromium.org/chromium/src/chrome/browser/ui/libgtkui/select_file_dialog_impl_gtk.cc
#if GTK_CHECK_VERSION(3, 90, 0)
// GTK stock items have been deprecated.  The docs say to switch to using the
// strings "_Open", etc.  However this breaks i18n.  We could supply our own
// internationalized strings, but the "_" in these strings is significant: it's
// the keyboard shortcut to select these actions.  TODO: Provide
// internationalized strings when GTK provides support for it.
const char* const kCancelLabel = "_Cancel";
const char* const kNoLabel = "_No";
const char* const kOkLabel = "_OK";
const char* const kOpenLabel = "_Open";
const char* const kSaveLabel = "_Save";
const char* const kYesLabel = "_Yes";
#else
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
const char* const kCancelLabel = GTK_STOCK_CANCEL;
const char* const kNoLabel = GTK_STOCK_NO;
const char* const kOkLabel = GTK_STOCK_OK;
const char* const kOpenLabel = GTK_STOCK_OPEN;
const char* const kSaveLabel = GTK_STOCK_SAVE;
const char* const kYesLabel = GTK_STOCK_YES;
G_GNUC_END_IGNORE_DEPRECATIONS
#endif

}  // namespace util_gtk
