// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_
#define SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_

#include <list>
#include <string>

#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "native_mate/dictionary.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "services/network/public/mojom/data_pipe_getter.mojom.h"
#include "shell/browser/api/event_emitter.h"

namespace electron {

namespace api {

class UploadDataPipeGetter;

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

  void Cancel();
  void Close();

  bool Write(v8::Local<v8::Value> data,
             bool is_last,
             v8::Local<v8::Value> extra);
  void FollowRedirect();
  bool SetExtraHeader(const std::string& name, const std::string& value);
  void RemoveExtraHeader(const std::string& name);
  void SetChunkedUpload(bool is_chunked_upload);
  void SetLoadFlags(int flags);
  mate::Dictionary GetUploadProgress();
  int StatusCode() const;
  std::string StatusMessage() const;
  net::HttpResponseHeaders* RawResponseHeaders() const;
  uint32_t ResponseHttpVersionMajor() const;
  uint32_t ResponseHttpVersionMinor() const;

  // SimpleURLLoaderStreamConsumer:
  void OnDataReceived(base::StringPiece string_piece,
                      base::OnceClosure resume) override;
  void OnComplete(bool success) override;
  void OnRetry(base::OnceClosure start_retry) override;

 private:
  friend class UploadDataPipeGetter;

  struct WriteData {
    WriteData(bool is_last,
              std::string data,
              base::OnceCallback<void(v8::Local<v8::Value>)> callback);
    ~WriteData();

    bool is_last = false;
    std::string data;
    base::OnceCallback<void(v8::Local<v8::Value>)> callback;
  };

  void OnResponseStarted(const GURL& final_url,
                         const network::ResourceResponseHead& response_head);
  void OnWrite(MojoResult result);

  // Write the first data of |pending_writes_|.
  void DoWrite();

  // Manage lifetime of wrapper.
  void Pin();
  void Unpin();

  // Emit events.
  template <typename... Args>
  void EmitRequestEvent(Args... args);
  template <typename... Args>
  void EmitResponseEvent(Args... args);

  std::unique_ptr<network::ResourceRequest> request_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  scoped_refptr<net::HttpResponseHeaders> response_headers_;

  // Upload data pipe.
  std::unique_ptr<UploadDataPipeGetter> upload_data_pipe_getter_;
  std::unique_ptr<mojo::StringDataPipeProducer> producer_;

  // Current status.
  int request_state_ = 0;
  int response_state_ = 0;

  // Pending writes that not yet sent to NetworkService.
  std::list<WriteData> pending_writes_;

  // Used by pin/unpin to manage lifetime.
  v8::Global<v8::Object> wrapper_;

  base::WeakPtrFactory<URLRequestNS> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestNS);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ATOM_API_URL_REQUEST_NS_H_
