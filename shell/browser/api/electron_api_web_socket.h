// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_SOCKET_H_
#define ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_SOCKET_H_

#include <deque>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/sequence_checker.h"
#include "gin/weak_cell.h"
#include "gin/wrappable.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/simple_watcher.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "shell/browser/event_emitter_mixin.h"
#include "shell/common/gc_plugin.h"
#include "shell/common/gin_helper/self_keep_alive.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "v8/include/v8-forward.h"

namespace gin {
class Arguments;
}  // namespace gin

namespace electron {
class ElectronBrowserContext;
}

namespace electron::api {

// A browser-process WebSocket client that goes through Chromium's network
// service. Implements the network.mojom.WebSocketHandshakeClient and
// WebSocketClient interfaces and translates them into JS events that the
// `net.WebSocket` TypeScript class consumes.
class WebSocketWrapper final
    : public gin::Wrappable<WebSocketWrapper>,
      public gin_helper::EventEmitterMixin<WebSocketWrapper>,
      private network::mojom::WebSocketHandshakeClient,
      private network::mojom::WebSocketClient {
 public:
  static WebSocketWrapper* Create(gin::Arguments* args);

  // gin::Wrappable
  static const gin::WrapperInfo kWrapperInfo;
  const gin::WrapperInfo* wrapper_info() const override;
  const char* GetHumanReadableName() const override;
  static const char* GetClassName() { return "WebSocketWrapper"; }
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  void Trace(cppgc::Visitor* visitor) const override;

  // Public for cppgc::MakeGarbageCollected.
  WebSocketWrapper(ElectronBrowserContext* browser_context,
                   const GURL& url,
                   std::vector<std::string> protocols,
                   std::vector<network::mojom::HttpHeaderPtr> headers,
                   const url::Origin& origin,
                   bool use_session_cookies);
  ~WebSocketWrapper() override;

  WebSocketWrapper(const WebSocketWrapper&) = delete;
  WebSocketWrapper& operator=(const WebSocketWrapper&) = delete;

 private:
  enum class State {
    kConnecting,
    kOpen,
    kClosing,
    kClosed,
  };

  struct PendingMessage {
    PendingMessage(network::mojom::WebSocketMessageType type,
                   std::vector<uint8_t> data);
    PendingMessage(PendingMessage&&);
    PendingMessage& operator=(PendingMessage&&);
    ~PendingMessage();

    network::mojom::WebSocketMessageType type;
    std::vector<uint8_t> data;
    size_t offset = 0;
    bool announced = false;
  };

  // JS-callable methods.
  void Send(gin::Arguments* args);
  void Close(gin::Arguments* args);
  uint64_t GetBufferedAmount() const { return buffered_amount_; }

  // network::mojom::WebSocketHandshakeClient:
  void OnOpeningHandshakeStarted(
      network::mojom::WebSocketHandshakeRequestPtr request) override;
  void OnFailure(const std::string& message,
                 int32_t net_error,
                 int32_t response_code) override;
  void OnConnectionEstablished(
      mojo::PendingRemote<network::mojom::WebSocket> websocket,
      mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
      network::mojom::WebSocketHandshakeResponsePtr response,
      mojo::ScopedDataPipeConsumerHandle readable,
      mojo::ScopedDataPipeProducerHandle writable) override;

  // network::mojom::WebSocketClient:
  void OnDataFrame(bool fin,
                   network::mojom::WebSocketMessageType type,
                   uint64_t data_len) override;
  void OnDropChannel(bool was_clean,
                     uint16_t code,
                     const std::string& reason) override;
  void OnClosingHandshake() override;

  // Connection lifecycle.
  void Start();
  // Vends a persistent handle to a gin::WeakCell so that callbacks bound to
  // `this` are no-ops once the object has been collected.
  cppgc::Persistent<gin::WeakCell<WebSocketWrapper>> WeakRef();
  void OnMojoDisconnect();
  void Fail(const std::string& message, int net_error);
  void EmitCloseAndTeardown(bool was_clean,
                            uint32_t code,
                            const std::string& reason);
  void Teardown();

  // Read side.
  void OnReadable(MojoResult result, const mojo::HandleSignalsState& state);
  void ProcessCompletedMessage();

  // Write side.
  void OnWritable(MojoResult result, const mojo::HandleSignalsState& state);
  void DrainWriteQueue();
  void MaybeStartClosingHandshake();

  SEQUENCE_CHECKER(sequence_checker_);

  State state_ = State::kConnecting;
  raw_ptr<ElectronBrowserContext> browser_context_;
  GURL url_;
  std::vector<std::string> requested_protocols_;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers_;
  url::Origin origin_;
  bool use_session_cookies_ = false;

  // Mojo plumbing.
  GC_PLUGIN_IGNORE(
      "Context tracking of receiver is not needed in the browser process.")
  mojo::Receiver<network::mojom::WebSocketHandshakeClient> handshake_receiver_{
      this};
  GC_PLUGIN_IGNORE(
      "Context tracking of receiver is not needed in the browser process.")
  mojo::Receiver<network::mojom::WebSocketClient> client_receiver_{this};
  GC_PLUGIN_IGNORE(
      "Context tracking of remote is not needed in the browser process.")
  mojo::Remote<network::mojom::WebSocket> websocket_;
  mojo::ScopedDataPipeConsumerHandle readable_;
  mojo::SimpleWatcher readable_watcher_;
  mojo::ScopedDataPipeProducerHandle writable_;
  mojo::SimpleWatcher writable_watcher_;

  // Read state. A WebSocket message may span multiple frames; we accumulate
  // them in |pending_read_data_| until we see a frame with fin=true.
  std::vector<uint8_t> pending_read_data_;
  size_t pending_read_offset_ = 0;
  bool pending_read_fin_ = false;
  network::mojom::WebSocketMessageType pending_read_type_ =
      network::mojom::WebSocketMessageType::CONTINUATION;

  // Write state.
  std::deque<PendingMessage> pending_writes_;
  uint64_t buffered_amount_ = 0;
  bool close_requested_ = false;
  uint16_t close_code_ = 0;
  std::string close_reason_;

  // Roots this object in the cppgc heap while the connection is live so it is
  // not collected if the JS wrapper goes out of scope mid-connection. Cleared
  // in Teardown().
  gin_helper::SelfKeepAlive<WebSocketWrapper> keep_alive_{this};
  gin::WeakCellFactory<WebSocketWrapper> weak_factory_{this};
};

}  // namespace electron::api

#endif  // ELECTRON_SHELL_BROWSER_API_ELECTRON_API_WEB_SOCKET_H_
