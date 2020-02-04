// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/plugins/plugin_response_interceptor_url_loader_throttle.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/guid.h"
#include "base/task/post_task.h"

#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_attach_helper.h"
#include "extensions/common/extension.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

#include "extensions/browser/api/mime_handler_private/mime_handler_private.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_stream_manager.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/plugins/plugin_utils.h"

namespace {

void SendExecuteMimeTypeHandlerEvent(
    const std::string& extension_id,
    const std::string& view_id,
    bool embedded,
    int frame_tree_node_id,
    int render_process_id,
    int render_frame_id,
    content::mojom::TransferrableURLLoaderPtr transferrable_loader,
    const GURL& original_url) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::WebContents* web_contents = nullptr;
  if (frame_tree_node_id != -1) {
    web_contents =
        content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  } else {
    web_contents = content::WebContents::FromRenderFrameHost(
        content::RenderFrameHost::FromID(render_process_id, render_frame_id));
  }
  if (!web_contents)
    return;

  /*
  // If the request was for a prerender, abort the prerender and do not
  // continue. This is because plugins cancel prerender, see
  // http://crbug.com/343590.
  prerender::PrerenderContents* prerender_contents =
      prerender::PrerenderContents::FromWebContents(web_contents);
  if (prerender_contents) {
    prerender_contents->Destroy(prerender::FINAL_STATUS_DOWNLOAD);
    return;
  }
  */

  auto* browser_context = web_contents->GetBrowserContext();

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(browser_context)
          ->enabled_extensions()
          .GetByID(extension_id);
  if (!extension)
    return;

  MimeTypesHandler* handler = MimeTypesHandler::GetHandler(extension);
  if (!handler->HasPlugin())
    return;

  // If the mime handler uses MimeHandlerViewGuest, the MimeHandlerViewGuest
  // will take ownership of the stream.
  GURL handler_url(
      extensions::Extension::GetBaseURLFromExtensionId(extension_id).spec() +
      handler->handler_url());
  int tab_id = -1;
  auto* api_contents = electron::api::WebContents::FromWrappedClass(
      v8::Isolate::GetCurrent(), web_contents);
  if (api_contents)
    tab_id = api_contents->ID();
  std::unique_ptr<extensions::StreamContainer> stream_container(
      new extensions::StreamContainer(
          tab_id, embedded, handler_url, extension_id,
          std::move(transferrable_loader), original_url));
  extensions::MimeHandlerStreamManager::Get(browser_context)
      ->AddStream(view_id, std::move(stream_container), frame_tree_node_id,
                  render_process_id, render_frame_id);
}

}  // namespace

PluginResponseInterceptorURLLoaderThrottle::
    PluginResponseInterceptorURLLoaderThrottle(int resource_type,
                                               int frame_tree_node_id)
    : resource_type_(resource_type), frame_tree_node_id_(frame_tree_node_id) {}

PluginResponseInterceptorURLLoaderThrottle::
    ~PluginResponseInterceptorURLLoaderThrottle() = default;

void PluginResponseInterceptorURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    network::mojom::URLResponseHead* response_head,
    bool* defer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (content::download_utils::MustDownload(response_url,
                                            response_head->headers.get(),
                                            response_head->mime_type)) {
    return;
  }

  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id_);
  if (!web_contents)
    return;

  std::string extension_id = PluginUtils::GetExtensionIdForMimeType(
      web_contents->GetBrowserContext(), response_head->mime_type);

  if (extension_id.empty())
    return;

  std::string view_id = base::GenerateGUID();
  // The string passed down to the original client with the response body.
  std::string payload = view_id;

  mojo::PendingRemote<network::mojom::URLLoader> dummy_new_loader;
  ignore_result(dummy_new_loader.InitWithNewPipeAndPassReceiver());
  mojo::Remote<network::mojom::URLLoaderClient> new_client;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> new_client_receiver =
      new_client.BindNewPipeAndPassReceiver();

  uint32_t data_pipe_size = 64U;
  // Provide the MimeHandlerView code a chance to override the payload. This is
  // the case where the resource is handled by frame-based MimeHandlerView.
  *defer = extensions::MimeHandlerViewAttachHelper::
      OverrideBodyForInterceptedResponse(
          frame_tree_node_id_, response_url, response_head->mime_type, view_id,
          &payload, &data_pipe_size,
          base::BindOnce(
              &PluginResponseInterceptorURLLoaderThrottle::ResumeLoad,
              weak_factory_.GetWeakPtr()));

  mojo::DataPipe data_pipe(data_pipe_size);
  uint32_t len = static_cast<uint32_t>(payload.size());
  CHECK_EQ(MOJO_RESULT_OK,
           data_pipe.producer_handle->WriteData(
               payload.c_str(), &len, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  new_client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

  network::URLLoaderCompletionStatus status(net::OK);
  status.decoded_body_length = len;
  new_client->OnComplete(status);

  mojo::PendingRemote<network::mojom::URLLoader> original_loader;
  mojo::PendingReceiver<network::mojom::URLLoaderClient> original_client;
  delegate_->InterceptResponse(std::move(dummy_new_loader),
                               std::move(new_client_receiver), &original_loader,
                               &original_client);

  // Make a deep copy of URLResponseHead before passing it cross-thread.
  auto deep_copied_response = response_head->Clone();
  if (response_head->headers) {
    deep_copied_response->headers =
        base::MakeRefCounted<net::HttpResponseHeaders>(
            response_head->headers->raw_headers());
  }

  auto transferrable_loader = content::mojom::TransferrableURLLoader::New();
  transferrable_loader->url = GURL(
      extensions::Extension::GetBaseURLFromExtensionId(extension_id).spec() +
      base::GenerateGUID());
  transferrable_loader->url_loader = std::move(original_loader);
  transferrable_loader->url_loader_client = std::move(original_client);
  transferrable_loader->head = std::move(deep_copied_response);
  transferrable_loader->head->intercepted_by_plugin = true;

  bool embedded =
      resource_type_ != static_cast<int>(content::ResourceType::kMainFrame);
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&SendExecuteMimeTypeHandlerEvent, extension_id, view_id,
                     embedded, frame_tree_node_id_, -1 /* render_process_id */,
                     -1 /* render_frame_id */, std::move(transferrable_loader),
                     response_url));
}

void PluginResponseInterceptorURLLoaderThrottle::ResumeLoad() {
  delegate_->Resume();
}
