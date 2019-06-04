// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_contents_preferences.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "electron/buildflags/buildflags.h"
#include "net/base/escape.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "atom/common/atom_constants.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/stream_info.h"
#include "net/url_request/url_request.h"
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

using content::BrowserThread;

namespace atom {

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {}

}  // namespace atom
