// Copyright (c) 2019 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_URL_LOADER_H_
#define SHELL_BROWSER_API_ATOM_API_URL_LOADER_H_

#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "shell/common/gin_helper/event_emitter.h"
#include "url/gurl.h"
#include "v8/include/v8.h"

namespace gin {
class Arguments;
}

namespace network {
class SimpleURLLoader;
}

namespace electron {

namespace api {

class SimpleURLLoaderWrapper
    : public gin_helper::EventEmitter<SimpleURLLoaderWrapper>,
      public network::SimpleURLLoaderStreamConsumer {
 public:
  ~SimpleURLLoaderWrapper() override;
  static mate::WrappableBase* New(gin::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

  void Cancel();

 private:
  explicit SimpleURLLoaderWrapper(
      std::unique_ptr<network::SimpleURLLoader> loader,
      network::mojom::URLLoaderFactory* url_loader_factory);

  // SimpleURLLoaderStreamConsumer:
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

  // SimpleURLLoader callbacks
  void OnResponseStarted(const GURL& final_url,
                         const network::mojom::URLResponseHead& response_head);

  std::unique_ptr<network::SimpleURLLoader> loader_;
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_URL_LOADER_H_

/*
 * js usage:
 *
 * new SimpleURLLoader({
 *   method: 'GET',
 *   url: 'https://...'
 * })
 *
 * new SimpleURLLoader({
 *   method: 'PUT',
 *   url: 'https://...',
 *   body: Buffer.from(...),
 * })
 *
 * new SimpleURLLoader({
 *   method: 'PUT',
 *   url: 'https://...',
 *   bodyFromFile: 'path/to/file',
 * })
 *
 * // Transfer-Encoding: chunked
 * new SimpleURLLoader({
 *   method: 'PUT',
 *   url: 'https://...',
 *   body: async function(write, done) {
 *     await write(Buffer.alloc(1024))
 *     await thing()
 *     await write(Buffer.alloc(1024*3))
 *     done()
 *   }
 * })
 *
 * // Streaming but not chunked
 * new SimpleURLLoader({
 *   method: 'PUT',
 *   url: 'https://...',
 *   body: {
 *     stream: someStream(),
 *     size: size_in_bytes
 *   }
 * })
 *
 *
 * // events
 * const loader = new SimpleURLLoader(params)
 * loader.on('response', ...)
 * loader.on('redirect', ...)
 * loader.on('upload-progress', ...)
 * loader.on('download-progress', ...)
 * loader.on('data', ...)
 * loader.on('complete', ...)
 * loader.on('retry', ...)
 * loader.on('error', ...)
 *
 */
