// Copyright (c) 2026 Salesforce, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/api/electron_api_web_socket.h"

#include <limits>
#include <utility>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/task/sequenced_task_runner.h"
#include "content/public/browser/storage_partition.h"
#include "gin/object_template_builder.h"
#include "gin/persistent.h"
#include "net/base/isolation_info.h"
#include "net/base/net_errors.h"
#include "net/http/http_util.h"
#include "net/storage_access_api/status.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/originating_process_id.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_converters/gurl_converter.h"
#include "shell/common/gin_converters/net_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/object_template_builder.h"
#include "shell/common/gin_helper/wrappable_pointer_tags.h"
#include "shell/common/process_util.h"
#include "shell/common/v8_util.h"
#include "url/url_constants.h"
#include "v8/include/cppgc/allocation.h"
#include "v8/include/v8-cppgc.h"
#include "v8/include/v8.h"

namespace electron::api {

namespace {

constexpr net::NetworkTrafficAnnotationTag kWebSocketTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("electron_net_web_socket", R"(
        semantics {
          sender: "Electron Net module"
          description:
            "Establish a WebSocket connection from the main process using "
            "Chromium's native networking library."
          trigger: "Using net.WebSocket"
          data: "Anything the user wants to send."
          destination: OTHER
        }
        policy {
          cookies_allowed: YES
          cookies_store: "user"
          setting: "This feature cannot be disabled."
        })");

}  // namespace

const gin::WrapperInfo WebSocketWrapper::kWrapperInfo =
    electron::MakeWrapperInfo(electron::kElectronWebSocket);

WebSocketWrapper::PendingMessage::PendingMessage(
    network::mojom::WebSocketMessageType type,
    std::vector<uint8_t> data)
    : type(type), data(std::move(data)) {}
WebSocketWrapper::PendingMessage::PendingMessage(PendingMessage&&) = default;
WebSocketWrapper::PendingMessage& WebSocketWrapper::PendingMessage::operator=(
    PendingMessage&&) = default;
WebSocketWrapper::PendingMessage::~PendingMessage() = default;

WebSocketWrapper::WebSocketWrapper(
    ElectronBrowserContext* browser_context,
    const GURL& url,
    std::vector<std::string> protocols,
    std::vector<network::mojom::HttpHeaderPtr> headers,
    const url::Origin& origin,
    bool use_session_cookies)
    : browser_context_(browser_context),
      url_(url),
      requested_protocols_(std::move(protocols)),
      additional_headers_(std::move(headers)),
      origin_(origin),
      use_session_cookies_(use_session_cookies),
      readable_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL),
      writable_watcher_(FROM_HERE, mojo::SimpleWatcher::ArmingPolicy::MANUAL) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

WebSocketWrapper::~WebSocketWrapper() = default;

// static
WebSocketWrapper* WebSocketWrapper::Create(gin::Arguments* args) {
  if (!electron::IsBrowserProcess()) {
    args->ThrowTypeError("net.WebSocket is only available in the main process");
    return nullptr;
  }

  gin_helper::Dictionary opts;
  if (!args->GetNext(&opts)) {
    args->ThrowTypeError("Expected an options object");
    return nullptr;
  }

  GURL url;
  if (!opts.Get("url", &url) || !url.is_valid() ||
      (!url.SchemeIs("ws") && !url.SchemeIs("wss"))) {
    args->ThrowTypeError("Expected a valid 'ws:' or 'wss:' URL");
    return nullptr;
  }

  std::vector<std::string> protocols;
  opts.Get("protocols", &protocols);

  std::vector<network::mojom::HttpHeaderPtr> headers;
  if (std::vector<std::pair<std::string, std::string>> extra_headers;
      opts.Get("headers", &extra_headers)) {
    for (const auto& [name, value] : extra_headers) {
      if (!net::HttpUtil::IsValidHeaderName(name) ||
          !net::HttpUtil::IsValidHeaderValue(value)) {
        args->ThrowTypeError("Invalid header name or value");
        return nullptr;
      }
      headers.push_back(network::mojom::HttpHeader::New(name, value));
    }
  }

  // The origin passed to CreateWebSocket() is used as the request initiator
  // (which affects SameSite cookie matching) and the `Origin` header. By
  // default, use the WebSocket URL's HTTP-equivalent origin so the connection
  // looks first-party — like a page on the same host opened the socket — and
  // SameSite cookies can be sent when `useSessionCookies` is enabled.
  url::Origin origin;
  if (std::string origin_str; opts.Get("origin", &origin_str)) {
    origin = url::Origin::Create(GURL(origin_str));
  } else {
    GURL::Replacements replacements;
    replacements.SetSchemeStr(url.SchemeIs(url::kWssScheme) ? url::kHttpsScheme
                                                            : url::kHttpScheme);
    origin = url::Origin::Create(url.ReplaceComponents(replacements));
  }

  const bool use_session_cookies =
      opts.ValueOrDefault("useSessionCookies", false);

  std::string partition;
  Session* session = nullptr;
  if (!opts.Get("session", &session)) {
    if (opts.Get("partition", &partition))
      session = Session::FromPartition(args->isolate(), partition);
    else
      session = Session::FromPartition(args->isolate(), "");
  }
  if (!session) {
    args->ThrowTypeError("Failed to resolve session");
    return nullptr;
  }

  v8::Isolate* isolate = args->isolate();
  auto* wrapper = cppgc::MakeGarbageCollected<WebSocketWrapper>(
      isolate->GetCppHeap()->GetAllocationHandle(), session->browser_context(),
      url, std::move(protocols), std::move(headers), origin,
      use_session_cookies);
  wrapper->Start();
  return wrapper;
}

cppgc::Persistent<gin::WeakCell<WebSocketWrapper>> WebSocketWrapper::WeakRef() {
  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  return gin::WrapPersistent(
      weak_factory_.GetWeakCell(isolate->GetCppHeap()->GetAllocationHandle()));
}

void WebSocketWrapper::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK(browser_context_);

  auto handshake_remote = handshake_receiver_.BindNewPipeAndPassRemote();
  handshake_receiver_.set_disconnect_handler(
      base::BindOnce(&WebSocketWrapper::OnMojoDisconnect, WeakRef()));

  auto* network_context =
      browser_context_->GetDefaultStoragePartition()->GetNetworkContext();
  // Note: no auth_handler is supplied. With a null auth handler the network
  // service fails an HTTP/proxy auth challenge immediately (no credentials),
  // which surfaces here as OnFailure(). Cached credentials in the network
  // process (e.g. from a prior net.request login) are still used.
  network_context->CreateWebSocket(
      url_, requested_protocols_, net::StorageAccessApiStatus::kNone,
      net::IsolationInfo::CreateForInternalRequest(origin_),
      std::move(additional_headers_), network::OriginatingProcessId::browser(),
      origin_, network::mojom::ClientSecurityState::New(),
      use_session_cookies_ ? network::mojom::kWebSocketOptionNone
                           : network::mojom::kWebSocketOptionBlockAllCookies,
      net::MutableNetworkTrafficAnnotationTag(kWebSocketTrafficAnnotation),
      std::move(handshake_remote),
      /*url_loader_network_observer=*/mojo::NullRemote(),
      /*auth_handler=*/mojo::NullRemote(),
      /*header_client=*/mojo::NullRemote(),
      /*throttling_profile_id=*/std::nullopt,
      /*network_restrictions_id=*/std::nullopt);
}

void WebSocketWrapper::OnMojoDisconnect() {
  if (state_ == State::kClosed)
    return;
  Fail("Connection to the network service was lost", net::ERR_FAILED);
}

void WebSocketWrapper::Fail(const std::string& message, int net_error) {
  if (state_ == State::kClosed)
    return;
  state_ = State::kClosed;
  Emit("error", message, net::ErrorToString(net_error));
  Emit("close", /*was_clean=*/false, /*code=*/static_cast<uint32_t>(1006),
       /*reason=*/std::string());
  Teardown();
}

void WebSocketWrapper::EmitCloseAndTeardown(bool was_clean,
                                            uint32_t code,
                                            const std::string& reason) {
  Emit("close", was_clean, code, reason);
  Teardown();
}

void WebSocketWrapper::Teardown() {
  state_ = State::kClosed;
  readable_watcher_.Cancel();
  writable_watcher_.Cancel();
  handshake_receiver_.reset();
  client_receiver_.reset();
  websocket_.reset();
  readable_.reset();
  writable_.reset();
  pending_writes_.clear();
  pending_read_data_.clear();
  // Allow this object to be collected once nothing in JS references it.
  keep_alive_.Clear();
}

// ---------------------------------------------------------------------------
// network::mojom::WebSocketHandshakeClient

void WebSocketWrapper::OnOpeningHandshakeStarted(
    network::mojom::WebSocketHandshakeRequestPtr request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void WebSocketWrapper::OnFailure(const std::string& message,
                                 int32_t net_error,
                                 int32_t response_code) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // After OnFailure the network service drops the handshake mojo pipe. Avoid
  // double-reporting via the disconnect handler.
  handshake_receiver_.set_disconnect_handler(base::DoNothing());
  Fail(message, net_error);
}

void WebSocketWrapper::OnConnectionEstablished(
    mojo::PendingRemote<network::mojom::WebSocket> websocket,
    mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
    network::mojom::WebSocketHandshakeResponsePtr response,
    mojo::ScopedDataPipeConsumerHandle readable,
    mojo::ScopedDataPipeProducerHandle writable) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ != State::kConnecting)
    return;

  websocket_.Bind(std::move(websocket));
  client_receiver_.Bind(std::move(client_receiver));

  readable_ = std::move(readable);
  CHECK_EQ(readable_watcher_.Watch(
               readable_.get(), MOJO_HANDLE_SIGNAL_READABLE,
               MOJO_TRIGGER_CONDITION_SIGNALS_SATISFIED,
               base::BindRepeating(&WebSocketWrapper::OnReadable, WeakRef())),
           MOJO_RESULT_OK);

  writable_ = std::move(writable);
  CHECK_EQ(writable_watcher_.Watch(
               writable_.get(), MOJO_HANDLE_SIGNAL_WRITABLE,
               MOJO_TRIGGER_CONDITION_SIGNALS_SATISFIED,
               base::BindRepeating(&WebSocketWrapper::OnWritable, WeakRef())),
           MOJO_RESULT_OK);

  // |handshake_receiver_| will disconnect soon. From now on we watch
  // |client_receiver_| and |websocket_| for network-service crashes.
  handshake_receiver_.set_disconnect_handler(base::DoNothing());
  client_receiver_.set_disconnect_handler(
      base::BindOnce(&WebSocketWrapper::OnMojoDisconnect, WeakRef()));
  websocket_.set_disconnect_handler(
      base::BindOnce(&WebSocketWrapper::OnMojoDisconnect, WeakRef()));

  websocket_->StartReceiving();
  state_ = State::kOpen;

  Emit("open", response->selected_protocol, response->extensions);
}

// ---------------------------------------------------------------------------
// network::mojom::WebSocketClient

void WebSocketWrapper::OnDataFrame(bool fin,
                                   network::mojom::WebSocketMessageType type,
                                   uint64_t data_len) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CHECK_EQ(pending_read_offset_, pending_read_data_.size());
  CHECK(!pending_read_fin_);

  if (type != network::mojom::WebSocketMessageType::CONTINUATION) {
    // First frame of a new message.
    pending_read_type_ = type;
  }

  if (data_len == 0) {
    if (fin)
      ProcessCompletedMessage();
    return;
  }

  const size_t old_size = pending_read_data_.size();
  const uint64_t new_size = old_size + data_len;
  if (data_len > std::numeric_limits<size_t>::max() - old_size ||
      new_size > std::numeric_limits<size_t>::max()) {
    Fail("WebSocket message too large", net::ERR_MSG_TOO_BIG);
    return;
  }

  pending_read_data_.resize(static_cast<size_t>(new_size));
  pending_read_fin_ = fin;
  // Pause so we don't get the next OnDataFrame until this frame's bytes have
  // been fully drained from the pipe.
  client_receiver_.Pause();
  OnReadable(MOJO_RESULT_OK, mojo::HandleSignalsState());
}

void WebSocketWrapper::OnReadable(MojoResult, const mojo::HandleSignalsState&) {
  if (state_ == State::kClosed)
    return;
  CHECK_LT(pending_read_offset_, pending_read_data_.size());

  size_t actually_read = 0;
  const MojoResult result = readable_->ReadData(
      MOJO_READ_DATA_FLAG_NONE,
      base::span(pending_read_data_).subspan(pending_read_offset_),
      actually_read);
  if (result == MOJO_RESULT_OK) {
    pending_read_offset_ += actually_read;
    DCHECK_LE(pending_read_offset_, pending_read_data_.size());
    if (pending_read_offset_ < pending_read_data_.size()) {
      readable_watcher_.ArmOrNotify();
    } else {
      client_receiver_.Resume();
      if (pending_read_fin_)
        ProcessCompletedMessage();
    }
  } else if (result == MOJO_RESULT_SHOULD_WAIT) {
    readable_watcher_.ArmOrNotify();
  } else {
    Fail("WebSocket read failed", net::ERR_FAILED);
  }
}

void WebSocketWrapper::ProcessCompletedMessage() {
  std::vector<uint8_t> data;
  data.swap(pending_read_data_);
  pending_read_offset_ = 0;
  pending_read_fin_ = false;
  const bool is_text =
      pending_read_type_ == network::mojom::WebSocketMessageType::TEXT;
  pending_read_type_ = network::mojom::WebSocketMessageType::CONTINUATION;

  v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
  v8::HandleScope scope(isolate);
  auto buffer = v8::ArrayBuffer::New(isolate, data.size());
  if (!data.empty()) {
    // SAFETY: The destination buffer was allocated with exactly data.size()
    // bytes by v8::ArrayBuffer::New above.
    UNSAFE_BUFFERS(
        base::span(static_cast<uint8_t*>(buffer->Data()), data.size()))
        .copy_from(data);
  }
  Emit("message", is_text, buffer);
}

void WebSocketWrapper::OnDropChannel(bool was_clean,
                                     uint16_t code,
                                     const std::string& reason) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ == State::kClosed)
    return;
  // Avoid the disconnect handler firing with a synthetic error after this.
  client_receiver_.set_disconnect_handler(base::DoNothing());
  websocket_.set_disconnect_handler(base::DoNothing());
  state_ = State::kClosed;
  Emit("close", was_clean, static_cast<uint32_t>(code), reason);
  Teardown();
}

void WebSocketWrapper::OnClosingHandshake() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ == State::kOpen)
    state_ = State::kClosing;
  Emit("closing");
}

// ---------------------------------------------------------------------------
// Send / Close

void WebSocketWrapper::Send(gin::Arguments* args) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  bool is_text = false;
  v8::Local<v8::ArrayBufferView> data;
  if (!args->GetNext(&is_text) || !args->GetNext(&data)) {
    args->ThrowTypeError("Expected (isText, data)");
    return;
  }

  const auto bytes = electron::util::as_byte_span(data);
  // Per the WebSocket spec, `bufferedAmount` is incremented even when the
  // data will never be sent (e.g. after close() has been called).
  buffered_amount_ += bytes.size();
  if (state_ != State::kOpen || close_requested_)
    return;

  pending_writes_.emplace_back(
      is_text ? network::mojom::WebSocketMessageType::TEXT
              : network::mojom::WebSocketMessageType::BINARY,
      std::vector<uint8_t>(bytes.begin(), bytes.end()));
  DrainWriteQueue();
}

void WebSocketWrapper::Close(gin::Arguments* args) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (state_ == State::kClosing || state_ == State::kClosed ||
      close_requested_) {
    return;
  }

  // Per spec, when no code is supplied, 1005 ("no status received") tells the
  // network service not to send a status code in the Close frame.
  uint32_t code = 1005;
  std::string reason;
  args->GetNext(&code);
  args->GetNext(&reason);
  close_code_ = static_cast<uint16_t>(code);
  close_reason_ = reason;
  close_requested_ = true;

  if (state_ == State::kConnecting) {
    // Abort the handshake. Per the WebSocket spec, closing while CONNECTING
    // fails the connection: fire 'close' with wasClean=false, code 1006, and
    // do not fire 'error'. The 'close' event must be fired in a queued task,
    // not synchronously from close(), so post it.
    handshake_receiver_.reset();
    state_ = State::kClosed;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&WebSocketWrapper::EmitCloseAndTeardown,
                                  WeakRef(), /*was_clean=*/false,
                                  /*code=*/1006u, std::string()));
    return;
  }

  // Drain any in-flight message bytes before sending the Close frame; the
  // network service is waiting for them.
  DrainWriteQueue();
}

void WebSocketWrapper::DrainWriteQueue() {
  while (!pending_writes_.empty()) {
    PendingMessage& msg = pending_writes_.front();

    if (!msg.announced) {
      websocket_->SendMessage(msg.type, msg.data.size());
      msg.announced = true;
    }

    while (msg.offset < msg.data.size()) {
      base::span<uint8_t> dest;
      MojoResult result = writable_->BeginWriteData(
          msg.data.size() - msg.offset, MOJO_BEGIN_WRITE_DATA_FLAG_NONE, dest);
      if (result == MOJO_RESULT_SHOULD_WAIT) {
        writable_watcher_.ArmOrNotify();
        return;
      }
      if (result != MOJO_RESULT_OK) {
        Fail("WebSocket write failed", net::ERR_FAILED);
        return;
      }
      const size_t chunk = std::min(dest.size(), msg.data.size() - msg.offset);
      dest.first(chunk).copy_from(
          base::span(msg.data).subspan(msg.offset, chunk));
      writable_->EndWriteData(chunk);
      msg.offset += chunk;
      buffered_amount_ -= chunk;
    }

    pending_writes_.pop_front();
  }

  MaybeStartClosingHandshake();
}

void WebSocketWrapper::OnWritable(MojoResult result,
                                  const mojo::HandleSignalsState&) {
  if (state_ == State::kClosed)
    return;
  if (result != MOJO_RESULT_OK) {
    Fail("WebSocket write failed", net::ERR_FAILED);
    return;
  }
  DrainWriteQueue();
}

void WebSocketWrapper::MaybeStartClosingHandshake() {
  if (!close_requested_ || !pending_writes_.empty() || state_ != State::kOpen)
    return;
  state_ = State::kClosing;
  websocket_->StartClosingHandshake(close_code_, close_reason_);
}

// ---------------------------------------------------------------------------
// gin

gin::ObjectTemplateBuilder WebSocketWrapper::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin_helper::EventEmitterMixin<
             WebSocketWrapper>::GetObjectTemplateBuilder(isolate)
      .SetMethod("send", &WebSocketWrapper::Send)
      .SetMethod("close", &WebSocketWrapper::Close)
      .SetMethod("getBufferedAmount", &WebSocketWrapper::GetBufferedAmount);
}

const gin::WrapperInfo* WebSocketWrapper::wrapper_info() const {
  return &kWrapperInfo;
}

const char* WebSocketWrapper::GetHumanReadableName() const {
  return "Electron / WebSocketWrapper";
}

void WebSocketWrapper::Trace(cppgc::Visitor* visitor) const {
  gin::Wrappable<WebSocketWrapper>::Trace(visitor);
  visitor->Trace(weak_factory_);
}

}  // namespace electron::api
