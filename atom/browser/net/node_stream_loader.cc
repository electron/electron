// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "atom/browser/net/node_stream_loader.h"

#include <utility>

#include "atom/common/api/event_emitter_caller.h"
#include "atom/common/native_mate_converters/callback.h"

#include "atom/common/node_includes.h"

namespace atom {

NodeStreamLoader::NodeStreamLoader(network::ResourceResponseHead head,
                                   network::mojom::URLLoaderRequest loader,
                                   network::mojom::URLLoaderClientPtr client,
                                   v8::Isolate* isolate,
                                   v8::Local<v8::Object> emitter)
    : binding_(this, std::move(loader)),
      client_(std::move(client)),
      isolate_(isolate),
      emitter_(isolate, emitter),
      weak_factory_(this) {
  // PostTask since it might destruct.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&NodeStreamLoader::Start,
                                weak_factory_.GetWeakPtr(), std::move(head)));
}

NodeStreamLoader::~NodeStreamLoader() {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  // Unsubscribe all handlers.
  for (const auto& it : handlers_) {
    v8::Local<v8::Value> args[] = {mate::StringToV8(isolate_, it.first),
                                   it.second.Get(isolate_)};
    node::MakeCallback(isolate_, emitter_.Get(isolate_), "removeListener",
                       node::arraysize(args), args, {0, 0});
  }
}

void NodeStreamLoader::Start(network::ResourceResponseHead head) {
  mojo::ScopedDataPipeConsumerHandle consumer;
  MojoResult rv = mojo::CreateDataPipe(nullptr, &producer_, &consumer);
  if (rv != MOJO_RESULT_OK) {
    OnError(nullptr);
    return;
  }

  client_->OnReceiveResponse(head);
  client_->OnStartLoadingResponseBody(std::move(consumer));

  auto weak = weak_factory_.GetWeakPtr();
  On("end", base::BindRepeating(&NodeStreamLoader::OnEnd, weak));
  On("error", base::BindRepeating(&NodeStreamLoader::OnError, weak));
  // Since every node::MakeCallback call has a micro scope itself, we have to
  // subscribe |data| at last otherwise |end|'s listener won't be called when
  // it is emitted in the same tick.
  On("data", base::BindRepeating(&NodeStreamLoader::OnData, weak));
}

void NodeStreamLoader::On(const char* event, EventCallback callback) {
  v8::Locker locker(isolate_);
  v8::Isolate::Scope isolate_scope(isolate_);
  v8::HandleScope handle_scope(isolate_);

  // emitter.on(event, callback)
  v8::Local<v8::Value> args[] = {
      mate::StringToV8(isolate_, event),
      mate::CallbackToV8(isolate_, std::move(callback)),
  };
  node::MakeCallback(isolate_, emitter_.Get(isolate_), "on",
                     node::arraysize(args), args, {0, 0});

  handlers_[event].Reset(isolate_, args[1]);
}

void NodeStreamLoader::OnData(mate::Arguments* args) {
  v8::Local<v8::Value> buffer;
  args->GetNext(&buffer);
  if (!node::Buffer::HasInstance(buffer)) {
    args->ThrowError("data must be Buffer");
    return;
  }

  size_t ssize = node::Buffer::Length(buffer);
  uint32_t size = base::saturated_cast<uint32_t>(ssize);
  MojoResult result = producer_->WriteData(node::Buffer::Data(buffer), &size,
                                           MOJO_WRITE_DATA_FLAG_NONE);
  if (result != MOJO_RESULT_OK || size < ssize) {
    OnError(nullptr);
    return;
  }
}

void NodeStreamLoader::OnEnd(mate::Arguments* args) {
  client_->OnComplete(network::URLLoaderCompletionStatus(net::OK));
  delete this;
}

void NodeStreamLoader::OnError(mate::Arguments* args) {
  client_->OnComplete(network::URLLoaderCompletionStatus(net::ERR_FAILED));
  delete this;
}

}  // namespace atom
