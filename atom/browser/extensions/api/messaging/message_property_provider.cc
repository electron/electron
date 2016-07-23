// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "atom/browser/extensions/api/messaging/message_property_provider.h"

#include <stdint.h>
#include <vector>
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_piece.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "crypto/ec_private_key.h"
#include "extensions/common/api/runtime.h"
#include "net/base/completion_callback.h"
#include "net/cert/asn1_util.h"
#include "net/cert/jwk_serializer.h"
#include "net/ssl/channel_id_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "url/gurl.h"
#include "atom/browser/atom_browser_context.h"

namespace extensions {

MessagePropertyProvider::MessagePropertyProvider() {}

void MessagePropertyProvider::GetChannelID(
    content::BrowserContext* browser_context,
    const GURL& source_url,
    const ChannelIDCallback& reply) {
  if (!source_url.is_valid()) {
    // This isn't a real URL, so there's no sense in looking for a channel ID
    // for it. Dispatch with an empty tls channel ID.
    reply.Run(std::string());
    return;
  }
  scoped_refptr<net::URLRequestContextGetter> request_context_getter(
      static_cast<atom::AtomBrowserContext*>(browser_context)
          ->GetRequestContext());
  content::BrowserThread::PostTask(content::BrowserThread::IO, FROM_HERE,
      base::Bind(&MessagePropertyProvider::GetChannelIDOnIOThread,
                 base::ThreadTaskRunnerHandle::Get(),
                 request_context_getter,
                 source_url.host(),
                 reply));
}

// Helper struct to bind the memory addresses that will be written to by
// ChannelIDService::GetChannelID to the callback provided to
// MessagePropertyProvider::GetChannelID.
struct MessagePropertyProvider::GetChannelIDOutput {
  std::unique_ptr<crypto::ECPrivateKey> channel_id_key;
  net::ChannelIDService::Request request;
};

// static
void MessagePropertyProvider::GetChannelIDOnIOThread(
    scoped_refptr<base::TaskRunner> original_task_runner,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter,
    const std::string& host,
    const ChannelIDCallback& reply) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  net::ChannelIDService* channel_id_service =
      request_context_getter->GetURLRequestContext()->
          channel_id_service();
  GetChannelIDOutput* output = new GetChannelIDOutput();
  net::CompletionCallback net_completion_callback =
      base::Bind(&MessagePropertyProvider::GotChannelID,
                 original_task_runner,
                 base::Owned(output),
                 reply);
  int status = channel_id_service->GetChannelID(
      host, &output->channel_id_key, net_completion_callback, &output->request);
  if (status == net::ERR_IO_PENDING)
    return;
  GotChannelID(original_task_runner, output, reply, status);
}

// static
void MessagePropertyProvider::GotChannelID(
    scoped_refptr<base::TaskRunner> original_task_runner,
    struct GetChannelIDOutput* output,
    const ChannelIDCallback& reply,
    int status) {
  base::Closure no_tls_channel_id_closure = base::Bind(reply, "");
  if (status != net::OK) {
    original_task_runner->PostTask(FROM_HERE, no_tls_channel_id_closure);
    return;
  }
  std::vector<uint8_t> spki_vector;
  if (!output->channel_id_key->ExportPublicKey(&spki_vector)) {
    original_task_runner->PostTask(FROM_HERE, no_tls_channel_id_closure);
    return;
  }
  base::StringPiece spki(reinterpret_cast<char*>(spki_vector.data()),
                         spki_vector.size());
  base::DictionaryValue jwk_value;
  if (!net::JwkSerializer::ConvertSpkiFromDerToJwk(spki, &jwk_value)) {
    original_task_runner->PostTask(FROM_HERE, no_tls_channel_id_closure);
    return;
  }
  std::string jwk_str;
  base::JSONWriter::Write(jwk_value, &jwk_str);
  original_task_runner->PostTask(FROM_HERE, base::Bind(reply, jwk_str));
}

}  // namespace extensions
