// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/ui/webui/pdf_viewer_ui.h"

#include <map>

#include "atom/browser/atom_browser_context.h"
#include "atom/browser/stream_manager.h"
#include "atom/browser/ui/webui/pdf_viewer_handler.h"
#include "components/pdf/common/pdf_messages.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/bindings_policy.h"
#include "grit/pdf_viewer_resources_map.h"
#include "net/base/mime_util.h"
#include "ui/base/resource/resource_bundle.h"

namespace atom {

namespace {

std::string PathWithoutParams(const std::string& path) {
  return GURL(PdfViewerUI::kOrigin + path).path().substr(1);
}

class BundledDataSource : public content::URLDataSource {
 public:
  BundledDataSource() {
    for (size_t i = 0; i < kPdfViewerResourcesSize; ++i) {
      base::FilePath resource_path =
          base::FilePath().AppendASCII(kPdfViewerResources[i].name);
      resource_path = resource_path.NormalizePathSeparators();

      DCHECK(path_to_resource_id_.find(resource_path) ==
             path_to_resource_id_.end());
      path_to_resource_id_[resource_path] = kPdfViewerResources[i].value;
    }
  }

  // content::URLDataSource implementation.
  std::string GetSource() const override { return PdfViewerUI::kHost; }

  void StartDataRequest(const std::string& path,
                        int render_process_id,
                        int render_frame_id,
                        const GotDataCallback& callback) override {
    std::string filename = PathWithoutParams(path);
    std::map<base::FilePath, int>::const_iterator entry =
        path_to_resource_id_.find(base::FilePath(filename));
    if (entry != path_to_resource_id_.end()) {
      int resource_id = entry->second;
      const ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
      callback.Run(rb.LoadDataResourceBytes(resource_id));
    }
  }

  std::string GetMimeType(const std::string& path) const override {
    std::string filename = PathWithoutParams(path);
    std::string mime_type;
    net::GetMimeTypeFromFile(base::FilePath(filename), &mime_type);
    return mime_type;
  }

  bool ShouldAddContentSecurityPolicy() const override { return false; }

  bool ShouldDenyXFrameOptions() const override { return false; }

  bool ShouldServeMimeTypeAsContentTypeHeader() const override { return true; }

 private:
  ~BundledDataSource() override {}

  // A map from a resource path to the resource ID.
  std::map<base::FilePath, int> path_to_resource_id_;

  DISALLOW_COPY_AND_ASSIGN(BundledDataSource);
};

}  // namespace

const char PdfViewerUI::kOrigin[] = "chrome://pdf-viewer/";
const char PdfViewerUI::kHost[] = "pdf-viewer";
const char PdfViewerUI::kId[] = "viewId";
const char PdfViewerUI::kSrc[] = "src";

PdfViewerUI::PdfViewerUI(content::BrowserContext* browser_context,
                         content::WebUI* web_ui,
                         const std::string& view_id,
                         const std::string& src)
    : src_(src),
      content::WebUIController(web_ui),
      content::WebContentsObserver(web_ui->GetWebContents()) {
  auto context = static_cast<AtomBrowserContext*>(browser_context);
  auto stream_manager = context->stream_manager();
  stream_ = stream_manager->ReleaseStream(view_id);
  web_ui->AddMessageHandler(new PdfViewerHandler(stream_.get(), src));
  content::URLDataSource::Add(browser_context, new BundledDataSource);
}

PdfViewerUI::~PdfViewerUI() {}

void PdfViewerUI::RenderViewCreated(content::RenderViewHost* rvh) {
  rvh->AllowBindings(content::BINDINGS_POLICY_WEB_UI);
}

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

void PdfViewerUI::OnSaveURLAs(const GURL& url,
                              const content::Referrer& referrer) {
  web_contents()->SaveFrame(url, referrer);
}

}  // namespace atom
