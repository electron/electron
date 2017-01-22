// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/login_handler.h"
#include "atom/browser/stream_manager.h"
#include "atom/browser/ui/webui/pdf_viewer_ui.h"
#include "atom/browser/web_contents_permission_helper.h"
#include "atom/common/platform_util.h"
#include "base/guid.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"
#include "net/ssl/client_cert_store.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

#if defined(USE_NSS_CERTS)
#include "net/ssl/client_cert_store_nss.h"
#elif defined(OS_WIN)
#include "net/ssl/client_cert_store_win.h"
#elif defined(OS_MACOSX)
#include "net/ssl/client_cert_store_mac.h"
#endif

using content::BrowserThread;

namespace atom {

namespace {

void OnOpenExternal(const GURL& escaped_url,
                    bool allowed) {
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

  auto permission_helper =
      WebContentsPermissionHelper::FromWebContents(web_contents);
  if (!permission_helper)
    return;

  GURL escaped_url(net::EscapeExternalHandlerValue(url.spec()));
  auto callback = base::Bind(&OnOpenExternal, escaped_url);
  permission_helper->RequestOpenExternalPermission(callback, has_user_gesture);
}

void OnPdfStreamCreated(
    std::unique_ptr<content::StreamInfo> stream,
    int64_t expected_content_size,
    const content::ResourceRequestInfo::WebContentsGetter& web_contents_getter,
    int render_process_id,
    int render_frame_id) {
  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  auto browser_context =
      static_cast<AtomBrowserContext*>(web_contents->GetBrowserContext());
  auto stream_manager = browser_context->stream_manager();
  std::string view_id = base::GenerateGUID();
  stream_manager->AddStream(std::move(stream), view_id, render_process_id,
                            render_frame_id);
  content::NavigationController::LoadURLParams params(
      GURL(base::StringPrintf("%sindex.html?%s=%s", PdfViewerUI::kOrigin,
                              PdfViewerUI::kId, view_id.c_str())));
  web_contents->GetController().LoadURLWithParams(params);
}

}  // namespace

AtomResourceDispatcherHostDelegate::AtomResourceDispatcherHostDelegate() {
}

bool AtomResourceDispatcherHostDelegate::HandleExternalProtocol(
    const GURL& url,
    content::ResourceRequestInfo* info) {
  BrowserThread::PostTask(BrowserThread::UI, FROM_HERE,
                          base::Bind(&HandleExternalProtocolInUI,
                                     url,
                                     info->GetWebContentsGetterForRequest(),
                                     info->HasUserGesture()));
  return true;
}

content::ResourceDispatcherHostLoginDelegate*
AtomResourceDispatcherHostDelegate::CreateLoginDelegate(
    net::AuthChallengeInfo* auth_info,
    net::URLRequest* request) {
  return new LoginHandler(auth_info, request);
}

std::unique_ptr<net::ClientCertStore>
AtomResourceDispatcherHostDelegate::CreateClientCertStore(
    content::ResourceContext* resource_context) {
  #if defined(USE_NSS_CERTS)
    return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreNSS(
        net::ClientCertStoreNSS::PasswordDelegateFactory()));
  #elif defined(OS_WIN)
    return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreWin());
  #elif defined(OS_MACOSX)
    return std::unique_ptr<net::ClientCertStore>(new net::ClientCertStoreMac());
  #elif defined(USE_OPENSSL)
    return std::unique_ptr<net::ClientCertStore>();
  #endif
}

bool AtomResourceDispatcherHostDelegate::ShouldInterceptResourceAsStream(
    net::URLRequest* request,
    const base::FilePath& plugin_path,
    const std::string& mime_type,
    GURL* origin,
    std::string* payload) {
  if (mime_type == "application/pdf") {
    *origin = GURL(PdfViewerUI::kOrigin);
    return true;
  }
  return false;
}

void AtomResourceDispatcherHostDelegate::OnStreamCreated(
    net::URLRequest* request,
    std::unique_ptr<content::StreamInfo> stream) {
  const content::ResourceRequestInfo* info =
      content::ResourceRequestInfo::ForRequest(request);
  content::BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::Bind(&OnPdfStreamCreated, base::Passed(&stream),
                 request->GetExpectedContentSize(),
                 info->GetWebContentsGetterForRequest(), info->GetChildID(),
                 info->GetRenderFrameID()));
}

}  // namespace atom
