// Copyright (c) 2024 Samuel Maddock <sam@samuelmaddock.com>.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/electron_api_sw_ipc_handler_impl.h"

#include <utility>

#include "base/containers/unique_ptr_adapters.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/electron_browser_context.h"
#include "shell/browser/javascript_environment.h"
#include "shell/common/gin_helper/dictionary.h"

namespace electron {

namespace {
const void* const kUserDataKey = &kUserDataKey;

class ServiceWorkerIPCList : public base::SupportsUserData::Data {
 public:
  std::vector<std::unique_ptr<ElectronApiSWIPCHandlerImpl>> list;

  static ServiceWorkerIPCList* Get(
      content::RenderProcessHost* render_process_host,
      bool create_if_not_exists) {
    auto* service_worker_ipc_list = static_cast<ServiceWorkerIPCList*>(
        render_process_host->GetUserData(kUserDataKey));
    if (!service_worker_ipc_list && !create_if_not_exists) {
      return nullptr;
    }
    if (!service_worker_ipc_list) {
      auto new_ipc_list = std::make_unique<ServiceWorkerIPCList>();
      service_worker_ipc_list = new_ipc_list.get();
      render_process_host->SetUserData(kUserDataKey, std::move(new_ipc_list));
    }
    return service_worker_ipc_list;
  }
};

}  // namespace

ElectronApiSWIPCHandlerImpl::ElectronApiSWIPCHandlerImpl(
    content::RenderProcessHost* render_process_host,
    int64_t version_id,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver)
    : render_process_host_(render_process_host), version_id_(version_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  receiver_.Bind(std::move(receiver));
  receiver_.set_disconnect_handler(
      base::BindOnce(&ElectronApiSWIPCHandlerImpl::RemoteDisconnected,
                     base::Unretained(this)));

  render_process_host_->AddObserver(this);
}

ElectronApiSWIPCHandlerImpl::~ElectronApiSWIPCHandlerImpl() {
  render_process_host_->RemoveObserver(this);
}

void ElectronApiSWIPCHandlerImpl::RemoteDisconnected() {
  receiver_.reset();
  Destroy();
}

void ElectronApiSWIPCHandlerImpl::Message(bool internal,
                                          const std::string& channel,
                                          blink::CloneableMessage arguments) {
  auto* session = GetSession();
  if (session) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin::Handle<gin_helper::internal::Event> event =
        MakeIPCEvent(isolate, internal);
    session->Message(event, channel, std::move(arguments));
  }
}

void ElectronApiSWIPCHandlerImpl::Invoke(bool internal,
                                         const std::string& channel,
                                         blink::CloneableMessage arguments,
                                         InvokeCallback callback) {
  auto* session = GetSession();
  if (session) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin::Handle<gin_helper::internal::Event> event =
        MakeIPCEvent(isolate, internal);
    session->Invoke(event, channel, std::move(arguments), std::move(callback));
  }
}

void ElectronApiSWIPCHandlerImpl::ReceivePostMessage(
    const std::string& channel,
    blink::TransferableMessage message) {
  auto* session = GetSession();
  if (session) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin::Handle<gin_helper::internal::Event> event =
        MakeIPCEvent(isolate, false);
    session->ReceivePostMessage(event, channel, std::move(message));
  }
}

void ElectronApiSWIPCHandlerImpl::MessageSync(bool internal,
                                              const std::string& channel,
                                              blink::CloneableMessage arguments,
                                              MessageSyncCallback callback) {
  auto* session = GetSession();
  if (session) {
    v8::Isolate* isolate = electron::JavascriptEnvironment::GetIsolate();
    v8::HandleScope handle_scope(isolate);
    gin::Handle<gin_helper::internal::Event> event =
        MakeIPCEvent(isolate, internal);
    session->MessageSync(event, channel, std::move(arguments),
                         std::move(callback));
  }
}

void ElectronApiSWIPCHandlerImpl::MessageHost(
    const std::string& channel,
    blink::CloneableMessage arguments) {
  NOTIMPLEMENTED();  // Service workers have no <webview>
}

ElectronBrowserContext* ElectronApiSWIPCHandlerImpl::GetBrowserContext() {
  auto* browser_context = static_cast<ElectronBrowserContext*>(
      render_process_host_->GetBrowserContext());
  return browser_context;
}

api::Session* ElectronApiSWIPCHandlerImpl::GetSession() {
  return api::Session::FromBrowserContext(GetBrowserContext());
}

gin::Handle<gin_helper::internal::Event>
ElectronApiSWIPCHandlerImpl::MakeIPCEvent(v8::Isolate* isolate, bool internal) {
  gin::Handle<gin_helper::internal::Event> event =
      gin_helper::internal::Event::New(isolate);
  v8::Local<v8::Object> event_object = event.ToV8().As<v8::Object>();

  gin_helper::Dictionary dict(isolate, event_object);
  dict.Set("type", "service-worker");
  dict.Set("versionId", version_id_);
  dict.Set("processId", render_process_host_->GetID());

  // Set session to provide context for getting preloads
  dict.Set("session", GetSession());

  if (internal)
    dict.SetHidden("internal", internal);

  return event;
}

void ElectronApiSWIPCHandlerImpl::Destroy() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  auto* service_worker_ipc_list = ServiceWorkerIPCList::Get(
      render_process_host_, /*create_if_not_exists=*/false);
  CHECK(service_worker_ipc_list);
  // std::erase_if will lead to a call to the destructor for this object.
  std::erase_if(service_worker_ipc_list->list, base::MatchesUniquePtr(this));
}

void ElectronApiSWIPCHandlerImpl::RenderProcessExited(
    content::RenderProcessHost* host,
    const content::ChildProcessTerminationInfo& info) {
  CHECK_EQ(host, render_process_host_);
  // TODO(crbug.com/1407197): Investigate clearing the user data from
  // RenderProcessHostImpl::Cleanup.
  Destroy();
  // This instance has now been deleted.
}

// static
void ElectronApiSWIPCHandlerImpl::BindReceiver(
    int render_process_id,
    int64_t version_id,
    mojo::PendingAssociatedReceiver<mojom::ElectronApiIPC> receiver) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* render_process_host =
      content::RenderProcessHost::FromID(render_process_id);
  if (!render_process_host) {
    return;
  }
  auto* service_worker_ipc_list = ServiceWorkerIPCList::Get(
      render_process_host, /*create_if_not_exists=*/true);
  service_worker_ipc_list->list.push_back(
      std::make_unique<ElectronApiSWIPCHandlerImpl>(
          render_process_host, version_id, std::move(receiver)));
}

}  // namespace electron
