// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_UI_GTK_UTIL_H_
#define ELECTRON_SHELL_BROWSER_UI_GTK_UTIL_H_

#include <gtk/gtk.h>

class SkBitmap;

namespace gtk_util {

const char* GetCancelLabel();
const char* GetOpenLabel();
const char* GetSaveLabel();
const char* GetOkLabel();
const char* GetNoLabel();
const char* GetYesLabel();

// Convert and copy a SkBitmap to a GdkPixbuf. NOTE: this uses BGRAToRGBA, so
// it is an expensive operation.  The returned GdkPixbuf will have a refcount of
// 1, and the caller is responsible for unrefing it when done.
GdkPixbuf* GdkPixbufFromSkBitmap(const SkBitmap& bitmap);

}  // namespace gtk_util

#endif  // ELECTRON_SHELL_BROWSER_UI_GTK_UTIL_H_
