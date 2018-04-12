// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/browser/web_contents_preferences.h"
#include "atom/common/platform_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/render_frame_host.h"
#include "net/base/escape.h"
#include "url/gurl.h"

#if defined(ENABLE_PDF_VIEWER)
#include "atom/common/atom_constants.h"
#include "base/strings/stringprintf.h"
#include "content/public/browser/stream_info.h"
#include "net/url_request/url_request.h"
#endif  // defined(ENABLE_PDF_VIEWER)

using content::BrowserThread;

namespace atom {

namespace {

void OnOpenExternal(const GURL& escaped_url, bool allowed) {
  if (allowed)
    platform_util::OpenExternal(
#if defined(OS_WIN)
        base::UTF8ToUTF16(escaped_url.spec()),
#else
        escaped_url,
#endif
        true);
}

void HandleExternalProtocolInUI(
    const GURL& url,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    bool has_user_gesture) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  auto* permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  auto callback = base::Bind(&OnOpenExternal, escaped_url);
  permission_helper->RequestOpenExternalPermission(callback, has_user_gesture,
                                                   url);
}

#if defined(ENABLE_PDF_VIEWER)
void OnPdfResourceIntercepted(
    const GURL& original_url,
    int render_process_host_id,
    int render_frame_id,
    const content::ResourceRequestInfo::WebContentsGetter&
        web_contents_getter) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  auto* web_preferences = WebContentsPreferences::From(web_contents);
  if (!web_preferences || !web_preferences->IsEnabled(options::kPlugins)) {
    auto* browser_context = web_contents->GetBrowserContext();
    auto* download_manager =
        content::BrowserContext::GetDownloadManager(browser_context);

    download_manager->DownloadUrl(
        content::DownloadUrlParameters::CreateForWebContentsMainFrame(
            web_contents, original_url, NO_TRAFFIC_ANNOTATION_YET));
    return;
  }

  // The URL passes the original pdf resource url, that will be requested
  // by the webui page.
  // chrome://pdf-viewer/index.html?src=https://somepage/123.pdf
  content::NavigationController::LoadURLParams params(GURL(base::StringPrintf(
      "%sindex.html?%s=%s", kPdfViewerUIOrigin, kPdfPluginSrc,
      net::EscapeUrlEncodedData(original_url.spec(), false).c_str())));

  content::RenderFrameHost* frame_host =
      content::RenderFrameHost::FromID(render_process_host_id, render_frame_id);
  if (!frame_host) {
    return;
  }

  params.frame_tree_node_id = frame_host->GetFrameTreeNodeId();
  web_contents->GetController().LoadURLWithParams(params);
}
#endif  // defined(ENABLE_PDF_VIEWER)

}  // namespace

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {}

bool AtomResourceDispatcherHostDelegate::HandleExternalProtocol(
    const GURL& url,
    content::ResourceRequestInfo* info) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::BindOnce(&HandleExternalProtocolInUI, url,
                                         info->GetWebContentsGetterForRequest(),
                                         info->HasUserGesture()));
  return true;
}

bool AtomResourceDispatcherHostDelegate::ShouldInterceptResourceAsStream(
    net::URLRequest* request,
    const std::string& mime_type,
    GURL* origin,
    std::string* payload) {
#if defined(ENABLE_PDF_VIEWER)
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);

  int render_process_host_id;
  int render_frame_id;
  if (!info->GetAssociatedRenderFrame(&render_process_host_id,
                                      &render_frame_id)) {
    return false;
  }

  if (mime_type == "application/pdf") {
    *origin = GURL(kPdfViewerUIOrigin);
    content::BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&OnPdfResourceIntercepted, request->url(),
                   render_process_host_id, render_frame_id,
                   info->GetWebContentsGetterForRequest()));
    return true;
  }
#endif  // defined(ENABLE_PDF_VIEWER)
  return false;
}

}  // namespace atom
