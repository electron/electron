// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/webui/pdf_viewer_ui.h"

#include <map>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/loader/layered_resource_handler.h"
#include "atom/browser/ui/webui/pdf_viewer_handler.h"
#include "atom/common/atom_constants.h"
#include "base/sequenced_task_runner_helpers.h"
#include "components/pdf/common/pdf_messages.h"
#include "content/browser/loader/resource_dispatcher_host_impl.h"
#include "content/browser/loader/resource_request_info_impl.h"
#include "content/browser/loader/stream_resource_handler.h"
#include "content/browser/resource_context_impl.h"
#include "content/browser/streams/stream.h"
#include "content/browser/streams/stream_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/stream_handle.h"
#include "content/public/browser/stream_info.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "grit/pdf_viewer_resources_map.h"
#include "net/base/load_flags.h"
#include "net/base/mime_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "ui/base/resource/resource_bundle.h"

using content::BrowserThread;

namespace atom {

namespace {

// Extracts the path value from the URL without the leading '/',
// which follows the mapping of names in pdf_viewer_resources_map.
std::string PathWithoutParams(const std::string& path) {
  return GURL(kPdfViewerUIOrigin + path).path().substr(1);
}

class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource() {
    for (size_t i = 0; i < kPdfViewerResourcesSize; ++i) {
      std::string resource_path = kPdfViewerResources[i].name;
      DCHECK(path_to_resource_id_.find(resource_path) ==
             path_to_resource_id_.end());
      path_to_resource_id_[resource_path] = kPdfViewerResources[i].value;
    }
  }

  // content::URLDataSource implementation.
  std::string GetSource() const override { return kPdfViewerUIHost; }

  void StartDataRequest(
      const std::string& path,
      const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
      const GotDataCallback& callback) override {
    std::string filename = PathWithoutParams(path);
    auto entry = path_to_resource_id_.find(filename);

    if (entry != path_to_resource_id_.end()) {
      int resource_id = entry->second;
      const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      callback.Run(rb.LoadDataResourceBytes(resource_id));
    } else {
      LOG(ERROR) << "Unable to find: " << path;
      callback.Run(new base::RefCountedString());
    }
  }

  std::string GetMimeType(const std::string& path) const override {
    std::string filename = PathWithoutParams(path);
    std::string mime_type;
    net::GetMimeTypeFromFile(
        base::FilePath::FromUTF8Unsafe(filename), &mime_type);
    return mime_type;
  }

  bool ShouldAddContentSecurityPolicy() const override { return false; }

  bool ShouldDenyXFrameOptions() const override { return false; }

  bool ShouldServeMimeTypeAsContentTypeHeader() const override { return true; }

 private:
  ~BundledDataSource() override {}

  // A map from a resource path to the resource ID.
  std::map<std::string, int> path_to_resource_id_;

  DISALLOW_COPY_AND_ASSIGN(BundledDataSource);
};

// Helper to convert from OnceCallback to Callback.
template <typename T>
void CallMigrationCallback(T callback,
                           std::unique_ptr<content::StreamInfo> stream_info) {
  std::move(callback).Run(std::move(stream_info));
}

}  // namespace

class PdfViewerUI::ResourceRequester
    : public base::RefCountedThreadSafe<ResourceRequester,
                                        BrowserThread::DeleteOnIOThread>,
      public atom::LayeredResourceHandler::Delegate {
 public:
  explicit ResourceRequester(StreamResponseCallback cb)
      : stream_response_cb_(std::move(cb)) {}

  void StartRequest(const GURL& url,
                    const GURL& origin,
                    int render_process_id,
                    int render_view_id,
                    int render_frame_id,
                    content::ResourceContext* resource_context) {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    const net::URLRequestContext* request_context =
        resource_context->GetRequestContext();
    std::unique_ptr<net::URLRequest> request(
        request_context->CreateRequest(url, net::DEFAULT_PRIORITY, nullptr));
    request->set_method("GET");

    content::ResourceDispatcherHostImpl::Get()->InitializeURLRequest(
        request.get(), content::Referrer(url, blink::WebReferrerPolicyDefault),
        false,  // download.
        render_process_id, render_view_id, render_frame_id, resource_context);

    content::ResourceRequestInfoImpl* info =
        content::ResourceRequestInfoImpl::ForRequest(request.get());
    content::StreamContext* stream_context =
        content::GetStreamContextForResourceContext(resource_context);

    std::unique_ptr<content::ResourceHandler> handler =
        base::MakeUnique<content::StreamResourceHandler>(
            request.get(), stream_context->registry(), origin);
    info->set_is_stream(true);
    stream_info_.reset(new content::StreamInfo);
    stream_info_->handle =
        static_cast<content::StreamResourceHandler*>(handler.get())
            ->stream()
            ->CreateHandle();
    stream_info_->original_url = request->url();

    // Helper to fill stream response details.
    handler.reset(new atom::LayeredResourceHandler(request.get(),
                                                   std::move(handler), this));

    content::ResourceDispatcherHostImpl::Get()->BeginURLRequest(
        std::move(request), std::move(handler),
        false,  // download
        false,  // content_initiated (download specific)
        false,  // do_not_prompt_for_login (download specific)
        resource_context);
  }

 protected:
  // atom::LayeredResourceHandler::Delegate:
  void OnResponseStarted(content::ResourceResponse* response) override {
    DCHECK_CURRENTLY_ON(BrowserThread::IO);

    auto resource_response_head = response->head;
    auto headers = resource_response_head.headers;
    auto mime_type = resource_response_head.mime_type;
    if (headers.get())
      stream_info_->response_headers =
          new net::HttpResponseHeaders(headers->raw_headers());
    stream_info_->mime_type = mime_type;

    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::Bind(&CallMigrationCallback<StreamResponseCallback>,
                   base::Passed(&stream_response_cb_),
                   base::Passed(&stream_info_)));
  }

 private:
  friend struct BrowserThread::DeleteOnThread<BrowserThread::IO>;
  friend class base::DeleteHelper<ResourceRequester>;
  ~ResourceRequester() override {}

  StreamResponseCallback stream_response_cb_;
  std::unique_ptr<content::StreamInfo> stream_info_;

  DISALLOW_COPY_AND_ASSIGN(ResourceRequester);
};

PdfViewerUI::PdfViewerUI(content::BrowserContext* browser_context,
                         content::WebUI* web_ui,
                         const std::string& src)
    : content::WebUIController(web_ui),
      content::WebContentsObserver(web_ui->GetWebContents()),
      src_(src) {
  pdf_handler_ = new PdfViewerHandler(src);
  web_ui->AddMessageHandler(pdf_handler_);
  content::URLDataSource::Add(browser_context, new BundledDataSource);
}

PdfViewerUI::~PdfViewerUI() {}

bool PdfViewerUI::OnMessageReceived(
    const IPC::Message& message,
    content::RenderFrameHost* render_frame_host) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(PdfViewerUI, message)
    IPC_MESSAGE_HANDLER(PDFHostMsg_PDFSaveURLAs, OnSaveURLAs)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void PdfViewerUI::OnPdfStreamCreated(
    std::unique_ptr<content::StreamInfo> stream) {
  stream_ = std::move(stream);
  if (pdf_handler_)
    pdf_handler_->SetPdfResourceStream(stream_.get());
  resource_requester_ = nullptr;
}

void PdfViewerUI::RenderFrameCreated(content::RenderFrameHost* rfh) {
  int render_process_id = rfh->GetProcess()->GetID();
  int render_frame_id = rfh->GetRoutingID();
  int render_view_id = rfh->GetRenderViewHost()->GetRoutingID();
  auto resource_context =
      web_contents()->GetBrowserContext()->GetResourceContext();
  auto callback =
      base::BindOnce(&PdfViewerUI::OnPdfStreamCreated, base::Unretained(this));
  resource_requester_ = new ResourceRequester(std::move(callback));
  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&ResourceRequester::StartRequest, resource_requester_,
                 GURL(src_), GURL(kPdfViewerUIOrigin), render_process_id,
                 render_view_id, render_frame_id, resource_context));
}

void PdfViewerUI::OnSaveURLAs(const GURL& url,
                              const content::Referrer& referrer) {
  web_contents()->SaveFrame(url, referrer);
}

}  // namespace atom
