// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/plugin_response_interceptor_url_loader_throttle.h"

#include "atom/browser/atom_resource_dispatcher_host_delegate.h"
#include "atom/common/atom_constants.h"
#include "base/feature_list.h"
#include "base/guid.h"
#include "base/task/post_task.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/download_utils.h"
#include "content/public/browser/stream_info.h"
#include "content/public/common/resource_type.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_attach_helper.h"
#include "extensions/common/extension.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "services/network/public/cpp/features.h"

PluginResponseInterceptorURLLoaderThrottle::
    PluginResponseInterceptorURLLoaderThrottle(int resource_type,
                                               int frame_tree_node_id)
    : resource_type_(resource_type), frame_tree_node_id_(frame_tree_node_id) {}

PluginResponseInterceptorURLLoaderThrottle::
    ~PluginResponseInterceptorURLLoaderThrottle() = default;

void PluginResponseInterceptorURLLoaderThrottle::WillProcessResponse(
    const GURL& response_url,
    network::ResourceResponseHead* response_head,
    bool* defer) {
  if (content::download_utils::MustDownload(response_url,
                                            response_head->headers.get(),
                                            response_head->mime_type)) {
    return;
  }

  // Don't intercept if the request went through the legacy resource loading
  // path, i.e., ResourceDispatcherHost, since that path doesn't need response
  // interception. ResourceDispatcherHost is only used if network service is
  // disabled (in which case this throttle was created because
  // ServiceWorkerServicification was enabled) and a service worker didn't
  // handle the request.
  if (!base::FeatureList::IsEnabled(network::features::kNetworkService) &&
      !response_head->was_fetched_via_service_worker) {
    return;
  }

  if (response_head->mime_type != "application/pdf")
    return;

  std::string view_id = base::GenerateGUID();
  // The string passed down to the original client with the response body.
  std::string payload = view_id;

  network::mojom::URLLoaderPtr dummy_new_loader;
  mojo::MakeRequest(&dummy_new_loader);
  network::mojom::URLLoaderClientPtr new_client;
  network::mojom::URLLoaderClientRequest new_client_request =
      mojo::MakeRequest(&new_client);

  uint32_t data_pipe_size = 64U;
  mojo::DataPipe data_pipe(data_pipe_size);
  uint32_t len = static_cast<uint32_t>(payload.size());
  CHECK_EQ(MOJO_RESULT_OK,
           data_pipe.producer_handle->WriteData(
               payload.c_str(), &len, MOJO_WRITE_DATA_FLAG_ALL_OR_NONE));

  new_client->OnStartLoadingResponseBody(std::move(data_pipe.consumer_handle));

  network::URLLoaderCompletionStatus status(net::OK);
  status.decoded_body_length = len;
  new_client->OnComplete(status);

  network::mojom::URLLoaderPtr original_loader;
  network::mojom::URLLoaderClientRequest original_client;
  delegate_->InterceptResponse(std::move(dummy_new_loader),
                               std::move(new_client_request), &original_loader,
                               &original_client);

  // Make a deep copy of ResourceResponseHead before passing it cross-thread.
  auto resource_response = base::MakeRefCounted<network::ResourceResponse>();
  resource_response->head = *response_head;
  auto deep_copied_response = resource_response->DeepCopy();

  auto transferrable_loader = content::mojom::TransferrableURLLoader::New();
  transferrable_loader->url = GURL(
      extensions::Extension::GetBaseURLFromExtensionId(atom::kPdfExtensionId)
          .spec() +
      base::GenerateGUID());
  transferrable_loader->url_loader = original_loader.PassInterface();
  transferrable_loader->url_loader_client = std::move(original_client);
  transferrable_loader->head = std::move(deep_copied_response->head);
  transferrable_loader->head.intercepted_by_plugin = true;

  bool embedded =
      resource_type_ != static_cast<int>(content::ResourceType::kMainFrame);
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(
          &atom::AtomResourceDispatcherHostDelegate::OnPdfResourceIntercepted,
          atom::kPdfExtensionId, view_id, embedded, frame_tree_node_id_,
          -1 /* render_process_id */, -1 /* render_frame_id */,
          nullptr /* stream */, std::move(transferrable_loader), response_url));
}
