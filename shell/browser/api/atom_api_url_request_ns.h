// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_

#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "shell/browser/api/event_emitter.h"

namespace electron {

namespace api {

class URLRequestNS : public mate::EventEmitter<URLRequestNS>,
                     public network::SimpleURLLoaderStreamConsumer {
 public:
  static mate::WrappableBase* New(mate::Arguments* args);

  static void BuildPrototype(v8::Isolate* isolate,
                             v8::Local<v8::FunctionTemplate> prototype);

 protected:
  explicit URLRequestNS(mate::Arguments* args);
  ~URLRequestNS() override;

  bool NotStarted() const;
  bool Finished() const;
  bool Canceled() const;
  bool Failed() const;

  void Cancel();
  void Close();

  bool Write(const std::string& data, bool is_last);
  void FollowRedirect();
  bool SetExtraHeader(const std::string& name, const std::string& value);
  void RemoveExtraHeader(const std::string& name);
  void SetChunkedUpload(bool is_chunked_upload);
  void SetLoadFlags(int flags);

  // SimpleURLLoaderStreamConsumer:
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

 private:
  void Pin();
  void Unpin();

  std::unique_ptr<mojo::StringDataPipeProducer> producer_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  // Current status.
  int request_state_ = 0;
  int response_state_ = 0;

  // Weak ref to request, which is managed by loader_.
  network::ResourceRequest* request_;

  // Used by pin/unpin to manage lifetime.
  v8::Global<v8::Object> wrapper_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestNS);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_
